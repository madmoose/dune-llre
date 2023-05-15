#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdio>
#include <cstdio>
#include <thread>

#include <fcntl.h>
#include <unistd.h>

#include "common/ptr_offset_t.h"
#include "dune.h"

using namespace std::chrono_literals;

constexpr bool dump_resources = true;

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

/*
 * Data Segment
 */

std::atomic_bool quitting = false;
std::thread *timer_thread = nullptr;
#include <resources.h>
int16_t ds_278e_active_bank_id;
int ds_35a6_hnm_fd;
byte *ds_39b7_alloc_next_addr;
Scene *ds_4854_intro_scene_current_scene;
byte ds_4948_tablat_bin[792];
byte *ds_ce68_alloc_last_addr;
uint16_t ds_ce70_res_index_needs_extended_memory;
byte ds_ce71_disable_hsq_detection;
uint16_t ds_ce74_framebuffer_size;
int ds_ce78_dat_resource;
byte ds_ceeb_language_setting;
std::atomic_uint16_t ds_ce7a_pit_timer_counter = 0;
std::atomic_uint16_t ds_ce7c_pit_timer_counter_hi = 0;
uint16_t ds_d820;
ptr_offset_t ds_d844_resource_ptr[146];
uint16_t ds_dabc_resource_last_used[124];
byte ds_dbb4_last_bank_palette;
byte ds_dbb5_hnm_flag;
int ds_dbba_dat_fd;
byte *ds_dbbc_dat;
byte *ds_dbd6_framebuffer_1;
byte *ds_dbd8_framebuffer_screen;
byte *ds_dbda_framebuffer_active;
byte *ds_dbdc_framebuffer_2;
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
uint16_t ds_dc24_hnm_decode_frame_flags;
uint16_t ds_dc1a_hnm_read_offset;
uint16_t ds_dc1c_hnm_sd_block_ofs;
uint16_t ds_dc1e_hnm_pl_block_ofs;
uint16_t ds_dc20_hnm_block_size;
bool ds_dce6_in_transition = false;

/**
 * VGA data segment
 */

uint8_t vga_01a1_no_vert_retrace = 0;
uint8_t vga_01a3_y_offset = 0;
uint8_t vga_01bd_is_monochrome = 0;
uint8_t vga_01be_pal_is_dirty = 1;
uint8_t vga_01bf_palette_dirty_entries[256] = {
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
};

