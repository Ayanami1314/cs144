#include "tcp_sender.hh"
#include "byte_stream.hh"
#include "tcp_config.hh"
#include "tcp_sender_message.hh"
#include "wrapping_integers.hh"
#include <algorithm>
#include <cassert>
#include <iostream>
using namespace std;

uint64_t TCPSender::sequence_numbers_in_flight() const
{
  // Your code here.
  return next_send_segno - received_max_ackno;
}

uint64_t TCPSender::consecutive_retransmissions() const
{
  // Your code here.
  return timer.get_consecutive_retransmissions();
}

void TCPSender::push( const TransmitFunction& transmit )
{
  // Your code here.

  uint64_t has_pushed = 0;
  bool finished = false;
  std::cerr << "--- Push Start" << std::endl;
  if ( finish_send ) {
    return;
  }

  if ( input_.reader().is_finished() && !finish_send && sequence_numbers_in_flight() < cur_window_size ) {
    // HINT: extra fin not send
    cerr << "extra fin" << endl;
    auto msg = make_empty_message();
    msg.FIN = true;
    if ( next_send_segno == 0 ) {
      msg.SYN = true;
    }
    outstanding_segs.push( { next_send_segno, msg } );
    next_send_segno += msg.sequence_length();
    finish_send = true;
    transmit( msg );
    return;
  }

  if ( !timer.is_active() ) {
    timer.start();
  }
  // HINT: window size == 0 's push special case
  auto fake_window_size = cur_window_size == 0 ? 1 : cur_window_size;
  auto cur_space
    = fake_window_size > sequence_numbers_in_flight() ? fake_window_size - sequence_numbers_in_flight() : 0;

  while ( has_pushed < cur_space && !finished ) {
    TCPSenderMessage msg;
    if ( next_send_segno == 0 ) {
      msg.SYN = true;
    }
    if ( input_.has_error() ) {
      msg.RST = true;
    }
    if ( cur_space - has_pushed > TCPConfig::MAX_PAYLOAD_SIZE + msg.SYN ) {
      // NOTE: window space > payload
      uint16_t pkg_sz = TCPConfig::MAX_PAYLOAD_SIZE;
      read( input_.reader(), pkg_sz, msg.payload );
      if ( input_.reader().is_finished() ) {
        msg.FIN = true;
      }
    } else {
      // NOTE: window limit seqno gap instead of actual payload bytes
      uint16_t pkg_sz = cur_space - has_pushed - msg.SYN;
      if ( input_.reader().bytes_buffered() < pkg_sz ) {
        read( input_.reader(), input_.reader().bytes_buffered(), msg.payload );
        msg.FIN = input_.reader().is_finished();
      } else {
        // HINT:for == case, FIN can't be send during this push. The **test points** that it should be pushed in
        // next call of push() if window has enough space (instead of placing it in outstanding segments or other
        // solutions)
        read( input_.reader(), pkg_sz, msg.payload );
      }
    }
    if ( msg.FIN ) {
      finished = true;
      finish_send = true;
    }

    msg.seqno = Wrap32( isn_ + next_send_segno );

    if ( msg.sequence_length() == 0 ) {
      // nothing to send
      break;
    }
    // update outstanding
    outstanding_segs.push( { next_send_segno, msg } );

    // update next_send
    next_send_segno += msg.sequence_length();

    std::cerr << "Send: " << msg << std::endl;
    // send msg
    transmit( msg );
    has_pushed += msg.sequence_length();
  }
  std::cerr << "--- Push End" << std::endl;
}

TCPSenderMessage TCPSender::make_empty_message() const
{
  // Your code here.
  TCPSenderMessage msg;
  msg.seqno = Wrap32( isn_ + next_send_segno );
  msg.RST = input_.has_error();
  return msg;
}

void TCPSender::receive( const TCPReceiverMessage& msg )
{
  // Your code here.
  (void)msg;
  if ( msg.RST ) {
    // TODO
    this->writer().set_error();
    timer.reset_all();
    while ( !outstanding_segs.empty() )
      outstanding_segs.pop();
  }
  if ( msg.ackno.has_value() ) {
    uint64_t abs_seq = msg.ackno->unwrap( this->isn_, writer().bytes_pushed() );
    // HINT: check if valid
    if ( abs_seq > received_max_ackno && abs_seq <= next_send_segno ) {
      // fully received, remove it
      while ( !outstanding_segs.empty()
              && outstanding_segs.top().abs_seq + outstanding_segs.top().msg.sequence_length() <= abs_seq ) {
        std::cerr << "Receive: " << outstanding_segs.top().msg << std::endl;
        outstanding_segs.pop();
      }
      received_max_ackno = abs_seq;
      timer.reset_all();
      timer.start();
    }
  }
  // HINT: not validate the window size change
  cerr << "change window size to: " << msg.window_size << endl;
  cur_window_size = msg.window_size;
}

void TCPSender::tick( uint64_t ms_since_last_tick, const TransmitFunction& transmit )
{
  // Your code here.
  (void)ms_since_last_tick;
  (void)transmit;
  bool expired = timer.update( ms_since_last_tick );
  if ( !expired ) {
    // cerr << "Not expired" << endl;
    return;
  }
  // retransmit earliest seg
  if ( !outstanding_segs.empty() ) {
    auto [_, msg] = outstanding_segs.top();
    transmit( msg );
    std::cerr << "Retransmit: " << msg << std::endl;
    if ( cur_window_size != 0 ) {
      // i. keep track of retransmission
      timer.add_consecutive_retransmissions();
      // ii. double the RTO and restart the timer
      timer.set_rto( 2 * timer.get_rto() );
      cerr << "Doubled RTO with window size " << cur_window_size << endl;
    }
    // reset the time and start
    timer.clear_timer();
    timer.start();
  }
}
