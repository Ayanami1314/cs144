#include <iostream>
#include <optional>
#include <stdexcept>
#include <utility>

#include "address.hh"
#include "arp_message.hh"
#include "ethernet_frame.hh"
#include "ethernet_header.hh"
#include "exception.hh"
#include "ipv4_datagram.hh"
#include "network_interface.hh"
#include "parser.hh"

using namespace std;

//! \param[in] ethernet_address Ethernet (what ARP calls "hardware") address of the interface
//! \param[in] ip_address IP (what ARP calls "protocol") address of the interface
NetworkInterface::NetworkInterface( string_view name,
                                    shared_ptr<OutputPort> port,
                                    const EthernetAddress& ethernet_address,
                                    const Address& ip_address )
  : name_( name )
  , port_( notnull( "OutputPort", move( port ) ) )
  , ethernet_address_( ethernet_address )
  , ip_address_( ip_address )
  , arp_table_()
  , arp_time_table_()
  , now_( 0 )
{
  cerr << "DEBUG: Network interface has Ethernet address " << to_string( ethernet_address ) << " and IP address "
       << ip_address.ip() << "\n";
}

//! \param[in] dgram the IPv4 datagram to be sent
//! \param[in] next_hop the IP address of the interface to send it to (typically a router or default gateway, but
//! may also be another host if directly connected to the same network as the destination) Note: the Address type
//! can be converted to a uint32_t (raw 32-bit IP address) by using the Address::ipv4_numeric() method.
void NetworkInterface::send_datagram( const InternetDatagram& dgram, const Address& next_hop )
{
  // Your code here.
  (void)dgram;
  (void)next_hop;

  if ( next_hop != ip_address_ && !arp_table_.contains( next_hop ) ) {
    // ARP broadcast
    if ( !arp_time_table_.contains( next_hop ) ) {
      arp_time_table_[next_hop] = std::nullopt;
    }
    if ( arp_time_table_[next_hop].has_value() && now_ - arp_time_table_[next_hop].value() < 5000 ) {
      // < 5s from last arp broadcast
      datagrams_to_send_.push_back( { dgram, next_hop } ); // wait for arp table update and send.
      return;
    }
    // arp broadcast
    arp_time_table_[next_hop] = now_;
    EthernetHeader header { .dst = ETHERNET_BROADCAST, .src = ethernet_address_, .type = EthernetHeader::TYPE_ARP };
    ARPMessage msg { .opcode = ARPMessage::OPCODE_REQUEST,
                     .sender_ethernet_address = ethernet_address_,
                     .sender_ip_address = ip_address_.ipv4_numeric(),
                     .target_ip_address = next_hop.ipv4_numeric() };
    auto payload = serialize<ARPMessage>( msg );
    EthernetFrame frame { .header = header, .payload = payload };
    output().transmit( *this, frame );

    datagrams_to_send_.push_back( { dgram, next_hop } ); // wait for arp table update and send.
    return;
  }
  EthernetAddress dst_ethernet_addr;
  if ( next_hop == ip_address_ ) {
    dst_ethernet_addr = ethernet_address_;
  } else {
    dst_ethernet_addr = arp_table_.at( next_hop ).first;
  }
  EthernetHeader header { .dst = dst_ethernet_addr, .src = ethernet_address_, .type = EthernetHeader::TYPE_IPv4 };

  auto payload = serialize<InternetDatagram>( dgram );
  EthernetFrame frame { .header = header, .payload = payload };
  output().transmit( *this, frame );
}

//! \param[in] frame the incoming Ethernet frame
void NetworkInterface::recv_frame( const EthernetFrame& frame )
{
  // Your code here.
  (void)frame;
  if ( frame.header.dst != ethernet_address_ && frame.header.dst != ETHERNET_BROADCAST ) {
    return;
  }

  bool ok = false;
  if ( frame.header.type == EthernetHeader::TYPE_IPv4 ) {
    InternetDatagram received_ipv4;
    ok = parse<InternetDatagram>( received_ipv4, frame.payload );
    if ( ok ) {
      datagrams_received_.push( received_ipv4 );
    }
  } else if ( frame.header.type == EthernetHeader::TYPE_ARP ) {
    ARPMessage received_arp;
    ok = parse<ARPMessage>( received_arp, frame.payload );
    if ( !ok ) {
      std::cerr << "Incorrect arp message" << std::endl;
    }
    Address sender_ip = Address::from_ipv4_numeric( received_arp.sender_ip_address );
    arp_table_.insert( std::make_pair( sender_ip, std::make_pair( received_arp.sender_ethernet_address, now_ ) ) );

    for ( const auto& [d, ip] : datagrams_to_send_ ) {
      if ( sender_ip == ip ) {
        std::cerr << "send " << ip.ipv4_numeric() << std::endl;
        send_datagram( d, ip );
      }
    }
    if ( received_arp.opcode == ARPMessage::OPCODE_REQUEST
         && received_arp.target_ip_address == ip_address_.ipv4_numeric() ) {
      ARPMessage respond;
      respond.target_ethernet_address = received_arp.sender_ethernet_address;
      respond.target_ip_address = received_arp.sender_ip_address;
      respond.sender_ip_address = ip_address_.ipv4_numeric();
      respond.sender_ethernet_address = ethernet_address_;
      respond.opcode = ARPMessage::OPCODE_REPLY;
      EthernetHeader respond_header {
        .dst = respond.target_ethernet_address, .src = ethernet_address_, .type = EthernetHeader::TYPE_ARP };
      EthernetFrame respond_frame { .header = respond_header, .payload = serialize<ARPMessage>( respond ) };
      output().transmit( *this, respond_frame );
    }
  } else {
    throw std::runtime_error( "Not Implemented" );
  }
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void NetworkInterface::tick( const size_t ms_since_last_tick )
{
  // Your code here.
  (void)ms_since_last_tick;
  now_ += ms_since_last_tick;
  for ( auto it = arp_table_.begin(); it != arp_table_.end(); ) {
    if ( now_ - it->second.second >= 30000 ) {
      // expired

      for ( auto to_send_it = datagrams_to_send_.begin(); to_send_it != datagrams_to_send_.end(); ) {
        if ( to_send_it->second == it->first ) {
          to_send_it = datagrams_to_send_.erase( to_send_it );
        } else {
          to_send_it++;
        }
      }
      it = arp_table_.erase( it );

    } else {
      it++;
    }
  }
}