RGB vga_05bf_palette_unapplied[256] = {
	{0x00, 0x00, 0x00}, {0x00, 0x00, 0x2A}, {0x00, 0x2A, 0x00}, {0x00, 0x2A, 0x2A}, {0x2A, 0x00, 0x00},
	{0x3F, 0x34, 0x14}, {0x2A, 0x15, 0x00}, {0x2A, 0x2A, 0x2A}, {0x15, 0x15, 0x15}, {0x15, 0x15, 0x3F},
	{0x15, 0x3F, 0x15}, {0x15, 0x3F, 0x3F}, {0x3F, 0x15, 0x15}, {0x3F, 0x15, 0x3F}, {0x3F, 0x3F, 0x15},
	{0x3F, 0x3F, 0x3F}, {0x00, 0x00, 0x00}, {0x00, 0x00, 0x00}, {0x00, 0x00, 0x00}, {0x00, 0x00, 0x00},
	{0x3E, 0x39, 0x1B}, {0x3F, 0x3F, 0x26}, {0x3F, 0x3D, 0x15}, {0x3F, 0x36, 0x1B}, {0x3F, 0x2D, 0x00},
	{0x36, 0x24, 0x0A}, {0x36, 0x18, 0x0A}, {0x2D, 0x10, 0x00}, {0x24, 0x12, 0x00}, {0x1B, 0x12, 0x0F},
	{0x1B, 0x0E, 0x08}, {0x12, 0x00, 0x0F}, {0x00, 0x00, 0x00}, {0x00, 0x00, 0x00}, {0x00, 0x00, 0x00},
	{0x00, 0x00, 0x00}, {0x00, 0x00, 0x00}, {0x00, 0x00, 0x00}, {0x00, 0x00, 0x00}, {0x00, 0x00, 0x00},
	{0x00, 0x00, 0x00}, {0x00, 0x00, 0x00}, {0x00, 0x00, 0x00}, {0x00, 0x00, 0x00}, {0x00, 0x00, 0x00},
	{0x00, 0x00, 0x00}, {0x00, 0x00, 0x00}, {0x00, 0x00, 0x00}, {0x00, 0x00, 0x00}, {0x00, 0x00, 0x00},
	{0x00, 0x00, 0x00}, {0x00, 0x00, 0x00}, {0x00, 0x00, 0x00}, {0x00, 0x00, 0x00}, {0x00, 0x00, 0x00},
	{0x00, 0x00, 0x00}, {0x00, 0x00, 0x00}, {0x00, 0x00, 0x00}, {0x00, 0x00, 0x00}, {0x00, 0x00, 0x00},
	{0x00, 0x00, 0x00}, {0x00, 0x00, 0x00}, {0x00, 0x00, 0x00}, {0x00, 0x00, 0x00}, {0x00, 0x00, 0x00},
	{0x00, 0x00, 0x00}, {0x00, 0x00, 0x00}, {0x00, 0x00, 0x00}, {0x00, 0x00, 0x00}, {0x00, 0x00, 0x00},
	{0x00, 0x00, 0x00}, {0x00, 0x00, 0x00}, {0x00, 0x00, 0x00}, {0x00, 0x00, 0x00}, {0x00, 0x00, 0x00},
	{0x00, 0x00, 0x00}, {0x00, 0x00, 0x00}, {0x00, 0x00, 0x00}, {0x00, 0x00, 0x00}, {0x00, 0x00, 0x00},
	{0x00, 0x00, 0x00}, {0x00, 0x00, 0x00}, {0x00, 0x00, 0x00}, {0x00, 0x00, 0x00}, {0x00, 0x00, 0x00},
	{0x00, 0x00, 0x00}, {0x00, 0x00, 0x00}, {0x00, 0x00, 0x00}, {0x00, 0x00, 0x00}, {0x00, 0x00, 0x00},
	{0x00, 0x00, 0x00}, {0x00, 0x00, 0x00}, {0x00, 0x00, 0x00}, {0x00, 0x00, 0x00}, {0x00, 0x00, 0x00},
	{0x00, 0x00, 0x00}, {0x00, 0x00, 0x00}, {0x00, 0x00, 0x00}, {0x00, 0x00, 0x00}, {0x00, 0x00, 0x00},
	{0x00, 0x00, 0x00}, {0x00, 0x00, 0x00}, {0x00, 0x00, 0x00}, {0x00, 0x00, 0x00}, {0x00, 0x00, 0x00},
	{0x00, 0x00, 0x00}, {0x00, 0x00, 0x00}, {0x00, 0x00, 0x00}, {0x00, 0x00, 0x00}, {0x00, 0x00, 0x00},
	{0x00, 0x00, 0x00}, {0x00, 0x00, 0x00}, {0x00, 0x00, 0x00}, {0x00, 0x00, 0x00}, {0x00, 0x00, 0x00},
	{0x00, 0x00, 0x00}, {0x00, 0x00, 0x00}, {0x00, 0x00, 0x00}, {0x00, 0x00, 0x00}, {0x00, 0x00, 0x00},
	{0x00, 0x00, 0x00}, {0x00, 0x00, 0x00}, {0x00, 0x00, 0x00}, {0x00, 0x00, 0x00}, {0x00, 0x00, 0x00},
	{0x00, 0x00, 0x00}, {0x00, 0x00, 0x00}, {0x00, 0x00, 0x00}, {0x00, 0x00, 0x00}, {0x00, 0x00, 0x00},
	{0x00, 0x00, 0x00}, {0x00, 0x00, 0x00}, {0x00, 0x00, 0x00}, {0x00, 0x00, 0x00}, {0x00, 0x00, 0x00},
	{0x00, 0x00, 0x00}, {0x00, 0x00, 0x00}, {0x00, 0x00, 0x00}, {0x00, 0x00, 0x00}, {0x00, 0x00, 0x00},
	{0x00, 0x00, 0x00}, {0x00, 0x00, 0x00}, {0x00, 0x00, 0x00}, {0x00, 0x00, 0x00}, {0x00, 0x00, 0x00},
	{0x00, 0x00, 0x00}, {0x00, 0x00, 0x00}, {0x00, 0x00, 0x00}, {0x00, 0x00, 0x00}, {0x00, 0x00, 0x00},
	{0x00, 0x00, 0x00}, {0x00, 0x00, 0x00}, {0x00, 0x00, 0x00}, {0x00, 0x00, 0x00}, {0x00, 0x00, 0x00},
	{0x00, 0x00, 0x00}, {0x00, 0x00, 0x00}, {0x00, 0x00, 0x00}, {0x00, 0x00, 0x00}, {0x00, 0x00, 0x00},
	{0x30, 0x3F, 0x34}, {0x28, 0x36, 0x2E}, {0x21, 0x2D, 0x28}, {0x1A, 0x24, 0x21}, {0x13, 0x1B, 0x1A},
	{0x0D, 0x13, 0x13}, {0x08, 0x04, 0x08}, {0x10, 0x04, 0x18}, {0x08, 0x00, 0x10}, {0x04, 0x00, 0x0C},
	{0x0A, 0x00, 0x02}, {0x14, 0x1C, 0x1A}, {0x00, 0x00, 0x00}, {0x00, 0x00, 0x00}, {0x14, 0x18, 0x1C},
	{0x01, 0x01, 0x02}, {0x04, 0x04, 0x05}, {0x07, 0x06, 0x09}, {0x09, 0x09, 0x0C}, {0x0C, 0x0B, 0x0F},
	{0x0F, 0x0E, 0x12}, {0x11, 0x11, 0x16}, {0x14, 0x13, 0x19}, {0x17, 0x16, 0x1C}, {0x1E, 0x1A, 0x21},
	{0x27, 0x1D, 0x27}, {0x2D, 0x20, 0x29}, {0x33, 0x23, 0x28}, {0x39, 0x28, 0x26}, {0x3F, 0x33, 0x29},
	{0x3F, 0x3F, 0x29}, {0x00, 0x00, 0x3F}, {0x02, 0x00, 0x3B}, {0x05, 0x00, 0x37}, {0x08, 0x00, 0x34},
	{0x0A, 0x00, 0x30}, {0x0C, 0x00, 0x2C}, {0x0E, 0x00, 0x29}, {0x0E, 0x00, 0x25}, {0x0F, 0x00, 0x21},
	{0x0F, 0x00, 0x1E}, {0x0F, 0x00, 0x1A}, {0x0E, 0x00, 0x16}, {0x0D, 0x00, 0x13}, {0x0B, 0x00, 0x0F},
	{0x09, 0x00, 0x0B}, {0x07, 0x00, 0x08}, {0x3B, 0x30, 0x28}, {0x3B, 0x31, 0x28}, {0x3B, 0x31, 0x29},
	{0x3C, 0x32, 0x2A}, {0x3C, 0x33, 0x2B}, {0x3C, 0x33, 0x2C}, {0x3C, 0x34, 0x2C}, {0x3D, 0x34, 0x2E},
	{0x3D, 0x35, 0x2E}, {0x3D, 0x36, 0x2F}, {0x08, 0x10, 0x0A}, {0x3F, 0x3F, 0x3F}, {0x2E, 0x3D, 0x32},
	{0x22, 0x30, 0x28}, {0x1F, 0x2A, 0x26}, {0x26, 0x34, 0x2C}, {0x08, 0x0A, 0x10}, {0x0B, 0x0B, 0x10},
	{0x0C, 0x0B, 0x10}, {0x0D, 0x0B, 0x10}, {0x0F, 0x0B, 0x10}, {0x10, 0x0B, 0x10}, {0x10, 0x0B, 0x0F},
	{0x10, 0x0B, 0x0D}, {0x10, 0x0B, 0x0C}, {0x10, 0x0B, 0x0B}, {0x32, 0x32, 0x32}, {0x2F, 0x2A, 0x37},
	{0x1E, 0x16, 0x3E}, {0x1C, 0x0B, 0x3F}, {0x0F, 0x10, 0x0B}, {0x3D, 0x34, 0x2E}, {0x0C, 0x10, 0x0B},
	{0x00, 0x00, 0x00}, {0x00, 0x00, 0x00}, {0x00, 0x00, 0x00}, {0x00, 0x00, 0x00}, {0x00, 0x00, 0x00},
	{0x00, 0x00, 0x00}, {0x00, 0x00, 0x00}, {0x00, 0x00, 0x00}, {0x00, 0x00, 0x00}, {0x00, 0x00, 0x00},
	{0x00, 0x00, 0x00}, {0x00, 0x00, 0x00}, {0x00, 0x00, 0x00}, {0x00, 0x00, 0x00}, {0x00, 0x00, 0x00},
	{0x3F, 0x3F, 0x3F}};

