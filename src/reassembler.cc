#include "reassembler.hh"
#include <algorithm>
#include <cassert>
#include <cstdint>
#include <iostream>
using namespace std;

void Reassembler::insert( uint64_t first_index, string data, bool is_last_substring )
{
  // Your code here.
  (void)first_index;
  (void)data;
  (void)is_last_substring;

  // NOTE: every byte pushed is valid sequence.
  // NOTE: cur index to be pushed might be partially pushed before, so need to check
  // NOTE: "That is, you can assume that there is a unique underlying byte-stream, and all
  // NOTE: substrings are (accurate) slices of it" ----  package may be lost but not corrupted
  // NOTE: buffer: start marker next_index, len is available_capacity
  std::cout << "first_index: " << first_index << " data: " << data << " is_last_substring: " << is_last_substring
            << std::endl;

  if ( output_.writer().available_capacity() > buffer.size() ) {
    // buffer = string( output_.writer().available_capacity() - buffer.size(), 'x' ) + buffer;
    buffer.resize( output_.writer().available_capacity() );
  }
  // duplicate one, drop
  if ( first_index + data.size() < next_index ) {
    return;
  }
  if ( first_index + data.size() == next_index ) {
    if ( is_last_substring ) {
      output_.writer().close();
      clear_buffer();
    }
    return;
  }
  // overlap
  if ( first_index < next_index ) {
    assert( first_index + data.size() > next_index );
    data = data.substr( next_index - first_index, output_.writer().available_capacity() );
    // add
    if ( !data.empty() ) {
      // has some space to contain data
      write_in_space( data, is_last_substring );
    }
    return;
  }
  // gap, exceed capacity
  if ( first_index >= next_index + buffer.size() ) {
    cout << "gap: drop package" << endl;
    return;
  }
  // gap, (partially) in capacity, store in buffer
  if ( first_index > next_index ) {
    if ( first_index + data.size() > next_index + buffer.size() ) {
      auto input_size = buffer.size() - ( first_index - next_index );
      // NOTE: partial overlap cannot be the last
      find_replace_buffer_item( data.substr( 0, input_size ), first_index, false );
    } else {
      find_replace_buffer_item( data, first_index, is_last_substring );
    }
    return;
  }

  // exactly equal, write
  assert( first_index == next_index );
  // when data.size() < buffer.size(), substr return data
  // NOTE: partial overlap cannot be the last
  if ( data.size() > buffer.size() ) {
    data = data.substr( 0, buffer.size() );
    write_in_space( data, false );
  } else {
    write_in_space( data, is_last_substring );
  }
}

uint64_t Reassembler::bytes_pending() const
{
  // Your code here.
  return bytes_pending_;
}
