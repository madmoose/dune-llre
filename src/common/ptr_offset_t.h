#pragma once

#include "bytes.h"
#include <cstdint>

struct ptr_offset_t {
	byte *m_begin;
	uint16_t m_offset;

	ptr_offset_t() {
		m_begin = nullptr;
		m_offset = 0;
	}

	ptr_offset_t(byte *begin, uint16_t offset = 0) {
		m_begin = begin;
		m_offset = offset;
	}

	ptr_offset_t(const ptr_offset_t &other) {
		m_begin = other.m_begin;
		m_offset = other.m_offset;
	}

	ptr_offset_t& operator=(const ptr_offset_t& other) {
		if (this == &other) return *this;
		m_begin = other.m_begin;
		m_offset = other.m_offset;
		return *this;
	}

	byte *ptr() const { return m_begin + m_offset; }

	byte *begin() const { return m_begin; }

	void set_offset(uint16_t offset) {
		m_offset = offset;
	}

	int offset() const { return m_offset; }

	void reset() {
		m_offset = 0;
	}

	ptr_offset_t &operator+=(int n)
	{
		m_offset += n;
		return *this;
	}

	const char *str() { return (const char *)(ptr()); }

	byte peekbyte() { return ::readbyte(ptr()); }

	byte readbyte()
	{
		byte v = ::readbyte(ptr());
		m_offset += 1;
		return v;
	}

	int8_t read_int8()
	{
		byte v = ::readbyte(ptr());
		m_offset += 1;
		return (int8_t)(v - INT8_MIN) + INT8_MIN;
	}

	void writebyte(byte v)
	{
		::writebyte(ptr(), v);
		m_offset += 1;
	}

	uint16_t peekle16()
	{
		uint16_t v = ::readle16(ptr());
		return v;
	}

	uint16_t peekle16_at_offset(int offset)
	{
		uint16_t v = ::readle16(ptr() + offset);
		return v;
	}

	uint16_t readle16()
	{
		uint16_t v = ::readle16(ptr());
		m_offset += 2;
		return v;
	}

	int16_t readle16i()
	{
		uint16_t v = ::readle16(ptr());
		m_offset += 2;
		return v;
	}

	void writele16(uint16_t v)
	{
		::writele16(ptr(), v);
		m_offset += 2;
	}

	uint32_t readle32()
	{
		uint32_t v = ::readle32(ptr());
		m_offset += 4;
		return v;
	}
};

ptr_offset_t operator+(ptr_offset_t a, int b) {
	a.m_offset += b;
	return a;
}

ptr_offset_t operator-(ptr_offset_t a, int b) {
	a.m_offset -= b;
	return a;
}

ptr_offset_t make_ptr_offset(byte *p)
{
	return ptr_offset_t{p, 0};
}