/*
 * Code Segment
 */

int main(int argc, char **argv)
{
	cs_0000_start();

	quitting = true;
	timer_thread->join();
}

void cs_0000_start()
{
	ds_39b7_alloc_next_addr = (byte *)calloc(16 * 1048576, 1);
	ds_ce68_alloc_last_addr = ds_39b7_alloc_next_addr + 16 * 1048576;

	cs_e594_initialize_system();
	cs_00b0_initialize_resources();
	cs_0580_play_intro();
}

void cs_00b0_initialize_resources()
{
	cs_00d1_initialize_resources();
	cs_0169_initialize_map();
}

void cs_00d1_initialize_resources()
{
	// Load TABLAT.BIN
	cs_f0b9_res_read_by_index_into_buffer(0xba, ds_4948_tablat_bin);

	// Load MAP.HSQ
	cs_f0b9_res_read_by_index_into_buffer(0xbf, nullptr);
}

void cs_0169_initialize_map() {
	TODO;
}

Scene cs_0337_intro_script[] = {
	{0, cs_061c_load_virgin_hnm, 0, 0x003a, cs_0625_play_virgin_hnm, 1},
	{0, cs_c0ad_gfx_clear_active_framebuffer, 0, 0x003a, cs_0f66_nullsub, 1},
	{0, cs_064d_load_cryo_hnm, 0, 0x0030, cs_0661_play_cryo_hnm, 1},
	{0, cs_0658_load_cryo2_hnm, 0x006f, 0x0030, cs_0661_play_cryo_hnm, 1},
	{0, cs_0f66_nullsub, 0x00a8, -1, cs_0f66_nullsub, 1},
	// {      0,      _sub_10678_load_PRESENT_HNM,            0, 0x003a, _sub_10684_play_PRESENT_HNM,            1 },
    // {      0,      _sub_1CEFC_load_IRULn_HSQ,              0, 0x003a, _sub_1CF1B_play_IRULx_HSQ,              1 },
    // {      0,      clear_screen,                           0, 0x003a, empty,                                  1 },
    // {      0,      _sub_1069E_load_INTRO_HNM,              0, 0x0036, empty,                             0x0190 },
    // {      0,      empty,                             0x0090, 0x0030, _sub_106AA_play_hnm_86_frames,     0x0190 },
    // {      0,      empty,                             0x010c,     -1, _sub_106BD_play_hnm_skippable,          1 },
    // {      0,      sub_107FD,                              0, 0x003a, loc_1085D,                         0x04b0 },
    // { 0x0148,      sub_106CE,                         0x014e, 0x0010, sub_10704,                         0x1900 },
    // { 0x024b,      sub_10972,                         0x024e, 0x0010, empty,                             0x0085 },
    // {      0,      sub_1098A,                         0x0258, 0x0010, empty,                             0x0085 },
    // {      0,      sub_10995,                         0x0262, 0x0010, loc_10798,                         0x0258 },
    // { 0x02ca,      sub_10771,                         0x02cf, 0x0010, loc_10798,                         0x0258 },
    // { 0x032e,      sub_107EE,                         0x0330, 0x0010, empty,                             0x00c8 },
    // { 0x033c,      sub_109A5,                         0x0344, 0x0010, empty,                             0x0085 },
    // {      0,      sub_106D3,                         0x034e, 0x0010, sub_10704,                         0x2648 },
    // { 0x04be,      sub_1077C,                         0x04c0, 0x0010, empty,                             0x0085 },
    // {      0,      sub_10788,                         0x04d0,     -1, sub_1078D,                         0x0258 },
    // { 0x050c,      sub_107A3,                         0x0510, 0x0010, loc_10798,                         0x0258 },
    // { 0x054c,      sub_107C6,                         0x0550, 0x0010, loc_10798,                         0x0258 },
    // { 0x0588,      sub_10868,                         0x058f, 0x0010, sub_1087B,                         0x0320 },
    // { 0x05ac,      sub_10886,                         0x05ae, 0x0030, loc_10798,                         0x0258 },
    // { 0x05ee,      sub_109AD,                         0x05f0, 0x0030, loc_10798,                         0x0258 },
    // { 0x0614,      empty,                                  0,     -1, sub_108B6,                         0x0032 },
    // { 0x061e,      sub_10ACD,                         0x061e, 0x0010, empty,                             0x07d0 },
    // { 0x064c,      sub_106D8,                              0, 0x0010, sub_106FC,                         0x0c80 },
    // { 0x06c6,      sub_10740,                         0x06c8, 0x003a, empty,                             0x0190 },
    // {      0,      sub_1075A,                         0x06ee, 0x0010, empty,                             0x0190 },
    // {      0,      sub_10752,                         0x06fe, 0x0010, empty,                             0x0190 },
    // {      0,      sub_1073C,                         0x070e, 0x0010, empty,                             0x0190 },
    // {      0,      sub_10756,                         0x072e, 0x0010, empty,                             0x0190 },
    // {      0,      sub_1075E,                         0x073e, 0x0010, empty,                             0x0190 },
    // {      0,      sub_10737,                         0x074e, 0x0010, empty,                             0x0190 },
    // {      0,      sub_10747,                         0x076e, 0x0010, empty,                             0x0190 },
    // {      0,      sub_107E0,                         0x077e, 0x0010, empty,                             0x04b0 },
    // { 0x07ac,      sub_106EA,                         0x07af, 0x0010, sub_10704,                         0x03e8 },
    // { 0x07ce,      sub_1074B,                         0x07ce, 0x0010, empty,                             0x0190 },
    // {      0,      sub_10711,                         0x07dc, 0x003a, sub_1071D,                              1 },
    // {      0,      sub_1076A,                         0x081c, 0x003a, empty,                             0x0190 },
    // {      0,      sub_10762,                         0x083e, 0x0010, empty,                             0x0190 },
    // {      0,      sub_10766,                         0x084e, 0x0010, empty,                             0x00c8 },
    // {      0,      clear_screen,                      0x085e, 0x003a, empty,                                  1 },
    // {      0,      sub_1076A,                         0x085e, 0x0036, empty,                             0x0190 },
    // {      0,      clear_screen,                      0x08e0, 0x0038, empty,                                  1 },
	{-1}};

