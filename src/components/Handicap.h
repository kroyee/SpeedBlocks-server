#ifndef HANDICAP_H
#define HANDICAP_H

#include <cstdint>

class Handicap {
  uint8_t m_handicap_percent = 0;
  float m_handicap = 1;
  float m_buffer = 0;

public:
  void set(uint8_t hcp);
  uint8_t send(uint8_t amount);
  void clear();
  uint8_t get();
};

#endif
