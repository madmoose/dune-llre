#include <algorithm>
#include <atomic>
#include <cassert>
#include <chrono>
#include <cstdio>
#include <thread>

#include <fcntl.h>
#include <unistd.h>

#include "common/ptr_offset_t.h"
#include "platform.h"

#include "dune.h"

using namespace std::chrono_literals;

constexpr bool dump_resources = false;

#define FUNC_NAME printf("\e[7m=== %s:%d ===\e[m\n", __func__, __LINE__)
#define TODO printf("\e[31;7m=== TODO: %s:%d ===\e[m\n", __func__, __LINE__)

void hexdump(const byte *p, int len, int offset = 0)
{
	for (int i = 0; i != len; i += std::min(len - i, 16)) {
		printf("%04x: ", i + offset);
		for (int j = i; j != i + 16; ++j) {
			if (j % 16 == 8) {
				printf(" ");
			}
			if (j < len) {
				printf("%02x ", p[j]);
			} else {
				printf("   ");
			}
		}
		for (int j = i; j != std::min(len, i + 16); ++j) {
			printf("%c", isprint(p[j]) ? p[j] : '.');
		}
		printf("\n");
	}
}

inline int8_t uint8_as_int8(uint8_t u)
{
	return u < INT8_MAX ? u : u - UINT8_MAX - 1;
}

inline int16_t uint16_as_int16(uint16_t u)
{
	return u < INT16_MAX ? u : u - UINT16_MAX - 1;
}

App *g_app;

#define PALACE_OUTSIDE 0x2001
#define PALACE_EQUIPMENT_ROOM 0x2002
#define PALACE_GREENHOUSE 0x2003
#define PALACE_CONFERENCE_ROOM 0x2004
#define PALACE_BALCONY 0x2005
#define PALACE_WEAPONS_ROOM 0x2006
#define PALACE_CORRIDOR 0x2007
#define PALACE_COMMUNICATIONS_ROOM 0x2008
#define PALACE_BED_ROOM 0x2009
#define PALACE_THRONE_ROOM 0x200a
#define PALACE_EMPTY_ROOM 0x200b
#define PALACE_COMMUNICATIONS_ROOM_CORRIDOR 0x200c

#pragma mark Data Section
/*
 * Data Segment
 */

std::atomic_bool quitting = false;
std::thread *timer_thread = nullptr;
#include <resources.h>

uint16_t cs_c21a_sprite_pal_offset = 0;

uint16_t ds_0000_rand_bits = 0;
uint16_t ds_0002_game_time = 2;
uint16_t ds_0004 = PALACE_THRONE_ROOM;
uint16_t ds_0006 = 0x0180;
uint16_t ds_0008_location = 0x0020;
uint16_t ds_0012 = 0;
byte ds_00e8_ui_head_index = 0;
int8_t ds_00ea = 0;
int8_t ds_00fb = -1;

