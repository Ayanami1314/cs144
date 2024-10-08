#pragma once

#include <memory>
#include <optional>
#include <string>
#include <utility>

#include "address.hh"
#include "exception.hh"
#include "network_interface.hh"
#include <set>
#include <vector>

// \brief A router that has multiple network interfaces and
// performs longest-prefix-match routing between them.
class Router
{
public:
  // Add an interface to the router
  // \param[in] interface an already-constructed network interface
  // \returns The index of the interface after it has been added to the router
  size_t add_interface( std::shared_ptr<NetworkInterface> interface )
  {
    _interfaces.push_back( notnull( "add_interface", std::move( interface ) ) );
    return _interfaces.size() - 1;
  }

  // Access an interface by index
  std::shared_ptr<NetworkInterface> interface( const size_t N ) { return _interfaces.at( N ); }

  // Add a route (a forwarding rule)
  void add_route( uint32_t route_prefix,
                  uint8_t prefix_length,
                  std::optional<Address> next_hop,
                  size_t interface_num );

  // Route packets between the interfaces
  void route();

private:
  // The router's collection of network interfaces
  struct RuleKey
  {
    uint32_t rule {};
    uint8_t len {};
    bool operator<( const RuleKey& rhs ) const { return len > rhs.len || ( len == rhs.len && rule > rhs.rule ); }
    bool operator==( const RuleKey& rhs ) const { return len == rhs.len && rule == rhs.rule; }
    bool operator!=( const RuleKey& rhs ) const { return !operator==( rhs ); }
    std::string to_string() const
    {
      return Address::from_ipv4_numeric( rule ).to_string() + "/" + std::to_string( len );
    }
  };
  struct RuleValue
  {
    size_t interface_num {};
    std::vector<std::optional<Address>> next_hop {};
  };

  bool match( uint32_t address, const RuleKey& r ) const;
  std::vector<std::shared_ptr<NetworkInterface>> _interfaces {};
  std::map<RuleKey, RuleValue> _rules {};
};
