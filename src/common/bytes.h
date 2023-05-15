#include <cstdint>

typedef uint8_t byte;

byte readbyte(const byte *p)
{
	return p[0];
}

void writebyte(byte *p, byte v)
{
	p[0] = v;
}

uint16_t readle16(const byte *p)
{
	return (((uint16_t)p[0]) << 0)
	     | (((uint16_t)p[1]) << 8);
}

void writele16(byte *p, uint16_t v)
{
	p[0] = ((v >> 0) & 0xff);
	p[1] = ((v >> 8) & 0xff);
}

uint32_t readle32(const byte *p)
{
	return (((uint32_t)p[0]) <<  0)
	     | (((uint32_t)p[1]) <<  8)
	     | (((uint32_t)p[2]) << 16)
	     | (((uint32_t)p[3]) << 24);
}