byte ds_0100[][28] = {{0x02, 0x01, 0x15, 0x19, 0xFC, 0xFF, 0x00, 0x00, 0x20, 0x00, 0x10, 0xFF, 0x00, 0x00,
                       0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
                      {0x01, 0x02, 0x98, 0x3A, 0xC2, 0xFF, 0x00, 0x00, 0x30, 0x43, 0x00, 0xFF, 0x00, 0x00,
                       0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x01, 0x00, 0x00, 0x00},
                      {0x01, 0x03, 0xFE, 0x2A, 0xCA, 0xFF, 0x00, 0x00, 0x28, 0x29, 0x80, 0xFF, 0x00, 0x00,
                       0x00, 0x00, 0x00, 0x00, 0x78, 0x00, 0x02, 0x02, 0x03, 0x03, 0x03, 0x03, 0x00, 0x00},
                      {0x01, 0x04, 0x1D, 0x3E, 0xD0, 0xFF, 0x00, 0x00, 0x2B, 0x1E, 0x80, 0xFF, 0x00, 0x00,
                       0x00, 0x00, 0x00, 0x00, 0x64, 0x00, 0x02, 0x03, 0x03, 0x03, 0x03, 0x03, 0x00, 0x00},
                      {0x01, 0x05, 0xAE, 0x27, 0xBD, 0xFF, 0x00, 0x00, 0x29, 0x35, 0x80, 0xFF, 0x00, 0x00,
                       0x00, 0x00, 0x00, 0x00, 0x7D, 0x00, 0x01, 0x02, 0x00, 0x01, 0x01, 0x01, 0x00, 0x00},
                      {0x01, 0x06, 0x37, 0x1B, 0xE0, 0xFF, 0x00, 0x00, 0x2D, 0x24, 0x80, 0xFF, 0x00, 0x00,
                       0x00, 0x00, 0x00, 0x00, 0xC8, 0x00, 0x02, 0x02, 0x00, 0x03, 0x01, 0x01, 0x00, 0x00},
                      {0x01, 0x07, 0xD0, 0x26, 0xD5, 0xFF, 0x00, 0x00, 0x2C, 0x21, 0x80, 0xFF, 0x00, 0x00,
                       0x00, 0x00, 0x00, 0x00, 0xB4, 0x00, 0x02, 0x03, 0x03, 0x03, 0x02, 0x00, 0x00, 0x00},
                      {0x01, 0x08, 0x0C, 0x33, 0xD9, 0xFF, 0x00, 0x00, 0x2E, 0x1B, 0x80, 0xFF, 0x00, 0x00,
                       0x00, 0x00, 0x00, 0x00, 0x2D, 0x00, 0x02, 0x02, 0x03, 0x03, 0x03, 0x00, 0x00, 0x00},
                      {0x01, 0x09, 0x89, 0x16, 0xCF, 0xFF, 0x00, 0x00, 0x2A, 0x27, 0x80, 0xFF, 0x00, 0x00,
                       0x00, 0x00, 0x00, 0x00, 0x8C, 0x00, 0x02, 0x02, 0x00, 0x02, 0x02, 0x00, 0x00, 0x00},
                      {0x01, 0x0A, 0x19, 0x3C, 0xD8, 0xFF, 0x00, 0x00, 0x21, 0x00, 0x80, 0x50, 0x00, 0x00,
                       0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
                      {0x02, 0x03, 0x48, 0x31, 0xEB, 0xFF, 0x00, 0x00, 0x02, 0x08, 0x80, 0x04, 0x00, 0x00,
                       0x00, 0x00, 0x00, 0x00, 0x78, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
                      {0x02, 0x04, 0x4E, 0x25, 0xFC, 0xFF, 0x00, 0x00, 0x01, 0x03, 0x40, 0x00, 0x00, 0x00,
                       0x00, 0x00, 0x00, 0x00, 0x3C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
                      {0x02, 0x05, 0x48, 0x26, 0x0A, 0x00, 0x00, 0x00, 0x00, 0x01, 0x40, 0x00, 0x00, 0x00,
                       0x00, 0x00, 0x00, 0x00, 0x54, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
                      {0x02, 0x06, 0x1E, 0x21, 0xED, 0xFF, 0x00, 0x00, 0x03, 0x02, 0x40, 0x00, 0x00, 0x00,
                       0x00, 0x00, 0x00, 0x00, 0x2D, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
                      {0x02, 0x07, 0x30, 0x11, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x3C, 0x00, 0x00,
                       0x00, 0x00, 0x00, 0x00, 0x8C, 0x00, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0A},
                      {0x03, 0x03, 0xC8, 0x32, 0x08, 0x00, 0x00, 0x00, 0x08, 0x05, 0xE0, 0x02, 0x00, 0x00,
                       0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x14},
                      {0x03, 0x04, 0x9A, 0x3D, 0x16, 0x00, 0x00, 0x00, 0x07, 0x00, 0x80, 0x11, 0x00, 0x00,
                       0x00, 0x00, 0x00, 0x00, 0xA0, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
                      {0x03, 0x05, 0x2F, 0x4D, 0xFC, 0xFF, 0x00, 0x00, 0x04, 0x09, 0x80, 0x04, 0x00, 0x00,
                       0x00, 0x00, 0x00, 0x00, 0xB4, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
                      {0x03, 0x06, 0x54, 0x3D, 0xEF, 0xFF, 0x00, 0x00, 0x03, 0x00, 0x80, 0x0C, 0x00, 0x00,
                       0x00, 0x00, 0x00, 0x00, 0xA0, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
                      {0x03, 0x07, 0x78, 0x46, 0x08, 0x00, 0x00, 0x00, 0x06, 0x00, 0x80, 0x10, 0x00, 0x00,
                       0x00, 0x00, 0x00, 0x00, 0xD2, 0x00, 0x01, 0x02, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00},
                      {0x03, 0x0A, 0x78, 0x50, 0x0F, 0x00, 0x00, 0x00, 0x21, 0x00, 0x80, 0x38, 0x00, 0x00,
                       0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
                      {0x04, 0x03, 0xC7, 0x31, 0x30, 0x00, 0x00, 0x00, 0x05, 0x07, 0x80, 0x14, 0x00, 0x00,
                       0x00, 0x00, 0x00, 0x00, 0xC8, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
                      {0x04, 0x04, 0x8D, 0x2C, 0x18, 0x00, 0x00, 0x00, 0x04, 0x04, 0x80, 0x14, 0x00, 0x00,
                       0x00, 0x00, 0x00, 0x00, 0xAA, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
                      {0x04, 0x05, 0xB1, 0x31, 0x23, 0x00, 0x00, 0x00, 0x07, 0x06, 0x80, 0x14, 0x00, 0x00,
                       0x00, 0x00, 0x00, 0x00, 0xF0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
                      {0x04, 0x07, 0xA8, 0x16, 0x1B, 0x00, 0x00, 0x00, 0x03, 0x00, 0x80, 0x16, 0x00, 0x00,
                       0x00, 0x00, 0x00, 0x00, 0xC8, 0x00, 0x01, 0x03, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00},
                      {0x04, 0x06, 0xD1, 0x0F, 0x46, 0x00, 0x00, 0x00, 0x09, 0x0A, 0xA0, 0x48, 0x00, 0x00,
                       0x00, 0x00, 0x00, 0x00, 0xF0, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xC8},
                      {0x05, 0x03, 0xF7, 0xDE, 0x17, 0x00, 0x00, 0x00, 0x05, 0x00, 0x80, 0x44, 0x00, 0x00,
                       0x00, 0x00, 0x00, 0x00, 0xB4, 0x00, 0x02, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00},
                      {0x05, 0x04, 0x52, 0xF7, 0x1E, 0x00, 0x00, 0x00, 0x02, 0x0B, 0x80, 0x48, 0x00, 0x00,
                       0x00, 0x00, 0x00, 0x00, 0xA0, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
                      {0x05, 0x05, 0xCA, 0xE6, 0x0A, 0x00, 0x00, 0x00, 0x05, 0x0C, 0x80, 0x48, 0x00, 0x00,
                       0x00, 0x00, 0x00, 0x00, 0x64, 0x00, 0x00, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00},
                      {0x05, 0x06, 0x9E, 0x02, 0x0D, 0x00, 0x00, 0x00, 0x07, 0x00, 0x80, 0x28, 0x00, 0x00,
                       0x00, 0x00, 0x00, 0x00, 0xFA, 0x00, 0x01, 0x01, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00},
                      {0x05, 0x0A, 0x75, 0x10, 0x12, 0x00, 0x00, 0x00, 0x21, 0x00, 0x80, 0x38, 0x00, 0x00,
                       0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
                      {0x06, 0x03, 0xF3, 0x07, 0xD8, 0xFF, 0x00, 0x00, 0x28, 0x2C, 0x80, 0xFF, 0x00, 0x00,
                       0x00, 0x00, 0x00, 0x00, 0x96, 0x00, 0x02, 0x03, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00},
                      {0x06, 0x04, 0xF0, 0xF0, 0xDD, 0xFF, 0x00, 0x00, 0x29, 0x2F, 0x80, 0xFF, 0x00, 0x00,
                       0x00, 0x00, 0x00, 0x00, 0x20, 0x00, 0x01, 0x00, 0x03, 0x03, 0x00, 0x00, 0x00, 0x00},
                      {0x06, 0x05, 0xFB, 0x06, 0xEB, 0xFF, 0x00, 0x00, 0x01, 0x00, 0x80, 0x50, 0x00, 0x00,
                       0x00, 0x00, 0x00, 0x00, 0x82, 0x00, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00},
                      {0x06, 0x06, 0x27, 0xE2, 0xF0, 0xFF, 0x00, 0x00, 0x00, 0x0D, 0x80, 0x50, 0x00, 0x00,
                       0x00, 0x00, 0x00, 0x00, 0x78, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00},
                      {0x06, 0x07, 0x29, 0xF9, 0xCA, 0xFF, 0x00, 0x00, 0x2A, 0x32, 0x80, 0xFF, 0x00, 0x00,
                       0x00, 0x00, 0x00, 0x00, 0xB4, 0x00, 0x02, 0x02, 0x02, 0x02, 0x02, 0x00, 0x00, 0x00},
                      {0x06, 0x08, 0x7C, 0xF6, 0xEB, 0xFF, 0x00, 0x00, 0x06, 0x00, 0x80, 0x3C, 0x00, 0x00,
                       0x00, 0x00, 0x00, 0x00, 0xB4, 0x00, 0x01, 0x01, 0x00, 0x02, 0x01, 0x00, 0x00, 0x00},
                      {0x06, 0x09, 0x46, 0x18, 0xC4, 0xFF, 0x00, 0x00, 0x2B, 0x34, 0x80, 0xFF, 0x00, 0x00,
                       0x00, 0x00, 0x00, 0x00, 0xD2, 0x00, 0x02, 0x02, 0x01, 0x01, 0x01, 0x00, 0x00, 0x00},
                      {0x06, 0x0A, 0x2F, 0xF1, 0xF5, 0xFF, 0x00, 0x00, 0x21, 0x00, 0x80, 0x47, 0x00, 0x00,
                       0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
                      {0x06, 0x0B, 0xCC, 0x0C, 0xB7, 0xFF, 0x00, 0x00, 0x2C, 0x36, 0x80, 0xFF, 0x00, 0x00,
                       0x00, 0x00, 0x00, 0x00, 0x78, 0x00, 0x00, 0x00, 0x01, 0x01, 0x01, 0x00, 0x00, 0x00},
                      {0x07, 0x03, 0x00, 0xE0, 0xC1, 0xFF, 0x00, 0x00, 0x2D, 0x37, 0x80, 0xFF, 0x00, 0x00,
                       0x00, 0x00, 0x00, 0x00, 0xFA, 0x00, 0x00, 0x00, 0x02, 0x02, 0x00, 0x00, 0x00, 0x00},
                      {0x07, 0x04, 0xE3, 0xD8, 0xD0, 0xFF, 0x00, 0x00, 0x2E, 0x39, 0x80, 0xFF, 0x00, 0x00,
                       0x00, 0x00, 0x00, 0x00, 0xB4, 0x00, 0x00, 0x00, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00},
                      {0x07, 0x05, 0x4F, 0xAD, 0xC3, 0xFF, 0x00, 0x00, 0x2F, 0x3B, 0x80, 0xFF, 0x00, 0x00,
                       0x00, 0x00, 0x00, 0x00, 0x78, 0x00, 0x00, 0x00, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00},
                      {0x07, 0x06, 0x6C, 0xC5, 0xC5, 0xFF, 0x00, 0x00, 0x28, 0x3A, 0x80, 0xFF, 0x00, 0x00,
                       0x00, 0x00, 0x00, 0x00, 0xC8, 0x00, 0x00, 0x00, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00},
                      {0x08, 0x03, 0x61, 0xB8, 0xDC, 0xFF, 0x00, 0x00, 0x07, 0x11, 0xA0, 0x2C, 0x00, 0x00,
                       0x00, 0x00, 0x00, 0x00, 0x96, 0x00, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x50},
                      {0x08, 0x04, 0x7F, 0xC4, 0xE8, 0xFF, 0x00, 0x00, 0x04, 0x0F, 0x80, 0x28, 0x00, 0x00,
                       0x00, 0x00, 0x00, 0x00, 0x6E, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00},
                      {0x08, 0x05, 0x74, 0x96, 0xDE, 0xFF, 0x00, 0x00, 0x05, 0x10, 0x80, 0x2C, 0x00, 0x00,
                       0x00, 0x00, 0x00, 0x00, 0x3C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
                      {0x08, 0x06, 0xC6, 0xA7, 0xE2, 0xFF, 0x00, 0x00, 0x06, 0x12, 0x80, 0x2C, 0x00, 0x00,
                       0x00, 0x00, 0x00, 0x00, 0xB4, 0x00, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00},
                      {0x08, 0x07, 0x70, 0xB1, 0xEB, 0xFF, 0x00, 0x00, 0x01, 0x0E, 0x80, 0x2C, 0x00, 0x00,
                       0x00, 0x00, 0x00, 0x00, 0x96, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00},
                      {0x08, 0x08, 0xF1, 0xAA, 0xD5, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x80, 0x2C, 0x00, 0x00,
                       0x00, 0x00, 0x00, 0x00, 0xA0, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00},
                      {0x09, 0x03, 0xE2, 0x6A, 0xF1, 0xFF, 0x00, 0x00, 0x0B, 0x13, 0xA0, 0x5A, 0x00, 0x00,
                       0x00, 0x00, 0x00, 0x00, 0xAA, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x28},
                      {0x09, 0x04, 0x32, 0x85, 0xF6, 0xFF, 0x00, 0x00, 0x0A, 0x14, 0xA0, 0x5A, 0x00, 0x00,
                       0x00, 0x00, 0x00, 0x00, 0xD2, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0A},
                      {0x09, 0x05, 0x00, 0x70, 0xE1, 0xFF, 0x00, 0x00, 0x29, 0x42, 0x80, 0xFF, 0x00, 0x00,
                       0x00, 0x00, 0x00, 0x00, 0xC3, 0x00, 0x01, 0x01, 0x00, 0x01, 0x01, 0x00, 0x00, 0x00},
                      {0x09, 0x06, 0x18, 0x5C, 0xDA, 0xFF, 0x00, 0x00, 0x2A, 0x3F, 0x80, 0xFF, 0x00, 0x00,
                       0x00, 0x00, 0x00, 0x00, 0xF0, 0x00, 0x02, 0x02, 0x02, 0x02, 0x02, 0x00, 0x00, 0x00},
                      {0x09, 0x07, 0xE3, 0x68, 0xD0, 0xFF, 0x00, 0x00, 0x2B, 0x3E, 0x80, 0xFF, 0x00, 0x00,
                       0x00, 0x00, 0x00, 0x00, 0xC8, 0x00, 0x01, 0x01, 0x00, 0x01, 0x01, 0x01, 0x00, 0x00},
                      {0x09, 0x08, 0x14, 0x67, 0xC3, 0xFF, 0x00, 0x00, 0x2C, 0x3D, 0x80, 0xFF, 0x00, 0x00,
                       0x00, 0x00, 0x00, 0x00, 0xD2, 0x00, 0x02, 0x01, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00},
                      {0x09, 0x09, 0xBE, 0x7C, 0xC5, 0xFF, 0x00, 0x00, 0x2D, 0x3C, 0x80, 0xFF, 0x00, 0x00,
                       0x00, 0x00, 0x00, 0x00, 0x32, 0x00, 0x01, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00},
                      {0x09, 0x0A, 0xF1, 0x82, 0xDD, 0xFF, 0x00, 0x00, 0x21, 0x00, 0x80, 0x5A, 0x00, 0x00,
                       0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
                      {0x0A, 0x03, 0xB5, 0x75, 0x05, 0x00, 0x00, 0x00, 0x03, 0x00, 0x80, 0x5A, 0x00, 0x00,
                       0x00, 0x00, 0x00, 0x00, 0xE6, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
                      {0x0A, 0x04, 0x95, 0x94, 0x02, 0x00, 0x00, 0x00, 0x0D, 0x00, 0xA0, 0x5A, 0x00, 0x00,
                       0x00, 0x00, 0x00, 0x00, 0xC8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xA0},
                      {0x0B, 0x03, 0x4C, 0x91, 0x18, 0x00, 0x00, 0x00, 0x08, 0x17, 0xA0, 0x58, 0x00, 0x00,
                       0x00, 0x00, 0x00, 0x00, 0xAA, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x64},
                      {0x0B, 0x04, 0xF3, 0xBC, 0x2F, 0x00, 0x00, 0x00, 0x0A, 0x19, 0xA0, 0x58, 0x00, 0x00,
                       0x00, 0x00, 0x00, 0x00, 0xC8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xC8},
                      {0x0B, 0x05, 0xDF, 0xC7, 0x16, 0x00, 0x00, 0x00, 0x10, 0x00, 0xA0, 0x55, 0x00, 0x00,
                       0x00, 0x00, 0x00, 0x00, 0x8C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x6E},
                      {0x0B, 0x06, 0xB0, 0xBB, 0x1F, 0x00, 0x00, 0x00, 0x0B, 0x18, 0xA0, 0x58, 0x00, 0x00,
                       0x00, 0x00, 0x00, 0x00, 0xAA, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x69},
                      {0x0B, 0x07, 0xDE, 0xCE, 0xF9, 0xFF, 0x00, 0x00, 0x03, 0x00, 0xA0, 0x28, 0x00, 0x00,
                       0x00, 0x00, 0x00, 0x00, 0x8C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x78},
                      {0x0B, 0x0A, 0xAE, 0xB2, 0xF5, 0xFF, 0x00, 0x00, 0x21, 0x00, 0x80, 0x58, 0x00, 0x00,
                       0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
                      {0x0C, 0x03, 0xB6, 0x5D, 0x24, 0x00, 0x00, 0x00, 0x0E, 0x15, 0xA0, 0x5A, 0x00, 0x00,
                       0x00, 0x00, 0x00, 0x00, 0xAA, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x64},
                      {0x0C, 0x04, 0x93, 0x71, 0x31, 0x00, 0x00, 0x00, 0x0F, 0x16, 0xA0, 0x58, 0x00, 0x00,
                       0x00, 0x00, 0x00, 0x00, 0xF0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xC8},
                      {0x0C, 0x05, 0xF5, 0x41, 0x27, 0x00, 0x00, 0x00, 0x03, 0x00, 0x80, 0xFF, 0x00, 0x00,
                       0x00, 0x00, 0x00, 0x00, 0x8C, 0x00, 0x01, 0x01, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00},
                      {0x0C, 0x06, 0x00, 0xC0, 0x47, 0x00, 0x00, 0x00, 0x0C, 0x1A, 0xA0, 0xFF, 0x00, 0x00,
                       0x00, 0x00, 0x00, 0x00, 0xF0, 0x00, 0x00, 0x00, 0x02, 0x01, 0x00, 0x00, 0x00, 0x96},
                      {0xFF, 0xFF}};
uint16_t ds_114e = 0xffff;

uint8_t ds_11c9 = 0;

sprite_position ds_11dd[] = {{49, 0, 76}, {1, 0, 134}, {0xffff, 0, 0}};

sprite_position ds_120b_palplan_sprite_list[] = {{0, 182, 12},
                                                 // {      3, 266,  65 },
                                                 // {      4, 238,  65 },
                                                 // {      5, 193,  65 },
                                                 {0xffff, 0, 0}};

byte ds_1225[] = {
	//    N     E     S     W
	0x4C, 0x02, 0x00, 0xFD, 0x00, //  1. Outside
	0x3A, 0x07, 0x00, 0x01, 0x8C, //  2. Equipment room
	0xCF, 0x00, 0x00, 0x00, 0x0B, //  3. Greenhouse
	0x62, 0x0A, 0x00, 0x07, 0x00, //  4. Conference room
	0x4B, 0x00, 0x00, 0x00, 0x0A, //  5. Balcony
	0x15, 0x0B, 0x00, 0x00, 0x00, //  6. Weapons room
	0x5D, 0x04, 0x8B, 0x02, 0x88, //  7. Corridor
	0x26, 0x00, 0x87, 0x0C, 0x00, //  8. Communications room
	0x63, 0x00, 0x00, 0x0A, 0x00, //  9. Bed room
	0x61, 0x09, 0x05, 0x04, 0x00, // 10. Throne room
	0x64, 0x00, 0x83, 0x06, 0x07, // 11. Empty room
	0x5E, 0x08, 0x02, 0x00, 0x00  // 12. Communications room corridor
};
byte ds_1261[] = {0x01, 0x02, 0xFE, 0xFD, 0xFC, 0x72, 0x00, 0x00, 0x01, 0x00};
byte ds_126b[] = {0x01, 0x02, 0xFE, 0xFD, 0xFC, 0x73, 0x00, 0x00, 0x01, 0x00};
byte ds_1275[] = {0x01, 0x02, 0xFE, 0xFD, 0xFC, 0x74, 0x00, 0x00, 0x01, 0x00};
byte ds_127f[] = {0x01, 0x02, 0xFE, 0xFD, 0xFC, 0x75, 0x00, 0x00, 0x01, 0x00};
byte ds_1289[] = {0x01, 0x02, 0xFE, 0xFD, 0xFC, 0x76, 0x00, 0x00, 0x01, 0x00};
byte ds_1293[] = {0x01, 0x02, 0xFE, 0xFD, 0xFC, 0x77, 0x03, 0x00, 0x01, 0x00, 0x72, 0x00, 0x00, 0x02, 0x00};
byte ds_12a2[] = {0x01, 0x03, 0xFE, 0xFD, 0xFC, 0x78, 0x00, 0x00, 0x03, 0x00, 0x7B, 0x02, 0x00, 0x01, 0x00};
byte ds_12b1[] = {0x01, 0x02, 0xFE, 0xFD, 0xFC, 0x7A, 0x03, 0x00, 0x01, 0x00, 0x72, 0x00, 0x00, 0x02, 0x00};
byte ds_12c0[] = {0x01, 0x03, 0xFE, 0xFD, 0xFC, 0x79, 0x04, 0x00, 0x03, 0x00,
                  0x7B, 0x02, 0x00, 0x01, 0x00, 0x7D, 0x00, 0x00, 0x02, 0x00};
byte ds_12d4[] = {0x01, 0x03, 0xFE, 0xFD, 0xFC, 0x7C, 0x04, 0x00, 0x03, 0x00,
                  0x7B, 0x02, 0x00, 0x01, 0x00, 0x7D, 0x00, 0x00, 0x02, 0x00};
byte ds_12e8[] = {0x01, 0x03, 0xFE, 0xFD, 0xFC, 0x74, 0x04, 0x00, 0x03, 0x00,
                  0x7B, 0x02, 0x00, 0x01, 0x00, 0x7D, 0x00, 0x00, 0x02, 0x00};
byte ds_12fc[] = {0x01, 0x02, 0xFE, 0xFD, 0xFC, 0x75, 0x04, 0x03, 0x01, 0x00,
                  0x72, 0x00, 0x00, 0x00, 0x02, 0x7D, 0x00, 0x00, 0x02, 0x00};
byte ds_1310[] = {0x01, 0x02, 0xFE, 0xFD, 0xFC, 0x76, 0x03, 0x00, 0x01, 0x00,
                  0x73, 0x04, 0x00, 0x02, 0x00, 0x7D, 0x00, 0x00, 0x03, 0x00};
byte ds_1324[] = {0x01, 0x02, 0xFE, 0xFD, 0xFC, 0x77, 0x03, 0x04, 0x01, 0x00,
                  0x72, 0x00, 0x00, 0x02, 0x00, 0x7D, 0x00, 0x00, 0x00, 0x02};
byte ds_1338[] = {0x01, 0x03, 0xFE, 0xFD, 0xFC, 0x78, 0x04, 0x00, 0x03, 0x00,
                  0x7B, 0x02, 0x00, 0x01, 0x00, 0x7D, 0x00, 0x00, 0x02, 0x00};
byte ds_134c[] = {0x01, 0x02, 0xFE, 0xFD, 0xFC, 0x7A, 0x03, 0x04, 0x01, 0x00,
                  0x72, 0x00, 0x00, 0x02, 0x00, 0x7D, 0x00, 0x00, 0x00, 0x02};
byte ds_1360[] = {0x01, 0x03, 0xFE, 0xFD, 0xFC, 0x7C, 0x05, 0x00, 0x03, 0x00, 0x7B, 0x02, 0x00,
                  0x01, 0x00, 0x7D, 0x00, 0x05, 0x00, 0x00, 0xDE, 0x00, 0x00, 0x02, 0x04};
byte ds_1379[] = {0x01, 0xFF, 0xFE, 0xFD, 0xFC};
byte ds_137e[] = {0x01, 0x02, 0xFE, 0xFD, 0xFC, 0xA7, 0x00, 0x03, 0x01, 0x00, 0xA6, 0x00, 0x00, 0x00, 0x02};
byte ds_138d[] = {0x02, 0x02, 0xFE, 0xFD, 0xFC, 0xA7, 0x00, 0x00, 0x01, 0x03, 0xA6, 0x00, 0x02, 0x00, 0x00};
byte ds_139c[] = {0x03, 0x02, 0xFE, 0xFD, 0xFC, 0xA7, 0x00, 0x03, 0x01, 0x00, 0xA6, 0x00, 0x00, 0x00, 0x02};
byte ds_13ab[] = {0x04, 0x02, 0xFE, 0xFD, 0xFC, 0xA7, 0x00, 0x00, 0x01, 0x03, 0xA6, 0x00, 0x02, 0x00, 0x00};
byte ds_13ba[] = {0x05, 0x02, 0xFE, 0xFD, 0xFC, 0xA8, 0x00, 0x00, 0x01, 0x00};

byte *ds_13c4[] = {
	ds_1261, //  0.
	ds_126b, //  1.
	ds_1275, //  2.
	ds_127f, //  3.
	ds_1289, //  4.
	ds_1293, //  5.
	ds_12a2, //  6.
	ds_12b1, //  7.
	ds_12c0, //  8.
	ds_12d4, //  9.
	ds_12e8, // 10.
	ds_12fc, // 11.
	ds_1310, // 12.
	ds_1324, // 13.
	ds_1338, // 14.
	ds_134c, // 15.
	ds_1360, // 16.
	ds_126b, // 17.
	ds_1275, // 18.
	ds_127f, // 19.
	ds_1289, // 20.
	ds_1293, // 21.
	ds_12a2, // 22.
	ds_12b1, // 23.
	ds_12c0, // 24.
	ds_12d4, // 25.
	ds_12e8, // 26.
	ds_12fc, // 27.
	ds_1310, // 28.
	ds_1324, // 29.
	ds_1338, // 30.
	ds_134c, // 31.
	ds_1225, // 32. Palace
	ds_1379, // 33.
	ds_1379, // 34.
	ds_1379, // 35.
	ds_1379, // 36.
	ds_1379, // 37.
	ds_1379, // 38.
	ds_1379, // 39.
	ds_137e, // 40.
	ds_138d, // 41.
	ds_139c, // 42.
	ds_13ab, // 43.
	ds_137e, // 44.
	ds_138d, // 45.
	ds_139c, // 46.
	ds_13ab, // 47.
	ds_13ba  // 48.
};

byte ds_144c = -1;

rect_t ds_1470_game_area_rect = {0, 0, 320, 152};

sprite_position ds_14c8_sprite_list[]{{18, 156, 100}, {19, 84, 92}, {0x4000 | 19, 221, 92}, {0xffff, 0, 0}};

sprite_position ds_1500_sprite_list[] = {{2, 0, 0},    {3, 0, 25},  {4, 0, 50},    {5, 0, 74},
                                         {6, 134, 92}, {0, 0, 102}, {0xffff, 0, 0}};

sprite_position ds_1526_sprite_list[] = {{0, 0, 0}, {1, 52, 25}, {2, 108, 51}, {0xffff, 0, 0}};

sprite_position ds_153a_sprite_list[] = {{0x4004, 0, 0}, {4, 236, 0}, {3, 83, 0}, {0xffff, 0, 0}};

sprite_position ds_154e_sprite_list[] = {{0x4004, 0, 0}, {4, 236, 0}, {0xffff, 0, 0}};

sprite_position ds_155c_sprite_list[] = {{5, -58, 12}, {6, -58, 12}, {5, -5, 17},   {6, -5, 17},
                                         {7, 204, 14}, {8, 204, 14}, {0xffff, 0, 0}};

rect_t ds_1582 = {0, 0, 320, 81};
rect_t ds_158a = {0, 81, 320, 134};

uint16_t ds_1954 = 0;

uint16_t ds_1ae4_ui_elements_len = 24;
// clang-format off
ui_element_t ds_1ae6_ui_elements[24] = {
	{  22, 161,  68, 196,    0, -1, 0xb8c6 }, //  0
	{   0, 152,   0, 152,    0,  0, 0x0f66 }, //  1 Bottom left corner book
	{ 228, 152, 300, 198,    0,  3, 0x0f66 }, //  2 Bottom right corner aliens
	{  24, 155,  69, 176, 0x80, -1, 0xaed6 }, //  3

	{  92, 152, 229, 159,    0, 14, 0x0f66 }, //  4 Arms
	{   2, 154,   2, 154,    0, -1, 0x0f66 }, //  5 Gems
	{ 317, 154, 317, 154,    0, -1, 0x0f66 }, //  6 Gems
	{  92, 159, 228, 167, 0x80, 27, 0xd443 }, //  7 Action bars
	{  92, 167, 228, 175, 0x80, 27, 0xd43e }, //  8 Action bars
	{  92, 175, 228, 183, 0x80, 27, 0xd439 }, //  9 Action bars
	{  92, 183, 228, 191, 0x80, 27, 0xd434 }, // 10 Action bars
	{  92, 191, 228, 199, 0x80, 27, 0xd42f }, // 11 Action bars

	{ 255, 162, 295, 192,    0, -1, 0x0f66 }, // 12
	{ 269, 162, 279, 172,    0, -1, 0x0f66 }, // 13
	{ 284, 172, 294, 182,    0, -1, 0x0f66 }, // 14
	{ 269, 181, 279, 191,    0, -1, 0x0f66 }, // 15
	{ 255, 172, 265, 182,    0, -1, 0x0f66 }, // 16
	{   0,   0,   0,   0,    0, -1, 0x0f66 }, // 17
	{   0,   0,   0,   0,    0, -1, 0x0f66 }, // 18
	{   0,   0,   0,   0,    0, -1, 0x945b }, // 19 : 0x010a
	{   0,   0, 320, 152,    0, -1, 0x941d }, // 20
	{  35, 182,  56, 196, 0x80, 64, 0x9215 }, // 21
	{  58, 182,  79, 196, 0x80, 64, 0x9215 }, // 22
	{   0,   4,  40,  46,    0, -1, 0xb1ee }, // 23
};

ui_element_flag_and_sprite_t ds_1c36_frieze_closed_book[4] = {
	{    0, -1 },
	{    0,  0 }, // Bottom left corner book
	{    0,  3 }, // Bottom right corner aliens in square
	{ 0x80, -1 },
};

ui_element_flag_and_sprite_t ds_1c46_frieze_sides_globe[4] = {
	{ 0x40, 13 },
	{    0,  6 },
	{    0,  3 },
	{    0, -1 },
};

ui_element_flag_and_sprite_t ds_1c56_frieze_sides_open_book[4] = {
	{    0, -1 },
	{    0,  9 },
	{    0,  3 },
	{    0, -1 },
};

ui_element_flag_and_sprite_t ds_1c66_frieze_sides_map[4] = {
	{ 0xc0, 13 },
	{    0,  6 },
	{    0,  3 },
	{    0, -1 },
};

// Palace navigation arrows
ui_element_t ds_1c76_ui_elements[] = {
	{ 255, 162, 295, 192, 0x00, 33, 0x0000 }, // offset nullsub_1
	{ 269, 162, 279, 172, 0x00, 29, 0x0000 }, // offset loc_13F15_up
	{ 284, 172, 294, 182, 0x80, 30, 0x0000 }, // offset sub_13F1A_right
	{ 269, 181, 279, 191, 0x80, 31, 0x0000 }, // offset sub_13F1F_down
	{ 255, 172, 265, 182, 0x80, 32, 0x0000 }, // offset sub_13F24_left
	{ 269, 173, 280, 181, 0x80, 36, 0x0000 }, // offset _sub_118EE_ui_draw_palace_plan
};

// Map navigation arrows
ui_element_t ds_1cca_ui_elements[] = {
	{ 266, 171, 285, 184,   0x80, 41, 0x0000 }, // offset sub_15B05
	{ 267, 162, 284, 171, 0x4080, 37, 0x0000 }, // offset _sub_18829_ui_click_map_up
	{ 285, 171, 297, 184, 0x4080, 38, 0x0000 }, // offset _sub_18824_ui_click_map_right
	{ 267, 184, 284, 193, 0x4080, 39, 0x0000 }, // offset _sub_1882E_ui_click_map_down
	{ 254, 171, 266, 184, 0x4080, 40, 0x0000 }, // offset _sub_1881F_ui_click_map_left
	{ 266, 171, 284, 183,    0x0, 53, 0x0000 }, // offset nullsub_1
};

ui_element_t ds_1dc6_ui_globe_rotation_controls[] = {
	{ 266, 171, 285, 184, 0x0080, 41, 0x0000 }, // offset loc_1BC81
	{  38, 159,  54, 172, 0x4080, 49, 0x0000 }, // offset sub_1B9B9
	{  54, 168,  72, 185, 0x4080, 50, 0x0000 }, // offset loc_1B9CC
	{  38, 183,  54, 199, 0x4080, 51, 0x0000 }, // offset loc_1B9C0
	{  20, 168,  37, 185, 0x4080, 52, 0x0000 }, // offset loc_1B9D3
	{  36, 172,  57, 182, 0x0080, 53, 0x0000 }, // offset sub_1BA9E
	// {   0,   0,   0,   0, 0x0000, -1, 0x0000 }, // offset nullsub_1
};

ui_element_t ds_1e6e[] = {
	{ 150, 137, 170, 160, 0x0096, 137 } // offset unk_100AA
};

pos_t ds_1e7e_sun_and_moon_positions[32] = {
	//  sun         moon
	{  6, 187 }, { 25, 186 },
	{  6, 186 }, { 26, 188 },
	{  6, 185 }, {  0,   0 },
	{  7, 183 }, {  0,   0 },
	{  9, 182 }, {  0,   0 },
	{ 10, 181 }, {  0,   0 },
	{ 13, 181 }, {  0,   0 },
	{ 16, 181 }, {  0,   0 },
	{ 18, 182 }, {  0,   0 },
	{ 20, 183 }, {  0,   0 },
	{ 20, 185 }, {  0,   0 },
	{ 20, 186 }, {  8, 188 },
	{ 20, 187 }, {  9, 186 },
	{  0,   0 }, { 12, 183 },
	{  0,   0 }, { 17, 182 },
	{  0,   0 }, { 23, 183 }
};

// clang-format on

rect_t ds_1efe_time_indicator_clip_rect = {6, 181, 30, 190};

byte ds_227d = 1;
uint16_t ds_22e3 = 1;
int16_t ds_22a6_talking_head_id = -1;

string_def_t ds_2482_strings[] = {{194, 16, 6, 0xf0fd}, {195, 216, 6, 0xf0fd}, {-1}};

string_def_t ds_2494_strings[] = {{196, 20, 69, 0xf03f},   {197, 48, 69, 0xf025},   {198, 8, 80, 0xf0fb},
                                  {199, 240, 60, 0xf03f},  {200, 272, 60, 0xf025},  {201, 236, 71, 0xf0fb},
                                  {202, 240, 131, 0xf03f}, {203, 272, 131, 0xf025}, {204, 236, 142, 0xf0fb},
                                  {175, 35, 125, 0xf025},  {176, 35, 139, 0xf03f},  {-1}};

struct {
	int x;
	int y;
} ds_24ee[12] = {
	{26, 62}, {54, 62}, {252, 54}, {280, 54}, {252, 125}, {280, 125},
};

sprite_position ds_2506[] = {{3, 11, 124}, {4, 11, 136}, {0xffff, 0, 0}};

struct {
	uint16_t unk0;
	uint16_t unk1;
	uint16_t unk2;
	uint16_t unk3;
} globe_rotation_lookup_table[100];
uint16_t ds_2460_globe_tilt_lookup_table[196];

void (*ds_2518_font_glyph_draw_func)(byte c);

// clang-format off
cursor_t ds_2584_cursor = {
	0, 0,
	{
		0b0011111111111111,
		0b0001111111111111,
		0b0000111111111111,
		0b0000011111111111,
		0b0000001111111111,
		0b0000000111111111,
		0b0000000011111111,
		0b0000000001111111,
		0b0000000000111111,
		0b0000000000111111,
		0b0000000111111111,
		0b0001000011111111,
		0b0011000011111111,
		0b1111100001111111,
		0b1111100001111111,
		0b1111110001111111,
	}, {
		0b0000000000000000,
		0b0100000000000000,
		0b0110000000000000,
		0b0111000000000000,
		0b0111100000000000,
		0b0111110000000000,
		0b0111111000000000,
		0b0111111100000000,
		0b0111111110000000,
		0b0111110000000000,
		0b0110110000000000,
		0b0100011000000000,
		0b0000011000000000,
		0b0000001100000000,
		0b0000001100000000,
		0b0000000000000000,
	}
};

cursor_t ds_25c8_cursor = {
	1, 0,
	{
		0b1100111111111111,
		0b1000001111111111,
		0b1000000111111111,
		0b1110000001111111,
		0b1111000000111111,
		0b1100000000000111,
		0b1100000000000011,
		0b1000000000000011,
		0b0000000000000001,
		0b0000000000000001,
		0b1000000000000000,
		0b1100000000000000,
		0b1110000000000000,
		0b1111000000000000,
		0b1111110000000000,
		0b1111111100000000,
	}, {
		0b0000000000000000,
		0b0011000000000000,
		0b0001110000000000,
		0b0000011000000000,
		0b0000001110000000,
		0b0000110100000000,
		0b0001011011111000,
		0b0001100111011000,
		0b0110110000111100,
		0b0011000010101100,
		0b0000001110111100,
		0b0001111111011110,
		0b0000111111111110,
		0b0000001110111110,
		0b0000000001111110,
		0b0000000001111110,
	}
};

cursor_t ds_260c_cursor = {
	4, 0,
	{
		0b1111101111111111,
		0b1111000111111111,
		0b1110000011111111,
		0b1100000001111111,
		0b1000000000111111,
		0b0000000000011111,
		0b0000000000011111,
		0b1110000011111111,
		0b1110000011111111,
		0b1111111111111111,
		0b1111111111111111,
		0b1111111111111111,
		0b1111111111111111,
		0b1111111111111111,
		0b1111111111111111,
		0b1111111111111111,
	}, {
		0b0000000000000000,
		0b0000010000000000,
		0b0000111000000000,
		0b0001111100000000,
		0b0011111110000000,
		0b0111111111000000,
		0b0000111000000000,
		0b0000111000000000,
		0b0000000000000000,
		0b0000000000000000,
		0b0000000000000000,
		0b0000000000000000,
		0b0000000000000000,
		0b0000000000000000,
		0b0000000000000000,
		0b0000000000000000,
	}
};

cursor_t ds_2650_cursor = {
	4, 2,
	{
		0b1100111111111111,
		0b1100011111111111,
		0b1100001111111111,
		0b0000000111111111,
		0b0000000011111111,
		0b0000000001111111,
		0b0000000011111111,
		0b0000000111111111,
		0b1100001111111111,
		0b1100011111111111,
		0b1100111111111111,
		0b1111111111111111,
		0b1111111111111111,
		0b1111111111111111,
		0b1111111111111111,
		0b1111111111111111,
	}, {
		0b0000000000000000,
		0b0001000000000000,
		0b0001100000000000,
		0b0001110000000000,
		0b0111111000000000,
		0b0111111100000000,
		0b0111111000000000,
		0b0001110000000000,
		0b0001100000000000,
		0b0001000000000000,
		0b0000000000000000,
		0b0000000000000000,
		0b0000000000000000,
		0b0000000000000000,
		0b0000000000000000,
		0b0000000000000000,
	}
};

cursor_t ds_2694_cursor = {
	4, 0,
	{
		0b1110000011111111,
		0b1110000011111111,
		0b0000000000011111,
		0b0000000000011111,
		0b1000000000111111,
		0b1100000001111111,
		0b1110000011111111,
		0b1111000111111111,
		0b1111101111111111,
		0b1111111111111111,
		0b1111111111111111,
		0b1111111111111111,
		0b1111111111111111,
		0b1111111111111111,
		0b1111111111111111,
		0b1111111111111111,
	}, {
		0b0000000000000000,
		0b0000111000000000,
		0b0000111000000000,
		0b0111111111000000,
		0b0011111110000000,
		0b0001111100000000,
		0b0000111000000000,
		0b0000010000000000,
		0b0000000000000000,
		0b0000000000000000,
		0b0000000000000000,
		0b0000000000000000,
		0b0000000000000000,
		0b0000000000000000,
		0b0000000000000000,
		0b0000000000000000,
	}
};

cursor_t ds_26d8_cursor = {
	5, 2,
	{
		0b1111100111111111,
		0b1111000111111111,
		0b1110000111111111,
		0b1100000001111111,
		0b1000000001111111,
		0b0000000001111111,
		0b1000000001111111,
		0b1100000001111111,
		0b1110000111111111,
		0b1111000111111111,
		0b1111100111111111,
		0b1111111111111111,
		0b1111111111111111,
		0b1111111111111111,
		0b1111111111111111,
		0b1111111111111111,
	}, {
		0b0000000000000000,
		0b0000010000000000,
		0b0000110000000000,
		0b0001110000000000,
		0b0011111100000000,
		0b0111111100000000,
		0b0011111100000000,
		0b0001110000000000,
		0b0000110000000000,
		0b0000010000000000,
		0b0000000000000000,
		0b0000000000000000,
		0b0000000000000000,
		0b0000000000000000,
		0b0000000000000000,
		0b0000000000000000,
	}
};

// clang-format on

int16_t ds_278e_active_bank_id = -1;

uint16_t ds_2886_settings_ui_global_x_offset = 40;
uint16_t ds_2888_settings_ui_global_y_offset = 1;

int ds_35a6_hnm_fd;
uint16_t ds_35a8_subtitle_frames_irulan[] = {
	119,  137,  138,  173,  186,  238,  248,  269,  270,  305,  314,  338,  348,       358,  360,  388,
	389,  415,  425,  460,  470,  518,  528,  571,  576,  604,  605,  659,  660,       685,  693,  744,
	746,  757,  761,  818,  827,  866,  875,  945,  950,  1000, 1012, 1042, 1044,      1075, 1085, 1119,
	1120, 1142, 1147, 1169, 1172, 1214, 1226, 1259, 1266, 1285, 1294, 1315, UINT16_MAX};
uint16_t *ds_3622_subtitle_next_frame;
byte *ds_39b7_alloc_next_addr;
byte ds_46d6 = 0;
byte ds_46d7 = 0;
byte ds_46df = 0;
byte ds_46d9 = 0;
byte ds_46f4;
uint16_t ds_46f8 = 0;
uint16_t ds_46fa = 0;
uint16_t ds_472d;
byte ds_4732;
uint16_t ds_479e;
byte ds_46eb;
byte *ds_47a0_font_width_table_ptr;
byte ds_47a4;
byte *ds_47b0_phrase_bin;
byte *ds_47ac_command_bin;
byte ds_46d8;
uint16_t ds_47c4;
uint16_t ds_477e_current_phrase_bin_resource_id;
ptr_offset_t ds_47ca_lip_sync_section_2_ptr;
ptr_offset_t ds_47cc_lip_sync_section_1_ptr;
byte ds_47d0;
ptr_offset_t ds_47d2_lip_sync_section_3_ptr;
Scene *ds_4854_intro_scene_current_scene;
ds_4856_t ds_4856 = {0, 0, 0, 0, 0, 0};
byte ds_4948_tablat_bin[792];
int16_t ds_494c_globe_rotation;
byte ds_4c60_globdata[64000];
byte *ds_bc6e_room_sheet_data;
byte *ds_ce68_alloc_last_addr;
uint16_t ds_ce70_res_index_needs_extended_memory;
byte ds_ce71_disable_hsq_detection;
uint16_t ds_ce74_framebuffer_size;
int ds_ce78_dat_resource;
byte ds_ceeb_language_setting;
byte font_bin[0x900];
std::atomic_uint16_t ds_ce7a_pit_timer_counter = 0;
std::atomic_uint16_t ds_ce7c_pit_timer_counter_hi = 0;
uint16_t ds_d820;
uint16_t ds_d824_rand_seed;
uint16_t ds_d826_rand_seed;
uint16_t ds_d82c_font_draw_position_x;
uint16_t ds_d82e_font_draw_position_y;
uint16_t ds_d830_font_draw_position_x_start;
uint16_t ds_d832_font_draw_position_y_start;
rect_t ds_d834_sprite_clip_rect;
ptr_offset_t ds_d844_resource_ptr[146];
uint16_t ds_dabc_resource_last_used[124];
ptr_offset_t ds_dbb0_current_resource_ptr;
byte ds_dbb4_last_bank_palette = -1;
byte ds_dbb5_hnm_flag;
int ds_dbba_dat_fd;
byte *ds_dbbc_dat;
byte *ds_dbd6_framebuffer_1;
byte *ds_dbd8_framebuffer_screen;
byte *ds_dbda_framebuffer_active;
byte *ds_dbdc_framebuffer_2;
uint16_t ds_dbe4_font_draw_color;
uint8_t ds_dbe7_hnm_finished_flag;
uint16_t ds_dbe8_hnm_frame_counter;
uint16_t ds_dbea_hnm_frame_counter_2;
uint16_t ds_dbec_hnm_frame_counter_3;
uint16_t ds_dbee_hnm_frame_counter_4;
uint32_t ds_dbf6_hnm_unknown_offset_1;
uint32_t ds_dbfa_hnm_unknown_offset_2;
uint16_t ds_dbfe_hnm_resource_flag;
uint16_t ds_dc00_hnm_id;
uint16_t ds_dc02_hnm_open_id;
uint32_t ds_dc04_hnm_file_offset;
uint32_t ds_dc08_hnm_file_remain;
ptr_offset_t ds_dc0c_hnm_read_buffer;
ptr_offset_t ds_dc10_hnm_read_buffer;
ptr_offset_t ds_dc14_hnm_decode_buffer;
uint16_t ds_dc18_hnm_framebuffer_size;
uint16_t ds_dc22_hnm_load_time;
uint16_t ds_dc24_hnm_decode_frame_tag;
uint16_t ds_dc1a_hnm_read_offset;
uint16_t ds_dc1c_hnm_sd_block_ofs;
uint16_t ds_dc1e_hnm_pl_block_ofs;
uint16_t ds_dc20_hnm_block_size;
byte *ds_dc32_hnm_buffer;
rect_t *ds_dc58_some_mouse_rect;
uint16_t ds_dc66_frame_task_time;
uint16_t ds_dc6a_frame_tasks_count;
frame_task_t ds_dc6c_frame_tasks[0x20];
byte ds_dce6_in_transition = 0;
byte *ds_dd00_map_res;
int16_t ds_dd0f_globe_fresco_offset;

extern uint16_t cs_c13c;

/**
 * VGA data segment
 */

byte *vga_018a_cursor_restore_area_ptr = nullptr;
uint16_t vga_018c_cursor_restore_w = 0;
uint16_t vga_018e_cursor_restore_h = 0;
uint8_t vga_01a1_no_vert_retrace = 1;
uint8_t vga_01a3_y_offset = 0;
uint8_t vga_01bd_is_monochrome = 0;
uint8_t vga_01be_pal_is_dirty = 1;

// clang-format off
uint8_t vga_01bf_palette_dirty_entries[256] = {
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
};
// clang-format on

byte vga_02bf_palette_unapplied[768] = {0};

byte vga_05bf_palette_unapplied[768] = {
	0x00, 0x00, 0x00, 0x00, 0x00, 0x2A, 0x00, 0x2A, 0x00, 0x00, 0x2A, 0x2A, 0x2A, 0x00, 0x00, 0x3F, 0x34, 0x14, 0x2A,
	0x15, 0x00, 0x2A, 0x2A, 0x2A, 0x15, 0x15, 0x15, 0x15, 0x15, 0x3F, 0x15, 0x3F, 0x15, 0x15, 0x3F, 0x3F, 0x3F, 0x15,
	0x15, 0x3F, 0x15, 0x3F, 0x3F, 0x3F, 0x15, 0x3F, 0x3F, 0x3F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x3E, 0x39, 0x1B, 0x3F, 0x3F, 0x26, 0x3F, 0x3D, 0x15, 0x3F, 0x36, 0x1B, 0x3F, 0x2D, 0x00, 0x36,
	0x24, 0x0A, 0x36, 0x18, 0x0A, 0x2D, 0x10, 0x00, 0x24, 0x12, 0x00, 0x1B, 0x12, 0x0F, 0x1B, 0x0E, 0x08, 0x12, 0x00,
	0x0F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x30, 0x3F, 0x34, 0x28, 0x36, 0x2E, 0x21, 0x2D, 0x28, 0x1A, 0x24, 0x21, 0x13, 0x1B,
	0x1A, 0x0D, 0x13, 0x13, 0x08, 0x04, 0x08, 0x10, 0x04, 0x18, 0x08, 0x00, 0x10, 0x04, 0x00, 0x0C, 0x0A, 0x00, 0x02,
	0x14, 0x1C, 0x1A, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x14, 0x18, 0x1C, 0x01, 0x01, 0x02, 0x04, 0x04, 0x05, 0x07,
	0x06, 0x09, 0x09, 0x09, 0x0C, 0x0C, 0x0B, 0x0F, 0x0F, 0x0E, 0x12, 0x11, 0x11, 0x16, 0x14, 0x13, 0x19, 0x17, 0x16,
	0x1C, 0x1E, 0x1A, 0x21, 0x27, 0x1D, 0x27, 0x2D, 0x20, 0x29, 0x33, 0x23, 0x28, 0x39, 0x28, 0x26, 0x3F, 0x33, 0x29,
	0x3F, 0x3F, 0x29, 0x00, 0x00, 0x3F, 0x02, 0x00, 0x3B, 0x05, 0x00, 0x37, 0x08, 0x00, 0x34, 0x0A, 0x00, 0x30, 0x0C,
	0x00, 0x2C, 0x0E, 0x00, 0x29, 0x0E, 0x00, 0x25, 0x0F, 0x00, 0x21, 0x0F, 0x00, 0x1E, 0x0F, 0x00, 0x1A, 0x0E, 0x00,
	0x16, 0x0D, 0x00, 0x13, 0x0B, 0x00, 0x0F, 0x09, 0x00, 0x0B, 0x07, 0x00, 0x08, 0x3B, 0x30, 0x28, 0x3B, 0x31, 0x28,
	0x3B, 0x31, 0x29, 0x3C, 0x32, 0x2A, 0x3C, 0x33, 0x2B, 0x3C, 0x33, 0x2C, 0x3C, 0x34, 0x2C, 0x3D, 0x34, 0x2E, 0x3D,
	0x35, 0x2E, 0x3D, 0x36, 0x2F, 0x08, 0x10, 0x0A, 0x3F, 0x3F, 0x3F, 0x2E, 0x3D, 0x32, 0x22, 0x30, 0x28, 0x1F, 0x2A,
	0x26, 0x26, 0x34, 0x2C, 0x08, 0x0A, 0x10, 0x0B, 0x0B, 0x10, 0x0C, 0x0B, 0x10, 0x0D, 0x0B, 0x10, 0x0F, 0x0B, 0x10,
	0x10, 0x0B, 0x10, 0x10, 0x0B, 0x0F, 0x10, 0x0B, 0x0D, 0x10, 0x0B, 0x0C, 0x10, 0x0B, 0x0B, 0x32, 0x32, 0x32, 0x2F,
	0x2A, 0x37, 0x1E, 0x16, 0x3E, 0x1C, 0x0B, 0x3F, 0x0F, 0x10, 0x0B, 0x3D, 0x34, 0x2E, 0x0C, 0x10, 0x0B, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x3F, 0x3F, 0x3F};

uint16_t vga_2768 = 8;
uint16_t vga_276a = 1;

byte *vga_2535_fb = 0;
byte *vga_2537_fb = 0;
byte *vga_2539_fb = 0;
byte *vga_253b_fb = 0;

int g_frame = 0;
byte g_screen_palette[768];

#define HNM_TAG_SD 0x6473
#define HNM_TAG_PL 0x6c70
#define HNM_TAG_MM 0x6d6d

void dump_framebuffer_raw(const char *postfix, byte *fb)
{
	char filename[256];
	sprintf(filename, "ppm/frame-%03d-%s.ppm", g_frame, postfix);
	FILE *f = fopen(filename, "wb");
	fwrite(fb, 3 * 320 * 200, 1, f);
	fclose(f);
}

void dump_ppm(const char *name, byte *fb, byte *pal, int w, int h)
{
	uint8_t pal_[768] = {
		0,  0,  0,  3,  6,  15, 10, 15, 27, 19, 27, 40, 32, 0,  0,  24, 15, 20, 24, 0,  0,  43, 35, 39, 14, 10, 12, 33,
		41, 53, 48, 0,  0,  44, 0,  0,  56, 0,  0,  33, 24, 29, 53, 48, 51, 63, 63, 63, 0,  0,  0,  0,  0,  0,  0,  0,
		0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
		0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
		0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
		0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
		0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
		0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
		0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
		0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
		0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
		0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
		0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
		0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  8,  4,  0,  13, 2,
		1,  18, 3,  2,  23, 3,  0,  28, 8,  0,  34, 11, 0,  39, 15, 1,  41, 17, 6,  44, 19, 9,  48, 22, 5,  52, 26, 9,
		52, 28, 10, 52, 31, 13, 56, 35, 17, 59, 38, 19, 13, 4,  11, 16, 5,  13, 18, 7,  15, 20, 8,  18, 23, 10, 20, 25,
		12, 22, 28, 14, 25, 30, 16, 27, 33, 19, 30, 35, 21, 33, 38, 24, 35, 40, 27, 38, 43, 30, 40, 45, 33, 43, 48, 37,
		46, 0,  11, 0,  0,  14, 0,  0,  18, 0,  0,  21, 0,  0,  24, 0,  0,  28, 0,  0,  31, 0,  0,  35, 0,  0,  38, 0,
		50, 0,  0,  45, 0,  0,  42, 0,  0,  39, 0,  0,  38, 0,  0,  32, 0,  0,  25, 0,  0,  6,  6,  7,  10, 10, 12, 13,
		13, 16, 17, 17, 20, 21, 21, 24, 24, 24, 29, 28, 28, 33, 32, 32, 37, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
		0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
		0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
		0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
		0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
		0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
		0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  10, 5,  0,  13, 4,  1,  19, 7,
		2,  24, 6,  0,  28, 9,  0,  34, 11, 1,  39, 14, 3,  41, 17, 6,  44, 19, 9,  47, 24, 12, 52, 30, 17, 53, 34, 19,
		56, 42, 26, 59, 48, 32, 63, 54, 38, 63, 60, 52};

	for (int i = 0; i != 256; ++i) {
		// printf("pal[%3d] = %02x%02x%02x\n", i, pal[3 * i + 0], pal[3 * i + 1], pal[3 * i + 2]);
	}

	int scale_w = 5;
	int scale_h = 6;

	byte *output = new byte[3 * w * scale_w * h * scale_h];
	for (int y = 0; y != scale_h * h; ++y) {
		for (int x = 0; x != scale_w * w; ++x) {
			byte c = fb[w * (y / scale_h) + (x / scale_w)];
			// printf("fb[%3d, %3d] = %02x = (%d, %d, %d)\n", x, y, c, pal_[3 * c + 0], pal_[3 * c + 1], pal_[3 * c +
			// 2]);
			for (int i = 0; i != 3; ++i) {
				output[3 * (w * scale_w * y + x) + i] = ((255 * pal_[3 * c + i]) / 63);
			}
		}
	}

	FILE *f = fopen(name, "wb");
	fprintf(f, "P6 %d %d 255\n", w * scale_w, h * scale_h);
	fwrite(output, 3 * w * scale_w * h * scale_h, 1, f);
	fclose(f);
	delete[] output;
}

void dump_framebuffer_ppm(const char *postfix, byte *fb, byte *pal)
{
	byte output[3 * 320 * 200];
	for (int y = 0; y != 200; ++y) {
		for (int x = 0; x != 320; ++x) {
			byte c = fb[320 * y + x];
			for (int i = 0; i != 3; ++i) {
				output[3 * (320 * y + x) + i] = (255 * pal[3 * c + i] / 63);
			}
		}
	}

	char filename[256];
	sprintf(filename, "ppm/frame-%03d-%s.ppm", g_frame++, postfix);

	FILE *f = fopen(filename, "wb");
	fprintf(f, "P6\n320 200\n255\n");
	fwrite(output, 3 * 320 * 200, 1, f);
	fclose(f);
}

void dump_palette_ppm(const char *postfix, byte *pal)
{
	const int scale = 8;
	const int w = 16 * scale;
	byte output[3 * w * w];
	memset(output, 0, 3 * w * w);
	for (int y = 0; y != w; ++y) {
		for (int x = 0; x != w; ++x) {
			int src = 16 * (y / scale) + (x / scale);
			int dst = w * y + x;

			output[3 * dst + 0] = ((255 * vga_05bf_palette_unapplied[3 * src + 0]) / 63);
			output[3 * dst + 1] = ((255 * vga_05bf_palette_unapplied[3 * src + 1]) / 63);
			output[3 * dst + 2] = ((255 * vga_05bf_palette_unapplied[3 * src + 2]) / 63);
		}
	}

	char filename[256];
	sprintf(filename, "ppm/frame-%03d-pal-%s.ppm", g_frame, postfix);
	FILE *f = fopen(filename, "wb");
	fprintf(f, "P6\n%d %d 255\n", w, w);
	fwrite(output, 3 * w * w, 1, f);
	fclose(f);
}

bool framebuffer_log_active = false;
byte *orig_framebuffer_1;
byte *orig_framebuffer_screen;
byte *orig_framebuffer_2;

uint32_t expected_next_ea = 0;
uint32_t consecutive_writes = 0;

void log_framebuffer_write(byte *p, int len)
{
	if (!framebuffer_log_active) {
		return;
	}

	// printf("%p: ds_dbd6_framebuffer_1\n", ds_dbd6_framebuffer_1);
	// printf("%p: ds_dbd8_framebuffer_screen\n", ds_dbd8_framebuffer_screen);
	// printf("%p: ds_dbda_framebuffer_active\n", ds_dbda_framebuffer_active);
	// printf("%p: ds_dbdc_framebuffer_2\n", ds_dbdc_framebuffer_2);
	// printf("\n");
	// printf("%p: orig_framebuffer_1\n", orig_framebuffer_1);
	// printf("%p: orig_framebuffer_screen\n", orig_framebuffer_screen);
	// printf("%p: orig_framebuffer_2\n", orig_framebuffer_2);

	uint32_t ea;
	const char *name;
	int offset = 0;
	if (orig_framebuffer_1 <= p && p < orig_framebuffer_1 + 65536) {
		name = "4487";
		offset = p - orig_framebuffer_1;
		ea = 0x10000000 + offset;
	} else if (orig_framebuffer_2 <= p && p < orig_framebuffer_2 + 65536) {
		name = "34e7";
		offset = p - orig_framebuffer_2;
		ea = 0x20000000 + offset;
	} else if (orig_framebuffer_screen <= p && p < orig_framebuffer_screen + 65536) {
		name = "a000";
		offset = p - orig_framebuffer_screen;
		ea = 0x30000000 + offset;
	} else if (ds_dc32_hnm_buffer <= p && p < ds_dc32_hnm_buffer + 65536) {
		name = "21f3";
		offset = p - ds_dc32_hnm_buffer;
		ea = 0x40000000 + offset;
	} else {
		printf("NOT A FRAMEBUFFER WRITE\n");
		return;
	}

	consecutive_writes += 1;
	if (ea != expected_next_ea) {
		consecutive_writes = 0;
	}
	expected_next_ea = ea + len;

	if (consecutive_writes < 64) {
		printf("FB WRITE: %s:[%04x..+%04x] ", name, offset, len);
		if (len > 6) {
			printf("\n");
		}
		hexdump(p, len);
	} else if (consecutive_writes == 64) {
		printf("FB WRITE: ...\n");
	}
}

void log_framebuffer_read(byte *p, int len)
{
	if (!framebuffer_log_active) {
		return;
	}

	const char *name;
	const char *active = "      ";
	int offset = 0;
	if (ds_dbd6_framebuffer_1 <= p && p < ds_dbd6_framebuffer_1 + 65536) {
		name = "DS:[DBD6] FB1";
		if (ds_dbd6_framebuffer_1 == ds_dbda_framebuffer_active) {
			active = " (ACT)";
		}
		offset = p - ds_dbd6_framebuffer_1;
	} else if (ds_dbd8_framebuffer_screen <= p && p < ds_dbd8_framebuffer_screen + 65536) {
		name = "DS:[DBD8] SCR";
		offset = p - ds_dbd8_framebuffer_screen;
	} else if (ds_dbdc_framebuffer_2 <= p && p < ds_dbdc_framebuffer_2 + 65536) {
		name = "DS:[DBDC] FB2";
		offset = p - ds_dbdc_framebuffer_2;
	} else if (ds_dbda_framebuffer_active <= p && p < ds_dbda_framebuffer_active + 65536) {
		name = "DS:[DBDC] ACT";
		offset = p - ds_dbda_framebuffer_active;
	} else {
		// printf("NOT A FRAMEBUFFER READ\n");
		return;
	}

	// printf("FB READ:  %s%s: [%04x..+%04x]\n", name, active, offset, len);
}

/*
 * Code Segment
 */

int dune_main(App &app, int argc, char **argv)
{
	g_app = &app;

	cs_0000_start();

	quitting = true;
	timer_thread->join();

	return 0;
}

void cs_0000_start()
{
	ds_39b7_alloc_next_addr = (byte *)calloc(16, 1048576);
	ds_ce68_alloc_last_addr = ds_39b7_alloc_next_addr + 16 * 1048576;

	cs_e594_initialize_system();
	cs_00b0_initialize_resources();

	cs_0580_play_intro();
	printf("-----------------\n");
	cs_0309_play_credits(true);
	cs_0083();
	// cs_b389_create_save(-1);
	cs_1860();
	// cs_b827_draw_globe_and_ui_to_front_buffer();

	// cs_d1ef_draw_all_ui_elements();

	// cs_c4cd_copy_fb1_to_screen();
	// dump_framebuffer_ppm("frame-000", ds_dbd6_framebuffer_1, g_screen_palette);

	cs_d815_game_loop();
}

void cs_0083()
{
	// cs_cfa0_check_language();

	// Tail-call by fallthrough
	cs_0086();
}

void cs_0086()
{
	cs_c07c_set_fb1_as_active_framebuffer();
	cs_c0ad_gfx_clear_active_framebuffer();
	cs_c097_gfx_call_with_fb1_as_screen(cs_d1ef_draw_all_ui_elements);
	cs_1797_ui_head_draw();
}

void cs_0098_adjust_subresource_pointers_IGNORE() {}

void cs_00b0_initialize_resources()
{
	cs_00d1_initialize_resources();
	cs_0169_initialize_map();
	cs_da53_remove_all_frame_tasks();
	// cs_b17a();
	// cs_b17a();
}

void cs_00d1_initialize_resources()
{
	// Load TABLAT.BIN
	cs_f0b9_res_read_by_index_into_buffer(0xba, ds_4948_tablat_bin);

	ptr_offset_t r = make_ptr_offset(ds_4948_tablat_bin);
	for (int i = 0; i != 99; ++i) {
		globe_rotation_lookup_table[i].unk0 = r.readbe16();
		globe_rotation_lookup_table[i].unk1 = r.readbe16();
		globe_rotation_lookup_table[i].unk2 = r.readbe16();
		globe_rotation_lookup_table[i].unk3 = r.readbe16();
	}

	// for (int i = 0; i != 99; ++i) {
	// 	uint16_t v = globe_rotation_lookup_table[4 * i + 1];
	// 	uint16_t w = 2 * v;
	// 	uint16_t inv = 0x10000 / w;
	// 	if (v < (0x10000 % w)) {
	// 		inv += 1;
	// 	}
	// 	globe_rotation_lookup_table[4 * i + 1] = inv;
	// }

	// hexdump(ds_4948_tablat_bin, 782);

	// printf("a: ---\n");
	// for (int i = 0; i != 99; ++i) {
	// 	printf("%04x %04x %04x %04x\n", globe_rotation_lookup_table[4 * i + 0], globe_rotation_lookup_table[4 * i + 1],
	// globe_rotation_lookup_table[4 * i + 2], globe_rotation_lookup_table[4 * i + 3]);
	// }

	// Load MAP.HSQ
	ds_dd00_map_res = cs_f0b9_res_read_by_index_into_buffer(0xbf);

	ds_47ac_command_bin = ds_39b7_alloc_next_addr;
	cs_f0ff_alloc_take(7500);

	ds_47b0_phrase_bin = ds_39b7_alloc_next_addr;
	cs_f0ff_alloc_take(44500);

	cs_cfb9();

	cs_c137_load_icones();
}

void cs_0169_initialize_map()
{
	TODO;
}

void cs_02e0(uint16_t cx)
{
	cs_0a44(cx);
}

void cs_02e3()
{
	cs_02e0(32);
	cs_b8a7_setup_draw_globe();
	cs_b85a_draw_globe_with_atmosphere();
	cs_c13e_open_sprite_bank_resource_by_index(44);
	cs_b8ea();
}

void cs_0309_play_credits(bool play)
{
	FUNC_NAME;
	if (play) {
		// cs_de4e();
		cs_c07c_set_fb1_as_active_framebuffer();
		cs_c0ad_gfx_clear_active_framebuffer();
		vga_0c06_gfx_33_set_global_y_offset(24);
		cs_c102_copy_pal_and_transition(cs_09ef_load_credits_hnm);
		// cs_ad50_play_music_wormsuit();

		printf("%p: ds_dbd6_framebuffer_1\n", ds_dbd6_framebuffer_1);
		printf("%p: ds_dbd8_framebuffer_screen\n", ds_dbd8_framebuffer_screen);
		printf("%p: ds_dbda_framebuffer_active\n", ds_dbda_framebuffer_active);
		printf("%p: ds_dbdc_framebuffer_2\n", ds_dbdc_framebuffer_2);
		printf("\n");
		printf("%p: orig_framebuffer_1\n", orig_framebuffer_1);
		printf("%p: orig_framebuffer_screen\n", orig_framebuffer_screen);
		printf("%p: orig_framebuffer_2\n", orig_framebuffer_2);

		do {
			cs_0a16();
		} while (!cs_cc85_hnm_is_complete() && !cs_dd63_has_user_input());
	}
	cs_0579_clear_global_y_offset();
}

// clang-format off
Scene cs_0337_intro_script[] = {
	// 0. Virgin logo
	{ 0x0000, cs_061c_load_virgin_hnm,              0x0000, 0x003a, cs_0625_play_virgin_hnm,         1 },

	// 1. Cryo logo 1
	{ 0x0000, cs_c0ad_gfx_clear_active_framebuffer, 0x0000, 0x003a, cs_0f66_nullsub,                 1 },

	// 2. Cryo logo 2
	{ 0x0000, cs_064d_load_cryo_hnm,                0x0000, 0x0030, cs_0661_play_cryo_hnm,           1 },
	{ 0x0000, cs_0658_load_cryo2_hnm,               0x006f, 0x0030, cs_0661_play_cryo_hnm,      0x0190 },
	{ 0x0000, cs_0f66_nullsub,                      0x00a8,     -1, cs_0f66_nullsub,                 1 },

	// 5. Virgin Games Presents
	{ 0x0000, cs_0678_load_present_hnm,             0x0000, 0x003a, cs_0684_play_present_hnm,        1 },

	// 6. Princess Irulan video
	{ 0x0000, cs_cefc_load_irulan_hnm,              0x0000, 0x003a, cs_cf1b_play_irulan_hnm,         1 },

	// 7. Dune logo
	{ 0x0000, cs_c0ad_gfx_clear_active_framebuffer, 0x0000, 0x003a, cs_0f66_nullsub,                 1 },
	{ 0x0000, cs_069e_load_intro_hnm,               0x0000, 0x0036, cs_0f66_nullsub,            0x0190 },
	{ 0x0000, cs_0f66_nullsub,                      0x0090, 0x0030, cs_06aa_play_intro_hnm,     0x0190 },
	{ 0x0000, cs_0f66_nullsub,                      0x010c,     -1, cs_06bd_play_hnm_skippable,      1 },

	// 11. Desert sky animation
	{ 0x0000, cs_07fd,                              0x0000, 0x003a, cs_085d,                    0x04b0 },

	// 12. Desert flyover video
	{ 0x0148, cs_06ce,                              0x014e, 0x0010, cs_0704,                    0x1900 },

	// 13. Palace, equipment room
	{ 0x024b, cs_0972,                              0x024e, 0x0010, cs_0f66_nullsub,            0x0085 },

	// 14. Palace, Lady Jessica
	{ 0x0000, cs_098a,                              0x0258, 0x0010, cs_0f66_nullsub,            0x0085 },

	// 15. Palace, Duke Leto
	{ 0x0000, cs_0995,                              0x0262, 0x0010, cs_0798,                    0x0258 },

	// 16. Palace, Lady Jessica
	{ 0x02ca, cs_0771,                              0x02cf, 0x0010, cs_0798,                    0x0258 },

	// 17. Paul on a red background
	{ 0x032e, cs_07ee,                              0x0330, 0x0010, cs_0f66_nullsub,            0x00c8 },

	// 18. Outside of palace
	{ 0x033c, cs_09a5,                              0x0344, 0x0010, cs_0f66_nullsub,            0x0085 },

	// 19. Desert flight to sietch
	{ 0x0000, cs_06d3,                              0x034e, 0x0010, cs_0704,                    0x2648 },

	// 20. Inside sietch
	{ 0x04be, cs_077c,                              0x04c0, 0x0010, cs_0f66_nullsub,            0x0085 },

	// 21. Chani in sietch
	{ 0x0000, cs_0788,                              0x04d0,     -1, cs_078d,                    0x0258 },

	// 22. Dr. Kynes
	{ 0x050c, cs_07a3,                              0x0510, 0x0010, cs_0798,                    0x0258 },

	// 23. Somebody in sietch
	{ 0x054c, cs_07c6,                              0x0550, 0x0010, cs_0798,                    0x0258 },

	// 24. Midnight
	{ 0x0588, cs_0868,                              0x058f, 0x0010, cs_087b,                    0x0320 },

	// 25. Sting
	{ 0x05ac, cs_0886,                              0x05ae, 0x0030, cs_0798,                    0x0258 },

	// 26. Baron
	{ 0x05ee, cs_09ad,                              0x05f0, 0x0030, cs_0798,                    0x0258 },

	// 27. Harkonnen with Baron
	{ 0x0614, cs_0f66_nullsub,                      0x0000,     -1, cs_08b6,                    0x0032 },

	// 28. Night War
	{ 0x061e, cs_0acd_intro_28_night_attack_start,  0x061e, 0x0010, cs_0f66_nullsub,            0x07d0 },

	// 29. Desert flight with palette transition
	{ 0x064c, cs_06d8,                              0x0000, 0x0010, cs_06fc,                    0x0c80 },

	// 30. INT05
	{ 0x06c6, cs_0740,                              0x06c8, 0x003a, cs_0f66_nullsub,            0x0190},

	// 31. INT10
	{ 0x0000, cs_075a,                              0x06ee, 0x0010, cs_0f66_nullsub,            0x0190 },

	// 32. INT08
	{ 0x0000, cs_0752,                              0x06fe, 0x0010, cs_0f66_nullsub,            0x0190 },

	// 33. INT04
	{ 0x0000, cs_073c,                              0x070e, 0x0010, cs_0f66_nullsub,            0x0190 },

	// 34. INT09
	{ 0x0000, cs_0756,                              0x072e, 0x0010, cs_0f66_nullsub,            0x0190 },

	// 35. INT11
	{ 0x0000, cs_075e,                              0x073e, 0x0010, cs_0f66_nullsub,            0x0190 },

	// 36. INT02
	{ 0x0000, cs_0737,                              0x074e, 0x0010, cs_0f66_nullsub,            0x0190 },

	// 37. INT06
	{ 0x0000, cs_0747,                              0x076e, 0x0010, cs_0f66_nullsub,            0x0190 },

	// 38. Ripples in cave water
	{ 0x0000, cs_07e0,                              0x077e, 0x0010, cs_0f66_nullsub,            0x04b0 },

	// 39. Plant
	{ 0x07ac, cs_06ea,                              0x07af, 0x0010, cs_0704,                    0x03e8 },

	// 40. Harkonnen palace
	{ 0x07ce, cs_074b,                              0x07ce, 0x0010, cs_0f66_nullsub,            0x0190 },

	// 41. Worm
	{ 0x0000, cs_0711,                              0x07dc, 0x003a, cs_071d,                         1 },

	// 42. INT15
	{ 0x0000, cs_076a,                              0x081c, 0x003a, cs_0f66_nullsub,            0x0190 },

	// 43. INT13
	{ 0x0000, cs_0762,                              0x083e, 0x0010, cs_0f66_nullsub,            0x0190 },

	// 44. INT14
	{ 0x0000, cs_0766,                              0x084e, 0x0010, cs_0f66_nullsub,            0x00c8 },

	// 45. Clear screen
	{ 0x0000, cs_c0ad_gfx_clear_active_framebuffer, 0x085e, 0x003a, cs_0f66_nullsub,                 1 },

	// 46. INT15
	{ 0x0000, cs_076a,                              0x085e, 0x0036, cs_0f66_nullsub,            0x0190 },

	// 47. Clear screen
	{ 0x0000, cs_c0ad_gfx_clear_active_framebuffer, 0x08e0, 0x0038, cs_0f66_nullsub,                 1 },

	{ -1 }
};
// clang-format on

void cs_0579_clear_global_y_offset()
{
	vga_0c06_gfx_33_set_global_y_offset(0);
}

void cs_0580_play_intro()
{
restart_intro:
	// TODO: cs_de54 check for keyboard esc

	vga_0a68_gfx_41_copy_pal_1_to_pal_2();

	// TODO: cs_aeb7 do midi

	cs_0945_intro_script_set_current_scene(cs_0337_intro_script + 28);

	for (;;) {
		printf("\nIntro segment %2d.\n=================\n\n",
		       (int)(ds_4854_intro_scene_current_scene - cs_0337_intro_script));

		vga_0c06_gfx_33_set_global_y_offset(24);
		if (ds_4854_intro_scene_current_scene->a == -1) {
			// goto restart_intro;
			break;
		}

		// TODO: cs_de0c_check_midi();
		cs_0911();

		vga_0a68_gfx_41_copy_pal_1_to_pal_2();

		stepfn load_fn = ds_4854_intro_scene_current_scene->b;
		cs_c097_gfx_call_with_fb1_as_screen(load_fn);

		// TODO: ds_47d1 &= 0x7f;
		// TODO: cs_39e6();

		uint16_t bx = ds_4854_intro_scene_current_scene->d;
		cs_de0c(bx);

		int32_t transition_type = ds_4854_intro_scene_current_scene->d;
		// TODO: cs_de0c_check_midi();
		if (transition_type >= 0) {
			// TODO;
			cs_c108_transition(transition_type, cs_0f66_nullsub);
			cs_c0f4();
			// cs_3a7c();
		}

		cs_c07c_set_fb1_as_active_framebuffer();
		// TODO: ds_4701 |= 0x80;
		cs_dd63_has_user_input();

		stepfn run_fn = ds_4854_intro_scene_current_scene->e;
		run_fn();

		if (ds_4854_intro_scene_current_scene->f) {
			cs_ddf0_wait_voice_interruptable(ds_4854_intro_scene_current_scene->f);
		}

		++ds_4854_intro_scene_current_scene;
	}

	exit(0);
	TODO;
}

void cs_061c_load_virgin_hnm()
{
	cs_ad57_play_music_morning();

	// Tail-call
	cs_ca1b_hnm_load(0x15);
}

void cs_0625_play_virgin_hnm()
{
	cs_c07c_set_fb1_as_active_framebuffer();
	do {
		do {
			if (cs_dd63_has_user_input()) {
				return;
			}
		} while (!cs_c9f4_hnm_do_frame_and_check_if_frame_advanced());

		cs_c4cd_copy_fb1_to_screen();
		// TODO; // Handle MIDI
	} while (!cs_cc85_hnm_is_complete());
}

void cs_064d_load_cryo_hnm()
{
	// cs_ad95_play_music(0x0a);

	// Tail-call
	cs_ca1b_hnm_load(22);
}

void cs_0658_load_cryo2_hnm()
{
	cs_c0ad_gfx_clear_active_framebuffer();

	// Tail-call
	cs_ca1b_hnm_load(23);
}

void cs_0661_play_cryo_hnm()
{
	cs_c07c_set_fb1_as_active_framebuffer();
	do {
		do {
			if (cs_dd63_has_user_input()) {
				return;
			}
		} while (!cs_c9f4_hnm_do_frame_and_check_if_frame_advanced());

		cs_c4cd_copy_fb1_to_screen();
	} while (!cs_cc85_hnm_is_complete());
}

void cs_0678_load_present_hnm()
{
	cs_0579_clear_global_y_offset();
	cs_c0ad_gfx_clear_active_framebuffer();
	cs_ca1b_hnm_load(24);
}

void cs_0684_play_present_hnm()
{
	// Tail-call
	cs_06bd_play_hnm_skippable();
}

void cs_069e_load_intro_hnm()
{
	cs_c07c_set_fb1_as_active_framebuffer();
	cs_0579_clear_global_y_offset();
	cs_ca1b_hnm_load(15);
}

void cs_06aa_play_intro_hnm()
{
	cs_0579_clear_global_y_offset();
	cs_c08e_set_screen_as_active_framebuffer();
	do {
		cs_c9e8_hnm_do_frame_skippable();
	} while (ds_dbe8_hnm_frame_counter != 86);
}

void cs_06bd_play_hnm_skippable()
{
	cs_0579_clear_global_y_offset();
	cs_c08e_set_screen_as_active_framebuffer();
	do {
		if (!cs_c9e8_hnm_do_frame_skippable()) {
			return;
		}
	} while (!cs_cc85_hnm_is_complete());
}

void cs_06ce()
{
	cs_06f3(16);
}

void cs_06d3()
{
	cs_06f3(17);
}

void cs_06d8()
{
	cs_06f3(18);
}

void cs_06ea()
{
	ds_0004 = 2;

	// Tail-call by fallthrough
	cs_06f3(19);
}

void cs_06f3(uint16_t ax)
{
	ds_dbda_framebuffer_active += 24 * 320;

	// Tail-call
	cs_ca1b_hnm_load(ax);
}

void cs_06fc() {}

void cs_0704()
{
	cs_da25_add_frame_task(0, cs_070c);
	cs_070c();
}

void cs_070c()
{
	cs_ca60_hnm_do_frame();
	g_app->update_screen(ds_dbd8_framebuffer_screen);
}

void cs_0711()
{
	ds_dbda_framebuffer_active += 24 * 320;

	// Tail-call
	cs_ca1b_hnm_load(14);
}

void cs_071d()
{
	for (;;) {
		cs_4b16();
		// TODO;
		// cs_4937();
		if (cs_cc85_hnm_is_complete()) {
			return;
		}

		if (cs_c9f4_hnm_do_frame_and_check_if_frame_advanced()) {
			cs_c4cd_copy_fb1_to_screen();
		}
		if (cs_dd63_has_user_input()) {
			return;
		}
	}
}

void cs_0737()
{
	cs_c0ad_gfx_clear_active_framebuffer();
	cs_c2f2(86);
}
void cs_073c()
{
	cs_c0ad_gfx_clear_active_framebuffer();
	cs_c2f2(87);
}
void cs_0740()
{
	cs_c0ad_gfx_clear_active_framebuffer();
	cs_c2f2(88);
}
void cs_0747()
{
	cs_c0ad_gfx_clear_active_framebuffer();
	cs_c2f2(89);
}
void cs_074b()
{
	cs_c0ad_gfx_clear_active_framebuffer();
	cs_c2f2(90);
}
void cs_0752()
{
	cs_c0ad_gfx_clear_active_framebuffer();
	cs_c2f2(91);
}
void cs_0756()
{
	cs_c0ad_gfx_clear_active_framebuffer();
	cs_c2f2(92);
}
void cs_075a()
{
	cs_c0ad_gfx_clear_active_framebuffer();
	cs_c2f2(93);
}
void cs_075e()
{
	cs_c0ad_gfx_clear_active_framebuffer();
	cs_c2f2(94);
}
void cs_0762()
{
	cs_c0ad_gfx_clear_active_framebuffer();
	cs_c2f2(95);
}
void cs_0766()
{
	cs_c0ad_gfx_clear_active_framebuffer();
	cs_c2f2(96);
}
void cs_076a()
{
	cs_c0ad_gfx_clear_active_framebuffer();
	cs_c2f2(97);
}

void cs_0771()
{
	cs_0978(PALACE_CONFERENCE_ROOM);
	cs_099d(1);
}

void cs_077c()
{
	cs_c0ad_gfx_clear_active_framebuffer();
	cs_0981(0x0803, 0x1080);
}

void cs_0788()
{
	cs_099d(7);
}

void cs_078d()
{
	cs_c0f4();
	TODO;
}

void draw_talking_head_image_group(byte image_group_id)
{
	ptr_offset_t r =
		ds_47cc_lip_sync_section_1_ptr + ds_47cc_lip_sync_section_1_ptr.peekle16_at_offset(2 * (image_group_id - 2));

	for (;;) {
		byte image_id = r.readbyte();
		if (image_id == 0) {
			printf("\timage_id: %02x\n", image_id);
			return;
		}

		byte x = r.readbyte() + ds_1ae6_ui_elements[19].x0;
		byte y = r.readbyte() + ds_1ae6_ui_elements[19].y0;

		printf("\timage_id: 0x%02x, x: 0x%02x, y: 0x%02x\n", image_id, x, y);

		cs_c305_draw_sprite_clipped(image_id - 1, x, y);
	}
}

bool draw_talking_head_frame_set(byte frame_set_id, byte frame_id)
{
	ptr_offset_t r = ds_47ca_lip_sync_section_2_ptr +
	                 ds_47ca_lip_sync_section_2_ptr.peekle16_at_offset(2 * frame_set_id) + 4 * frame_id;

	for (int i = 0;; ++i) {
		byte image_group_id = r.readbyte();
		if (image_group_id == 0xff) {
			return false;
		}
		if (image_group_id == 0) {
			return true;
		}

		if (image_group_id > 1) {
			draw_talking_head_image_group(image_group_id);
		}
	}
}

uint16_t slide = 0;

void cs_0798()
{
	ptr_offset_t r = ds_47d2_lip_sync_section_3_ptr;
	for (;;) {
		int n = 0;
		for (;;) {
			printf("slide: %3d, n: %3d\n", slide, n);
			bool more = draw_talking_head_frame_set(slide, n++);
			if (!more) {
				break;
			}

			cs_c4cd_copy_fb1_to_screen();
			dump_framebuffer_ppm("talking-head", ds_dbd6_framebuffer_1, g_screen_palette);
			// for (int i = 0; i != 10; ++i)
			g_app->wait_for_vsync();
			g_app->update_screen(ds_dbda_framebuffer_active);
		}
		slide++;
	}

	TODO;
}

void cs_07a3()
{
	cs_c13e_open_sprite_bank_resource_by_index(46); // SUNRS.HSQ
	cs_c21b_draw_sprite_list(ds_1500_sprite_list);
	cs_c22f_draw_sprite(1, 84, 11);
	cs_c412_copy_active_framebuffer_to_fb2();
	cs_099d(6);
}

void cs_07c6()
{
	cs_0981(0x802, 0x1080);
}

void cs_07e0() // Ripples in cave water
{
	// cs_abdb(4);
	cs_0981(0x804, 0x1080);
}

void cs_07ee()
{
	cs_c13e_open_sprite_bank_resource_by_index(48);
	cs_c21b_draw_sprite_list(ds_1526_sprite_list);

	// Tail-call
	cs_0960();
}

void cs_07fd()
{
	cs_c0ad_gfx_clear_active_framebuffer();

	// Tail-call by fall-through
	cs_0802(8);
}

void cs_0802(uint8_t bl)
{
	byte *p = cs_0820(bl);
	vga_09e2_set_palette_unapplied(p, 384, 240);

	ds_46d7 = 0;
	cs_c13e_open_sprite_bank_resource_by_index(46);

	// Tail-call
	cs_c21b_draw_sprite_list(ds_1500_sprite_list);
}

byte *cs_0820(uint8_t bl)
{
	// Tail-call
	ptr_offset_t p = cs_3978(46, bl);
	return p.ptr();
}

void cs_0826()
{
	if (ds_46d7 == 0) {
		uint8_t al = ds_46d6 + 1;
		assert(al != 0);

		if (al == 11) {
			// What's this for?
			cs_c13e_open_sprite_bank_resource_by_index(9);
			return;
		}
		if (al == 14) {
			return;
		}
		uint8_t ah = 10;
		if (al != 13) {
			ah = 30;
		}
		ds_46d7 = ah;
		uint8_t bl = al;
		byte *p = cs_0820(bl);
		vga_0a40_gfx_38_set_palette_2(p, 384, 240);
	}

	cs_391d();
}

void cs_085d()
{
	cs_da25_add_frame_task(9, cs_0826);
}

void cs_0868() {}

void cs_087b() {}

void cs_0886()
{
	cs_c13e_open_sprite_bank_resource_by_index(48); // BACK.HSQ
	vga_197b_gfx_10_fill_rect(ds_dbda_framebuffer_active, &ds_1470_game_area_rect, 0xde);

	rect_t clip_rect = {0, 0, 320, 200};
	cs_5b99_copy_rect(&ds_d834_sprite_clip_rect, &clip_rect);

	// Draw background
	cs_c21b_draw_sprite_list(ds_154e_sprite_list);

	// Draw harkonnen guards
	cs_5ba8_copy_game_area_rect_to_sprite_clip_rect();
	cs_c21b_draw_sprite_list(ds_155c_sprite_list);

	cs_c412_copy_active_framebuffer_to_fb2();

	cs_09c7(10, 58);
	cs_978e();
}

void cs_08b6() {}

void cs_08f0(uint16_t location_and_room, uint16_t bx)
{
	printf("cs_08f0(0x%04x, 0x%04x)\n", location_and_room, bx);
	ds_47a4 = 0;
	ds_46df = 0;
	ds_0004 = location_and_room;
	ds_0006 = bx;
	ds_0008_location = hi(location_and_room);
	ds_114e = hi(bx);
	printf("ds_0008_location: %d, ds_114e: %d\n", ds_0008_location, ds_114e);

	// Tail-call
	cs_2d74_open_sal_resource();
}

void cs_0911()
{
	// TODO;
	ds_22e3 = 1;
	// cs_da5f_remove_frame_task(cs_070c);
	// cs_da5f_remove_frame_task(cs_3916);
	// cs_0a3e();
	cs_da5f_remove_frame_task(cs_0826);
}

void cs_0945_intro_script_set_current_scene(Scene *scene)
{
	ds_4854_intro_scene_current_scene = scene;
}

void cs_0960()
{
	TODO;
}

void cs_0972()
{
	cs_c0ad_gfx_clear_active_framebuffer();

	// Tail-call by fallthrough
	cs_0978(PALACE_EQUIPMENT_ROOM);
}

void cs_0978(uint16_t location_and_room)
{
	ds_0012 = 0;
	cs_097e(location_and_room);
}

void cs_097e(uint16_t location_and_room)
{
	cs_0981(location_and_room, 0x180);
}

void cs_0981(uint16_t location_and_room, uint16_t bx)
{
	cs_08f0(location_and_room, bx);
	cs_37b2();
}

void cs_098a()
{
	ds_0012 = 2;
	cs_097e(PALACE_CONFERENCE_ROOM);
}

void cs_0995()
{
	ds_0012 = 2;
	cs_0978(PALACE_THRONE_ROOM);

	// Tail-call by fallthrough
	cs_099d(0);
}

void cs_099d(byte al)
{
	cs_09c7(al, 0);

	// Tail-call
	cs_978e();
}

void cs_09a5()
{
	cs_c0ad_gfx_clear_active_framebuffer();

	// Tail-call
	cs_c2f2(98);
}

void cs_09ad()
{
	cs_c13e_open_sprite_bank_resource_by_index(48); // BACK.HSQ
	cs_c21b_draw_sprite_list(ds_153a_sprite_list);
	cs_c412_copy_active_framebuffer_to_fb2();
	cs_09c7(9, 0x52);
	cs_978e();
}

void cs_09c7(byte al, uint16_t talking_head_offset)
{
	ds_47c4 = al;
	cs_91a0_setup_lip_sync_data_from_sprite_sheet(al);
	if (talking_head_offset >= ds_1ae6_ui_elements[19].x0) {
		ds_1ae6_ui_elements[19].x0 += talking_head_offset;
		ds_1ae6_ui_elements[19].x1 += talking_head_offset;
		if (ds_1ae6_ui_elements[19].x1 > 320) {
			ds_1ae6_ui_elements[19].x1 = 320;
		}
	}
}

void cs_09ef_load_credits_hnm()
{
	FUNC_NAME;
	// Tail-call
	framebuffer_log_active = true;
	cs_ca1b_hnm_load(20);
}

void cs_0acd_intro_28_night_attack_start()
{
	// REMOVE: For testing only
	ds_d826_rand_seed = 0xfe17;
	// END

	cs_c13c = 0x2b; // ATTACK.HSQ
	cs_c13b_open_onmap_resource();

	cs_5ba8_copy_game_area_rect_to_sprite_clip_rect();
	cs_c07c_set_fb1_as_active_framebuffer();

	cs_c370_blit_repeated_x(ds_dbda_framebuffer_active, &ds_1582, 2);
	cs_c370_blit_repeated_x(ds_dbda_framebuffer_active, &ds_158a, 3);
	cs_c21b_draw_sprite_list(ds_11dd);
	// cs_5ba0_copy_rect();

	memset(&ds_4856, 0, sizeof ds_4856);

	cs_da5f_remove_frame_task(cs_0b45);
	cs_da25_add_frame_task(3, cs_0b45);

	cs_c412_copy_active_framebuffer_to_fb2();

	// Tail-call
	// cs_ab15_audio_start_voc(3);
}

void cs_0a16()
{
	FUNC_NAME;
	byte *tmp = ds_dbda_framebuffer_active;

	cs_0a23();

	ds_dbda_framebuffer_active = tmp;
}

void cs_0a23()
{
	FUNC_NAME;
	if (ds_227d) {
		cs_c08e_set_screen_as_active_framebuffer();

		printf("%p: ds_dbd6_framebuffer_1\n", ds_dbd6_framebuffer_1);
		printf("%p: ds_dbd8_framebuffer_screen\n", ds_dbd8_framebuffer_screen);
		printf("%p: ds_dbda_framebuffer_active\n", ds_dbda_framebuffer_active);
		printf("%p: ds_dbdc_framebuffer_2\n", ds_dbdc_framebuffer_2);
		printf("\n");
		printf("%p: orig_framebuffer_1\n", orig_framebuffer_1);
		printf("%p: orig_framebuffer_screen\n", orig_framebuffer_screen);
		printf("%p: orig_framebuffer_2\n", orig_framebuffer_2);

		// Tail-call
		cs_ca60_hnm_do_frame();
	} else {
		TODO;
		exit(0);
	}
}

void cs_0a44(uint16_t cx)
{
	cs_c13e_open_sprite_bank_resource_by_index(44);
	cs_5ba8_copy_game_area_rect_to_sprite_clip_rect();
	cs_c07c_set_fb1_as_active_framebuffer();
	cs_c432_clear_game_area_rect();

	int offset = -(((cx & 0xff) * (cx & 0xff)) >> 1);
	cs_c305_draw_sprite_clipped(0, offset, 0);
	cs_c305_draw_sprite_clipped(1, offset + 304, 0);
	cs_c30d_draw_sprite_clipped(2, offset + 608, 0);
}

void cs_0b45()
{
	FUNC_NAME;
	printf("{ %d, %d, %d, %d, %d, %d }\n", ds_4856.ds_4856_offset_0, ds_4856.ds_4857_offset_1, ds_4856.ds_4858_offset_2,
	       ds_4856.ds_485a_offset_4, ds_4856.ds_485c_offset_6, ds_4856.ds_485d_offset_7);

	ds_4856.ds_485d_offset_7 -= 1;
	if (ds_00ea <= 0 && ds_4856.ds_485d_offset_7 <= 16) {
		cs_0d0d();
	}

	if (ds_479e || ds_46eb) {
		return;
	}

	ds_4856.ds_485a_offset_4 -= 1;
	if (ds_4856.ds_485a_offset_4 < 0) {
		ds_4856.ds_485c_offset_6 -= 1;
		if (ds_4856.ds_485c_offset_6 < 0) {
			uint16_t ax = cs_e3cc_rand();
			ds_4856.ds_485c_offset_6 = ax & 0x7f;
			ds_4856.ds_485a_offset_4 = (ax >> 8);
		} else {
			uint16_t ax = cs_e3cc_rand();
			byte bx = ax & 0x7f;
			uint16_t dx = ((msb(lo(ax))) << 9) + hi(ax);
			if (bx > 0x2f && bx < 0x60 && dx < 320) {
				cs_c60b((bx & 7) + 28);
			}
		}
	}

	ds_4856.ds_4856_offset_0 -= 1;
	if (ds_4856.ds_4856_offset_0 < 0) {
		// cs_0c3b();
	}

	/*

	puVar10 = (uint *)0x3cc0;
	iVar8 = *(int *)0x3cbe;
	do {
	if (iVar8 == 0) {
	    return;
	}
	uVar6 = puVar10[4];
	if ((byte)uVar6 < 0x14) {
	    puVar3 = (uint *)0x0;
	    uVar4 = *puVar3;
	    *puVar3 = *puVar3 << 1 | (uint)((int)uVar4 < 0);
	    puVar3 = (uint *)0x0;
	    uVar5 = *puVar3;
	    *puVar3 = *puVar3 << 1 | (uint)((int)uVar5 < 0);
	    uVar6 = ((uVar6 >> 2) << 1 | (uint)((int)uVar4 < 0)) << 1 | (uint)((int)uVar5 < 0);
	LAB_1000_0c09:
	    puVar10[4] = uVar6;
	    FUN_1000_c661();
	    if (((0x13f < *puVar10) || ((int)puVar10[2] < 0)) || ((int)puVar10[3] < 0))
	    goto LAB_1000_0c2f;
	}
	else {
	    if ((byte)uVar6 < 0x1c) {
	    uVar7 = *(undefined2 *)((int)puVar10 + 0xd);
	    FUN_1000_0cea();
	    FUN_1000_0cea();
	    *(undefined2 *)((int)puVar10 + 0xd) = uVar7;
	    uVar6 = *(uint *)0x0;
	    uVar6 = uVar6 << 3 | uVar6 >> 0xd;
	    *(uint *)0x0 = uVar6;
	    uVar6 = (uint)(byte)(((byte)uVar6 & 7) + 0x14);
	    goto LAB_1000_0c09;
	    }
	    uVar6 = uVar6 + 1;
	    if ((byte)uVar6 < 0x2e) goto LAB_1000_0c09;
	LAB_1000_0c2f:
	    FUN_1000_c58a();
	}
	puVar10 = (uint *)((int)puVar10 + 0x11);
	iVar8 = iVar8 + -1;
	} while( true );

	*/
}
/*
void cs_0c3b(ds_4856_t &ds_4856)
{
    ds_4856.ds_4857_offset_1 -= 1;
    if (ds_4856.ds_4857_offset_1 < 0) {
        if ((ds_0000_rand_bits & 3) == 0) {
            ds_4856.ds_485d_offset_7 = 0xb;
            if ((ds_0000_rand_bits & 0xc) == 0) {
                ds_4856.ds_485d_offset_7 = 0x11;
            }
        }

        uint16_t ax = cs_e3cc_rand();
        if (ds_473a) {
            ax &= 0xffef;
        }
        uint16_t cx = ax;
        ax = cs_e3b7_rand(0x7);
        ds_4856.ds_4857_offset_1 = lo(ax);
        if (ax >= 4) {
            cx |= 0x4000;
        }
        ds_4856.ds_4858_offset_2 = cx;
    }


    exit(0);
}
*/
void cs_0d0d()
{
	TODO;
	exit(0);
}

void cs_0f66_nullsub() {}

void cs_1797_ui_head_draw()
{
	int16_t previous_bank_id = ds_278e_active_bank_id;
	cs_c137_load_icones();

	// Shoulders
	cs_c22f_draw_sprite(15, 126, 148);

	// Head
	cs_c22f_draw_sprite(16 + ds_00e8_ui_head_index, 150, 137);

	// Tail-call
	cs_c13e_open_sprite_bank_resource_by_index(previous_bank_id);
}

void cs_17be()
{
	cs_c07c_set_fb1_as_active_framebuffer();
	// if (ds_00e8_ui_head_index == 0) {
	// 	cs_c446_copy_rect_fb2_to_fb1();
	// } else {
	// 	vga_1c76_gfx_26_copy_framebuffer_rect();
	// }
}

void cs_17e6_ui_head_animate_up()
{
	if (ds_00e8_ui_head_index == 10) {
		return;
	}
	++ds_00e8_ui_head_index;
	cs_17be();
}

void cs_1843()
{
	if (ds_00e8_ui_head_index == 0) {
		return;
	}
	TODO;
	exit(0);
	ds_00e8_ui_head_index = 9;
	cs_17be();

	cs_e387_wait(8);

	ds_00e8_ui_head_index = 8;
	// Tail-call
	cs_17be();
}

void cs_1860()
{
	if (ds_11c9 != 0) {
		return;
	}

	cs_1843();

	cs_daa3_clear_some_mouse_rect();

	ds_00fb = -ds_00fb;
	if (ds_00fb < 0) {
		TODO;
		// cs_5a1a();
		exit(0);
	}
	cs_d2bd();
	cs_5adf();

	TODO;
	// cs_28e7 = ds_28e8;
	// cs_b930_remove_globe_frame_tasks();

	ds_1ae6_ui_elements[21].flags = 0x80;
	ds_1ae6_ui_elements[22].flags = 0x80;

	cs_c097_gfx_call_with_fb1_as_screen(cs_d7b2_ui_set_and_draw_frieze_sides_globe);

	// Tail call by fall-through
	cs_189a(52);
}

void cs_189a(int transition_type)
{
	if (ds_46d9 == 0) {
		cs_2db1();
		return;
	}

	cs_c108_transition(transition_type, cs_2db1);
	cs_c07c_set_fb1_as_active_framebuffer();
	TODO;
	// cs_ae04(); Music related
	// cs_17e6(); // Raise head
}

void cs_1a34_ui_draw_date_and_time_indicator()
{
	pos_t *p = &ds_1e7e_sun_and_moon_positions[2 * (ds_0002_game_time & 0xf)];

	cs_1a9b_ui_draw_date_and_time(p, 74);
	cs_1a9b_ui_draw_date_and_time(p, 75);

	cs_d075_font_select_small_font();
	ds_dbe4_font_draw_color = 0xf1fa;

	uint16_t day = cs_1ad1_game_time_get_day() % 365 + 1;
	uint16_t x = 11;
	uint16_t y = 190;
	if (day < 100) {
		x -= 2;
		if (day < 10) {
			x -= 2;
		}
	}
	cs_e290_font_draw_number_at_position_right_aligned(x, y, day);
	ds_2518_font_glyph_draw_func(' ');
}

void cs_1a9b_ui_draw_date_and_time(pos_t *&p, uint16_t sprite_id)
{
	uint16_t x = p->x;
	uint16_t y = p->y;
	++p;

	// printf("x, y: (%3d, %3d)\n", x, y);

	if (!x) {
		return;
	}

	ptr_offset_t sprite_data = cs_c1f4(sprite_id);

	uint16_t w0 = sprite_data.readle16();
	uint16_t w1 = sprite_data.readle16();

	byte flags = ((w0 & 0xfe00) >> 8);
	int width = (w0 & 0x01ff);
	byte pal_offset = (w1 & 0xff00) >> 8;
	int height = (w1 & 0x00ff);

	vga_1452_blit_clipped(ds_dbd8_framebuffer_screen, x, y, sprite_data.ptr(), width, height, flags, pal_offset,
	                      ds_1efe_time_indicator_clip_rect);
}

uint16_t cs_1ad1_game_time_get_day()
{
	return (ds_0002_game_time + 3) / 16;
}

void cs_274e()
{
	cs_c08e_set_screen_as_active_framebuffer();
	cs_c13e_open_sprite_bank_resource_by_index(0x15); // COMM.HSQ
	cs_c21b_draw_sprite_list(ds_14c8_sprite_list);
	cs_275e(1);
}

void cs_275e(byte al)
{
	cs_c08e_set_screen_as_active_framebuffer();
	al = (al & 7) + 11;
	cs_c22f_draw_sprite(al, 100, 86);

	// Tail-call
	// cs_070c();
}

void cs_2db1() {}

void cs_2d74_open_sal_resource()
{
	printf("cs_2d74_open_sal_resource: ds_114e = %d\n", ds_114e);

	if (ds_114e < 1) {
		return;
	}
	uint16_t ax = cs_5e4f(ds_114e, 0);
	ax = 0;
	if (ax < 2 && (ds_4732 & 1)) {
		return;
	}
	if (ax > 4) {
		return;
	}
	if (ax == 4) {
		ax -= 1;
	}
	if (ds_144c == ax) {
		return;
	}

	ds_bc6e_room_sheet_data = (byte *)malloc(65536);
	memset(ds_bc6e_room_sheet_data, 0, 64000);
	cs_f0b9_res_read_by_index_into_buffer(161 + ax, ds_bc6e_room_sheet_data);
	cs_0098_adjust_subresource_pointers_IGNORE();
}

void cs_37b2()
{
	TODO;
	// cs_98e6();
	// Tail-call by fallthrough
	cs_37b5();
}

void cs_37b5()
{
	cs_4d00();
	ds_472d = 0;
	cs_5ba8_copy_game_area_rect_to_sprite_clip_rect();
	cs_c432_clear_game_area_rect();

	byte al = 0xff;
	if (ds_0008_location != al) {
		uint16_t dx = ds_0004;
		byte *p = cs_3efe(dx);
		al = *p;
	}

	al &= 0x7f;
	if ((al & 0x80) == 0) {
		cs_39ec_room_draw(al);
		return;
	}

	// TODO;
	exit(0);
}

void cs_3916()
{
	TODO;
}

void cs_391d()
{
	uint16_t count = 3 * 151;
	uint16_t offset = 3 * 73;
	if (ds_22e3 == 0) {
		count = 3 * 80;
		offset = 3 * 128;
	}
	byte al = ds_46d7;
	vga_0ad7_gfx_39(al, offset, count);
	if (ds_227d == 0) {
		uint16_t count = 3 * 16;
		uint16_t offset = 3 * 240;
		vga_0ad7_gfx_39(al, offset, count);
	}
	if (ds_46d7-- == 0) {
		cs_da5f_remove_frame_task(cs_3916);
	}
}

ptr_offset_t cs_3978(uint8_t al, uint8_t bl)
{
	ds_dbb4_last_bank_palette = al;
	cs_c13e_open_sprite_bank_resource_by_index(al);

	ds_46d6 = bl;
	return cs_c1f4(bl) + 6;
}

void cs_39ec_room_draw(byte sprite_bank_index_and_room_index)
{
	TODO;
	cs_3b59_room_draw(sprite_bank_index_and_room_index);
}

void cs_3b59_room_draw(byte sprite_bank_index_and_room_index)
{
	sprite_bank_index_and_room_index = 0x72;
	printf("cs_3b59_room_draw(0x%04x)\n", sprite_bank_index_and_room_index);
	sprite_bank_index_and_room_index -= 1;
	uint16_t sprite_bank_index = ((uint16_t)sprite_bank_index_and_room_index) >> 4;
	if (sprite_bank_index) {
		cs_c13e_open_sprite_bank_resource_by_index(sprite_bank_index + 19);
	}
	int room_index = sprite_bank_index_and_room_index & 0x0f;

	printf("room_index = %d\n", room_index);
	ptr_offset_t si = make_ptr_offset(ds_bc6e_room_sheet_data);
	uint16_t offset = si.peekle16_at_offset(2 * room_index);
	// printf("offset = %d\n", offset);
	si += offset;
	// exit(0);

	int8_t position_markers[23];

	cs_3d83_room_read_position_markers(si, position_markers);

	uint8_t polygon_color = 16;
	for (int i = 0;; ++i) {
		// printf("%03d\t", i);
		uint16_t cmd = si.readle16();
		if (cmd == 0xffff) {
			break;
		}

		if ((cmd & 0x8000) == 0) {
			// Draw sprite
			// printf("Draw sprite\n");

			uint16_t x = si.readbyte() + ((cmd & 0x0200) ? 256 : 0);
			uint16_t y = si.readbyte();
			uint16_t pal_offset = si.readbyte();

			if ((cmd & 0x1ff) == 1) {
				// Marker
				// printf("MARKER: %04x: (%3x, %02x, %02x)\n", cmd, x, y, pal_offset);
				// Call draw_sprite from PERS.HSQ
				continue;
			}

			cs_c21a_sprite_pal_offset = pal_offset;
			uint16_t flags_and_id = (cmd & 0xfdff) - 1;

			// continue;
			cs_c22f_draw_sprite(flags_and_id, x, y);
		} else if ((cmd & 0x4000) == 0) {
			// Draw polygon
			// printf("Draw polygon\n");

			cs_3be9_room_draw_polygon(si);
		} else if ((cmd & 0x4000)) {
			// Draw line
			// printf("Draw line\n");

			uint16_t x0 = si.readle16();
			uint16_t y0 = si.readle16();
			uint16_t x1 = si.readle16();
			uint16_t y1 = si.readle16();
			byte al = cmd & 0xff;
			uint16_t bp = 0xffff;
			rect_t rect = {0, 0, 320, 152};

			// continue;
			vga_1a07_draw_line(ds_dbda_framebuffer_active, x0, y0, x1, y1, al, bp, &rect);
		}

		// char filename[32];
		// sprintf(filename, "room-part-%02d.ppm", i);
		// dump_ppm(filename, ds_dbda_framebuffer_active, vga_05bf_palette_unapplied, 320, 200);
	}
	// exit(0);

	cs_c21a_sprite_pal_offset = 0;
}

uint16_t *xs0 = nullptr;

void cs_3be9_room_draw_polygon(ptr_offset_t &si)
{
	uint16_t polygon_side_down[200] = {0};
	uint16_t polygon_side_up[200] = {0};

	uint16_t cmd = si.peekle16_at_offset(-2);

	int16_t h_gradiant = 16 * si.read_int8();
	int16_t v_gradiant = 16 * si.read_int8();

	uint16_t start_x = si.readle16();
	uint16_t start_y = si.readle16();

	// Part 1
	// printf("Part 1\n");
	uint16_t x, y;
	uint16_t last_x = start_x & 0x3fff;
	uint16_t last_y = start_y;
	uint16_t *xs;

	xs0 = xs = polygon_side_down;
	do {
		x = si.readle16();
		y = si.readle16();
		cs_3e13_room_add_polygon_section(last_x, last_y, x & 0x3fff, y, start_y, xs, "down", xs0);
		last_x = x & 0x3fff;
		last_y = y;
	} while ((x & 0x4000) == 0);
	uint16_t final_x = last_x;
	uint16_t final_y = last_y;

	// Part 2
	// printf("Part 2\n");
	last_x = start_x;
	last_y = start_y;
	xs0 = xs = polygon_side_up;
	if ((x & 0x8000) == 0) {
		do {
			x = si.readle16();
			y = si.readle16();
			cs_3e13_room_add_polygon_section(last_x, last_y, x & 0x3fff, y, start_y, xs, "up", xs0);
			last_x = x & 0x3fff;
			last_y = y;
		} while ((x & 0x8000) == 0);
	}

	// Part 3
	// printf("Part 3\n");
	cs_3e13_room_add_polygon_section(last_x, last_y, final_x, final_y, start_y, xs, "up", xs0);

	static uint16_t polygon_color = 16;

	uint16_t galois_seed = !!(cmd & 0x3e00);
	uint16_t galois_mask = (cmd & 0x3e00) | 2;

	// printf("galois_seed = %04x\n", galois_seed);
	// printf("galois_mask = %04x\n", galois_mask);

	if ((cmd & 0x0100) == 0) {
		uint16_t ax = lo(cmd) << 8;
		// printf("start_y = %d\n", start_y);
		// printf("final_y = %d\n", final_y);
		for (y = 0; y < final_y - start_y; ++y) {
			// printf("      y = %d\n", y);
			int x0 = polygon_side_up[y];
			int x1 = polygon_side_down[y];
			if (x1 < x0) {
				std::swap(x0, x1);
			}
			int w = x1 - x0 + 1;

			v_gradiant = 0;
			h_gradiant = 0;

			// printf("draw_gradient_line_with_noise(%d, %d, %d);\n", x0, y + start_y, w);

			vga_39e9_gfx_36_draw_gradient_line_with_noise(x0, y + start_y, w, 1, galois_seed, galois_mask, h_gradiant,
			                                              ax);
			ax += v_gradiant;
		}
	} else {
		uint16_t ax = lo(cmd) << 8;

		for (y = 0; y < final_y - start_y; ++y) {
			int x0 = polygon_side_up[y];
			int x1 = polygon_side_down[y];
			if (x1 < x0) {
				std::swap(x0, x1);
			}
			int w = x1 - x0 + 1;

			v_gradiant = 0;
			h_gradiant = 0;

			vga_39e9_gfx_36_draw_gradient_line_with_noise(x0 + w - 1, y + start_y, w, -1, galois_seed, galois_mask,
			                                              h_gradiant, ax);
			ax += v_gradiant;
		}
	}

	polygon_color += 37;
}

void cs_3d83_room_read_position_markers(ptr_offset_t &r, int8_t *position_markers)
{
	for (int i = 0; i != 23; ++i) {
		position_markers[i] = -1;
	}

	r.readbyte();

	uint16_t ds_4774 = 0;
	if (ds_4774) {

	} else {
	}
	TODO;
}

void cs_3e2f_add_polygon_section_horizontal(uint16_t x0, uint16_t x1, uint16_t y1, uint16_t delta_x, uint16_t *&xs,
                                            const char *s, uint16_t *xs0)
{
	// printf("\t%s[%ld] = %d\n", s, xs - xs0, std::min(x0, x1));
	*xs++ = std::min(x0, x1);
}

void cs_3e52_add_polygon_section_vertical(uint16_t x0, uint16_t y0, int delta_y, int sign_y, uint16_t *&xs,
                                          const char *s, uint16_t *xs0)
{
	if (sign_y < 0) {
		y0 -= delta_y;
	} else {
		y0 += delta_y;
	}
	int n = delta_y + 1;
	do {
		// printf("\t%s[%ld] = %d\n", s, xs - xs0, x0);
		*xs++ = x0;
	} while (--n);
}

void cs_3e13_room_add_polygon_section(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t start_y,
                                      uint16_t *&xs, const char *s, uint16_t *xs0)
{
	// printf("polygon section: (%d, %d) -> (%d, %d)\n", x0, y0, x1, y1);
	int delta_x = -(x0 - x1);
	int delta_y = -(y0 - y1);

	if (delta_x == 0 && delta_y == 0) {
		return;
	}

	if (delta_y == 0) {
		// printf("\tdelta_y = 0\n");
		cs_3e2f_add_polygon_section_horizontal(x0, x1, y1, delta_x, xs, s, xs0);
		return;
	}
	int sign_y = 1;
	if (delta_y < 0) {
		delta_y = -delta_y;
		sign_y = -sign_y;
	}

	if (delta_x == 0) {
		// printf("\tdelta_x = 0\n");
		cs_3e52_add_polygon_section_vertical(x0, y0, delta_y, sign_y, xs, s, xs0);
		return;
	}
	int sign_x = 1;
	if (delta_x < 0) {
		sign_x = -sign_x;
		delta_x = -delta_x;
	}

	int16_t bp_6 = sign_y;
	int16_t bp_4 = sign_x;
	int16_t bp_2 = sign_y;
	int16_t bp_0 = sign_x;

	int minor_delta = delta_y;
	int major_delta = delta_x;

	if (delta_x > delta_y) {
		bp_2 = 0;
	} else {
		if (delta_y == 0)
			return;
		std::swap(minor_delta, major_delta);
		bp_0 = 0;
	}

	int16_t ax = major_delta / 2;
	int16_t cx = major_delta;
	do {
		ax += minor_delta;

		int16_t dx, bx;
		if (ax >= major_delta) {
			ax -= major_delta;
			dx = bp_4;
			bx = bp_6;
		} else {
			dx = bp_0;
			bx = bp_2;
		}

		dx += x0;

		if (bx == 1) {
			// printf("\t%s[%3ld] = %3d\n", s, xs - xs0, x0);
			// printf("*");
			*xs++ = x0;
		}
		// printf("\n");

		x0 = dx;
	} while (--cx);
}

byte *cs_3efe(uint16_t dx)
{
	byte dh = hi(dx);
	byte dl = lo(dx);
	byte *p = ds_13c4[dh] + 5 * (dl - 1);

	return p;
}

void cs_4afd()
{
	if (ds_227d) {
		cs_4b16();
		return;
	}
	TODO;
	exit(0);
}

void cs_4b16()
{
	if (ds_dbd6_framebuffer_1 == ds_dbd8_framebuffer_screen) {
		return;
	}
	vga_1be7_copy_game_area(ds_dbd8_framebuffer_screen + 24 * 320, ds_dbd6_framebuffer_1);
}

void cs_4bb9()
{
	TODO;
}

void cs_4d00()
{
	cs_da5f_remove_frame_task(cs_4bb9);
}

void cs_5adf()
{
	cs_7b36();

	TODO; // Clear a bunch of variables

	// cs_da5f_remove_frame_task(cs_6b36);
}

bool cs_a2ef_is_pcm_enabled()
{
	// return ds_dbc8 & 1;
	return true;
}

void cs_5b99_copy_rect(rect_t *dst, rect_t *src)
{
	dst->x0 = src->x0;
	dst->x1 = src->x1;
	dst->y0 = src->y0;
	dst->y1 = src->y1;
}

void cs_5ba8_copy_game_area_rect_to_sprite_clip_rect()
{
	cs_5b99_copy_rect(&ds_d834_sprite_clip_rect, &ds_1470_game_area_rect);
}

uint16_t cs_5e4f(uint16_t si, uint16_t ax)
{
	printf("cs_5e4f: si: %04x, ax: %04x, ", si, ax);
	byte cl = ds_0100[si - 1][8];
	printf("ds_0100[%d][8]: %d -> ", si - 1, cl);
	if (cl < 32) {
		printf("ax: %04x\n", ax);
		return ax;
	}
	ax += 1;
	if (cl < 33) {
		printf("ax: %04x\n", ax);
		return ax;
	}
	ax += 1;
	if (cl < 40) {
		printf("ax: %04x\n", ax);
		return ax;
	}
	ax += 1;
	if (cl < 48) {
		printf("ax: %04x\n", ax);
		return ax;
	}
	ax += 1;
	printf("ax: %04x\n", ax);
	return ax;
}

void cs_5f79()
{
	uint16_t ax = 0;
	std::swap(ax, ds_46f8);
	if (ds_46f8 != 0) {
		TODO;
		exit(0);
	}
}

void cs_79de()
{
	uint16_t ax = 0;
	std::swap(ax, ds_46fa);
	if (ds_46fa == 0) {
		cs_c07c_set_fb1_as_active_framebuffer();
		return;
	}
	TODO;
	exit(0);
}

void cs_7b36()
{
	ds_46d8 = 1;
	ds_dce6_in_transition = 0x80;

	cs_8770();
	cs_5f79();
	cs_79de();

	ds_dce6_in_transition = 0;
	ds_46f4 = 0;
}

void cs_8770()
{
	if (ds_1954 != 0) {
		TODO;
		exit(0);
	}
}

byte cs_9123(uint8_t al)
{
	if (al >= 17) {
		TODO;
		exit(0);
	}
	uint8_t ah = 0;
	if (al < 0x0d) {
		ds_47d0 = al;
		return al;
	}
	TODO;
	exit(0);
}

void cs_91a0_setup_lip_sync_data_from_sprite_sheet(uint16_t ax)
{
	if (ax == 0x0c) {
		TODO;
		exit(0);
	}
	ax = cs_9123(ax);
	if (ax == ds_22a6_talking_head_id) {
		// Tail-call
		cs_920f_open_talking_head_resource(ax);
		return;
	}

	// cs_98b2();
	ds_22a6_talking_head_id = ax;
	cs_920f_open_talking_head_resource(ax);

	ptr_offset_t r = ds_dbb0_current_resource_ptr;

	uint16_t toc_start = r.offset();
	uint16_t toc_length = r.peekle16();

	// Seek to last block
	r += toc_length - 2;

	uint16_t last_block_offset = r.peekle16();

	r.set_offset(toc_start + last_block_offset + 2);
	uint16_t len = r.readle16();

	printf("Talking head data:\n\n");

	rect_t clip_rect;
	clip_rect.x0 = r.readle16();
	clip_rect.y0 = r.readle16();
	clip_rect.x1 = r.readle16();
	clip_rect.y1 = r.readle16();

	ds_1ae6_ui_elements[19].x0 = clip_rect.x0;
	ds_1ae6_ui_elements[19].y0 = clip_rect.y0;
	ds_1ae6_ui_elements[19].x1 = clip_rect.x1;
	ds_1ae6_ui_elements[19].y1 = clip_rect.y1;

	printf("Clip rect: (%d, %d, %d, %d)\n\n", clip_rect.x0, clip_rect.y0, clip_rect.x1, clip_rect.y1);

	uint16_t section_1_len = r.peekle16();
	ds_47cc_lip_sync_section_1_ptr = r + 2;
	printf("Section 1:\n");
	// hexdump(ds_47cc_lip_sync_section_1_ptr.ptr(), section_1_len - 2, ds_47cc_lip_sync_section_1_ptr.offset());
	// printf("\n");

	{
		ptr_offset_t section_1_toc = ds_47cc_lip_sync_section_1_ptr;
		// printf("section_1_toc: %04x\n", section_1_toc.offset());

		uint16_t toc_entries = section_1_toc.peekle16() / 2;

		hexdump(ds_47cc_lip_sync_section_1_ptr.ptr(), toc_entries / 2, ds_47cc_lip_sync_section_1_ptr.offset());

		for (int i = 0; i != toc_entries; ++i) {
			// printf("%04x - %04x\n", section_1_toc.peekle16_at_offset(2 * i), section_1_toc.peekle16_at_offset(2 * (i
			// + 1)));
			ptr_offset_t sub_r = section_1_toc + section_1_toc.peekle16_at_offset(2 * i);
			int sub_len;
			if (i < toc_entries - 1) {
				sub_len = section_1_toc.peekle16_at_offset(2 * (i + 1)) - section_1_toc.peekle16_at_offset(2 * i);
			} else {
				sub_len = section_1_len - 2 - section_1_toc.peekle16_at_offset(2 * i);
			}
			printf("%02x: ", i);
			hexdump(sub_r.ptr(), sub_len, sub_r.offset());
			// printf("\n");
		}
	}

	r += section_1_len;
	ds_47ca_lip_sync_section_2_ptr = r;

	uint16_t sub_toc_len = r.peekle16();

	printf("Section 2 Table of Contents:\n");
	hexdump(ds_47ca_lip_sync_section_2_ptr.ptr(), sub_toc_len, ds_47ca_lip_sync_section_2_ptr.offset());
	printf("\n");

	for (int i = 0; i != sub_toc_len / 2; ++i) {
		uint16_t sub_offset = r.peekle16_at_offset(2 * i);
		ptr_offset_t sub_r = r + sub_offset;
		uint16_t sub_len = 0;
		for (int j = 0;; ++j) {
			if (sub_r.peek_byte_at_offset(j) == 0xff) {
				sub_len = j + 1;
				break;
			}
		}
		// printf("sublen: 0x%04x\n", sub_len);

		printf("Section 2, child %d\n", i);
		hexdump(sub_r.ptr(), sub_len, sub_r.offset());
		printf("\n");
	}

	uint16_t last_entry_offset = r.peekle16_at_offset(sub_toc_len - 2);

	ds_47d2_lip_sync_section_3_ptr = r + last_entry_offset;
	hexdump(ds_47d2_lip_sync_section_3_ptr.ptr(), 0x1d, ds_47d2_lip_sync_section_3_ptr.offset());
	printf("\n");
}

void cs_920f_open_talking_head_resource(uint16_t ax)
{
	// Tail-call
	cs_c13e_open_sprite_bank_resource_by_index(ax + 2);
}

void cs_978e()
{
	// cs_4aca();
	if (ds_47c4 == 0xffff) {
		return;
	}

	cs_91a0_setup_lip_sync_data_from_sprite_sheet(ds_47c4);

	TODO;

	cs_c0f4();
}

void cs_a3f9_settings_ui_draw()
{
	byte *tmp = ds_dbda_framebuffer_active;
	// cs_c08e_set_screen_as_active_framebuffer();

	cs_c13e_open_sprite_bank_resource_by_index(0x55); // MIXR
	cs_c22f_draw_sprite(0, ds_2886_settings_ui_global_x_offset, ds_2888_settings_ui_global_y_offset);

	ds_dbda_framebuffer_active = tmp;
}

void cs_aa0f_decode_sound_block() {}

void cs_ad57_play_music_morning()
{
	TODO;
}

void cs_b827_draw_globe_and_ui_to_front_buffer()
{
	// ds_dbda_framebuffer_active = ds_dbd6_framebuffer_1;
	// cs_b84a_draw_globe_with_blue_background();
	ds_dd0f_globe_fresco_offset = 0;
	cs_b87e_draw_globe_frescos();
	cs_1797_ui_head_draw();
	// cs_b941();
	cs_c097_gfx_call_with_fb1_as_screen(cs_d7b2_ui_set_and_draw_frieze_sides_globe);

	byte *old_screen = ds_dbd8_framebuffer_screen;
	ds_dbd8_framebuffer_screen = ds_dbd6_framebuffer_1;
	cs_d72b(ds_1dc6_ui_globe_rotation_controls);
	ds_dbd8_framebuffer_screen = old_screen;

	// cs_c097_gfx_call_with_fb1_as_screen(cs_d1ef_draw_all_ui_elements);
}

void cs_b84a_draw_globe_with_blue_background()
{
	cs_c07c_set_fb1_as_active_framebuffer();
	printf("#$#@$#@$@#$@ %d\n", ds_dbda_framebuffer_active == ds_dbd8_framebuffer_screen);
	vga_197b_gfx_10_fill_rect(ds_dbda_framebuffer_active, &ds_1470_game_area_rect, 240);

	// Tail-call by fallthrough
	cs_b85a_draw_globe_with_atmosphere();
}

void cs_b85a_draw_globe_with_atmosphere()
{
	cs_c13e_open_sprite_bank_resource_by_index(1); // FRESK

	// Globe atmosphere
	cs_c22f_draw_sprite(2, 91, 20);

	cs_b977_draw_globe();
}

void cs_b87e_draw_globe_frescos()
{
	cs_5ba8_copy_game_area_rect_to_sprite_clip_rect();
	cs_c13e_open_sprite_bank_resource_by_index(1);

	ds_dd0f_globe_fresco_offset = -96;

	// Left globe fresco
	cs_c305_draw_sprite_clipped(0, ds_dd0f_globe_fresco_offset, 0);

	// Right globe fresco
	cs_c305_draw_sprite_clipped(1, 214 - ds_dd0f_globe_fresco_offset, 0);
}

void cs_b8a7_setup_draw_globe()
{
	cs_f0b9_res_read_by_index_into_buffer(0x92, ds_4c60_globdata); // GLOBDATA.HSQ
	cs_ba75_set_globe_tilt_and_rotation(-40, 6500);

	cs_c13e_open_sprite_bank_resource_by_index(1); // FRESK
	cs_c0f4();
}

void cs_b8ea()
{
	cs_da25_add_frame_task(1, cs_b9ae);
}

void cs_b8f3_globe_slide_decorations_open()
{
	cs_b84a_draw_globe_with_blue_background();
	cs_c097_gfx_call_with_fb1_as_screen(cs_be1d_fresk_draw_results_text_and_icons);
	cs_b87e_draw_globe_frescos();
	cs_c474_copy_game_area_rect_fb1_to_fb2();
	do {
		cs_c43e_copy_game_area_rect_fb2_to_fb1();
		cs_b87e_draw_globe_frescos();
		// cs_c4dd();
		ds_dd0f_globe_fresco_offset -= 16;
	} while (ds_dd0f_globe_fresco_offset > -116);
}

void cs_b977_draw_globe()
{
	vga_1cb6_gfx_23(ds_dbd6_framebuffer_1, ds_dd00_map_res, ds_4c60_globdata, 0, 0);
}

void cs_b9ae() {}

void cs_b9e0_globe_rotate(int delta_rotation)
{
	uint16_t globe_rotation = globe_rotation_lookup_table[0].unk2;

	globe_rotation += delta_rotation;

	if (globe_rotation < 0) {
		globe_rotation += 398;
	}
	if (globe_rotation >= 398) {
		globe_rotation -= 398;
	}

	globe_rotation_lookup_table[0].unk2 = globe_rotation;

	cs_b9f6_precalculate_globe_rotation_table();
}

void cs_b9f6_precalculate_globe_rotation_table()
{
	uint16_t dx = globe_rotation_lookup_table[0].unk2;
	uint32_t dxax = (dx << 16) | 0x8000;

	uint16_t ax = dxax / 398;
	dx = dxax % 398;

	// printf("ax: %04x, dx: %04x\n", ax, dx);

	uint16_t bx = ax;
	for (int i = 1; i != 99; ++i) {
		uint32_t dxax = 2 * uint32_t(bx) * uint32_t(globe_rotation_lookup_table[i].unk1);
		globe_rotation_lookup_table[i].unk2 = (dxax >> 16) & 0xffff;
		globe_rotation_lookup_table[i].unk3 = (dxax >> 0) & 0xffff;
	}
}

void cs_ba15_precalculate_globe_tilt_table(int16_t globe_tilt)
{
	int i = 0;

	if (globe_tilt < -98) {
		globe_tilt = -98;
	}
	if (globe_tilt > 98) {
		globe_tilt = 98;
	}

	// For stage 1, count down from globe_tilt-99 to -98
	if (globe_tilt > 0) {
		int v = globe_tilt - 98;
		do {
			ds_2460_globe_tilt_lookup_table[i++] = uint8_t(--v);
		} while (i != 196 && v > -98);
	}

	if (i == 196) {
		return;
	}

	// For stage 2, count down from 98 to 0
	int v = globe_tilt + 98 - i;
	do {
		ds_2460_globe_tilt_lookup_table[i++] = uint8_t(v--);
	} while (i != 196 && v >= 0);

	if (i == 196) {
		return;
	}

	// For stage 3, count up from 1 to 98, binary-or'ed with 0xff00
	v = 1;
	do {
		ds_2460_globe_tilt_lookup_table[i++] = v++ | 0xff00;
	} while (i != 196 && v <= 98);

	if (i == 196) {
		return;
	}

	// For stage 4, count up from -98 to 0, binary-or'ed with 0xff00
	v = -98;
	do {
		ds_2460_globe_tilt_lookup_table[i++] = v++ | 0xff00;
	} while (i != 196 && v <= 0);
}

void cs_ba75_set_globe_tilt_and_rotation(int16_t tilt, uint16_t rotation)
{
	uint32_t dxax = 398 * uint32_t(rotation);

	globe_rotation_lookup_table[0].unk2 = (dxax >> 16);
	globe_rotation_lookup_table[0].unk3 = 0;

	if (tilt > 32) {
		tilt = 32;
	}
	if (tilt < -32) {
		tilt = -32;
	}

	cs_b9f6_precalculate_globe_rotation_table();
	cs_ba15_precalculate_globe_tilt_table(tilt);
}

void cs_bdfa_ui_stats_draw_ingame_day_and_charisma()
{
	cs_d068_font_select_tall_font();
	cs_d1a6_font_draw_phrase_list(ds_2482_strings);
}

void cs_be1d_fresk_draw_results_text_and_icons()
{
	cs_c13e_open_sprite_bank_resource_by_index(1); // FRESK.HSQ
	// Draw Atriedes and Harkonnen house icons
	cs_c21b_draw_sprite_list(ds_2506);

	cs_bdfa_ui_stats_draw_ingame_day_and_charisma();

	// TODO: Game tiem

	cs_d075_font_select_small_font();
	cs_d1a6_font_draw_phrase_list(ds_2494_strings);

	// cs_da25_add_frame_task(12, cs_be57);
	cs_be57();
	// return;
}

void cs_be57()
{
	cs_c08e_set_screen_as_active_framebuffer();

	// TODO
	// cs_db74_restore_mouse_if_rect_intersects(ds_1470_game_area_rect);

	cs_c137_load_icones();

	for (int i = 0; i != 6; ++i) {
		cs_c22f_draw_sprite(55, ds_24ee[i].x, ds_24ee[i].y);
		cs_c22f_draw_sprite(57, ds_24ee[i].x, ds_24ee[i].y - 10);
	}

	cs_c07c_set_fb1_as_active_framebuffer();
}

void cs_e290_font_draw_number_at_position_right_aligned(int x, int y, uint16_t v)
{
	cs_d04e_font_set_draw_position(x, y);
	cs_e297_font_draw_number_right_aligned(v);
}

void cs_e297_font_draw_number_right_aligned(uint16_t v)
{
	byte hundreds_digit = v / 100 + '0';
	if (hundreds_digit == '0') {
		hundreds_digit = ' ';
	}
	ds_2518_font_glyph_draw_func(hundreds_digit);
	v %= 100;

	byte tens_digit = v / 10 + '0';
	if (hundreds_digit == ' ' && tens_digit == '0') {
		tens_digit = ' ';
	}
	ds_2518_font_glyph_draw_func(tens_digit);
	v %= 10;

	byte ones_digit = v + '0';
	ds_2518_font_glyph_draw_func(ones_digit);
}

void cs_e387_wait(int n)
{
	for (int i = 0; i < n; ++i) {
		uint16_t timer = ds_ce7a_pit_timer_counter;
		while (timer == ds_ce7a_pit_timer_counter) {
			std::this_thread::yield();
		}
	}
}

uint16_t cs_e3b7_rand(uint16_t mask)
{
	uint32_t v = ds_d824_rand_seed * uint32_t(0xe56d);
	ds_d824_rand_seed = v + 1;
	return (v >> 8) & mask;
}

uint16_t cs_e3cc_rand()
{
	uint32_t v = ds_d826_rand_seed * uint32_t(0xcbd1);
	ds_d826_rand_seed = v + 1;
	return v >> 8;
}

void cs_e594_initialize_system()
{
	FUNC_NAME;

	ds_dc32_hnm_buffer = (byte *)calloc(1, 65536);

	cs_e675_dat_open();

	vga_09d9_get_screen_buffer(&ds_dbd8_framebuffer_screen, &ds_ce74_framebuffer_size);

	cs_c08e_set_screen_as_active_framebuffer();
	ds_dbdc_framebuffer_2 = (byte *)calloc(1, 65536);
	ds_dbd6_framebuffer_1 = (byte *)calloc(1, 65536);

	orig_framebuffer_1 = ds_dbd6_framebuffer_1;
	orig_framebuffer_screen = ds_dbd8_framebuffer_screen;
	orig_framebuffer_2 = ds_dbdc_framebuffer_2;

	vga_0967_set_graphics_mode();

	// TODO: Parse command line arguments
	ds_ceeb_language_setting = 0;

	// UNIMPLEMENTED: Initialize joystick
	// TODO: Initialize mouse

	cs_ea7b_init_extended_memory_allocator();

	{
		// vga_0b68_set_palette_to_screen();
		vga_0975(false);
	}

	timer_thread = new std::thread([] {
		auto clock = std::chrono::steady_clock();
		auto desired_interval = std::chrono::duration_cast<std::chrono::nanoseconds>(4.99253ms);
		auto next = clock.now() + desired_interval;

		while (!quitting) {
			auto now = clock.now();
			while (next < now) {
				next += desired_interval;
			}
			std::this_thread::sleep_until(next);

			++ds_ce7a_pit_timer_counter;
			if (ds_ce7a_pit_timer_counter == 0) {
				++ds_ce7c_pit_timer_counter_hi;
			}

			next += desired_interval;
		}
	});

	cs_ce6c_hnm_initialize();
}

void cs_c07c_set_fb1_as_active_framebuffer()
{
	FUNC_NAME;
	ds_dbda_framebuffer_active = ds_dbd6_framebuffer_1;
}

void cs_c085_set_hnm_buffer_as_active_framebuffer()
{
	ds_dbda_framebuffer_active = ds_dc32_hnm_buffer;
}

void cs_c08e_set_screen_as_active_framebuffer()
{
	FUNC_NAME;
	ds_dbda_framebuffer_active = ds_dbd8_framebuffer_screen;
}

void cs_c097_gfx_call_with_fb1_as_screen(void (*fn)(void))
{
	FUNC_NAME;
	cs_c07c_set_fb1_as_active_framebuffer();
	byte *old_screen = ds_dbd8_framebuffer_screen;
	ds_dbd8_framebuffer_screen = ds_dbd6_framebuffer_1;
	fn();
	ds_dbd8_framebuffer_screen = old_screen;
}

void cs_c0ad_gfx_clear_active_framebuffer()
{
	FUNC_NAME;
	memset(ds_dbda_framebuffer_active, 0, 64000);
}

void cs_c0f4()
{
	FUNC_NAME;
	if (ds_dbd6_framebuffer_1 != ds_dbd8_framebuffer_screen) {
		vga_0b0c_gfx_32_set_screen_palette();
	}
}

uint16_t cs_c13c = 0x25; // ONMAP.HSQ

void cs_c13b_open_onmap_resource()
{
	cs_c13e_open_sprite_bank_resource_by_index(cs_c13c);
}

void cs_c13e_open_sprite_bank_resource_by_index(int16_t index)
{
	if (index < 0 || ds_278e_active_bank_id == index) {
		return;
	}
	int16_t prev_active_bank_id = ds_278e_active_bank_id;
	ds_278e_active_bank_id = index;

	if (prev_active_bank_id >= 0) {
		uint16_t timer_lo = ds_ce7a_pit_timer_counter;
		uint16_t timer_hi = ds_ce7c_pit_timer_counter_hi;
		uint32_t timer = (timer_hi << 16) + timer_lo;
		ds_dabc_resource_last_used[prev_active_bank_id] = (timer >> 8) & 0xffff;
	}

	ptr_offset_t res_ptr = ds_d844_resource_ptr[index];
	if (!res_ptr.begin()) {
		byte *buf = cs_f0b9_res_read_by_index_into_buffer(index);
		res_ptr = make_ptr_offset(buf);

		uint16_t res_head = res_ptr.peekle16();
		if (res_head > 2) {
			cs_c1aa_apply_bank_palette(res_ptr.begin());
		} else {
			printf("Resource has NO palette\n");
		}

		ds_d844_resource_ptr[index] = ptr_offset_t(buf);
	} else {
		FUNC_NAME;
		if (res_ptr.peekle16() > 2) {
			cs_c1aa_apply_bank_palette(res_ptr.begin());
		} else {
			printf("Resource has NO palette\n");
		}
	}

	uint16_t data_offset = res_ptr.peekle16();
	res_ptr.set_offset(data_offset);
	ds_dbb0_current_resource_ptr = res_ptr;
}

void cs_c102_copy_pal_and_transition(void (*fn)())
{
	FUNC_NAME;
	vga_0a68_gfx_41_copy_pal_1_to_pal_2();

	// Tail-call by fallthrough
	cs_c108_transition(0x3A, fn);
}

void cs_c108_transition(int transition_type, void (*fn)())
{
	FUNC_NAME;
	ds_dce6_in_transition = 0x80;
	cs_c097_gfx_call_with_fb1_as_screen(fn);

	vga_25e7_transition(ds_ce7a_pit_timer_counter, transition_type, ds_dbdc_framebuffer_2, ds_dbd8_framebuffer_screen,
	                    ds_dbd6_framebuffer_1);

	cs_c4cd_copy_fb1_to_screen();
	vga_0b0c_gfx_32_set_screen_palette();

	ds_dce6_in_transition = 0x00;
}

void cs_c137_load_icones()
{
	cs_c13e_open_sprite_bank_resource_by_index(0);
}

void cs_c1aa_apply_bank_palette(const byte *p)
{
	FUNC_NAME;
	if (ds_dbb4_last_bank_palette == ds_278e_active_bank_id) {
		return;
	}
	cs_c1ba_apply_palette(p + 2);
}

void cs_c1ba_apply_palette(const byte *p)
{
	FUNC_NAME;
	for (;;) {
		uint16_t v;

		for (;;) {
			v = readle16(p);
			p += 2;

			if (v != 256) {
				break;
			}
			p += 3;
		}
		if (v == 0xffff) {
			break;
		}

		uint16_t count = hi(v);
		uint16_t offset = lo(v);

		if (count == 0) {
			count = 256;
		}

		offset *= 3;
		count *= 3;

		vga_09e2_set_palette_unapplied(p, offset, count);
		p += count;
	}
}

ptr_offset_t cs_c1f4(uint16_t ax)
{
	uint16_t offset = ::readle16(ds_dbb0_current_resource_ptr.ptr() + 2 * ax);
	return ds_dbb0_current_resource_ptr + offset;
}

void cs_c21b_draw_sprite_list(sprite_position list[])
{
	for (sprite_position *p = list; p->flags_and_id != 0xffff; ++p) {
		cs_c22f_draw_sprite(p->flags_and_id, p->x, p->y);
	}
}

void cs_c2a1_decompress_4bpp_rle_image(ptr_offset_t &src, int w, int h, byte *dst)
{
	for (int y = 0; y != h; ++y) {
		int line_remain = 2 * ((w + 3) / 4);
		do {
			int count;

			byte cmd = src.readbyte();
			if (cmd & 0x80) {
				count = 257 - cmd;
				byte value = src.readbyte();
				for (int i = 0; i != count; ++i) {
					*dst++ = value;
				}
			} else {
				count = cmd + 1;
				for (int i = 0; i != count; ++i) {
					byte value = src.readbyte();
					*dst++ = value;
				}
			}
			// printf("line_remain = %d, count = %d, line_remain - count = %d\n", line_remain, count, line_remain -
			// count);
			line_remain -= count;
		} while (line_remain > 0);
	}
}

void cs_c22f_draw_sprite(uint16_t flags_and_id, uint16_t x, uint16_t y)
{
	byte *dst = ds_dbda_framebuffer_active;
	ptr_offset_t sprite_data = ds_dbb0_current_resource_ptr;

	uint16_t id = flags_and_id & 0x1ff;
	printf("id = %d\n", id);

	uint16_t offset = sprite_data.peekle16_at_offset(2 * id);
	printf("offset = %d\n", offset);
	sprite_data += offset;

	uint16_t w0 = sprite_data.readle16();
	uint16_t w1 = sprite_data.readle16();

	printf("w0 = 0x%04x, w1 = 0x%04x\n", w0, w1);

	byte flags = ((w0 & 0xfe00) >> 8);
	int width = (w0 & 0x01ff);
	byte pal_offset = (w1 & 0xff00) >> 8;
	int height = (w1 & 0x00ff);

	printf("flags = %d, width = %d, pal_offset = %d, height = %d\n", flags, width, pal_offset, height);

	byte draw_flags = (flags_and_id >> 8) & 0xfe;
	flags |= draw_flags;

	bool rle = (flags & 0x80) != 0;
	bool flip_x = (flags & 0x40) != 0;
	bool flip_y = (flags & 0x20) != 0;
	byte scale = (flags & 0x1c) >> 2;
	int bpp = (pal_offset < 254) ? 4 : 8;

	printf("scale = %d\n", scale);

	if (cs_c21a_sprite_pal_offset != 0) {
		pal_offset = cs_c21a_sprite_pal_offset;
	}

	// printf("cs_c22f_draw_sprite: %d\n", ds_dbda_framebuffer_active == ds_dbd8_framebuffer_screen);

	if (scale == 0) {
		// if (rle) {
		// 	uint16_t pitch = 2 * ((width + 3) / 4);
		// 	byte *tmp = new byte[pitch * height];
		// 	memset(tmp, 0, pitch * height);

		// 	cs_c2a1_decompress_4bpp_rle_image(sprite_data, width, height, tmp);
		// }

		vga_0f5b_blit(dst, x, y, sprite_data, width, height, flags, pal_offset);
		return;
	}

	uint16_t scale_factors[8] = {0x100, 0x120, 0x140, 0x160, 0x180, 0x1C0, 0x200, 0x280};
	uint16_t scale_factor_fp = scale_factors[scale];

	uint16_t dst_w = (width << 8) / scale_factor_fp;
	uint16_t dst_h = (height << 8) / scale_factor_fp;

	uint16_t pitch = 2 * ((width + 3) / 4);

	if (!rle) {
		draw_4bpp_scaled(sprite_data.ptr(), pitch, x, y, dst_w, dst_h, flags, pal_offset, scale_factor_fp);
	} else {
		byte *tmp = new byte[pitch * height];
		memset(tmp, 0, pitch * height);

		cs_c2a1_decompress_4bpp_rle_image(sprite_data, width, height, tmp);
		draw_4bpp_scaled(tmp, pitch, x, y, dst_w, dst_h, flags, pal_offset, scale_factor_fp);
	}
}

void cs_c2f2(byte resource_index)
{
	cs_c13e_open_sprite_bank_resource_by_index(resource_index);

	// Tail-call by fallthrough
	cs_c2fd_draw_sprite(0, 0, 0);
}

void cs_c2fd_draw_sprite(uint16_t flags_and_id, uint16_t x, uint16_t y)
{
	cs_c22f_draw_sprite(flags_and_id, x, y);
}

void cs_c305_draw_sprite_clipped(uint16_t flags_and_id, int16_t x, int16_t y)
{
	cs_c30d_draw_sprite_clipped(flags_and_id, x, y);
}

void cs_c30d_draw_sprite_clipped(uint16_t flags_and_id, int16_t x, int16_t y)
{
	byte *dst = ds_dbda_framebuffer_active;
	ptr_offset_t sprite_data = ds_dbb0_current_resource_ptr;

	uint16_t id = flags_and_id & 0x1ff;

	uint16_t offset = sprite_data.peekle16_at_offset(2 * id);
	sprite_data += offset;
	uint16_t w0 = sprite_data.readle16();
	uint16_t w1 = sprite_data.readle16();

	byte draw_flags = (flags_and_id >> 8) & 0x60;
	byte flags = ((w0 & 0xfe00) >> 8) | draw_flags;
	int width = (w0 & 0x01ff);
	int height = (w1 & 0x00ff);
	byte pal_offset = (w1 & 0xff00) >> 8;

	vga_1452_blit_clipped(dst, x, y, sprite_data, width, height, flags, pal_offset, ds_d834_sprite_clip_rect);
}

void cs_c370_blit_repeated_x(byte *fb, rect_t *rect, byte sprite_id)
{
	vga_1979_gfx_09_clear_rect(fb, rect);
	uint16_t dst_x = rect->x0;
	uint16_t dst_y = rect->y0;
	uint16_t dst_w = rect->x1 - rect->x0;
	uint16_t dst_h = rect->y1 - rect->y0;

	ptr_offset_t sprite_data = ds_dbb0_current_resource_ptr;
	uint16_t offset = sprite_data.peekle16_at_offset(2 * sprite_id);
	sprite_data += offset;

	uint16_t w0 = sprite_data.readle16();
	uint16_t w1 = sprite_data.readle16();

	byte flags = ((w0 & 0xfe00) >> 8);
	uint16_t src_w = (w0 & 0x01ff);
	byte pal_offset = (w1 & 0xff00) >> 8;
	uint16_t src_h = (w1 & 0x00ff);

	vga_0f5b_blit(fb, dst_x, dst_y, sprite_data, src_w, src_h, flags, pal_offset);

	uint16_t src_x = dst_x;
	uint16_t src_y = dst_y;
	dst_w -= src_w;
	while (dst_w >= src_w) {
		dst_x += src_w;
		vga_19c9_gfx_31_copy_rect(fb, src_x, src_y, src_w, src_h, dst_x, dst_y);
		dst_w -= src_w;
	}

	if (dst_w > 0) {
		vga_19c9_gfx_31_copy_rect(fb, src_x, src_y, dst_w, src_h, dst_x, dst_y);
	}
}

void cs_c412_copy_active_framebuffer_to_fb2()
{
	FUNC_NAME;
	vga_1b7c_gfx_11_copy_framebuffer(ds_dbdc_framebuffer_2, ds_dbda_framebuffer_active);
}

void cs_c432_clear_game_area_rect()
{
	vga_1979_gfx_09_clear_rect(ds_dbda_framebuffer_active, &ds_1470_game_area_rect);
}

void cs_c43e_copy_game_area_rect_fb2_to_fb1()
{
	cs_c446_copy_rect_fb2_to_fb1(&ds_1470_game_area_rect);
}

void cs_c446_copy_rect_fb2_to_fb1(rect_t *rect)
{
	if (rect->y1 > rect->y0 && rect->x1 > rect->x0) {
		vga_1b8e_gfx_12_copy_framebuffer_rect(ds_dbdc_framebuffer_2, ds_dbd6_framebuffer_1, rect);
	}
}

void cs_c474_copy_game_area_rect_fb1_to_fb2()
{
	cs_c477_copy_rect_fb1_to_fb2(&ds_1470_game_area_rect);
}

void cs_c477_copy_rect_fb1_to_fb2(rect_t *rect)
{
	if (rect->y1 > rect->y0 && rect->x1 > rect->x0) {
		vga_1b8e_gfx_12_copy_framebuffer_rect(ds_dbdc_framebuffer_2, ds_dbd6_framebuffer_1, rect);
	}
}

void cs_c4cd_copy_fb1_to_screen()
{
	FUNC_NAME;
	vga_1b7c_gfx_11_copy_framebuffer(ds_dbd8_framebuffer_screen, ds_dbd6_framebuffer_1);
}

void cs_c60b(uint16_t ax)
{
	TODO;
	exit(0);
}

void cs_c661(struct Particle* particle, int16_t dx_velocity, int16_t bx_velocity) {
    // C661: call _sub_1C13B_open_onmap_resource
    cs_c13b_open_onmap_resource();

    // C664-C670: mov si, di; sub sp, 8; mov di, sp; push ds; pop es; movsw x4
    // mov si, di
    // Copy first 8 bytes of particle to stack buffer
    rect_t particle_rect = particle.bounding_box;

    // C671-C681: sub si, 8; sub di, 8; add [si], dx; add [si+2], bx; add [si+4], dx; add [si+6], bx
    // Apply velocity deltas to particle position and secondary position
    particle->x_pos += dx_velocity;      // add [si], dx
    particle->y_pos += bx_velocity;      // add [si+2], bx
    particle->x_pos2 += dx_velocity;     // add [si+4], dx
    particle->y_pos2 += bx_velocity;     // add [si+6], bx

    // C682-C692: or dx, dx; js short loc_1C68E; mov ax, [si+4]; mov [di+4], ax; jmp short loc_1C692
    // loc_1C68E: mov ax, [si]; mov [di], ax
    // Handle X velocity direction - copy appropriate X coordinate to local buffer
    if (dx_velocity >= 0) {
        particle_rect.x1 = particle->x_pos2; // [di+4] = [si+4]
    } else {
        particle_rect.x0 = particle->x_pos;   // [di] = [si]
    }

    // C692-C6A4: or bx, bx; js short loc_1C69E; mov ax, [si+6]; mov [di+6], ax; jmp short loc_1C6A4
    // loc_1C69E: mov ax, [si+2]; mov [di+2], ax
    // Handle Y velocity direction - copy appropriate Y coordinate to local buffer
    if (bx_velocity >= 0) {
        particle_rect.y1 = particle->y_pos2; // [di+6] = [si+6]
    } else {
        particle_rect.y0 = particle->y_pos;   // [di+2] = [si+2]
    }

    cs_c6ad_particle_system_draw_single_particle(particle_rect);
}

void cs_c6ad_particle_system_draw_single_particle(rect_t rect) {
    // C6AD: call _sub_1C13B_open_onmap_resource
    cs_c13b_open_onmap_resource();

    // C6B0-C6E3: Mouse cursor collision detection and redraw logic
    // cmp ds:_byte_2D0F6_cursor_hide_counter, 0; js short loc_1C6E4
    if (byte_2D0F6_cursor_hide_counter >= 0) {
        uint16_t mouse_x = word_2D0F4_mouse_draw_pos_x;
        uint16_t mouse_y = word_2D0F2_mouse_draw_pos_y;

        // Check if particle overlaps with mouse cursor (16x16 area)
        if (mouse_x < rect.y1 &&                    // mouse_x < right edge
            (mouse_x + 16) > rect.y0 &&             // mouse right > left edge
            mouse_y < rect.x1 &&                    // mouse_y < bottom edge
            (mouse_y + 16) > (rect.x0 & 0x0FFF)) {  // mouse bottom > top edge (masked)

            // C6DB-C6E3: Queue mouse cursor redraw
            cs_dbb2_call_restore_cursor(sub_1DBEC_draw_mouse, rect);
        }
    }

    // C6E4-C717: Calculate clipped rectangle for rendering
    struct ClipRect {
        uint16_t x0, y0, x1, y1;
    } *clip_rect = (struct ClipRect*)0x2CCE4; // _unk_2CCE4_sprite_clip_rect

    struct ClipRect bounds;
    struct ClipRect* screen_bounds = (struct ClipRect*)(0x2CCE4 + 8); // offset +8

    // Clip coordinates to screen bounds (with clamping)
    // C6EE-C6F5: x0 = max(rect.x0, screen_bounds->x0)
    bounds.x0 = (rect.x0 >= screen_bounds->x0) ? rect.x0 : screen_bounds->x0;

    // C6F6-C6FD: y0 = max(rect.y0, screen_bounds->y0)
    bounds.y0 = (rect.y0 >= screen_bounds->y0) ? rect.y0 : screen_bounds->y0;

    // C6FE-C708: x1 = min(rect.x1, screen_bounds->x1), early exit if x1 <= x0
    bounds.x1 = (rect.x1 <= screen_bounds->x1) ? rect.x1 : screen_bounds->x1;
    if (bounds.x1 <= bounds.x0) {
        return; // locret_1C6AC - no visible area
    }

    // C70B-C715: y1 = min(rect.y1, screen_bounds->y1), early exit if y1 <= y0
    bounds.y1 = (rect.y1 <= screen_bounds->y1) ? rect.y1 : screen_bounds->y1;
    if (bounds.y1 <= bounds.y0) {
        return; // locret_1C6AC - no visible area
    }

    // Copy calculated bounds to clip rect
    *clip_rect = bounds;

    // C718: Copy clipped area from framebuffer 2 to screen
    sub_1C443_copy_clip_rect_to_screen_from_fb2();

    // C71B-C759: Build list of particles within the clipping rectangle
    struct Particle** particle_list = (struct Particle**)alloca(0x200); // sub sp, 200h
    int particle_count_in_rect = 0;

    uint16_t total_particles = word_2316E_particle_system_particle_count;
    if (total_particles > 0) {
        struct Particle* particle = (struct Particle*)0x23170; // _unk_23170_particle_system_particles

        for (int i = 0; i < total_particles; i++) {
            // C738-C756: Check if particle is active and within clipping bounds
            if (!(particle->flags & 0x80) &&           // not marked for removal
                particle->x_pos < bounds.x1 &&         // left edge < right bound
                particle->y_pos < bounds.y1 &&         // top edge < bottom bound
                particle->x_pos2 > bounds.x0 &&        // right edge > left bound
                particle->y_pos2 > bounds.y0) {        // bottom edge > top bound

                particle_list[particle_count_in_rect++] = particle;
            }
            particle++; // add si, 11h - next particle (17 bytes)
        }
    }

    // C75B-C77D: Sort and draw particles by depth/priority
    if (particle_count_in_rect > 0) {
        for (int i = 0; i < particle_count_in_rect; i++) {
            // C763-C766: Get next particle from sorted list
            struct Particle** sorted_list = particle_list;

            // C766-C76A: Call sort/priority function
            int sort_result = ((int(*)(struct Particle**))off_21C36)(sorted_list);
            if (sort_result < 0) break; // js short loc_1C77F

            // C76C-C779: Draw the particle sprite
            struct Particle* p = sorted_list[i];
            if (p != NULL) {
                uint16_t sprite_id = p->type;           // [si+8]
                uint16_t x_pos = p->x_pos;              // [si]
                uint16_t y_pos = p->y_pos;              // [si+2]
                sub_1C30D_draw_sprite_clipped_clobbering_bx_dx(sprite_id, x_pos, y_pos);
            }
        }
    }

    // C780-C79F: Conditional UI overlay drawing
    if (byte_2172D == 0) { // Some UI state check
        // Check if clip rect overlaps with UI area (head/shoulders region)
        if (clip_rect->y1 >= 137 &&     // bottom >= 137
            clip_rect->x1 >= 126 &&     // right >= 126
            clip_rect->x0 < 194) {      // left < 194
            sub_11797_ui_draw_head_and_shoulders();
        }
    }

    // C7A2-C7BB: Additional overlay drawing (two more UI elements)
    uint16_t overlay1 = word_2D090;
    if (overlay1 != 0) {
        sub_1C7D4(overlay1, clip_rect);
    }

    uint16_t overlay2 = word_2D092;
    if (overlay2 != 0) {
        sub_1C7D4(overlay2, clip_rect);
    }

    // C7BE-C7CF: Update screen with the rendered rectangle
    sub_1C51E_update_screen_rect(clip_rect->x0, clip_rect->y0,
                                 clip_rect->x1, clip_rect->y1);
}

Resource *cs_c921_hnm_get_resource_by_id(uint16_t id)
{
	return ds_31ff_resources[id + 210];
}

bool cs_c92b_hnm_open_and_load_palette(uint16_t id)
{
	FUNC_NAME;
	ds_dc00_hnm_id = id;

	cs_ca01_hnm_close_file();
	cs_ce1a_hnm_reset_buffers();

	ds_dbe7_hnm_finished_flag = 0;
	cs_ce01_hnm_reset_frame_counters();

	// Tail call by fall-through
	return cs_c93c_hnm_read_header();
}

bool cs_c93c_hnm_read_header()
{
	FUNC_NAME;
	ds_dc02_hnm_open_id = ds_dc00_hnm_id;
	Resource *res = cs_c921_hnm_get_resource_by_id(ds_dc00_hnm_id);

	ds_dbfe_hnm_resource_flag = res->alloc_size;

	int fd;
	uint32_t size;
	uint32_t offset;

	cs_f229_res_open_or_die(res->name, &fd, &size, &offset);

	ds_35a6_hnm_fd = fd;
	ds_dc04_hnm_file_offset = offset;
	ds_dc08_hnm_file_remain = size;

	uint16_t dc1a_hnm_read_offset_saved = ds_dc1a_hnm_read_offset;
	ptr_offset_t dc0c_hnm_read_buffer_ptr_offset_saved = ds_dc0c_hnm_read_buffer;

	uint16_t header_size;
	ptr_offset_t buf;
	cs_cd8f_hnm_read_header_size(&header_size, &buf);
	if (ds_dc0c_hnm_read_buffer.offset() + header_size > ds_ce74_framebuffer_size) {
		ds_dc0c_hnm_read_buffer.reset();
	}
	bool r = cs_cdbf_hnm_read_bytes(header_size - 2);
	ds_dc1a_hnm_read_offset = dc1a_hnm_read_offset_saved;
	ds_dc0c_hnm_read_buffer = dc0c_hnm_read_buffer_ptr_offset_saved;
	if (!r) {
		return false;
	}

	auto c = ds_dc0c_hnm_read_buffer;
	auto size_again = c.readle16();
	// TODO: Buffer overflow checking

	ds_dbb4_last_bank_palette = 0xff;
	cs_c1ba_apply_palette(c.ptr());

	while (c.peekbyte() == 0xff) {
		c += 1;
	}

	uint16_t bx = 0;
	if (ds_dbfe_hnm_resource_flag & 0x4) {
		TODO;
		exit(0);
		bx = 0x10;
	}

	uint32_t first_frame_offset = c.readle32();

	ds_dbf6_hnm_unknown_offset_1 = ds_dc04_hnm_file_offset + first_frame_offset;
	ds_dbfa_hnm_unknown_offset_2 = ds_dc08_hnm_file_remain - first_frame_offset;

	return true;
}

bool cs_c9e8_hnm_do_frame_skippable()
{
	cs_ca60_hnm_do_frame();
	g_app->update_screen(ds_dbd8_framebuffer_screen);
	if (cs_dd63_has_user_input()) {
		return false;
	}

	// cs_de4e_clear_scancode();
	return true;
}

static int wait_for_frame_counter = 0;

bool cs_c9f4_hnm_do_frame_and_check_if_frame_advanced()
{
	uint16_t old_frame_counter = ds_dbe8_hnm_frame_counter;
	cs_ca60_hnm_do_frame();

	return old_frame_counter != ds_dbe8_hnm_frame_counter;
}

uint16_t cs_cc4e()
{
	FUNC_NAME;
	ptr_offset_t r = ds_dc10_hnm_read_buffer;
	uint16_t ax = r.readle16();
	printf("ax = %02x\n", ax);

	ds_dc1a_hnm_read_offset -= ax;

	uint16_t old_offset = r.offset();
	r += ax;
	printf("r.offset()               = %04x\n", r.offset());
	printf("old_offset               = %04x\n", old_offset);
	printf("ds_ce74_framebuffer_size = %04x\n", ds_ce74_framebuffer_size);
	if (r.offset() < old_offset || r.offset() > ds_ce74_framebuffer_size) {
		FUNC_NAME;
		ds_dc10_hnm_read_buffer.set_offset(ax - 2);
		ax = 0;
	}

	FUNC_NAME;
	ds_dc10_hnm_read_buffer += ax;
	printf("ds_dc10_hnm_read_buffer.offset() = %04x\n", ds_dc10_hnm_read_buffer.offset());

	uint16_t new_frame_counter = ds_dbe8_hnm_frame_counter + 1;
	if (new_frame_counter > ds_dbec_hnm_frame_counter_3) {
		new_frame_counter = 1;
		ds_dbec_hnm_frame_counter_3 = -1;
	}
	ds_dbe8_hnm_frame_counter = new_frame_counter;

	return ds_dbe8_hnm_frame_counter;
}

void cs_ca01_hnm_close_file()
{
	FUNC_NAME;
	int old_hnm_fd = 0;
	std::swap(ds_35a6_hnm_fd, old_hnm_fd);

	if (old_hnm_fd && old_hnm_fd != ds_dbba_dat_fd) {
		cs_ce01_hnm_reset_frame_counters();
		close(old_hnm_fd);
	}
}

void cs_ca1b_hnm_load(uint16_t id)
{
	FUNC_NAME;
	if (!cs_c92b_hnm_open_and_load_palette(id)) {
		cs_ca01_hnm_close_file();
		return;
	}

	if (!cs_cda0_hnm_open_at_body()) {
		cs_ca01_hnm_close_file();
		return;
	}

	ds_dce6_in_transition = 0;

	ptr_offset_t c = ds_dc10_hnm_read_buffer;
	uint16_t len = c.readle16();

	cs_ccf4_hnm_demux_frame(c, len, ds_dbdc_framebuffer_2);
	cs_aa0f_decode_sound_block();
	cs_cc96_decode_video_block();
	cs_ce1a_hnm_reset_buffers();
	ds_dbe8_hnm_frame_counter++;
	ds_dbea_hnm_frame_counter_2++;
	if (!(ds_dbfe_hnm_resource_flag & 0x40)) {
		for (int i = 50; i != 0; --i) {
			cs_cb1a();
		}
	}

	// Tail-call by fall-through
	cs_ca59_hnm_set_load_time();
}

void cs_ca59_hnm_set_load_time()
{
	FUNC_NAME;
	ds_dc22_hnm_load_time = ds_ce7a_pit_timer_counter;
}

void cs_ca60_hnm_do_frame()
{
restart:
	FUNC_NAME;

	if (!ds_35a6_hnm_fd) {
		cs_ca9a_hnm_clear_flag();
		return;
	}

	if (ds_dbfe_hnm_resource_flag & 0x80) {
		cs_ca8f();
	}

	if (!cs_caa0()) {
		cs_cb1a();
		goto restart;
	}

	if (cs_cad4_wait_for_frame()) {
		if (ds_dc1e_hnm_pl_block_ofs != 0xffff) {
			cs_ce3b_hnm_handle_pal_chunk();
		}
		cs_cc96_decode_video_block();
		cs_cc4e();
	}

	// Tail-call by fall-through
	cs_ca8f();
}

void cs_ca8f()
{
	ds_dbb5_hnm_flag = ds_dbfe_hnm_resource_flag & 0x80;
	cs_cb1a();

	// Tail-call by fall-through
	cs_ca9a_hnm_clear_flag();
}

void cs_ca9a_hnm_clear_flag()
{
	ds_dbb5_hnm_flag = 0;
}

bool cs_caa0()
{
	FUNC_NAME;
	if (ds_dc14_hnm_decode_buffer.offset() != 0) {
		return true;
	}

	uint16_t read_offset = ds_dc1a_hnm_read_offset;
	if (read_offset == 0) {
		return false;
	}

	ptr_offset_t c = ds_dc10_hnm_read_buffer;
	uint16_t len = c.readle16();
	uint16_t tag = (ds_dc10_hnm_read_buffer + 2).readle16();

	printf("tag: %04x, len: %5d\n", tag, len);

	if (tag != HNM_TAG_MM && len > read_offset) {
		return false;
	}

	printf("ds_dbfe_hnm_resource_flag: %04x\n", ds_dbfe_hnm_resource_flag);
	printf("%p: ds_dbd6_framebuffer_1\n", ds_dbd6_framebuffer_1);
	printf("%p: ds_dbd8_framebuffer_screen\n", ds_dbd8_framebuffer_screen);
	printf("%p: ds_dbda_framebuffer_active\n", ds_dbda_framebuffer_active);
	printf("%p: ds_dbdc_framebuffer_2\n", ds_dbdc_framebuffer_2);
	printf("\n");
	printf("%p: orig_framebuffer_1\n", orig_framebuffer_1);
	printf("%p: orig_framebuffer_screen\n", orig_framebuffer_screen);
	printf("%p: orig_framebuffer_2\n", orig_framebuffer_2);

	byte *dst = ds_dbd6_framebuffer_1;
	if (ds_dbfe_hnm_resource_flag & 0x40) {
		dst = ds_dc32_hnm_buffer;
	}
	cs_ccf4_hnm_demux_frame(c, len, dst);

	return true;
}

bool cs_cad4_wait_for_frame()
{
	FUNC_NAME;
	// printf("ds_dc1c_hnm_sd_block_ofs: %04x\n", ds_dc1c_hnm_sd_block_ofs);
	if (ds_dc1c_hnm_sd_block_ofs == 0xffff) {
		uint16_t time_elapsed = ds_ce7a_pit_timer_counter - ds_dc22_hnm_load_time;
		if (time_elapsed < hi(ds_dbfe_hnm_resource_flag)) {
			return false;
		}
		cs_ca59_hnm_set_load_time();
		return true;
	} else {
		uint16_t time_elapsed = ds_ce7a_pit_timer_counter - ds_dc22_hnm_load_time;
		if (time_elapsed < hi(ds_dbfe_hnm_resource_flag)) {
			return false;
		}
		cs_ca59_hnm_set_load_time();

		ptr_offset_t sd_ptr(ds_dc0c_hnm_read_buffer.begin(), ds_dc1c_hnm_sd_block_ofs);
		hexdump(sd_ptr.ptr(), sd_ptr.peekle16());
		uint16_t sd_block_len = sd_ptr.readle16();
		uint16_t data_len = sd_block_len - 4;
		audio_buffer_t *buffer = g_app->allocate_audio_buffer(data_len);
		memcpy(buffer->data, sd_ptr.ptr(), data_len);
		g_app->enqueue_audio_buffer(buffer);

		return true;
	}
}

void cs_cb1a(/*ptr_offset_t &c*/)
{
	FUNC_NAME;
	for (;;) {
		uint16_t block_size = ds_dc20_hnm_block_size;

		if (block_size != 0) {
			if ((ds_dbfe_hnm_resource_flag & 0x80) == 0) {
				uint16_t ceiled_file_offset_remainder = (-ds_dc04_hnm_file_offset & 0x7ff) + 0x800;
				block_size = std::min(block_size, ceiled_file_offset_remainder);
			}
			bool b = cs_cc2b(block_size);
			if (!b) {
				return;
			}
			ds_dc20_hnm_block_size -= block_size;
			cs_cdbf_hnm_read_bytes(block_size);
			return;
		}

		while (ds_dbea_hnm_frame_counter_2 == ds_dbee_hnm_frame_counter_4 || ds_dc08_hnm_file_remain == 0) {
			if (!(ds_dbfe_hnm_resource_flag & 1)) {
				ds_dbe7_hnm_finished_flag |= 1;
				if (ds_dc1a_hnm_read_offset == 0) {
					ds_dbe7_hnm_finished_flag |= 2;
					cs_ca01_hnm_close_file();
				}
				return;
			}
			if (!cs_cc2b(0x1000)) {
				return;
			}

			cs_ce07_hnm_reset_other_frame_counters();
			ds_dbec_hnm_frame_counter_3 = ds_dbea_hnm_frame_counter_2;

			cs_ca9a_hnm_clear_flag();

			if (ds_dc02_hnm_open_id != ds_dc00_hnm_id) {
				Resource *res = cs_c921_hnm_get_resource_by_id(ds_dc02_hnm_open_id);
				if ((res->alloc_size & 8) == 0) { // WTF? This also tests ((byte*)res - 6) against 0?
					ds_dc00_hnm_id = ds_dc02_hnm_open_id;
					cs_c93c_hnm_read_header();
					return;
				}
				ds_dc00_hnm_id = ds_dc02_hnm_open_id;
				TODO;

				cs_cdf7_hnm_advance_read_ptr(2);

				TODO;
				// cs_cc0c();

				cs_cdf7_hnm_advance_read_ptr(8);
				// cs_5b99_copy_rect();
				// ds_dbf6_hnm_unknown_offset_1 = ((byte*)res - 8)?
			}
			ds_dc08_hnm_file_remain = ds_dbfa_hnm_unknown_offset_2;
			ds_dc04_hnm_file_offset = ds_dbf6_hnm_unknown_offset_1;

			if (ds_dbfe_hnm_resource_flag & 4) {
				cs_c13e_open_sprite_bank_resource_by_index(ds_dc00_hnm_id + 0x61);
				TODO;
				exit(0);
			}
		}

		uint16_t size;
		ptr_offset_t buf;
		if (!cs_cd8f_hnm_read_header_size(&size, &buf)) {
			return;
		}
		cs_cc0c(buf, size);
	}
}

void cs_cc0c(ptr_offset_t &dst, uint16_t block_size)
{
	FUNC_NAME;
	int end = ds_dc0c_hnm_read_buffer.offset() + block_size;
	bool wraps_src_buffer = end > UINT16_MAX;
	bool exceeds_dst_buffer = end > ds_ce74_framebuffer_size;

	if (wraps_src_buffer || exceeds_dst_buffer) {
		uint16_t read_offset = ds_dc0c_hnm_read_buffer.offset();
		ds_dc0c_hnm_read_buffer.reset();
		ds_dc18_hnm_framebuffer_size = read_offset;
	}
	ds_dc20_hnm_block_size = block_size - 2;
	ds_dbea_hnm_frame_counter_2 += 1;
}

bool cs_cc2b(uint16_t param_1)
{
	FUNC_NAME;
	uint16_t uVar1;
	uint16_t uVar2;
	bool bVar3 = false;

	uVar2 = ds_dc0c_hnm_read_buffer.offset();
	uVar1 = ds_dc10_hnm_read_buffer.offset();

	if ((uVar1 <= uVar2) || (bVar3 = uVar1 < uVar2 + param_1 + 0x12, !bVar3)) {
		uVar2 = ds_dc1a_hnm_read_offset + 10;
		bVar3 = (uVar2 + param_1 > UINT16_MAX);
		if (!bVar3) {
			bVar3 = ds_dc18_hnm_framebuffer_size < uVar2 + param_1;
		}
	}
	return !bVar3;
}

bool cs_cc85_hnm_is_complete()
{
	FUNC_NAME;
	return !(ds_dbe7_hnm_finished_flag == 0 || ds_dbe7_hnm_finished_flag == 1);
}

void *cs_cc94_blt_func;

void cs_cc96_decode_video_block()
{
	FUNC_NAME;
	// cc96
	// cs_cc94_blt_func = x_vtable_func_17_copy_fbuf_explode_and_center

	// cc9d
	ptr_offset_t src = ds_dc14_hnm_decode_buffer;
	ds_dc14_hnm_decode_buffer = ptr_offset_t();

	if (!src.begin()) {
		FUNC_NAME;
		return;
	}

	FUNC_NAME;
	// ccab
	uint16_t flag = ds_dbfe_hnm_resource_flag;
	if ((ds_dbfe_hnm_resource_flag & 0x30) == 0) {
		FUNC_NAME;
		if (ds_dc24_hnm_decode_frame_tag & 0x400) {
			FUNC_NAME;
			return;
		}

		// printf("%p: ds_dbd6_framebuffer_1\n", ds_dbd6_framebuffer_1);
		// printf("%p: ds_dbd8_framebuffer_screen\n", ds_dbd8_framebuffer_screen);
		// printf("%p: ds_dbda_framebuffer_active\n", ds_dbda_framebuffer_active);
		// printf("%p: ds_dbdc_framebuffer_2\n", ds_dbdc_framebuffer_2);
		// printf("\n");
		// printf("%p: orig_framebuffer_1\n", orig_framebuffer_1);
		// printf("%p: orig_framebuffer_screen\n", orig_framebuffer_screen);
		// printf("%p: orig_framebuffer_2\n", orig_framebuffer_2);

		// ccb9
		byte *dst = ds_dbda_framebuffer_active;

		uint16_t h0 = src.readle16();
		uint16_t h1 = src.readle16();

		printf("h01 = %04x %04x\n", h0, h1);

		uint16_t di = (h0 & 0xf9ff);
		uint8_t flags = hi(di & 0xf800);
		uint16_t width = di & 0x01ff;

		uint16_t mode = hi(h1);
		uint16_t height = lo(h1);

		if (height == 0) {
			FUNC_NAME;
			return;
		}

		uint16_t dst_x = src.readle16();
		uint16_t dst_y = src.readle16();

		printf("dst_x = %04x, dst_y = %04x\n", dst_x, dst_y);

		if (ds_dc00_hnm_id < 25) {
			vga_0f5b_blit(dst, dst_x, dst_y, src, width, height, flags, mode);
		} else {
			vga_1bca_copy_interlaced(dst, dst_x, dst_y, src, width, height);
		}
		return;
	}

	if ((ds_dbfe_hnm_resource_flag & 0x20) == 0) {
		cs_4afd();
		return;
	}

	// jmp cs:4AEB
	TODO;
	exit(0);
}

void cs_ccf4_hnm_demux_frame(ptr_offset_t src, uint16_t len, byte *dst_ptr)
{
	FUNC_NAME;
	ds_dc1c_hnm_sd_block_ofs = -1;
	ds_dc1e_hnm_pl_block_ofs = -1;

	printf("src.offset() = %04x, len = %04x\n", src.offset(), len);
	if (src.offset() + len > UINT16_MAX || src.offset() + len > 64000) {
		printf("src.offset()\n");
		src.reset();
	}

	uint16_t tag;
	uint16_t block_len;

	for (;;) {
		tag = src.readle16();

		if (tag == HNM_TAG_SD) {
			if (cs_a2ef_is_pcm_enabled()) {
				ds_dc1c_hnm_sd_block_ofs = src.offset();
			}
			block_len = src.readle16();
			src += block_len - 4;
		} else if (tag == HNM_TAG_PL) {
			block_len = src.readle16();
			ds_dc1e_hnm_pl_block_ofs = src.offset();
			src += block_len - 4;
		} else {
			break;
		}
	}

	// "mm"
	if (tag == HNM_TAG_MM) {
		TODO;
		exit(1);
	}

	// Frame
	if ((tag & 0x400) != 0) {
		dst_ptr = ds_dbda_framebuffer_active;
	}
	ds_dc14_hnm_decode_buffer = dst_ptr;
	ds_dc24_hnm_decode_frame_tag = tag;

	ptr_offset_t dst = ds_dc14_hnm_decode_buffer;
	uint16_t height_and_mode = src.readle16();

	if (!(tag & 0x400)) {
		dst.writele16(tag);
		dst.writele16(height_and_mode);
		if (tag == 0) {
			return;
		}
		if (!(tag & 0x200)) {
			ds_dc14_hnm_decode_buffer = src - 4;
			return;
		}
	}

	cs_f403_unpack_hsq_skip_header(src, dst);
}

bool cs_cd8f_hnm_read_header_size(uint16_t *size, ptr_offset_t *buf)
{
	FUNC_NAME;
	if (!cs_cdbf_hnm_read_bytes(2)) {
		return false;
	}

	*buf = ds_dc0c_hnm_read_buffer;
	*size = (*buf - 2).readle16();
	return true;
}

bool cs_cda0_hnm_open_at_body()
{
	FUNC_NAME;
	cs_ce1a_hnm_reset_buffers();

	uint16_t header_size;
	ptr_offset_t buf;
	cs_cd8f_hnm_read_header_size(&header_size, &buf);

	ds_dc10_hnm_read_buffer.m_offset = ds_ce74_framebuffer_size - header_size - 2;

	ds_dc0c_hnm_read_buffer = ds_dc10_hnm_read_buffer;
	ds_dc0c_hnm_read_buffer.writele16(header_size);

	// Tail-call by fall-through
	return cs_cdbf_hnm_read_bytes(header_size - 2);
}

bool cs_cdbf_hnm_read_bytes(int count)
{
	FUNC_NAME;
	ds_35a6_hnm_fd = ds_dbba_dat_fd;

	if (ds_35a6_hnm_fd < 1) {
		return false;
	}

	ssize_t offset = lseek(ds_35a6_hnm_fd, 0, SEEK_CUR);

	int c = read(ds_35a6_hnm_fd, ds_dc0c_hnm_read_buffer.ptr(), count);
	log_framebuffer_write(ds_dc0c_hnm_read_buffer.ptr(), count);

	ds_dc04_hnm_file_offset += c;
	ds_dc08_hnm_file_remain -= c;

	// Tail-call by fall-through
	cs_cdf7_hnm_advance_read_ptr(c);

	return true;
}

void cs_cdf7_hnm_advance_read_ptr(uint16_t n)
{
	ds_dc0c_hnm_read_buffer += n;
	ds_dc1a_hnm_read_offset += n;
}

void cs_ce01_hnm_reset_frame_counters()
{
	ds_dbe8_hnm_frame_counter = 0;

	// Tail-call by fall-through
	cs_ce07_hnm_reset_other_frame_counters();
}

void cs_ce07_hnm_reset_other_frame_counters()
{
	FUNC_NAME;
	ds_dbea_hnm_frame_counter_2 = 0;
	ds_dbec_hnm_frame_counter_3 = -1;
	ds_dbee_hnm_frame_counter_4 = -1;
}

void cs_ce1a_hnm_reset_buffers()
{
	FUNC_NAME;
	ds_dc0c_hnm_read_buffer = ds_dbdc_framebuffer_2;
	ds_dc10_hnm_read_buffer = ds_dbdc_framebuffer_2;

	ds_dc1a_hnm_read_offset = 0;
	ds_dc20_hnm_block_size = 0;

	ds_dc14_hnm_decode_buffer = nullptr;
	ds_dc18_hnm_framebuffer_size = ds_ce74_framebuffer_size;
}

void cs_ce3b_hnm_handle_pal_chunk()
{
	FUNC_NAME;
	ptr_offset_t pal_ptr(ds_dc0c_hnm_read_buffer.begin(), ds_dc1e_hnm_pl_block_ofs);
	cs_c1ba_apply_palette(pal_ptr.ptr());
	vga_0b0c_gfx_32_set_screen_palette();
}

void cs_ce6c_hnm_initialize()
{
	for (uint16_t id = 2; id != 9; ++id) {
		Resource *r = cs_c921_hnm_get_resource_by_id(id);
		r->alloc_size &= 0xfffb;
	}

	for (uint16_t id = 2; id != 8; ++id) {
		cs_ceb0_hnm(id);
	}

	cs_ca01_hnm_close_file();
}

void cs_ceb0_hnm(uint16_t id)
{
	Resource *r = cs_c921_hnm_get_resource_by_id(id);
	if (!cs_c92b_hnm_open_and_load_palette(id)) {
		return;
	}

	// TODO; // Read rect?
	// if ((r->alloc_size & 8)) {
	// 	printf("\t\t\t0x%04x - %s\n", r->alloc_size, r->name);
	// }
}

void cs_cefc_load_irulan_hnm()
{
	cs_c13e_open_sprite_bank_resource_by_index(ds_ceeb_language_setting + 105);
	ds_3622_subtitle_next_frame = &ds_35a8_subtitle_frames_irulan[0];
	vga_0c06_gfx_33_set_global_y_offset(0);
	cs_c0ad_gfx_clear_active_framebuffer();
	cs_ca1b_hnm_load(25);
}

void cs_cf1b_play_irulan_hnm()
{
	byte *old_active_frame_buffer = ds_dbda_framebuffer_active;
	cs_c08e_set_screen_as_active_framebuffer();

	do {
		if (*ds_3622_subtitle_next_frame <= ds_dbe8_hnm_frame_counter) {
			cs_cf4b_subtitle_draw_or_clear();
		}

		if (!cs_c9e8_hnm_do_frame_skippable()) {
			return;
		}
	} while (!cs_cc85_hnm_is_complete());

	ds_dbda_framebuffer_active = old_active_frame_buffer;
}

void cs_cf4b_subtitle_draw_or_clear()
{
	ds_3622_subtitle_next_frame++;
	uint16_t index = ds_3622_subtitle_next_frame - ds_35a8_subtitle_frames_irulan;
	if (index % 2) {
		cs_c22f_draw_sprite(index / 2, 0, 190);
	} else {
		memset(ds_dbd8_framebuffer_screen + 190 * 320, 0, 10 * 320);
	}
}

char *cs_cf70_get_phrase_or_command_string(uint16_t id)
{
	if (id & 0x800) {
		cs_d00f_load_phrase_file();
		uint16_t offset = readle16(ds_47b0_phrase_bin + 2 * id);
		return (char *)(ds_47b0_phrase_bin + offset);
	}

	uint16_t offset = readle16(ds_47ac_command_bin + 2 * id);
	return (char *)(ds_47ac_command_bin + offset);
}

void cs_cfb9()
{
	TODO;
	cs_f0b9_res_read_by_index_into_buffer(0xbb, font_bin);

	cs_f0b9_res_read_by_index_into_buffer(0xc0 + ds_ceeb_language_setting, ds_47ac_command_bin);
}

void cs_d00f_load_phrase_file()
{
	uint8_t resource_id = ds_ceeb_language_setting + 0x93;
	if (resource_id != ds_477e_current_phrase_bin_resource_id) {
		ds_477e_current_phrase_bin_resource_id = resource_id;
		cs_f0b9_res_read_by_index_into_buffer(resource_id, ds_47b0_phrase_bin);
		// cs_0098_adjust_sub_resource_pointers
	}
}

void cs_d04e_font_set_draw_position(uint16_t x, uint16_t y)
{
	// printf("cs_d04e_font_set_draw_position(%d, %d)\n", x, y);
	ds_d82c_font_draw_position_x = x;
	ds_d82e_font_draw_position_y = y;
	ds_d830_font_draw_position_x_start = x;
	ds_d832_font_draw_position_y_start = y;
}

void cs_d05f_font_get_draw_position(uint16_t *x, uint16_t *y)
{
	*x = ds_d82c_font_draw_position_x;
	*y = ds_d82e_font_draw_position_y;
}

void cs_d068_font_select_tall_font()
{
	ds_2518_font_glyph_draw_func = cs_d096_font_glyph_draw_tall;
	ds_47a0_font_width_table_ptr = font_bin;
}

void cs_d075_font_select_small_font()
{
	ds_2518_font_glyph_draw_func = cs_d12f_font_glyph_draw_small;
	ds_47a0_font_width_table_ptr = font_bin + 0x80;
}

void cs_d096_font_glyph_draw_tall(byte c)
{
	byte *font_glyph_ptr = &font_bin[0x100 + 9 * c];

	uint8_t w = font_bin[c];
	uint16_t x, y;
	cs_d05f_font_get_draw_position(&x, &y);
	ds_d82c_font_draw_position_x += w;

	vga_1bfe_gfx_7_draw_glyph(ds_dbda_framebuffer_active, x, y, font_glyph_ptr, w, 9, ds_dbe4_font_draw_color);
}

void cs_d12f_font_glyph_draw_small(byte c)
{
	byte *font_glyph_ptr = &font_bin[0x580 + 7 * c];

	uint8_t w = font_bin[0x80 + c];
	uint16_t x, y;
	cs_d05f_font_get_draw_position(&x, &y);
	ds_d82c_font_draw_position_x += w;

	vga_1bfe_gfx_7_draw_glyph(ds_dbda_framebuffer_active, x, y, font_glyph_ptr, w, 7, ds_dbe4_font_draw_color);
}

void cs_d194_font_draw_phrase_or_command_string_with_color_at_pos(uint16_t id, uint16_t x, uint16_t y, uint16_t color)
{
	ds_dbe4_font_draw_color = color;
	cs_d04e_font_set_draw_position(x, y);
	cs_d19b_font_draw_phrase_or_command_string(id);
}

void cs_d19b_font_draw_phrase_or_command_string(uint16_t id)
{
	const char *str = cs_cf70_get_phrase_or_command_string(id);
	// printf("str: %s\n", str);
	cs_d1bb_font_draw_string(str);
}

void cs_d1a6_font_draw_phrase_list(string_def_t *p)
{
	while (p->id != -1) {
		cs_d194_font_draw_phrase_or_command_string_with_color_at_pos(p->id - 1, p->x, p->y, p->color);
		++p;
	}
}

void cs_d1bb_font_draw_string(const char *str)
{
	const char *p = str;
	for (;;) {
		char c = *p++;
		if (c == -1) {
			break;
		}

		if (c == 0x0d) {
			ds_d82c_font_draw_position_x = ds_d830_font_draw_position_x_start;

			uint16_t glyph_height = ds_2518_font_glyph_draw_func == cs_d12f_font_glyph_draw_small ? 7 : 10;
			ds_d832_font_draw_position_y_start += glyph_height;
			ds_d82e_font_draw_position_y += glyph_height;
			continue;
		}

		if (c < 0) {
			c = 0x40;
		}

		ds_2518_font_glyph_draw_func(c);
	}
}

void cs_d1ef_draw_all_ui_elements()
{
	// Tail-call by fallthrough
	cs_d1f2_draw_ui_element_list(ds_1ae6_ui_elements, ds_1ae4_ui_elements_len);
}

void cs_d1f2_draw_ui_element_list(ui_element_t *list, int count)
{
	cs_c137_load_icones();
	for (int i = 0; i != count; ++i) {
		printf("%3d: ", i);
		cs_d200_draw_ui_element(&list[i]);
	}
}

void cs_d200_draw_ui_element(ui_element_t *e)
{
	byte *prev_active_framebuffer = ds_dbda_framebuffer_active;
	cs_c08e_set_screen_as_active_framebuffer();

	printf("cs_d200_draw_ui_element: %04x %3d\n", e->flags, e->id);

	if (e->flags & 0x40) {
		rect_t rect = {e->x0, e->y0, e->x1, e->y1};
		vga_1979_gfx_09_clear_rect(ds_dbda_framebuffer_active, &rect);
	}

	if ((e->flags & 0x20) == 0) {
		if (e->id != -1) {
			cs_c22f_draw_sprite(e->id, e->x0, e->y0);
		}
	}

	ds_dbda_framebuffer_active = prev_active_framebuffer;
}

void cs_d2bd()
{
	byte was_in_transition = ds_dce6_in_transition;

	TODO;

	ds_dce6_in_transition = was_in_transition;
}

void cs_d72b(ui_element_t *controls)
{
	for (int i = 0; i != 6; ++i) {
		ds_1ae6_ui_elements[12 + i] = controls[i];
	}

	cs_d741();
	cs_d1f2_draw_ui_element_list(&ds_1ae6_ui_elements[12], 6);
}

void cs_d741()
{
	if (ds_1ae6_ui_elements[2].id < 3) {
		rect_t r = {254, 167, 296, 193};
		vga_197b_gfx_10_fill_rect(ds_dbd8_framebuffer_screen, &r, 0xf0);
	}
}

void cs_d75a_ui_set_and_draw_frieze_sides_closed_book()
{
	cs_d795_ui_set_and_draw_frieze_sides(ds_1c36_frieze_closed_book);
	cs_1a34_ui_draw_date_and_time_indicator();
}

void cs_d792_ui_set_and_draw_frieze_sides_map()
{
	cs_d795_ui_set_and_draw_frieze_sides(ds_1c66_frieze_sides_map);
}

void cs_d795_ui_set_and_draw_frieze_sides(const ui_element_flag_and_sprite_t *updates)
{
	for (int i = 0; i != 4; ++i) {
		ds_1ae6_ui_elements[i].flags = updates[i].flags;
		ds_1ae6_ui_elements[i].id = updates[i].id;
	}
	cs_d1f2_draw_ui_element_list(ds_1ae6_ui_elements, 3);
}

void cs_d7ad_ui_set_and_draw_frieze_sides_open_book()
{
	cs_d795_ui_set_and_draw_frieze_sides(ds_1c56_frieze_sides_open_book);
}

void cs_d7b2_ui_set_and_draw_frieze_sides_globe()
{
	cs_d795_ui_set_and_draw_frieze_sides(ds_1c46_frieze_sides_globe);
}

void cs_d815_game_loop()
{
	// ds_dc86_game_start_time = ds_ce7a_pit_timer_counter;
	// ds_dc4b = 0;

	for (;;) {
		// if (ds_cee8_key_scancode == 0x2f && ds_ce9e == 0xff) {
		// 	cs_b270(); // Handle Ctrl-V
		// }

		// if (ds_46d9) {
		// 	cs_d7b7();
		// 	cs_1b0d();
		// }

		cs_d9d2_process_frame_tasks();
		// if (ds_46d9) {
		// 	cs_0d8e();
		// }

		cs_dc20_draw_mouse();

		ds_0000_rand_bits = cs_e3cc_rand();

		vga_0b0c_gfx_32_set_screen_palette();
		g_app->wait_for_vsync();
		g_app->update_screen(ds_dbd8_framebuffer_screen);
	}

	vga_0b0c_gfx_32_set_screen_palette();
	g_app->update_screen(ds_dbd8_framebuffer_screen);
}

void cs_d9d2_process_frame_tasks()
{
	// cs_ace6()
	uint16_t now = ds_ce7a_pit_timer_counter;
	uint16_t previous_frame_task_time = ds_dc66_frame_task_time;
	ds_dc66_frame_task_time = now;
	uint16_t ticks_since_last = now - previous_frame_task_time;

	if (ds_dc6a_frame_tasks_count == 0) {
		return;
	}

	for (int i = 0; i != ds_dc6a_frame_tasks_count; ++i) {
		uint16_t next_task_ticks = ticks_since_last + ds_dc6c_frame_tasks[i].ticks_since_last;
		if (next_task_ticks < ds_dc6c_frame_tasks[i].interval) {
			ds_dc6c_frame_tasks[i].ticks_since_last = next_task_ticks;
			continue;
		}
		if (ds_dc6c_frame_tasks[i].interval != 0) {
			ds_dc6c_frame_tasks[i].ticks_since_last = next_task_ticks % ds_dc6c_frame_tasks[i].interval;
		}

		if (ds_dc6c_frame_tasks[i].fn) {
			ds_dc6c_frame_tasks[i].fn();
		}
	}
}

void cs_da25_add_frame_task(uint8_t interval, frame_task_fn_t fn)
{
	if (ds_dc6a_frame_tasks_count >= 20) {
		return;
	}
	frame_task_t &task = ds_dc6c_frame_tasks[ds_dc6a_frame_tasks_count++];
	task.interval = interval;
	task.ticks_since_last = 0;
	task.fn = fn;
}

void cs_da53_remove_all_frame_tasks()
{
	ds_dc6a_frame_tasks_count = 0;
	ds_46d7 = 0;
}

void cs_da5f_remove_frame_task(frame_task_fn_t fn)
{
	for (int i = 0; i != ds_dc6a_frame_tasks_count; ++i) {
		if (ds_dc6c_frame_tasks[i].fn == fn) {
			for (int j = i + 1; j != ds_dc6a_frame_tasks_count; ++j) {
				ds_dc6c_frame_tasks[i] = ds_dc6c_frame_tasks[j];
			}
			ds_dc6a_frame_tasks_count -= 1;
			return;
		}
	}
}

void cs_daa3_clear_some_mouse_rect()
{
	ds_dc58_some_mouse_rect = nullptr;
}

void cs_daaa_set_some_mouse_rect(rect_t *rect)
{
	ds_dc58_some_mouse_rect = rect;
}

bool has_restore_area = false;

void cs_dc20_draw_mouse()
{

	int x, y;
	g_app->get_mouse_pos(&x, &y);

	int16_t ds_dc36_mouse_x = x;
	int16_t ds_dc38_mouse_y = y;

	cursor_t *c = &ds_2584_cursor;

	if (has_restore_area) {
		vga_1940_gfx_04_restore_cursor_area();
	}
	vga_1888_gfx_03_draw_cursor(ds_dc36_mouse_x, ds_dc38_mouse_y, c);
	has_restore_area = true;
}

bool cs_dd63_has_user_input()
{
	// TODO;

	ds_0000_rand_bits = cs_e3cc_rand();
	cs_d9d2_process_frame_tasks();

	return false;
}

bool cs_ddb0_wait_interruptable(uint16_t max_wait_ticks)
{
	int32_t signed_max_wait_ticks = max_wait_ticks;

	do {
		if (cs_dd63_has_user_input()) {
			return true;
		}

		uint16_t now = ds_ce7a_pit_timer_counter;
		do {
			std::this_thread::yield(); // Ugh
		} while (now == ds_ce7a_pit_timer_counter);
		signed_max_wait_ticks -= ds_ce7a_pit_timer_counter - now;
	} while (signed_max_wait_ticks >= 0);

	return false;
}

bool cs_ddf0_wait_voice_interruptable(uint16_t fallback_wait_ticks)
{
	// TODO: Check PCM voice

	return cs_ddb0_wait_interruptable(fallback_wait_ticks);
}

void cs_de0c(uint16_t bx) {}

bool cs_e675_dat_open()
{
	FUNC_NAME;

	// UNIMPLEMENTED: Dune attempts to load and execute dnmaj1-9

	ds_dbba_dat_fd = open("DUNE.DAT", O_RDONLY);
	assert(ds_dbba_dat_fd > 0);
	if (ds_dbba_dat_fd <= 0) {
		return false;
	}

	ds_dbbc_dat = cs_e741_dat_read_toc();

	{
		ptr_offset_t c = make_ptr_offset(ds_dbbc_dat);
		c.writele16(325);
		for (int i = 0; i != 333; ++i) {
			c.writebyte(0xff);
		}
		ds_d820 = c.offset();
	}

	byte *dat_toc_ptr = ds_dbbc_dat + 0x4000;
	ptr_offset_t datc = make_ptr_offset(dat_toc_ptr);

	uint16_t entries = datc.readle16();
	(void)entries;

	// Iterate over the entries in the DAT toc.
	do {
		uint8_t dl;

		// printf("%3d: Looking up '%s'\n", ++n, datc.str());
		auto maybe_res_index = cs_f314_dat_get_res_index_by_name(datc.str(), dl);
		if (maybe_res_index) {
			uint16_t res_index = maybe_res_index.value();
			// printf("\tFound id: %Xh, dl = %d\n", res_index, dl);
			uint16_t di;
			bool r = cs_f3a7_dat_locate_map_entry(res_index, dl, di);
			// printf("\tEntry address 1F0A:%04X\n", di);
			if (!r) {
				uint16_t cx = ds_d820 - di;
				uint16_t si = ds_d820 - 2;

				for (int i = 0; i != cx / 2; ++i) {
					uint16_t v = readle16(&ds_dbbc_dat[si]);
					writele16(&ds_dbbc_dat[si + 10], v);
					si -= 2;
				}
			}

			cs_e75b_res_store_in_lookup_table(res_index, dl, ds_dbbc_dat + di, datc.ptr());
			ds_d820 += 10;
		}
		datc += 0x19;
	} while (*datc.ptr());

	// UNIMPLEMENTED: Move the lookup table to the beginning of the allocation.

	return true;
}

/*
 * The DUNE.DAT table of contents is loaded into the latter part of an oversized allocation.
 *
 * In the first part the game then creates a table of resource size and offset based on resource index.
 */
byte *cs_e741_dat_read_toc()
{
	int fd;
	cs_f2d6_dat_seek(&fd, 0);

	byte *p = (byte *)calloc(1, 0x14000);

	int r = cs_f2ea_dat_read(0xffff, p + 0x4000);

	assert(r > 0);
	return p;
}

void cs_e75b_res_store_in_lookup_table(uint16_t res_id, byte dl, byte *dat_index_entry, byte *dat_toc)
{
	ptr_offset_t index_entry = make_ptr_offset(dat_index_entry);
	ptr_offset_t toc_entry = make_ptr_offset(dat_toc);

	index_entry.writele16(res_id);
	index_entry.writebyte(dl);
	toc_entry += 0x10;
	index_entry.writele16(toc_entry.readle16());
	index_entry.writebyte(toc_entry.readbyte());
	toc_entry += 1;
	index_entry.writele16(toc_entry.readle16());
	index_entry.writele16(toc_entry.readle16());
}

void cs_ea7b_init_extended_memory_allocator()
{
	// TODO: Lots of crap
	ds_ce70_res_index_needs_extended_memory = 0; // 0xb9;
}

byte *cs_f0a0_res_open_without_hsq(uint16_t index, uint8_t *buffer = nullptr)
{
	ds_ce71_disable_hsq_detection = true;
	byte *p = cs_f0b9_res_read_by_index_into_buffer(index, buffer);
	ds_ce71_disable_hsq_detection = false;
	return p;
}

byte *cs_f0b9_res_read_by_index_into_buffer(uint16_t index, uint8_t *buffer)
{
	ds_ce78_dat_resource = index;
	Resource *res = ds_31ff_resources[index];

	printf("Loading resource: %s\n", res->name);

	if (res->alloc_size == 0) {
		cs_f0d6_res_read(buffer, 0);
	} else {
		buffer = cs_f11c_alloc_check(res->alloc_size);
		uint32_t size = cs_f0d6_res_read(buffer, res->alloc_size);
		cs_f0ff_alloc_take(size);
	}

	return buffer;
}

uint32_t cs_f0d6_res_read(byte *buffer, uint32_t size)
{
	if (ds_ce78_dat_resource < ds_ce70_res_index_needs_extended_memory) {
		assert(0 && "unimplemented");
	}
	Resource *res = ds_31ff_resources[ds_ce78_dat_resource];
	cs_f244_res_read_to_buffer(res->name, buffer, &size);

	if (ds_ce78_dat_resource < ds_ce70_res_index_needs_extended_memory) {
		assert(0 && "unimplemented");
	}

	return cs_f3d3_maybe_uncompress_hsq(buffer);
}

byte *cs_f0f6_alloc_get_addr()
{
	return ds_39b7_alloc_next_addr;
}

void cs_f0ff_alloc_take(uint16_t bytes)
{
	uint16_t paragraph_count = ((uint32_t)bytes + 15) / 16;
	if (16 * paragraph_count > ds_ce68_alloc_last_addr - ds_39b7_alloc_next_addr) {
		cs_f131_out_of_standard_memory();
	}
	ds_39b7_alloc_next_addr += 16 * paragraph_count;
}

byte *cs_f11c_alloc_check(uint16_t pages)
{
	if (16 * pages > ds_ce68_alloc_last_addr - ds_39b7_alloc_next_addr) {
		cs_f131_out_of_standard_memory();
	}
	return ds_39b7_alloc_next_addr;
}

void cs_f131_out_of_standard_memory()
{
	// UNIMPLEMENTED: Call cs_003a_exit_with_error(...)

	printf("Not enough standard memory to run Dune");
	exit(1);
}

bool cs_f1fb_res_open(const char *filename, int *fd, uint32_t *size, uint32_t *offset)
{
	// printf("cs_f1fb_res_open: \e[7m%s\e[m\n", filename);
	if (cs_f2a7_dat_seek_to_name(filename, fd, size, offset)) {
		return true;
	}

	// TODO: Try opening resource as actual file.
	assert(0 && "unimplemented");

	return false;
}

void cs_f229_res_open_or_die(const char *filename, int *fd, uint32_t *size, uint32_t *offset)
{
	if (!cs_f1fb_res_open(filename, fd, size, offset)) {
		printf("File not found: %s\n", filename);
		exit(1);
	}
}

uint16_t cs_f244_res_read_to_buffer(const char *filename, byte *buffer, uint32_t *size)
{
	// TODO; // Check if resource fd == dat fd

	int fd;
	uint32_t offset;

	cs_f229_res_open_or_die(filename, &fd, size, &offset);
	// cs_f2a7_dat_seek_to_name(filename, &fd, &_size, &offset);
	// TODO;
	return cs_f2ea_dat_read(*size, buffer);
}

bool cs_f2a7_dat_seek_to_name(const char *filename, int *fd, uint32_t *size, uint32_t *offset)
{
	if (ds_dbba_dat_fd <= 0) {
		return false;
	}

	uint8_t dl;
	auto index = cs_f314_dat_get_res_index_by_name(filename, dl);
	if (!index) {
		return false;
	}

	uint16_t di;
	cs_f3a7_dat_locate_map_entry(index.value(), dl, di);
	byte *entry = ds_dbbc_dat + di;
	uint8_t bp = readbyte(entry + 5);
	uint16_t cx = readle16(entry + 3);

	*fd = ds_dbba_dat_fd;
	*size = (bp << 16) + cx;
	*offset = readle32(entry + 6);

	cs_f2d6_dat_seek(fd, *offset);

	return true;
}

void cs_f2d6_dat_seek(int *fd, uint32_t offset)
{
	*fd = ds_dbba_dat_fd;
	lseek(ds_dbba_dat_fd, offset, SEEK_SET);
}

uint16_t cs_f2ea_dat_read(int bytes, byte *p)
{
	ssize_t r = read(ds_dbba_dat_fd, p, bytes);
	assert(r < 0x10000);

	return r;
}

std::optional<uint16_t> cs_f314_dat_get_res_index_by_name(const char *filename, uint8_t &dl)
{
	bool in_p_folder = filename[2] == '\\' && filename[3] == 'P';

	if (!in_p_folder) {
		int len = strlen(filename);
		assert(len < 16);

		dl = 0;

		Resource *res = ds_31ff_resources[ds_ce78_dat_resource];
		if (strcmp(res->name, filename) == 0) {
			return ds_ce78_dat_resource;
		}
		for (int i = 0; i != 0xf7; ++i) {
			res = ds_31ff_resources[i];
			if (res && strcmp(res->name, filename) == 0) {
				return i;
			}
		}
		return {};
	}

	bool debug = false;

	// What is going on here? Is this the worst attempt
	// at hashing known to humankind?

	auto c = make_ptr_offset((byte *)filename);
	c += 4;
	dl = c.readbyte() - 0x40;
	if (debug)
		printf("%d: dl = %Xh\n", __LINE__, dl);
	uint16_t bx = 0;

	int i;
	for (i = 0; i != 3; ++i) {
		uint8_t a = c.readbyte();
		if (debug)
			printf("%d: A: %04X\n", __LINE__, a);
		if (a >= 'A') {
			a -= 7;
		}
		bx = (bx << 4) | (a & 0x0f);
		if (debug)
			printf("%d: bx = %Xh\n", __LINE__, bx);
	}
	dl = (dl << 1) | (c.readbyte() >= 0x4f);
	if (debug)
		printf("%d: dl = %Xh\n", __LINE__, dl);

	uint8_t a = c.readbyte();
	if (a >= 0x41) {
		bx |= (a - 0x41) << 12;
		if (debug)
			printf("%d: bx = %Xh\n", __LINE__, bx);
	}
	if (debug)
		printf("%d: bx = %Xh\n", __LINE__, bx);
	return bx;
}

bool cs_f3a7_dat_locate_map_entry(uint16_t index, uint8_t dl, uint16_t &di)
{
	bool debug = false;

	// int di = 0;
	di = 0;
	bool b1, b2;

	di -= 5;
	do {
		di += 5;

		if (debug)
			printf("di: %04x\n", di);
		if (debug)
			printf("dl: %02x\tds_dbbc_dat[di + 4]: %02x\n", dl, readbyte(ds_dbbc_dat + di + 4));

		b1 = dl < readbyte(ds_dbbc_dat + di + 4);
		b2 = dl == readbyte(ds_dbbc_dat + di + 4);
		if (b2) {
			if (debug)
				printf("index: %02x\tds_dbbc_dat[di + 4]: %04x\n", index, readle16(ds_dbbc_dat + di + 2));
			b1 = index < readle16(ds_dbbc_dat + di + 2);
			b2 = index == readle16(ds_dbbc_dat + di + 2);
		}
	} while (!b1 && !b2);

	di = readle16(ds_dbbc_dat + di);

	di -= 10;
	do {
		di += 10;

		if (debug)
			printf("di: %04x\n", di);
		if (debug)
			printf("dl: %02x\tds_dbbc_dat[di + 2]: %02x\n", dl, readbyte(ds_dbbc_dat + di + 2));

		b1 = dl < ds_dbbc_dat[di + 2];
		b2 = dl == ds_dbbc_dat[di + 2];
		if (b2) {
			if (debug)
				printf("index: %02x\tds_dbbc_dat[di]: %04x\n", index, readle16(ds_dbbc_dat + di));
			b1 = index < readle16(ds_dbbc_dat + di);
			b2 = index == readle16(ds_dbbc_dat + di);
		}
	} while (!b1 && !b2);

	return b2;
}

uint16_t cs_f3d3_maybe_uncompress_hsq(byte *buffer)
{
	if (ds_ce71_disable_hsq_detection) {
		return 0;
	}

	uint8_t checksum = 0;
	for (int i = 0; i != 6; ++i) {
		checksum += buffer[i];
	}
	if (checksum == 0xab) {
		ptr_offset_t c = make_ptr_offset(buffer);
		uint16_t unpacked_size = c.readle16();

		if (c.readbyte() == 0) {
			cs_f40d_unpack_hsq(c, unpacked_size);
		}
		return unpacked_size;
	}
	return 1;
}

void cs_f403_unpack_hsq_skip_header(ptr_offset_t &src, ptr_offset_t &dst)
{
	FUNC_NAME;
	src += 6;
	cs_f435_hsq_unpack_no_header(src, dst);
}

void cs_f40d_unpack_hsq(ptr_offset_t src, uint16_t unpacked_size)
{
	uint16_t offset = src.offset();
	uint16_t packed_size = src.readle16();
	assert(unpacked_size >= packed_size);

	src += -5;
	ptr_offset_t orig_src = src;

	ptr_offset_t new_src = src.ptr() + unpacked_size + 0x40;
	memmove(new_src.ptr(), src.ptr() + 6, packed_size - 6);

	cs_f435_hsq_unpack_no_header(new_src, src);
}

static inline byte getbit(uint16_t &queue, ptr_offset_t &c)
{
	byte bit;

	bit = queue & 1;
	queue >>= 1;

	if (queue == 0) {
		int ofs = c.offset();
		queue = c.readle16();
		bit = queue & 1;
		queue = 0x8000 | (queue >> 1);
	}

	return bit;
}

void cs_f435_hsq_unpack_no_header(ptr_offset_t &src, ptr_offset_t &dst)
{
	FUNC_NAME;
	uint16_t queue = 0;

	for (;;) {
		if (getbit(queue, src)) {
			byte b = src.readbyte();
			// if (framebuffer_log_active)
			// 	printf("A: dst[%5d] = %02x\n", dst.offset(), b);
			dst.writebyte(b);
		} else {
			uint16_t count;
			uint16_t offset;

			if (getbit(queue, src)) {
				uint16_t word = src.readle16();

				count = word & 0x07;
				offset = 8192 - (word >> 3);
				if (count == 0) {
					count = src.readbyte();
				}
				if (count == 0) {
					break;
				}
			} else {
				byte b0 = getbit(queue, src);
				byte b1 = getbit(queue, src);

				count = 2 * b0 + b1;
				int ofs = src.offset();
				byte b = src.readbyte();
				offset = 256 - b;
			}

			// printf("B: dst[%5d..%5d] = dst[%5d..%5d]\n", from, from + count + 1, to, to + count + 1);
			for (int i = 0; i != count + 2; ++i) {
				uint16_t from = dst.offset() - offset;
				byte b = *(dst.begin() + from);
				dst.writebyte(b);
			}
		}
	}
}

void vga_015a_gfx_30(byte ax, byte *di, byte *ds, byte *es)
{
	vga_3200_screen_effect(ax, di, ds, es);
}

void vga_0967_set_graphics_mode() {}

void vga_0975(bool monochrome)
{
	vga_0b0c_gfx_32_set_screen_palette();
}

void vga_09d9_get_screen_buffer(byte **buffer, uint16_t *size)
{
	*size = 320 * 200;
	*buffer = (byte *)calloc(1, 65536);
	printf("vga_09d9_get_screen_buffer -> %p\n", *buffer);
}

void vga_09e2_set_palette_unapplied(const byte *src, int byte_offset, int byte_count)
{
	byte *dst = vga_05bf_palette_unapplied + byte_offset;

	for (int i = 0; i != byte_count; ++i) {
		// printf("vga_05bf[%3d] = %02x\n", i + byte_offset, src[i]);
	}

	if (memcmp(dst, src, byte_count) == 0) {
		return;
	}

	vga_01be_pal_is_dirty = true;

	memcpy(dst, src, byte_count);

	uint16_t offset = byte_offset;
	uint16_t count = byte_count;
	vga_0a21_pal_byte_offset_and_byte_count_to_index(&offset, &count);

	memset(vga_01bf_palette_dirty_entries + offset, 1, count);
}

void vga_0a21_pal_byte_offset_and_byte_count_to_index(uint16_t *offset, uint16_t *count)
{
	*offset /= 3;
	*count /= 3;

	*count = std::min<uint16_t>(*count, 256);
}

void vga_0a40_gfx_38_set_palette_2(const byte *src, int byte_start, int len)
{
	memcpy(vga_02bf_palette_unapplied + byte_start, src, len);
}

void vga_0a58_copy_pal_1_to_pal_2()
{
	memcpy(vga_02bf_palette_unapplied, vga_05bf_palette_unapplied, 3 * 256);
}

void vga_0a68_gfx_41_copy_pal_1_to_pal_2()
{
	vga_0a58_copy_pal_1_to_pal_2();
}

void vga_0ad7_gfx_39(uint8_t speed, uint16_t offset, uint16_t count)
{
	speed = max(speed, 1);
	byte *dst = vga_05bf_palette_unapplied + offset;
	byte *src = vga_02bf_palette_unapplied + offset;
	uint16_t dx = offset;

	for (int i = 0; i != count; ++i) {
		int d = src[i] - dst[i];
		dst[i] += d / speed;
	}
	vga_0a21_pal_byte_offset_and_byte_count_to_index(&offset, &count);
	vga_0b68_set_palette_to_screen(vga_05bf_palette_unapplied, offset, count);
}

void vga_0b0c_gfx_32_set_screen_palette()
{
	FUNC_NAME;
	dump_palette_ppm("vga_05bf_palette_unapplied", vga_05bf_palette_unapplied);

	if (!vga_01be_pal_is_dirty) {
		return;
	}

	vga_01be_pal_is_dirty = false;

	for (int i = 0; i != 256; ++i) {
		if (vga_01bf_palette_dirty_entries[i]) {
			memcpy(&g_screen_palette[3 * i], &vga_05bf_palette_unapplied[3 * i], 3);
		}
	}
	g_app->update_palette(g_screen_palette);

	memset(vga_01bf_palette_dirty_entries, 0, 256);
}

void vga_0b68_set_palette_to_screen(byte *pal, int start, int entries)
{
	FUNC_NAME;
	memcpy(&g_screen_palette[3 * start], &pal[3 * start], 3 * entries);
	g_app->update_palette(g_screen_palette);
	return;
}

void vga_0c06_gfx_33_set_global_y_offset(uint8_t y)
{
	FUNC_NAME;
	vga_01a3_y_offset = y;
}

static inline void rol_u16(uint16_t &v)
{
	uint16_t carry = (v & 0x8000) >> 15;
	v <<= 1;
	v |= carry;
}

byte ds_fa00_cursor_restore_area[16 * 16] = {0};

void vga_1888_gfx_03_draw_cursor(int16_t x, int16_t y, cursor_t *c)
{
	FUNC_NAME;
	y += vga_01a3_y_offset;
	x = std::clamp(x - c->hotspot_x, 0, 319);
	y = std::clamp(y - c->hotspot_y, 0, 199);

	uint16_t w = std::min(320 - x, 16);
	uint16_t h = std::min(200 - y, 16);

	vga_018a_cursor_restore_area_ptr = &ds_dbd8_framebuffer_screen[320 * y + x];
	vga_018c_cursor_restore_w = w;
	vga_018e_cursor_restore_h = h;

	for (uint16_t dy = 0; dy != h; ++dy) {
		for (uint16_t dx = 0; dx != w; ++dx) {
			ds_fa00_cursor_restore_area[16 * dy + dx] = ds_dbd8_framebuffer_screen[320 * (y + dy) + (x + dx)];
		}
	}

	for (uint16_t dy = 0; dy != h; ++dy) {
		uint16_t black_mask_row = c->black_mask[dy];
		for (uint16_t dx = 0; dx != w; ++dx) {
			if ((black_mask_row & (0x8000 >> dx)) == 0) {
				ds_dbd8_framebuffer_screen[320 * (y + dy) + (x + dx)] = 0;
			}
		}
		uint16_t white_mask_row = c->white_mask[dy];
		for (uint16_t dx = 0; dx != w; ++dx) {
			if ((white_mask_row & (0x8000 >> dx)) != 0) {
				ds_dbd8_framebuffer_screen[320 * (y + dy) + (x + dx)] = 0xf;
			}
		}
	}
}

void vga_1940_gfx_04_restore_cursor_area()
{
	byte *dst = vga_018a_cursor_restore_area_ptr;
	uint16_t w = vga_018c_cursor_restore_w;
	uint16_t h = vga_018e_cursor_restore_h;

	for (uint16_t dy = 0; dy != h; ++dy) {
		for (uint16_t dx = 0; dx != w; ++dx) {
			dst[320 * dy + dx] = ds_fa00_cursor_restore_area[16 * dy + dx];
		}
	}
}

void vga_1979_gfx_09_clear_rect(byte *dst, rect_t *rect)
{
	vga_197b_gfx_10_fill_rect(dst, rect, 0);
}

void vga_197b_gfx_10_fill_rect(byte *dst, rect_t *rect, byte color)
{
	for (int y = rect->y0; y != rect->y1; ++y) {
		byte *p = &dst[(y + vga_01a3_y_offset) * 320 + rect->x0];
		memset(p, color, rect->x1 - rect->x0);
	}
}

void vga_19c9_gfx_31_copy_rect(byte *fb, uint16_t src_x, uint16_t src_y, uint16_t w, uint16_t h, uint16_t dst_x,
                               uint16_t dst_y)
{
	FUNC_NAME;
	printf("vga_19c9_gfx_31_copy_rect: src: (%3d, %3d), wh:(%3d, %3d), dst: (%3d, %3d)\n", src_x, src_y, w, h, dst_x,
	       dst_y);

	byte *src = fb + 320 * (src_y + vga_01a3_y_offset) + src_x;
	byte *dst = fb + 320 * (dst_y + vga_01a3_y_offset) + dst_x;

	while (h--) {
		memcpy(dst, src, w);
		src += 320;
		dst += 320;
	}
}

void vga_1a07_draw_line(byte *dst, uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, byte al, uint16_t bp,
                        rect_t *rect)
{
	int16_t delta_x = -(x0 - x1);
	int16_t delta_y = -(y0 - y1);
	vga_1adc_draw_line(dst, x0, y0, delta_x, delta_y, al, bp, rect);
}

void vga_1a3a_draw_line_horizontal(byte *dst, uint16_t x0, uint16_t y0, int16_t delta_x, byte al, uint16_t bp,
                                   rect_t *rect)
{
	int y = y0;
	int x = x0;
	int len = delta_x;

	if (len < 0) {
		x = x0 + len;
		len = -len;
	}

	if (y < rect->y0)
		return;
	if (y >= rect->y1)
		return;

	len += 1;
	do {
		rol_u16(bp);
		if (bp & 1 && x >= rect->x0 && x < rect->x1) {
			dst[320 * (y + vga_01a3_y_offset) + x] = al;
		}
		x += 1;
	} while (--len);
}

void vga_1a86_draw_line_vertical(byte *dst, uint16_t x0, uint16_t y0, int16_t delta_y, byte al, uint16_t bp,
                                 rect_t *rect)
{
	TODO;
	exit(0);
}

void vga_1adc_draw_line(byte *dst, uint16_t x0, uint16_t y0, int16_t delta_x, int16_t delta_y, byte al, uint16_t bp,
                        rect_t *si)
{
	if (delta_y == 0) {
		vga_1a3a_draw_line_horizontal(dst, x0, y0, delta_x, al, bp, si);
		return;
	}
	int sign_y = 1;
	if (delta_y < 0) {
		delta_y = -delta_y;
		sign_y = -sign_y;
	}

	if (delta_x == 0) {
		vga_1a86_draw_line_vertical(dst, x0, y0, delta_y, al, bp, si);
		return;
	}
	int sign_x = 1;
	if (delta_x < 0) {
		sign_x = -sign_x;
		delta_x = -delta_x;
	}

	int16_t bp_6 = sign_y;
	int16_t bp_4 = sign_x;
	int16_t bp_2 = sign_y;
	int16_t bp_0 = sign_x;

	int minor_delta = delta_y;
	int major_delta = delta_x;

	if (delta_x > delta_y) {
		bp_2 = 0;
	} else {
		if (delta_y == 0)
			return;

		std::swap(minor_delta, major_delta);
		bp_0 = 0;
	}

	int16_t ax = major_delta / 2;
	int16_t cx = major_delta;
	do {
		ax += minor_delta;

		int16_t dx, bx;
		if (ax >= major_delta) {
			ax -= major_delta;
			dx = bp_4;
			bx = bp_6;
		} else {
			dx = bp_0;
			bx = bp_2;
		}

		dx += x0;
		bx += y0;
		x0 = dx;
		y0 = bx;

		rol_u16(bp);
		if ((bp & 1) && dx >= si->x0 && bx >= si->y0 && dx < si->x1 && bx < si->y1) {
			dst[320 * (bx + vga_01a3_y_offset) + dx] = al;
		}
	} while (--cx);
}

void vga_1b7c_gfx_11_copy_framebuffer(byte *dst, byte *src)
{
	FUNC_NAME;
	// memcpy(dst, src, 320 * 200);
	for (int y = 0; y != 200; ++y) {
		for (int x = 0; x != 320; x += 2) {
			dst[320 * y + x + 0] = src[320 * y + x + 0];
			dst[320 * y + x + 1] = src[320 * y + x + 1];
			log_framebuffer_write(&dst[320 * y + x], 2);
		}
	}
	if (dst == orig_framebuffer_screen) {
		g_app->update_screen(ds_dbd8_framebuffer_screen);
	}
}

void vga_1bca_copy_interlaced(byte *dst, int dst_x, int dst_y, ptr_offset_t src, int width, int height)
{
	FUNC_NAME;
	dst_y += vga_01a3_y_offset;

	for (int y = 0; y != height; ++y) {
		for (int x = 0; x < width; ++x) {
			dst[320 * (2 * y + dst_y) + 2 * (x + dst_x)] = src.ptr()[width * y + x];
		}
	}
}

void vga_1be7_copy_game_area(byte *dst, byte *src)
{
	FUNC_NAME;
	memcpy(dst, src, 152 * 320);
}

void vga_1b8e_gfx_12_copy_framebuffer_rect(byte *dst, byte *src, rect_t *rect)
{
	FUNC_NAME;
	byte *dstp = dst + (rect->y0 + vga_01a3_y_offset) * 320 + rect->x0;
	byte *srcp = src + (rect->y0 + vga_01a3_y_offset) * 320 + rect->x0;
	int h = rect->y1 - rect->y0;
	int w = rect->x1 - rect->x0;
	for (int i = 0; i != h; ++i) {
		memcpy(dstp, srcp, w);
		dstp += 320;
		srcp += 320;
	}
}

void vga_1bfe_gfx_7_draw_glyph(byte *dst, int x, int y, byte *src, uint8_t w, uint8_t h, uint16_t color)
{
	byte bg = hi(color);
	byte fg = lo(color);

	// printf("\tx: %3d, y: %3d, w: %d, h: %d, fg: 0x%02x, bg: 0x%02x\n", x, y, w, h, fg, bg);

	dst += (y + vga_01a3_y_offset) * 320 + x;

	for (int dy = 0; dy != h; ++dy) {
		byte b = *src++;
		for (int dx = 0; dx != w; ++dx) {
			if (b & 0x80) {
				*dst = fg;
			} else if (bg) {
				*dst = bg;
			}
			b <<= 1;
			++dst;
		}
		dst += 320 - w;
	}
}

void vga_1c76_gfx_26_copy_framebuffer_rect(byte *dst, byte *src, rect_t *rect)
{
	FUNC_NAME;
	vga_1b8e_gfx_12_copy_framebuffer_rect(dst, src, rect);
}

void vga_1cb6_gfx_23(byte *dst, byte *map, byte *globdata, int dx, int ax)
{
	FUNC_NAME;
	uint16_t cs_1CA8 = 1;    // offset into globdata
	uint16_t cs_1CA6 = 1;    // offset into globdata
	uint16_t cs_1CAA = 3290; // offset into globdata
	uint16_t cs_1CAC = 98;   // offset into ds_2460_globe_tilt_lookup_table

	uint16_t cs_1CB4 = -320; // screen width

	uint16_t cs_1CB0 = 0x6360;
	uint16_t cs_1CB2 = 0x6360;
	uint16_t cs_1CAE = 0x6360 - 1;

	bool drawing_southern_hemisphere;

	drawing_southern_hemisphere = false;

	do {
		uint16_t di = cs_1CA6; // offset into globdata

		uint16_t ax = globdata[di++];
		uint16_t bx, cx, dx;

		if (uint8_as_int8(ax & 0xff) < 0) {
			drawing_southern_hemisphere = true;
			di = cs_1CA8;

			cs_1CB4 = -cs_1CB4;
			if (uint16_as_int16(cs_1CB4) < 0) {
				return;
			}
			cs_1CB0 = 0x64A0;
			cs_1CB2 = 0x64A0;
			cs_1CAE = 0x64A0 - 1;

			ax = globdata[di - 1];
			di -= uint8_as_int8(ax & 0xff);
			ax = globdata[di++];
		}

		uint16_t si = cs_1CAA; // offset into globdata

		do {
			if (drawing_southern_hemisphere) {
				ax = -ax;
			}

			uint16_t ax_ = ax;
			ax = ds_2460_globe_tilt_lookup_table[cs_1CAC + uint8_as_int8(ax & 0xff)];

			if (uint16_as_int16(ax) >= 0) {
				ax = uint8_as_int8(ax & 0xff);
				if (uint16_as_int16(ax) < 0) {
					ax = -ax;
					uint16_t bp = ax;
					bx = globdata[bp + si];
					ax = globdata[bp + si + 0x64];

					bp = bx / 2;
					bx = globe_rotation_lookup_table[bp].unk0;
					cx = globe_rotation_lookup_table[bp].unk1;
					dx = globe_rotation_lookup_table[bp].unk2;

					ax = cx - ax;
				} else {
					uint16_t bp = ax;
					bx = globdata[bp + si];
					ax = globdata[bp + si + 0x64];

					bp = bx / 2;
					bx = globe_rotation_lookup_table[bp].unk0;
					cx = globe_rotation_lookup_table[bp].unk1;
					dx = globe_rotation_lookup_table[bp].unk2;
				}
			} else {
				ax = uint8_as_int8(ax & 0xff);
				if (uint16_as_int16(ax) < 0) {
					ax = -ax;
					uint16_t bp = ax;
					bx = globdata[bp + si];
					ax = globdata[bp + si + 0x64];

					bp = bx / 2;
					bx = globe_rotation_lookup_table[bp].unk0;
					cx = globe_rotation_lookup_table[bp].unk1;
					dx = globe_rotation_lookup_table[bp].unk2;

					ax = cx - ax;
					bx = -bx;
				} else {
					uint16_t bp = ax;
					bx = globdata[bp + si];
					ax = globdata[bp + si + 0x64];

					bp = bx / 2;
					bx = globe_rotation_lookup_table[bp].unk0;
					cx = globe_rotation_lookup_table[bp].unk1;
					dx = globe_rotation_lookup_table[bp].unk2;

					bx = -bx;
				}
			}

			cx *= 2;

			uint16_t bp = dx - ax;
			if (uint16_as_int16(bp) < 0) {
				bp += cx;
			}
			bp += bx;
			dx += ax;

			ax = map[0x62FC + uint16_as_int16(bp)];
			{
				uint8_t al = ax & 0x0f;
				if ((ax & 0x30) == 0x10) {
					if (al < 8) {
						al += 12;
					}
				}
				al += 0x10;

				dst[cs_1CAE--] = al;
			}

			bp = dx - cx;
			if (uint16_as_int16(bp) < 0) {
				bp += cx;
			}
			bp += bx;

			ax = map[0x62FC + uint16_as_int16(bp)];
			{
				uint8_t al = ax & 0x0f;
				if ((ax & 0x30) == 0x10) {
					if (al < 8) {
						al += 12;
					}
				}
				al += 0x10;

				dst[cs_1CB0++] = al;
			}

			si += 200;
			ax = globdata[di++];
		} while (uint8_as_int8(ax) >= 0);

		cs_1CA6 = di;
		ax = cs_1CB4 + cs_1CB2;
		cs_1CB2 = ax;
		cs_1CB0 = ax;
		cs_1CAE = ax - 1;
	} while (true);
}

void vga_0f5b_blit(byte *dst, int dst_x, int dst_y, ptr_offset_t src, int width, int height, uint8_t flags,
                   uint8_t mode)
{
	FUNC_NAME;
	rect_t clip_rect = {0, 0, 320, 200};
	vga_1452_blit_clipped(dst, dst_x, dst_y, src, width, height, flags, mode, clip_rect);
}

void vga_1452_blit_clipped(byte *dst, int dst_x, int dst_y, ptr_offset_t src, int width, int height, uint8_t flags,
                           uint8_t mode, rect_t clip_rect)
{
	// FUNC_NAME;
	bool flip_x = flags & 0x40;
	bool flip_y = flags & 0x20;

	clip_rect.y0 += vga_01a3_y_offset;
	clip_rect.y1 += vga_01a3_y_offset;
	dst_y += vga_01a3_y_offset;

	// printf("clip_rect: (%3d, %3d, %3d, %3d)\n", clip_rect.x0, clip_rect.y0, clip_rect.x1, clip_rect.y1);
	// printf("vga_1452_blit_clipped: flags: %02x, mode: %d\n", flags, mode);

	if (mode < 254) {
		if ((flags & 0x80) == 0) {
			// printf("draw_4bpp\n");
			draw_4bpp(dst, flip_x, flip_y, src, dst_x, dst_y, width, height, mode, clip_rect);
		} else {
			// printf("draw_4bpp_rle\n");
			draw_4bpp_rle(dst, flip_x, flip_y, src, dst_x, dst_y, width, height, mode, clip_rect);
		}
	} else {
		if ((flags & 0x80) == 0) {
			// printf("draw_8bpp\n");
			draw_8bpp(dst, flip_x, flip_y, src, dst_x, dst_y, width, height, mode, clip_rect);
		} else {
			// printf("draw_8bpp_rle\n");
			draw_8bpp_rle(dst, flip_x, flip_y, src, dst_x, dst_y, width, height, mode, clip_rect);
		}
	}
}

inline void write_pixel(byte *dst, int x, int y, uint8_t v, rect_t clip_rect)
{
	if (clip_rect.contains(x, y)) {
		dst[320 * y + x] = v;
		log_framebuffer_write(&dst[320 * y + x], 1);
	}
}

#define START_Y (!flip_y ? 0 : h - 1)
#define END_Y (!flip_y ? h : -1)
#define DELTA_Y (!flip_y ? 1 : -1)

#define START_X (!flip_x ? 0 : w - 1)
#define END_X (!flip_x ? w : -1)
#define DELTA_X (!flip_x ? 1 : -1)

void draw_4bpp(byte *dst, bool flip_x, bool flip_y, ptr_offset_t &src, int dst_x, int dst_y, int w, int h, uint8_t mode,
               rect_t clip_rect)
{
	for (int y = START_Y; y != END_Y; y += DELTA_Y) {
		int line_remain = 4 * ((w + 3) / 4);
		int x = START_X;

		do {
			byte value = src.readbyte();
			byte p1 = (value & 0x0f);
			byte p2 = (value >> 4);
			if (p1) {
				write_pixel(dst, dst_x + x, dst_y + y, p1 + mode, clip_rect);
			}
			x += DELTA_X;
			if (p2) {
				write_pixel(dst, dst_x + x, dst_y + y, p2 + mode, clip_rect);
			}
			x += DELTA_X;
			line_remain -= 2;
		} while (line_remain > 0);
	}
}

void draw_4bpp_rle(byte *dst, bool flip_x, bool flip_y, ptr_offset_t &src, int dst_x, int dst_y, int w, int h,
                   uint8_t mode, rect_t clip_rect)
{
	int src_x = 0, src_y = 0;

	for (int y = START_Y; y != END_Y; y += DELTA_Y) {
		int line_remain = 4 * ((w + 3) / 4);
		int x = START_X;

		do {
			byte cmd = src.readbyte();
			if (cmd & 0x80) {
				int count = 257 - cmd;
				byte value = src.readbyte();

				byte p1 = (value & 0x0f);
				byte p2 = (value >> 4);
				for (int i = 0; i != count; ++i) {
					if (p1 && y >= src_y && x >= src_x) {
						write_pixel(dst, dst_x + x - src_x, dst_y + y - src_y, p1 + mode, clip_rect);
					}
					x += DELTA_X;
					if (p2 && y >= src_y && x >= src_x) {
						write_pixel(dst, dst_x + x - src_x, dst_y + y - src_y, p2 + mode, clip_rect);
					}
					x += DELTA_X;
				}
				line_remain -= 2 * count;
			} else {
				int count = cmd + 1;
				for (int i = 0; i != count; ++i) {
					byte value = src.readbyte();

					byte p1 = (value & 0x0f);
					byte p2 = (value >> 4);
					if (p1 && y >= src_y && x >= src_x) {
						write_pixel(dst, dst_x + x - src_y, dst_y + y - src_y, p1 + mode, clip_rect);
					}
					x += DELTA_X;
					if (p2 && y >= src_y && x >= src_x) {
						write_pixel(dst, dst_x + x - src_x, dst_y + y - src_y, p2 + mode, clip_rect);
					}
					x += DELTA_X;
				}
				line_remain -= 2 * count;
			}
		} while (line_remain > 0);
	}
}

void draw_8bpp(byte *dst, bool flip_x, bool flip_y, ptr_offset_t &src, int dst_x, int dst_y, int w, int h, uint8_t mode,
               rect_t clip_rect)
{
	for (int y = START_Y; y != END_Y; y += DELTA_Y) {
		int x = START_X;

		if (mode == 255) {
			// mode 255 means that 0 is transparent.
			for (int i = 0; i != w; ++i) {
				byte value = src.readbyte();
				if (value) {
					write_pixel(dst, dst_x + x, dst_y + y, value, clip_rect);
				}
				x += DELTA_X;
			}
		} else {
			// mode 254 means that 0 isn't transparent
			for (int i = 0; i != w; ++i) {
				byte value = src.readbyte();
				write_pixel(dst, dst_x + x, dst_y + y, value, clip_rect);
				x += DELTA_X;
			}
		}
	}
}

void draw_8bpp_rle(byte *dst, bool flip_x, bool flip_y, ptr_offset_t &src, int dst_x, int dst_y, int w, int h,
                   uint8_t mode, rect_t clip_rect)
{
	for (int y = START_Y; y != END_Y; y += DELTA_Y) {
		int line_remain = w;
		int x = START_X;

		do {
			byte cmd = src.readbyte();
			if (cmd & 0x80) {
				int count = 257 - cmd;
				byte value = src.readbyte();
				if (mode == 255 && value == 0) {
					x += flip_x ? -count : count;
				} else {
					for (int i = 0; i != count; ++i) {
						write_pixel(dst, dst_x + x, dst_y + y, value, clip_rect);
						x += DELTA_X;
					}
				}
				line_remain -= count;
			} else {
				int count = cmd + 1;
				for (int i = 0; i != count; ++i) {
					byte value = src.readbyte();
					if (!(mode == 255 && value == 0)) {
						write_pixel(dst, dst_x + x, dst_y + y, value, clip_rect);
					}
					x += DELTA_X;
				}
				line_remain -= count;
			}
		} while (line_remain > 0);
	}
}

void draw_4bpp_scaled(byte *src, uint16_t src_pitch, int dst_x, int dst_y, int w, int h, byte flags, byte pal_offset,
                      uint16_t src_delta_fp)
{
	byte *dst = ds_dbda_framebuffer_active;

	bool flip_x = flags & 0x40;
	bool flip_y = flags & 0x20;

	// Source coordinates in 8.8 fixed point
	uint16_t src_x_fp = 0;
	uint16_t src_y_fp = 0;

	rect_t clip_rect = {0, 0, 320, 200};

	for (int y = START_Y; y != END_Y; y += DELTA_Y, src_y_fp += src_delta_fp) {
		uint16_t src_y = (src_y_fp >> 8);
		src_x_fp = 0;
		for (int x = START_X; x != END_X; x += DELTA_X, src_x_fp += src_delta_fp) {
			uint16_t src_x = (src_x_fp >> 8);

			byte value = src[src_y * src_pitch + src_x / 2];
			if (src_x & 1) {
				value >>= 4;
			}
			value &= 0x0f;
			if (value != 0) {
				write_pixel(dst, dst_x + x, dst_y + y + vga_01a3_y_offset, value + pal_offset, clip_rect);
			}
		}
	}
}

#undef ADVANCE

void vga_2572_wait_frame(std::atomic_uint16_t &timer, uint16_t start)
{
	FUNC_NAME;
	g_app->wait_for_vsync();
	return;
	// if (!vga_01a1_no_vert_retrace) {
	// app.update_screen();

	// dump_framebuffer_ppm("vga_2572_wait_frame-frame", ds_dbd8_framebuffer_screen, g_screen_palette);
	// dump_palette_ppm("vga_2572_wait_frame-pal", g_screen_palette);
	// g_frame++;

	// return;
	// }

	uint16_t now;
	while ((now = timer.load()) - start < 3) {
		std::this_thread::yield();
	}
}

void vga_25e7_transition(std::atomic_uint16_t &timer, uint8_t type, uint8_t *si, uint8_t *es, uint8_t *ds)
{
	vga_2768 = 8;
	vga_276a = 1;
	vga_2535_fb = si;
	vga_2537_fb = ds;
	vga_2539_fb = es;
	uint16_t cx = 152;

	switch (type & 0xfe) {
	case 0x10:
		vga_2dc3_transition_effect_0x10(timer, cx);
		break;
	case 0x30:
		vga_2757_transition_effect_0x30();
		break;
	case 0x3a:
		vga_272e_transition_effect_0x3a(timer);
		break;
	default:
		TODO;
		printf("Unhandled transition type 0x%02x\n", type);
		// exit(0);
	}
}

uint16_t ds_261b = 0x40;

void vga_264d(std::atomic_uint16_t &timer, uint16_t cx, uint8_t dh, uint8_t dl)
{
	FUNC_NAME;
	vga_1b7c_gfx_11_copy_framebuffer(vga_2539_fb, vga_2537_fb);

	do {
		for (uint16_t bx = 0; bx < 765; bx += cx) {
			uint16_t start = timer.load();
			byte *dst = &vga_05bf_palette_unapplied[bx];
			byte *src = &vga_02bf_palette_unapplied[bx];
			for (int i = 0; i != cx; ++i) {
				uint8_t al = *src++ - *dst;
				if (al != 0) {
					uint16_t quotient = al / dh;
					uint16_t remainder = al % dh;
					quotient += 1;
					if (remainder == 0) {
						quotient -= 1;
						remainder = dh;
					}
					if (quotient >= dl) {
						*dst += remainder;
					}
				}
				++dst;
			}

			vga_0b68_set_palette_to_screen(vga_05bf_palette_unapplied, bx / 3, std::min(cx / 3, 256));
			vga_261d_maybe_wait_frame(timer, start);
		}
	} while (--dl != 0);
}

void vga_26e3(std::atomic_uint16_t &timer, uint16_t ax, uint8_t fade_step_size, uint8_t fade_steps)
{
	TODO;
	ds_261b = ax;

	do {
		for (int bx = 0; bx < 255; bx += ds_261b) {
			uint16_t start = timer.load();
			byte *p = &vga_05bf_palette_unapplied[3 * bx];
			for (int cx = 0; cx != 3 * ax; ++cx) {
				*p = std::max(*p - fade_step_size, 0);
				++p;
			}
			vga_0b68_set_palette_to_screen(vga_05bf_palette_unapplied, bx, ax);
			vga_261d_maybe_wait_frame(timer, start);
		}
	} while (--fade_steps != 0);
}

void vga_261d_maybe_wait_frame(std::atomic_uint16_t &timer, uint16_t start)
{
	FUNC_NAME;
	vga_2572_wait_frame(timer, start);
}

void vga_272e_transition_effect_0x3a(std::atomic_uint16_t &timer)
{
	for (int i = 1; i != 3 * 255; ++i) {
		std::swap(vga_02bf_palette_unapplied[i], vga_05bf_palette_unapplied[i]);
	}
	vga_26e3(timer, 85, 3, 22);
	vga_264d(timer, 255, 3, 22);
}

void vga_2757_transition_effect_0x30()
{
	vga_0b0c_gfx_32_set_screen_palette();
	memcpy(vga_2539_fb + vga_01a3_y_offset, vga_2537_fb + vga_01a3_y_offset, 152 * 320);
}

void vga_2dc3_transition_effect_0x10(std::atomic_uint16_t &timer, uint16_t cx)
{
	uint16_t start = timer.load();

	for (int i = 0; i != 16; ++i) {
		for (int y = 0; y != cx / 4; ++y) {
			for (int x = 0; x != 320 / 4; ++x) {
				int offset = 320 * (4 * y + vga_01a3_y_offset) + 4 * x + vga_2fd7_transition_offsets[i];
				vga_2539_fb[offset] = 0;
			}
		}
		g_app->update_screen(vga_2539_fb);
		vga_2572_wait_frame(timer, start);
	}

	vga_0b0c_gfx_32_set_screen_palette();

	for (int i = 0; i != 16; ++i) {
		for (int y = 0; y != cx / 4; ++y) {
			for (int x = 0; x != 320 / 4; ++x) {
				int offset = 320 * (4 * y + vga_01a3_y_offset) + 4 * x + vga_2fd7_transition_offsets[i];
				vga_2539_fb[offset] = vga_2537_fb[offset];
			}
		}
		g_app->update_screen(vga_2539_fb);
		vga_2572_wait_frame(timer, start);
	}
}

#define XY(x, y) (320 * y + x)

// clang-format off
uint16_t vga_2fd7_transition_offsets[16] = {
	XY(1, 1), XY(0, 3), XY(3, 2), XY(2, 0),
	XY(0, 1), XY(2, 3), XY(0, 0), XY(1, 2),
	XY(3, 0), XY(1, 3), XY(2, 1), XY(3, 3),
	XY(2, 2), XY(1, 0), XY(3, 1), XY(0, 2)
};
// clang-format on
#undef XY

void vga_3200_screen_effect(byte ax, byte *di, byte *ds, byte *es)
{
	ax &= 0xfe;
	while (ax >= 0x1a) {
		ax -= 0x1a;
	}

	switch (ax) {
	case 0x00:
		assert(0);
		break;
	case 0x02:
		assert(0);
		break;
	case 0x04:
		assert(0);
		break;
	case 0x06:
		assert(0);
		break;
	case 0x08:
		assert(0);
		break;
	case 0x0a:
		assert(0);
		break;
	case 0x0c:
		assert(0);
		break;
	case 0x0e:
		assert(0);
		break;
	case 0x10:
		assert(0);
		break;
	case 0x12:
		assert(0);
		break;
	case 0x14:
		assert(0);
		break;
	case 0x16:
		assert(0);
		break;
	case 0x18:
		assert(0);
		break;
	}
}

void vga_39e9_gfx_36_draw_gradient_line_with_noise(int x, int y, int w, int dir, uint16_t &galois_seed,
                                                   uint16_t galois_mask, uint16_t di, uint16_t color)
{
	assert(w > 0);

	y += vga_01a3_y_offset;
	byte *dst = ds_dbda_framebuffer_active + 320 * y + x;

	do {
		bool lsb = galois_seed & 1;
		galois_seed >>= 1;
		if (lsb) {
			galois_seed ^= galois_mask;
		}

		byte v = (galois_seed & 3) - 1 + hi(color);
		color += di;

		int offset = dst - ds_dbda_framebuffer_active;
		*dst = lo(v);
		dst += dir;
	} while (--w);
}
