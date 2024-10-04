#include "tcp_receiver.hh"
#include "tcp_sender_message.hh"
#include "wrapping_integers.hh"
#include <cstdint>

using namespace std;

void TCPReceiver::receive( TCPSenderMessage message )
{
  // Your code here.
  (void)message;
  if ( message.SYN ) {
    isn_ = Wrap32( message.seqno );
  }
  if ( message.RST ) {
    reassembler_.reader().set_error();
  }
  if ( !isn_ ) {
    return;
  }
  uint64_t checkpoint = reassembler_.get_next_index();
  // uint64_t abs_index = message.seqno.unwrap( isn_.value(), checkpoint ) - message.SYN + 1;
  // 1: SYN
  uint64_t data_index = message.seqno.unwrap( isn_.value(), checkpoint ) + message.SYN - 1;
  // if ( message.FIN || message.RST ) {
  //   // isn_.reset();
  // }

  reassembler_.insert( data_index, message.payload, message.FIN );
}

TCPReceiverMessage TCPReceiver::send() const
{
  // Your code here.
  // return {};
  TCPReceiverMessage msg;
  msg.window_size = reassembler_.writer().available_capacity() > UINT16_MAX
                      ? UINT16_MAX
                      : reassembler_.writer().available_capacity();
  // +1: SYN seqNo
  if ( reassembler_.writer().has_error() ) {
    msg.RST = true;
  }
  if ( !isn_.has_value() ) {
    return msg;
  }

  msg.ackno
    = Wrap32::wrap( reassembler_.writer().bytes_pushed() + 1 + reassembler_.writer().is_closed(), isn_.value() );
  return msg;
}