void cs_0580_play_intro()
{
restart_intro:
	cs_0945_intro_script_set_current_scene(cs_0337_intro_script);

	for (;;) {
		FUNC_NAME;

		vga_0c06_set_y_offset(24);
		if (ds_4854_intro_scene_current_scene->a == -1) {
			// goto restart_intro;
			return;
		}

		stepfn load_fn = ds_4854_intro_scene_current_scene->b;
		load_fn();

		int32_t transition_type = ds_4854_intro_scene_current_scene->d;
		if (transition_type) {
			TODO;
			// cs_c108_transition(transition_type);
			// cs_c0f4();
			// cs_3a7c();
		}

		stepfn run_fn = ds_4854_intro_scene_current_scene->e;
		run_fn();

		++ds_4854_intro_scene_current_scene;
	}
}

void cs_0945_intro_script_set_current_scene(Scene *scene)
{
	ds_4854_intro_scene_current_scene = scene;
}

void cs_061c_load_virgin_hnm()
{
	cs_ad57_play_music_morning();

	// Tail-call
	cs_ca1b_hnm_load(0x15);
}

void cs_0625_play_virgin_hnm()
{
	FUNC_NAME;

	// cs_c07c_set_frontbuffer_as_active_framebuffer();
	do {
		do {
			if (cs_dd63_has_user_input()) {
				return;
			}
		} while (!cs_c9f4_hnm_do_frame_and_check_if_frame_advanced());
		// cs_c4cd_gfx_copy_framebuf_to_screen();
		TODO; // Handle MIDI
	} while (!cs_cc85_hnm_is_complete());
}

void cs_064d_load_cryo_hnm()
{
	FUNC_NAME;
}

void cs_0658_load_cryo2_hnm()
{
	FUNC_NAME;
}

void cs_0661_play_cryo_hnm()
{
	FUNC_NAME;
}

void cs_0f66_nullsub() {}

void cs_aa0f_decode_sound_block()
{
}

void cs_ad57_play_music_morning()
{
	TODO;
}

void cs_b8a7_setup_draw_globe()
{
	// cs_f0b9_res_read_by_index_into_buffer(0x92, ds_4c60); // GLOBDATA.HSQ
	// cs_1ba7_set_globe_tilt_and_rotation(ds_197c, ds_197e);
}

void cs_e594_initialize_system()
{
	FUNC_NAME;

	cs_e675_dat_open();

	vga_09d9_get_screen_buffer(&ds_dbd8_framebuffer_screen, &ds_ce74_framebuffer_size);
	cs_c08e_set_screen_as_active_framebuffer();
	ds_dbdc_framebuffer_2 = (byte *)malloc(ds_ce74_framebuffer_size);
	ds_dbd6_framebuffer_1 = (byte *)malloc(ds_ce74_framebuffer_size);

	printf("ds_dbdc_framebuffer_2: %p - %p\n", ds_dbdc_framebuffer_2, ds_dbdc_framebuffer_2 + ds_ce74_framebuffer_size);
	printf("ds_dbd6_framebuffer_1: %p - %p\n", ds_dbd6_framebuffer_1, ds_dbd6_framebuffer_1 + ds_ce74_framebuffer_size);

	vga_0967_set_graphics_mode();

	// TODO: Parse command line arguments
	ds_ceeb_language_setting = 0;

	// UNIMPLEMENTED: Initialize joystick
	// TODO: Initialize mouse

	cs_ea7b_init_extended_memory_allocator();

	// UNIMPLEMENTED: Set monochrome mode

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

void cs_c08e_set_screen_as_active_framebuffer()
{
	ds_dbda_framebuffer_active = ds_dbd8_framebuffer_screen;
}

void cs_c0ad_gfx_clear_active_framebuffer()
{
	FUNC_NAME;
	memset(ds_dbda_framebuffer_active, 0, 64000);
}

void cs_c13e_open_resource_by_index(int16_t index)
{
	FUNC_NAME;
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

		uint16_t res_head = res_ptr.peekle16();
		if (res_head >= 2) {
			cs_c1aa_apply_bank_palette(res_ptr.begin());
		}

		// uint16_t cx = ds_31ff_resources[index]->alloc_size - res_head;
		// uint16_t ax = ds_278e_active_bank_id;

		ds_d844_resource_ptr[index] = ptr_offset_t(buf);
	} else {
		if (res_ptr.peekle16() >= 2) {
			cs_c1aa_apply_bank_palette(res_ptr.begin());
		}
	}
}

void cs_c180_transition()
{
	FUNC_NAME;
	TODO;
}

void cs_c1aa_apply_bank_palette(const byte *p)
{
	if (ds_dbb4_last_bank_palette == ds_278e_active_bank_id) {
		return;
	}
	cs_c1aa_apply_bank_palette(p);
}

