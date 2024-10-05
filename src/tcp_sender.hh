#pragma once

#include "byte_stream.hh"
#include "retransmit_timer.hh"
#include "tcp_receiver_message.hh"
#include "tcp_sender_message.hh"
#include "wrapping_integers.hh"

#include <cstdint>
#include <functional>
#include <list>
#include <memory>
#include <optional>
#include <ostream>
#include <queue>

class TCPSender
{
public:
  /* Construct TCP sender with given default Retransmission Timeout and possible ISN */
  TCPSender( ByteStream&& input, Wrap32 isn, uint64_t initial_RTO_ms )
    : input_( std::move( input ) )
    , isn_( isn )
    , outstanding_segs()
    , received_max_ackno( 0 )
    , next_send_segno( 0 )
    , cur_window_size( 1 )
    , timer( initial_RTO_ms )
    , finish_send( false )
  {}

  /* Generate an empty TCPSenderMessage */
  TCPSenderMessage make_empty_message() const;

  /* Receive and process a TCPReceiverMessage from the peer's receiver */
  void receive( const TCPReceiverMessage& msg );

  /* Type of the `transmit` function that the push and tick methods can use to send messages */
  using TransmitFunction = std::function<void( const TCPSenderMessage& )>;

  /* Push bytes from the outbound stream */
  void push( const TransmitFunction& transmit );

  /* Time has passed by the given # of milliseconds since the last time the tick() method was called */
  void tick( uint64_t ms_since_last_tick, const TransmitFunction& transmit );

  // Accessors
  uint64_t sequence_numbers_in_flight() const;  // How many sequence numbers are outstanding?
  uint64_t consecutive_retransmissions() const; // How many consecutive *re*transmissions have happened?
  Writer& writer() { return input_.writer(); }
  const Writer& writer() const { return input_.writer(); }

  // Access input stream reader, but const-only (can't read from outside)
  const Reader& reader() const { return input_.reader(); }

private:
  // Variables initialized in constructor
  ByteStream input_;
  Wrap32 isn_;
  struct outstanding_pair
  {
    uint64_t abs_seq;
    TCPSenderMessage msg;
    // priority less, abs_seq greater
    bool operator<( const outstanding_pair& rhs ) const { return abs_seq > rhs.abs_seq; }
  };
  std::priority_queue<outstanding_pair> outstanding_segs;
  uint64_t received_max_ackno;
  uint64_t next_send_segno;
  uint64_t cur_window_size;
  RetransmitTimer timer;
  bool finish_send;
};
