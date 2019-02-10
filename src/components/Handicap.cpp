#include "Handicap.h"

void Handicap::set(uint8_t hcp) {
	if (hcp > 90)
		hcp = 90;

	m_handicap_percent = hcp;

	m_handicap = 1 - hcp / 100.f;
}

uint8_t Handicap::send(uint8_t amount) {
  m_buffer += amount * m_handicap;
  amount = m_buffer;
  m_buffer -= amount;

  return amount;
}

void Handicap::clear() {
  m_buffer = 0;
}

uint8_t Handicap::get() {
  return m_handicap_percent;
}