void cs_c1ba_apply_palette(const byte *p)
{
	FUNC_NAME;

	for (;;) {
		uint16_t v = readle16(p);

		p += 2;
		while (v == 256) {
			p += 3;
			v = readle16(p);
			p += 2;
		}
		if (v == 0xffff) {
			return;
		}
		uint16_t count = hi(v);
		uint16_t offset = lo(v);

		if (count == 0) {
			count = 256;
		}

		offset *= 3;
		count *= 3;

		const byte *pal = p;
		p += count;
		vga_09e2_set_palette_unapplied(pal, offset, count);
	}
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
	printf("ds_dbfe_hnm_resource_flag: 0x%04x\n", ds_dbfe_hnm_resource_flag);

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
		bx = 0x10;
	}

	uint32_t first_frame_offset = c.readle32();

	ds_dbf6_hnm_unknown_offset_1 = ds_dc04_hnm_file_offset + first_frame_offset;
	ds_dbfa_hnm_unknown_offset_2 = ds_dc08_hnm_file_remain - first_frame_offset;

	return true;
}

bool cs_c9f4_hnm_do_frame_and_check_if_frame_advanced()
{
	uint16_t old_frame_counter = ds_dbe8_hnm_frame_counter;
	cs_ca60_hnm_do_frame();
	return old_frame_counter != ds_dbe8_hnm_frame_counter;
}

void cs_ca01_hnm_close_file()
{
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

	ds_dce6_in_transition = false;

	ptr_offset_t c = ds_dc10_hnm_read_buffer;
	uint16_t len = c.readle16();

	cs_ccf4_hnm_decode_frame(c, len, ds_dbdc_framebuffer_2);
	cs_aa0f_decode_sound_block();
	cs_cc96_decode_video_block();
	cs_ce1a_hnm_reset_buffers();
	ds_dbe8_hnm_frame_counter++;
	ds_dbea_hnm_frame_counter_2++;
	if (!(ds_dbfe_hnm_resource_flag & 0x40)) {
		for (int i = 50; i != 0; --i) {
			cs_cb1a();
		}
		ds_dc22_hnm_load_time = ds_ce7a_pit_timer_counter;
	}
}

void cs_ca60_hnm_do_frame()
{
restart:
	TODO;

	if (ds_35a6_hnm_fd) {
		if (ds_dbfe_hnm_resource_flag & 0x80) {
			TODO;
			cs_ca8f(); // refill buffer?
		}
		if (cs_caa0()) {
			TODO;
			cs_cb1a();
			goto restart;
		}
		if (cs_cad4_process_sound_block()) {
			TODO;
			if (ds_dc1e_hnm_pl_block_ofs != 0xffff) {
				cs_ce3b_hnm_handle_pal_chunk();
			}
			cs_cc96_decode_video_block();
			TODO;
			// cs_cc4e_();
		}
	}
	ds_dbb5_hnm_flag = 0;
}

void cs_ca8f()
{
	TODO;
}

void cs_ca9a_hnm_clear_flag()
{
	ds_dbb5_hnm_flag = 0;
}

bool cs_caa0()
{
	bool r = false;
	if (ds_dc14_hnm_decode_buffer.offset() == 0) {
		uint16_t read_offset = ds_dc1a_hnm_read_offset;
		r = true;
		if (read_offset) {
			ptr_offset_t c = ds_dc10_hnm_read_buffer;
			uint16_t len = c.readle16();
			uint16_t tag = (ds_dc10_hnm_read_buffer + 2).readle16();

			hexdump(ds_dc10_hnm_read_buffer.ptr(), 64);

			if ((tag == 0x6d6d) || (r = read_offset < ds_dc10_hnm_read_buffer.offset(), !r)) {
				byte *dst = ds_dbd6_framebuffer_1;
				if (ds_dbfe_hnm_resource_flag & 0x40) {
					TODO;
					// dst = ds_dc32;
				}
				cs_ccf4_hnm_decode_frame(c, len, dst);
				r = false;
			}
		}
	}
	return r;
}

bool cs_cad4_process_sound_block()
{
	// if (ds_dc1c_hnm_sd_block_ofs == 0xffff) {
	// 	uint16_t time_elapsed = ds_ce7a_pit_timer_counter - ds_dc22_hnm_load_time;
	// 	cs_ca59();
	// } else {
	// 	ds_3824_
	// }
	return true;
}

void cs_cb1a(/*ptr_offset_t &c*/)
{
	for (;;) {
		uint16_t block_size = ds_dc20_hnm_block_size;

		// printf("\t\tblock_size: 0x%04x\n", block_size);

		if (block_size != 0) {
			if ((ds_dbfe_hnm_resource_flag & 0x80) == 0) {
				uint16_t ceiled_file_offset_remainder = (-ds_dc04_hnm_file_offset & 0x7ff) + 0x800;
				block_size = std::min(block_size, ceiled_file_offset_remainder);
				// printf("\t\tReduced blocks_size to 0x%04x\n", block_size);
			}
			bool b = cs_cc2b(block_size);
			// printf("\t\tcs_cc2b(0x%04x) returned %d\n", block_size, b);
			if (!b) {
				return;
			}
			ds_dc20_hnm_block_size -= block_size;
			// printf("\t\tUpdating ds_dc20_hnm_block_size to %04x\n", ds_dc20_hnm_block_size);
			cs_cdbf_hnm_read_bytes(block_size);
			return;
		}

		// Block size > 0

		// cs:cb00
		// printf("\t\t0x%04x <=> 0x%04x\n", ds_dbea_hnm_frame_counter_2, ds_dbee_hnm_frame_counter_4);
		// printf("\t\t%08x\n", ds_dc08_hnm_file_remain);
		while (ds_dbea_hnm_frame_counter_2 == ds_dbee_hnm_frame_counter_4 || ds_dc08_hnm_file_remain == 0) {
			TODO;
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
				cs_c13e_open_resource_by_index(ds_dc00_hnm_id + 0x61);
				TODO;
				// do {
					// ds_dc0c_hnm_read_buffer
				// }

				// ptr_offset_t dst = ds_dc0c_hnm_read_buffer;
				// dst.writele16(0x6d6d); // 'mm'
			}
		}

		uint16_t size;
		ptr_offset_t buf;
		if (!cs_cd8f_hnm_read_header_size(&size, &buf)) {
			return;
		}
		// printf("\t\t\tofs: 0x%04x\n", c.offset());
		// printf("\t\t\tsize: 0x%04x\n", size);
		cs_cc0c(buf, size);

		// printf("\t\t0x%04x <=> 0x%04x\n", ds_dbea_hnm_frame_counter_2, ds_dbee_hnm_frame_counter_4);
		// printf("\t\t%08x\n", ds_dc08_hnm_file_remain);
	}
}

