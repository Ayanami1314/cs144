#include "byte_stream.hh"
#include <cassert>
#include <string>
#include <string_view>

using namespace std;

ByteStream::ByteStream( uint64_t capacity )
  : capacity_( capacity )
  , closed_( false )
  , error_( false )
  , bytes_pushed_( 0 )
  , bytes_popped_( 0 )
  , buffer( std::string() )
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
  if ( closed_ ) {
    set_error();
    return;
  }
  auto size = data.size();
  if ( size > available_capacity() ) {
    data.resize( available_capacity() );
    size = available_capacity();
  }
  bytes_pushed_ += size;
  buffer += data;
}

void Writer::close()
{
  // Your code here.
  closed_ = true;
}

uint64_t Writer::available_capacity() const
{
  // Your code here.
  return capacity_ - buffer.size();
}

uint64_t Writer::bytes_pushed() const
{
  // Your code here.
  return bytes_pushed_;
}

bool Reader::is_finished() const
{
  // Your code here.
  // return read_idx == write_idx && closed_;
  return buffer.empty() && closed_;
}

uint64_t Reader::bytes_popped() const
{
  // Your code here.
  return bytes_popped_;
}

string_view Reader::peek() const
{
  // Your code here.
  return string_view( buffer );
}

void Reader::pop( uint64_t len )
{
  // Your code here.
  (void)len;
  auto size = buffer.size();
  if ( len > size ) {
    bytes_popped_ += size;
    buffer.clear();
    return;
  }
  bytes_popped_ += len;
  buffer = buffer.substr( len );
}

uint64_t Reader::bytes_buffered() const
{
  // Your code here.
  return bytes_pushed_ - bytes_popped_;
}
