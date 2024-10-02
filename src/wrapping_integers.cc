#include "wrapping_integers.hh"
#include <cstdint>
#include <iostream>
#include <sys/stat.h>
using namespace std;

Wrap32 Wrap32::wrap( uint64_t n, Wrap32 zero_point )
{
  // Your code here.
  (void)n;
  (void)zero_point;
  return zero_point + static_cast<uint32_t>( n );
}

uint64_t Wrap32::unwrap( Wrap32 zero_point, uint64_t checkpoint ) const
{
  // Your code here.
  (void)zero_point;
  (void)checkpoint;
  uint32_t mask = 0xFFFFFFFF;
  uint32_t cp_low = checkpoint & mask;
  uint32_t diff = 0;
  if ( this->raw_value_ > zero_point.raw_value_ ) {
    diff = this->raw_value_ - zero_point.raw_value_;
  } else {
    uint32_t gap = zero_point.raw_value_ - this->raw_value_;
    diff = 0xFFFFFFFF - gap + 1;
  }
  // std::cout << "cp_low:" << cp_low << " ,diff:" << diff << ",checkpoint:" << checkpoint << std::endl;
  if ( diff > cp_low ) {
    if ( diff - cp_low < 0x80000000 ) {
      return checkpoint - cp_low + diff;
    } else {
      uint64_t a = static_cast<uint64_t>( 0x80000000 ) << 1;
      // HINT: a "less than 0" abs seqNo means nothing
      return checkpoint - cp_low + diff > a ? checkpoint - cp_low + diff - a : checkpoint - cp_low + diff;
    }
  } else {

    if ( cp_low - diff < 0x80000000 ) {
      return checkpoint - cp_low + diff;
    } else {
      return checkpoint - cp_low + diff + ( static_cast<uint64_t>( 0x80000000 ) << 1 );
    }
  }
}