void cs_cc0c(ptr_offset_t &dst, uint16_t block_size)
{
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
	uint16_t uVar1;
	uint16_t uVar2;
	bool bVar3 = false;

	uVar2 = ds_dc0c_hnm_read_buffer.offset();
	uVar1 = ds_dc10_hnm_read_buffer.offset();

	printf("\t\t[ds:dc0c] = 0x%04x\n", ds_dc0c_hnm_read_buffer.offset());
	printf("\t\t[ds:dc10] = 0x%04x\n", ds_dc10_hnm_read_buffer.offset());
	printf("\t\t[ds:dc1a] = 0x%04x\n", ds_dc1a_hnm_read_offset);
	printf("\t\t[ds:dc18] = 0x%04x\n", ds_dc18_hnm_framebuffer_size);

	// printf("uVar1: 0x%04x\n", uVar1);
	// printf("uVar2: 0x%04x\n", uVar2);
	// printf("uVar1 <= uVar2: %d\n", uVar1 <= uVar2);
	// printf("uVar1 < uVar2 + param_1 + 0x12: %d\n", uVar1 < uVar2 + param_1 + 0x12);
	if ((uVar1 <= uVar2) || (bVar3 = uVar1 < uVar2 + param_1 + 0x12, !bVar3)) {
		uVar2 = ds_dc1a_hnm_read_offset + 10;
		bVar3 = (uVar2 + param_1 > UINT16_MAX);
		// printf("uVar2 + param_1 > UINT16_MAX: %d\n", uVar2 + param_1 > UINT16_MAX);
		if (!bVar3) {
			bVar3 = ds_dc18_hnm_framebuffer_size < uVar2 + param_1;
			// printf("ds_dc18_hnm_framebuffer_size < uVar2 + param_1: %d\n", ds_dc18_hnm_framebuffer_size < uVar2 + param_1);
		}
	}
	return !bVar3;

	/*
	uint16_t var2 = ds_dc0c_hnm_read_buffer.peekle16();
	uint16_t var1 = ds_dc10_hnm_read_buffer.peekle16();

	if (var2 < var1) {
		var2 += block_size + 10;
		if (var1 < var2) {
			return false;
		}
	}

	if (ds_dc1a_hnm_read_offset + block_size + 10 > UINT16_MAX) {
		return false;
	}

	if (ds_dc18_hnm_framebuffer_size >= var2 + block_size) {
		return false;
	}

	return true;
	*/
}

bool cs_cc85_hnm_is_complete()
{
	return ds_dbe7_hnm_finished_flag == 0 || ds_dbe7_hnm_finished_flag == 1;
}

void cs_cc96_decode_video_block()
{
	if (!ds_dc14_hnm_decode_buffer.begin()) {
		return;
	}

	ptr_offset_t src = ds_dc14_hnm_decode_buffer;
	ds_dc14_hnm_decode_buffer = ptr_offset_t();

	if ((ds_dbfe_hnm_resource_flag & 0x30) == 0) {
		printf("ds_dc24_hnm_decode_frame_flags: 0x%0x\n", ds_dc24_hnm_decode_frame_flags);
		if (ds_dc24_hnm_decode_frame_flags & 0x400) {
			TODO;
			// return;
		}
		byte *dst = ds_dbda_framebuffer_active;

		uint16_t di = (src.readle16() & 0xf9ff);
		uint16_t cx = src.readle16();
		uint16_t dx = src.readle16();
		uint16_t bx = src.readle16();

		printf("\tdi = 0x%04x\n", di);
		printf("\tcx = 0x%04x\n", cx);
		printf("\tdx = 0x%04x\n", dx);
		printf("\tbx = 0x%04x\n", bx);

		if (ds_dc00_hnm_id != 25) {
			static int frame_no = 1;

			char filename[256];
			sprintf(filename, "frame-%03d.ppm", frame_no++);
			FILE *f = fopen(filename, "wb");
			fprintf(f, "P6 320 200 255\n");
			for (int y = 0; y != 200; ++y) {
				for (int x = 0; x != 320; ++x) {
					byte c = dst[320 * y + x];
					RGB rgb = vga_05bf_palette_unapplied[c];
					byte r = (rgb.r * 255) / 63;
					byte g = (rgb.g * 255) / 63;
					byte b = (rgb.b * 255) / 63;
					fwrite(&r, 1, 1, f);
					fwrite(&g, 1, 1, f);
					fwrite(&b, 1, 1, f);
				}
			}
			fclose(f);
			exit(1);
		}

		return;
	}
	if (ds_dbfe_hnm_resource_flag & 0x20) {
		TODO;
		exit(1);
	}
}

#define HNM_TAG_SD 0x6473
#define HNM_TAG_PL 0x6c70
#define HNM_TAG_MM 0x6d6d

