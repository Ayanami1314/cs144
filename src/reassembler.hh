#pragma once
#include "byte_stream.hh"
#include <algorithm>
#include <cassert>
#include <cstdint>
#include <iterator>
#include <set>
#include <vector>
class Reassembler
{
private:
  /**
   * @brief cut all items start before index
   */
  void rebuild_buffer_items( uint64_t index )
  {
    int cnt = 0;
    for ( const auto& item : buffered_items ) {
      if ( item.index + item.size <= index ) {
        // NOTE:can be partially overlapped
        bytes_pending_ -= item.size;
        cnt++;
      } else {
        break;
      }
    }
    if ( cnt > 0 ) {
      buffered_items.erase( buffered_items.begin(), std::next( buffered_items.begin(), cnt ) );
    }
  }
  void clear_buffer()
  {
    buffered_items.clear();
    buffer.resize( 0 );
    bytes_pending_ = 0;
    next_index = 0;
  }
  /**
   * @brief DO buffer opeartions and capacity check OUTSIDE!!!
   * @param  data
   * @param  start
   * @param  is_last_substring
   */
  void find_replace_buffer_item( const std::string& data, uint64_t start, bool is_last_substring )
  {
    // auto it = find_if( buffered_items.begin(), buffered_items.end(), [start, size]( const auto& item ) {
    //   return ( item.index <= start && start <= item.size + item.index )
    //          || ( start <= item.index && item.index <= start + size );
    // } );
    assert( start > next_index );
    auto size = data.size();
    buffer.replace( start - next_index, size, data );
    buffered_items.emplace( start, size, is_last_substring );
    // combine the next item
    // NOTE: now the set has been sorted
    std::set<SubStringTuple> new_buffer;
    uint64_t seg_start = start;
    uint64_t seg_end = start + size;
    bool seg_is_last = is_last_substring;
    for ( const auto& item : buffered_items ) {
      if ( item.index + item.size >= seg_start && item.index <= seg_start ) {
        seg_start = item.index;
      }
      if ( item.index <= seg_end && item.size + item.index >= seg_end ) {
        seg_end = item.index + item.size;
        seg_is_last = item.is_last_substring;
      }
    }
    bytes_pending_ = 0;
    for ( const auto& item : buffered_items ) {
      if ( item.index + item.size < seg_start || item.index > seg_end ) {
        new_buffer.insert( item );
        bytes_pending_ += item.size;
      }
    }
    new_buffer.emplace( seg_start, seg_end - seg_start, seg_is_last );
    bytes_pending_ += seg_end - seg_start;
    buffered_items.clear();
    buffered_items = new_buffer;
  }
  /**
   * @brief there has enough space(from next_index) to write data in buffer
   * @param  data
   */
  void write_in_space( const std::string& data, bool is_last_substring )
  {
    output_.writer().push( data );
    buffer = buffer.substr( data.size() );
    next_index += data.size();
    rebuild_buffer_items( next_index );
    bool end = is_last_substring;
    while ( !end ) {
      auto it = std::find_if( buffered_items.begin(), buffered_items.end(), [this]( const auto& item ) {
        return item.index <= next_index && item.index + item.size > next_index;
      } );
      if ( it == buffered_items.end() ) {
        break;
      }
      assert( it->size > ( next_index - it->index ) );
      auto append_len = it->size - ( next_index - it->index );
      output_.writer().push( buffer.substr( 0, append_len ) );
      buffer = buffer.substr( append_len );
      next_index += append_len;
      end = it->is_last_substring;
      rebuild_buffer_items( next_index );
    }
    if ( end ) {
      clear_buffer();
      output_.writer().close();
    }
  }

public:
  // Construct Reassembler to write into given ByteStream.
  explicit Reassembler( ByteStream&& output )
    : output_( std::move( output ) )
    , bytes_pending_( 0 )
    , buffered_items()
    , buffer( output.writer().available_capacity(), '\0' )
    , next_index( 0 )
  {}

  /*
   * Insert a new substring to be reassembled into a ByteStream.
   *   `first_index`: the index of the first byte of the substring
   *   `data`: the substring itself
   *   `is_last_substring`: this substring represents the end of the stream
   *   `output`: a mutable reference to the Writer
   *
   * The Reassembler's job is to reassemble the indexed substrings (possibly out-of-order
   * and possibly overlapping) back into the original ByteStream. As soon as the Reassembler
   * learns the next byte in the stream, it should write it to the output.
   *
   * If the Reassembler learns about bytes that fit within the stream's available capacity
   * but can't yet be written (because earlier bytes remain unknown), it should store them
   * internally until the gaps are filled in.
   *
   * The Reassembler should discard any bytes that lie beyond the stream's available capacity
   * (i.e., bytes that couldn't be written even if earlier gaps get filled in).
   *
   * The Reassembler should close the stream after writing the last byte.
   */
  void insert( uint64_t first_index, std::string data, bool is_last_substring );

  // How many bytes are stored in the Reassembler itself?
  uint64_t bytes_pending() const;

  // Access output stream reader
  Reader& reader() { return output_.reader(); }
  const Reader& reader() const { return output_.reader(); }

  // Access output stream writer, but const-only (can't write from outside)
  const Writer& writer() const { return output_.writer(); }
  struct SubStringTuple
  {
    uint64_t index;
    uint64_t size;
    bool is_last_substring;
    bool operator<( const SubStringTuple& rhs ) const
    {
      return index < rhs.index || ( index == rhs.index && size < rhs.size );
    }
    void setIndex( uint64_t i ) { this->index = i; }
    void setSize( uint64_t s ) { this->size = s; }
    void setLast( bool l ) { this->is_last_substring = l; }
  };
  uint64_t get_next_index() const { return next_index; }
  // auto get_remain_capacity() const { return buffer.size(); };

private:
  ByteStream output_;      // the Reassembler writes to this ByteStream
  uint64_t bytes_pending_; // number of bytes stored in the Reassembler itself
  std::set<SubStringTuple> buffered_items;
  std::string buffer;
  uint64_t next_index;
};
