#include "byte_stream.hh"
#include <cassert>
#include <string>
#include <string_view>

using namespace std;

ByteStream::ByteStream( uint64_t capacity )
  : capacity_( capacity )
  , closed_( false )
  , error_( false )
  , read_idx( 0 )
  , write_idx( 0 )
  , bytes_pushed_( 0 )
  , bytes_popped_( 0 )
  , buffer( string( capacity, '\0' ) )
{}

bool Writer::is_closed() const
{
  // Your code here.
  return closed_;
}

void Writer::push( string data )
{
  // Your code here.
  (void)data;
  if ( available_capacity() == 0 ) {
    set_error();
    return;
  }
  auto size = data.size();
  if ( read_idx > 0 && write_idx + size > capacity_ ) {
    // copy, reset read idx to support peek
    for ( auto i = read_idx; i < write_idx; ++i ) {
      buffer[i - read_idx] = buffer[i];
    }
    write_idx = write_idx - read_idx;
    read_idx = 0;
  }
  if ( size > available_capacity() ) {
    assert( read_idx == 0 );
    set_error();
    buffer.replace( write_idx, capacity_ - write_idx, data.substr( 0, capacity_ - write_idx ) );
    bytes_pushed_ += capacity_ - write_idx;
    write_idx = capacity_;
    return;
  }
  bytes_pushed_ += size;
  buffer.replace( write_idx, size, data );
  write_idx += size;
  assert( write_idx <= capacity_ );
}

void Writer::close()
{
  // Your code here.
  closed_ = true;
}

uint64_t Writer::available_capacity() const
{
  // Your code here.
  return read_idx + capacity_ - write_idx;
}

uint64_t Writer::bytes_pushed() const
{
  // Your code here.
  return bytes_pushed_;
}

bool Reader::is_finished() const
{
  // Your code here.
  return read_idx == write_idx && closed_;
}

uint64_t Reader::bytes_popped() const
{
  // Your code here.
  return bytes_popped_;
}

string_view Reader::peek() const
{
  // Your code here.
  return string_view( buffer ).substr( read_idx, write_idx - read_idx );
}

void Reader::pop( uint64_t len )
{
  // Your code here.
  (void)len;
  if ( write_idx - read_idx < len ) {
    set_error();
    return;
  }
  bytes_popped_ += len;
  read_idx += len;
}

uint64_t Reader::bytes_buffered() const
{
  // Your code here.
  return bytes_pushed_ - bytes_popped_;
}