void cs_ccf4_hnm_decode_frame(ptr_offset_t src, uint16_t len, byte *dst_ptr)
{
	ds_dc1c_hnm_sd_block_ofs = -1;
	ds_dc1e_hnm_pl_block_ofs = -1;

	if (src.offset() + len > UINT16_MAX) {
		TODO;
		exit(1);
	}

	uint16_t tag;
	uint16_t block_len;

	for (;;) {
		tag = src.readle16();

		if (tag == HNM_TAG_SD) {
			TODO;
			ds_dc1c_hnm_sd_block_ofs = src.offset();
			block_len = src.readle16();
			printf("TAG: %c%c, LEN: %04x\n", tag >> 8, tag & 0x0f, block_len);
			src += block_len;
		} else if (tag == HNM_TAG_PL) {
			TODO;
			ds_dc1e_hnm_pl_block_ofs = src.offset();
			block_len = src.readle16();
			printf("TAG: %c%c, LEN: %04x\n", tag >> 8, tag & 0x0f, block_len);
			src += block_len;
		} else {
			break;
		}
	}

	// "mm"
	if (tag == 0x6d6d) {
		TODO;
		exit(1);
	}

	// Frame
	if (tag & 0x400) {
		dst_ptr = ds_dbda_framebuffer_active;
	}
	ds_dc14_hnm_decode_buffer = dst_ptr;
	ds_dc24_hnm_decode_frame_flags = tag;

	ptr_offset_t dst = ds_dc14_hnm_decode_buffer;
	uint16_t height_and_mode = src.readle16();

	if (!(tag & 0x400)) {
		TODO;
		dst.writele16(tag);
		dst.writele16(height_and_mode);
		if (tag == 0) {
			TODO;
			return;
		}
		if (!(tag & 0x200)) {
			TODO;
		}
	}
	cs_f403_unpack_hsq_skip_header(src, dst);
}

bool cs_cd8f_hnm_read_header_size(uint16_t *size, ptr_offset_t *buf)
{
	// FUNC_NAME;
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
	ds_35a6_hnm_fd = ds_dbba_dat_fd;

	if (ds_35a6_hnm_fd < 1) {
		return false;
	}

	ssize_t offset = lseek(ds_35a6_hnm_fd, 0, SEEK_CUR);

	int c = read(ds_35a6_hnm_fd, ds_dc0c_hnm_read_buffer.ptr(), count);
	assert(c == count);

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
	FUNC_NAME;

	ds_dbe8_hnm_frame_counter = 0;

	// Tail-call by fall-through
	cs_ce07_hnm_reset_other_frame_counters();
}

void cs_ce07_hnm_reset_other_frame_counters()
{
	FUNC_NAME;

	ds_dbea_hnm_frame_counter_2 =  0;
	ds_dbec_hnm_frame_counter_3 = -1;
	ds_dbee_hnm_frame_counter_4 = -1;
}

void cs_ce1a_hnm_reset_buffers()
{
	ds_dc0c_hnm_read_buffer = ds_dbdc_framebuffer_2;
	ds_dc10_hnm_read_buffer = ds_dbdc_framebuffer_2;

	ds_dc1a_hnm_read_offset = 0;
	ds_dc20_hnm_block_size = 0;

	ds_dc14_hnm_decode_buffer = nullptr;
	ds_dc18_hnm_framebuffer_size = ds_ce74_framebuffer_size;
}

void cs_ce3b_hnm_handle_pal_chunk()
{
	TODO;
}

void cs_ce6c_hnm_initialize()
{
	FUNC_NAME;

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
	FUNC_NAME;

	Resource *r = cs_c921_hnm_get_resource_by_id(id);
	if (!cs_c92b_hnm_open_and_load_palette(id)) {
		return;
	}

	TODO; // Read rect?
	// if ((r->alloc_size & 8)) {
	// 	printf("\t\t\t0x%04x - %s\n", r->alloc_size, r->name);
	// }
}

bool cs_dd63_has_user_input()
{
	TODO;
	return false;
}

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

	byte *p = (byte *)malloc(0x4000);

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
	// UNIMPLEMENTED: Lots of crap
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

	printf("%s: %04x %s\n", __func__, res->alloc_size, res->name);
	if (res->alloc_size == 0) {
		cs_f0d6_res_read(buffer, res->alloc_size);
	} else {
		buffer = cs_f11c_alloc_check(res->alloc_size);
		uint16_t size = cs_f0d6_res_read(buffer, res->alloc_size);
		cs_f0ff_alloc_take(size);
	}

	return buffer;
}

