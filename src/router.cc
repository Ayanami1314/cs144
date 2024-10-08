#include "router.hh"
#include "address.hh"
#include "ipv4_datagram.hh"

#include <algorithm>
#include <iostream>
#include <optional>
#include <stdexcept>

using namespace std;

bool Router::match( uint32_t address, const RuleKey& r ) const
{
  if ( r.len > 32 ) {
    throw std::runtime_error( "Invalid len" );
  }
  uint32_t mask = 0xFFFFFFFF;
  for ( int i = 0; i < 32 - r.len; ++i ) {
    mask <<= 1;
  }
  // cout << "DEBUG: mask: " << mask << "Rule: " << r.rule << endl;

  return ( address & mask ) == ( r.rule & mask );
}

// route_prefix: The "up-to-32-bit" IPv4 address prefix to match the datagram's destination address against
// prefix_length: For this route to be applicable, how many high-order (most-significant) bits of
//    the route_prefix will need to match the corresponding bits of the datagram's destination address?
// next_hop: The IP address of the next hop. Will be empty if the network is directly attached to the router (in
//    which case, the next hop address should be the datagram's final destination).
// interface_num: The index of the interface to send the datagram out on.
void Router::add_route( const uint32_t route_prefix,
                        const uint8_t prefix_length,
                        const optional<Address> next_hop,
                        const size_t interface_num )
{
  cerr << "DEBUG: adding route " << Address::from_ipv4_numeric( route_prefix ).ip() << "/"
       << static_cast<int>( prefix_length ) << " => " << ( next_hop.has_value() ? next_hop->ip() : "(direct)" )
       << " on interface " << interface_num << "\n";

  // Your code here.
  _rules[{ route_prefix, prefix_length }].interface_num = interface_num;
  _rules[{ route_prefix, prefix_length }].next_hop.push_back( next_hop );
}

// Go through all the interfaces, and route every incoming datagram to its proper outgoing interface.
void Router::route()
{
  // Your code here.
  // cout << "DEBUG: routing from " << _rules.size() << "rules and " << _interfaces.size() << " interfaces" << endl;
  for ( auto interface : _interfaces ) {
    // NOTE: & is important
    auto& queue = interface->datagrams_received();
    while ( !queue.empty() ) {
      auto datagram = queue.front();

      cout << interface->name() << " received datagram " << datagram.header.to_string() << endl;
      cout << endl << endl;
      queue.pop();
      if ( datagram.header.ttl <= 1 ) {

        continue;
      }
      // decrease ttl and re-calculate the checksum
      datagram.header.ttl--;
      datagram.header.compute_checksum();
      // route: find the interface to send next
      RuleKey target_key = {};
      RuleValue target_value = {};
      target_key.len = 0;
      bool found = false;
      // for ( const auto& r : _rules ) {
      //   cout << r.to_string() << endl;
      // }

      for ( const auto& r : _rules ) {
        // NOTE: =, len = 0, 0.0.0.0/0
        if ( match( datagram.header.dst, r.first ) && r.first.len >= target_key.len ) {
          target_key = r.first;
          target_value = r.second;
          found = true;
        }
      }
      if ( !found ) {
        continue;
      }
      cout << Address::from_ipv4_numeric( datagram.header.dst ).ip()
           << " matched ip: " << Address::from_ipv4_numeric( target_key.rule ).ip() << endl;
      // cout << "found" << endl;
      auto send_interface = this->interface( target_value.interface_num );
      auto next_hops = target_value.next_hop;
      for ( const auto& hop : next_hops ) {
        if ( !hop.has_value() ) {
          send_interface->send_datagram( datagram, Address::from_ipv4_numeric( datagram.header.dst ) );
          break;
        }
        send_interface->send_datagram( datagram, hop.value() );
      }
    }
  }
}
