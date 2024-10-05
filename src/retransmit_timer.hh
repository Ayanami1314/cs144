#pragma once
#include <stdexcept>
#include <stdint.h>
class RetransmitTimer
{
  uint64_t cur_timer_ms_;
  uint64_t consecutive_retransmissions;
  const uint64_t initial_RTO_ms_;
  uint64_t cur_RTO_ms_;
  bool active;

public:
  RetransmitTimer( uint64_t initial_RTO_ms )
    : cur_timer_ms_( 0 )
    , consecutive_retransmissions( 0 )
    , initial_RTO_ms_( initial_RTO_ms )
    , cur_RTO_ms_( initial_RTO_ms )
    , active( false )
  {}
  uint64_t get_time_ms_() const { return cur_timer_ms_; }

  // return if expired
  bool update( uint64_t ms_since_last_tick )
  {
    if ( !active ) {
      throw std::runtime_error( "try to update when timer is not active" );
    }
    cur_timer_ms_ += ms_since_last_tick;
    return cur_timer_ms_ >= cur_RTO_ms_;
  }
  void start() { active = true; }
  void stop() { active = false; }
  // getter & setter
  uint64_t get_rto() { return cur_RTO_ms_; }
  uint64_t get_init_rto() const { return initial_RTO_ms_; }
  uint64_t get_consecutive_retransmissions() const { return consecutive_retransmissions; }
  void set_rto( uint64_t rto ) { cur_RTO_ms_ = rto; }
  void clear_timer()
  {
    cur_timer_ms_ = 0;
    active = false;
  }
  void set_consecutive_retransmissions( uint64_t num ) { consecutive_retransmissions = num; }
  void add_consecutive_retransmissions() { consecutive_retransmissions++; }
  void reset_rto() { cur_RTO_ms_ = initial_RTO_ms_; }
  void reset_all()
  {
    cur_RTO_ms_ = initial_RTO_ms_;
    consecutive_retransmissions = 0;
    cur_timer_ms_ = 0;
    active = false;
  }
  bool is_active() const { return active; }
};