uint16_t cs_f0d6_res_read(byte *buffer, uint16_t size)
{
	printf("%s: %2x %2x\n", __func__, ds_ce78_dat_resource, ds_ce70_res_index_needs_extended_memory);
	if (ds_ce78_dat_resource < ds_ce70_res_index_needs_extended_memory) {
		assert(0 && "unimplemented");
	}
	Resource *res = ds_31ff_resources[ds_ce78_dat_resource];
	cs_f244_res_read_to_buffer(res->name, buffer, size);

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

byte *cs_f11c_alloc_check(uint16_t bytes)
{
	if (bytes > ds_ce68_alloc_last_addr - ds_39b7_alloc_next_addr) {
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
	printf("cs_f1fb_res_open: \e[7m%s\e[m\n", filename);
	if (cs_f2a7_dat_seek_to_name(filename, fd, size, offset)) {
		if (dump_resources) {
			byte *buffer = (byte*)malloc(*size);
			ssize_t r = read(*fd, buffer, *size);
			assert(r == *size);
			lseek(*fd, *offset, SEEK_SET);

			// printf("opening %s\n", filename);
			FILE *f = fopen(filename, "wb");
			assert(f);
			fwrite(buffer, *size, 1, f);
			fclose(f);
		}
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

uint16_t cs_f244_res_read_to_buffer(const char *filename, byte *buffer, uint16_t size)
{
	// TODO; // Check if resource fd == dat fd

	int fd;
	uint32_t _size;
	uint32_t offset;

	cs_f229_res_open_or_die(filename, &fd, &_size, &offset);
	// cs_f2a7_dat_seek_to_name(filename, &fd, &_size, &offset);
	// TODO;
	return cs_f2ea_dat_read(size, buffer);
}

bool cs_f2a7_dat_seek_to_name(const char *filename, int *fd, uint32_t *size, uint32_t *offset)
{
	FUNC_NAME;

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

	printf("cs_f2a7_dat_seek_to_name: fd: %d, size: 0x%4x, offset: 0x%x\n", *fd, *size, *offset);

	cs_f2d6_dat_seek(fd, *offset);

	return true;
}

void cs_f2d6_dat_seek(int *fd, uint32_t offset)
{
	*fd = ds_dbba_dat_fd;
	printf("cs_f2d6_dat_seek: seeking fd %d to 0x%x\n", ds_dbba_dat_fd, offset);
	lseek(ds_dbba_dat_fd, offset, SEEK_SET);
}

uint16_t cs_f2ea_dat_read(int bytes, byte *p)
{
	ssize_t r = read(ds_dbba_dat_fd, p, bytes);
	assert(r < 0x10000);

	// hexdump(p, bytes);

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

	auto c = make_ptr_offset((byte*)filename);
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
	// printf("CHECKSUM: 0x%02x\n", checksum);
	if (checksum == 0xab) {
		ptr_offset_t c = make_ptr_offset(buffer);
		uint16_t unpacked_size = c.readle16();

		if (c.readbyte() == 0) {
			cs_f40d_unpack_hsq(c.ptr(), unpacked_size);
		}
		return unpacked_size;
	}
	return 1;
}

void cs_f403_unpack_hsq_skip_header(ptr_offset_t &src, ptr_offset_t &dst)
{
	src += 6;
	cs_f435_unpack_no_header(src, dst);
}

void cs_f40d_unpack_hsq(byte *buffer, uint16_t unpacked_size)
{
	TODO;
	exit(1);
	// ptr_offset_t c = make_ptr_offset(buffer);
	// uint16_t packed_size = c.readle16();
	// assert(unpacked_size >= packed_size);
	// memcpy(buffer + unpacked_size + 0x40, buffer + 3, packed_size - 3);

	// ptr_offset_t src = make_ptr_offset(buffer + unpacked_size + 0x40);
	// ptr_offset_t dst = make_ptr_offset(buffer);
}

static inline byte getbit(uint16_t &queue, ptr_offset_t &c)
{
	byte bit;

	bit = queue & 1;
	queue >>= 1;

	if (queue == 0) {
		int ofs = c.offset();
		queue = c.readle16();
		// printf("queue: %04x @ 0x%x\n", queue, ofs);
		bit = queue & 1;
		queue = 0x8000 | (queue >> 1);
	}

	return bit;
}

void cs_f435_unpack_no_header(ptr_offset_t &src, ptr_offset_t &dst)
{
	uint16_t queue = 0;

	for (;;) {
		if (getbit(queue, src)) {
			// assert(dst.offset() < unpacked_size);
			int ofs = src.offset();
			byte b = src.readbyte();
			dst.writebyte(b);
		} else {
			int count;
			int offset;

			if (getbit(queue, src)) {
				uint16_t word = src.readle16();

				count = word & 0x07;
				offset = (word >> 3) - 8192;
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
				offset = int(b) - 256;
			}

			assert(dst.offset() + offset >= 0);
			for (int i = 0; i != count + 2; ++i) {
				dst.writebyte(dst.ptr()[offset]);
			}
		}
	}
}

void vga_0967_set_graphics_mode() {}

void vga_09d9_get_screen_buffer(byte **buffer, uint16_t *size)
{
	*size = 320 * 200;
	*buffer = (byte*)malloc(*size);
	// const auto &screenBuffer = app.screenBuffer();
	// *buffer = screenBuffer.pixels;
	// *size = screenBuffer.size.w * screenBuffer.size.h;
}

void vga_09e2_set_palette_unapplied(const byte *src, int byte_offset, int byte_count)
{
	byte *dst = (byte *)vga_05bf_palette_unapplied + byte_offset;

	if (memcmp(dst, src, byte_count) == 0) {
		return;
	}

	vga_01be_pal_is_dirty = true;

	memcpy(dst, src, byte_count);

	uint16_t offset = byte_offset;
	uint16_t count = byte_count;
	vga_0a21_pal_byte_offset_and_byte_count_to_index(&offset, &count);
	memset(vga_01bf_palette_dirty_entries + offset, count, 1);
}

void vga_0a21_pal_byte_offset_and_byte_count_to_index(uint16_t *offset, uint16_t *count)
{
	*offset /= 3;
	*count /= 3;
	*count = std::min<uint16_t>(*count, 256);
}

void vga_0b6b_set_palette_to_screen(RGB *pal, int start, int entries)
{
	// UNIMPLEMENTED: Wait for vsync
	if (!vga_01bd_is_monochrome) {
		// app.set_palette(pal, start, entries);
		return;
	}

	for (int i = 0; i != entries; ++i) {
		RGB c = pal[i];
		uint16_t gray = 5 * (c.r & 0x3f) + 9 * (c.g & 0x3f) + 2 * (c.b & 0x3f);
		gray /= 16;
		c = {(uint8_t)gray, (uint8_t)gray, (uint8_t)gray};
		// app.set_palette(&c, start + i, 1);
	}
}

void vga_0c06_set_y_offset(uint8_t y)
{
	vga_01a3_y_offset = y;
}

void vga_257a_wait_frame(std::atomic_uint16_t &timer, uint16_t start)
{
	if (!vga_01a1_no_vert_retrace) {
		// app.update_screen();
		return;
	}

	while (timer.load() - start < 3) {
		std::this_thread::yield();
	}
}

void vga_25e7_transition(std::atomic_uint16_t &timer, uint8_t type, uint8_t *src, uint8_t *dst)
{
	switch (type) {
	case 0x10:
		vga_2dc3_transition_effect_0x10(timer, src, dst);
		break;
	}
}

void vga_2dc3_transition_effect_0x10(std::atomic_uint16_t &timer, uint8_t *src, uint8_t *dst)
{
	uint16_t *vga_2fd7_transition_offsets;

	uint16_t start = timer.load();

	for (int i = 0; i != 16; ++i) {
		for (int y = 0; y != 200 / 4; ++y) {
			for (int x = 0; x != 320 / 4; ++x) {
				int offset = 320 * y + x + vga_2fd7_transition_offsets[i] + vga_01a3_y_offset;
				dst[offset] = 0;
			}
		}
		// app.update_screen();
	}

	vga_257a_wait_frame(timer, start);
}

// void vga_26e7_

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
