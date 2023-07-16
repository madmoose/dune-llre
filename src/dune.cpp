#include <algorithm>
#include <atomic>
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

App *g_app;

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

uint8_t vga_01a1_no_vert_retrace = 1;
uint8_t vga_01a3_y_offset = 0;
uint8_t vga_01bd_is_monochrome = 0;
uint8_t vga_01be_pal_is_dirty = 1;
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

byte vga_02bf_palette_unapplied[768] = { 0 };

byte vga_05bf_palette_unapplied[768] = {
	0x00, 0x00, 0x00, 0x00, 0x00, 0x2A, 0x00, 0x2A, 0x00, 0x00, 0x2A, 0x2A, 0x2A, 0x00, 0x00, 0x3F, 0x34, 0x14, 0x2A, 0x15, 0x00, 0x2A, 0x2A, 0x2A,
	0x15, 0x15, 0x15, 0x15, 0x15, 0x3F, 0x15, 0x3F, 0x15, 0x15, 0x3F, 0x3F, 0x3F, 0x15, 0x15, 0x3F, 0x15, 0x3F, 0x3F, 0x3F, 0x15, 0x3F, 0x3F, 0x3F,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3E, 0x39, 0x1B, 0x3F, 0x3F, 0x26, 0x3F, 0x3D, 0x15, 0x3F, 0x36, 0x1B,
	0x3F, 0x2D, 0x00, 0x36, 0x24, 0x0A, 0x36, 0x18, 0x0A, 0x2D, 0x10, 0x00, 0x24, 0x12, 0x00, 0x1B, 0x12, 0x0F, 0x1B, 0x0E, 0x08, 0x12, 0x00, 0x0F,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x30, 0x3F, 0x34, 0x28, 0x36, 0x2E, 0x21, 0x2D, 0x28, 0x1A, 0x24, 0x21, 0x13, 0x1B, 0x1A, 0x0D, 0x13, 0x13, 0x08, 0x04, 0x08, 0x10, 0x04, 0x18,
	0x08, 0x00, 0x10, 0x04, 0x00, 0x0C, 0x0A, 0x00, 0x02, 0x14, 0x1C, 0x1A, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x14, 0x18, 0x1C, 0x01, 0x01, 0x02,
	0x04, 0x04, 0x05, 0x07, 0x06, 0x09, 0x09, 0x09, 0x0C, 0x0C, 0x0B, 0x0F, 0x0F, 0x0E, 0x12, 0x11, 0x11, 0x16, 0x14, 0x13, 0x19, 0x17, 0x16, 0x1C,
	0x1E, 0x1A, 0x21, 0x27, 0x1D, 0x27, 0x2D, 0x20, 0x29, 0x33, 0x23, 0x28, 0x39, 0x28, 0x26, 0x3F, 0x33, 0x29, 0x3F, 0x3F, 0x29, 0x00, 0x00, 0x3F,
	0x02, 0x00, 0x3B, 0x05, 0x00, 0x37, 0x08, 0x00, 0x34, 0x0A, 0x00, 0x30, 0x0C, 0x00, 0x2C, 0x0E, 0x00, 0x29, 0x0E, 0x00, 0x25, 0x0F, 0x00, 0x21,
	0x0F, 0x00, 0x1E, 0x0F, 0x00, 0x1A, 0x0E, 0x00, 0x16, 0x0D, 0x00, 0x13, 0x0B, 0x00, 0x0F, 0x09, 0x00, 0x0B, 0x07, 0x00, 0x08, 0x3B, 0x30, 0x28,
	0x3B, 0x31, 0x28, 0x3B, 0x31, 0x29, 0x3C, 0x32, 0x2A, 0x3C, 0x33, 0x2B, 0x3C, 0x33, 0x2C, 0x3C, 0x34, 0x2C, 0x3D, 0x34, 0x2E, 0x3D, 0x35, 0x2E,
	0x3D, 0x36, 0x2F, 0x08, 0x10, 0x0A, 0x3F, 0x3F, 0x3F, 0x2E, 0x3D, 0x32, 0x22, 0x30, 0x28, 0x1F, 0x2A, 0x26, 0x26, 0x34, 0x2C, 0x08, 0x0A, 0x10,
	0x0B, 0x0B, 0x10, 0x0C, 0x0B, 0x10, 0x0D, 0x0B, 0x10, 0x0F, 0x0B, 0x10, 0x10, 0x0B, 0x10, 0x10, 0x0B, 0x0F, 0x10, 0x0B, 0x0D, 0x10, 0x0B, 0x0C,
	0x10, 0x0B, 0x0B, 0x32, 0x32, 0x32, 0x2F, 0x2A, 0x37, 0x1E, 0x16, 0x3E, 0x1C, 0x0B, 0x3F, 0x0F, 0x10, 0x0B, 0x3D, 0x34, 0x2E, 0x0C, 0x10, 0x0B,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3F, 0x3F, 0x3F
};

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

void dump_framebuffer_raw(const char *postfix, byte *fb) {
	char filename[256];
	sprintf(filename, "ppm/frame-%03d-%s.ppm", g_frame, postfix);
	FILE *f = fopen(filename, "wb");
	fwrite(fb, 3 * 320 * 200, 1, f);
	fclose(f);
}

void dump_framebuffer_ppm(const char *postfix, byte *fb, byte *pal) {
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
	sprintf(filename, "ppm/frame-%03d-%s.ppm", g_frame, postfix);

	FILE *f = fopen(filename, "wb");
	fprintf(f, "P6\n320 200 255\n");
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

void cs_0169_initialize_map()
{
	TODO;
}

Scene cs_0337_intro_script[] = {
	{      0, cs_061c_load_virgin_hnm,                   0, 0x003a, cs_0625_play_virgin_hnm,       1 },
	{      0, cs_c0ad_gfx_clear_active_framebuffer,      0, 0x003a, cs_0f66_nullsub,               1 },
	{      0, cs_064d_load_cryo_hnm,                     0, 0x0030, cs_0661_play_cryo_hnm,         1 },
	{      0, cs_0658_load_cryo2_hnm,               0x006f, 0x0030, cs_0661_play_cryo_hnm,         1 },
	{      0, cs_0f66_nullsub,                      0x00a8,      1, cs_0f66_nullsub,               1 },
	{      0, cs_0678_load_present_hnm,                  0, 0x003a, cs_0684_play_present_hnm,      1 },
	{      0, cs_cefc_load_irulan_hnm,                   0, 0x003a, cs_cf1b_play_irulan_hnm,       1 },
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

void cs_0579_clear_global_y_offset()
{
	vga_0c06_set_y_offset(0);
}

void cs_0580_play_intro()
{
restart_intro:
	// TODO: cs_de54 check for keyboard esc

	vga_0a68_copy_pal_1_to_pal_2();

	// TODO: cs_aeb7 do midi

	cs_0945_intro_script_set_current_scene(cs_0337_intro_script);

	for (;;) {
		vga_0c06_set_y_offset(24);
		if (ds_4854_intro_scene_current_scene->a == -1) {
			// goto restart_intro;
			return;
		}

		// TODO: cs_de0c_check_midi();
		// TODO: cs_0911(); BIG STUFF

		stepfn load_fn = ds_4854_intro_scene_current_scene->b;
		cs_c097_gfx_call_with_front_buffer_as_screen(load_fn);

		// TODO: ds_47d1 &= 0x7f;
		// TODO: cs_39e6();

		int32_t transition_type = ds_4854_intro_scene_current_scene->d;
		// TODO: cs_de0c_check_midi();
		if (transition_type) {
			// TODO;
			cs_c108_transition(transition_type, cs_0f66_nullsub);
			// cs_c0f4();
			// cs_3a7c();
		}

		cs_c07c_set_front_buffer_as_active_framebuffer();
		// TODO: ds_4701 |= 0x80;
		// TODO: cs_dd63_has_user_input()

		stepfn run_fn = ds_4854_intro_scene_current_scene->e;
		run_fn();

		// TODO: cs_ddf0()

		++ds_4854_intro_scene_current_scene;
	}

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
	cs_c07c_set_frontbuffer_as_active_framebuffer();
	do {
		do {
			if (cs_dd63_has_user_input()) {
				return;
			}
		} while (!cs_c9f4_hnm_do_frame_and_check_if_frame_advanced());

		cs_c4cd_gfx_copy_framebuf_to_screen();
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
	FUNC_NAME;

	cs_c07c_set_frontbuffer_as_active_framebuffer();
	do {
		do {
			if (cs_dd63_has_user_input()) {
				return;
			}
		} while (!cs_c9f4_hnm_do_frame_and_check_if_frame_advanced());

		cs_c4cd_gfx_copy_framebuf_to_screen();
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

void cs_0945_intro_script_set_current_scene(Scene *scene)
{
	ds_4854_intro_scene_current_scene = scene;
}

void cs_0f66_nullsub() {}

bool cs_a2ef_is_pcm_enabled()
{
	// return ds_dbc8 & 1;
	return false;
}

void cs_aa0f_decode_sound_block() {}

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
	ds_dbdc_framebuffer_2 = (byte *)calloc(1, ds_ce74_framebuffer_size);
	ds_dbd6_framebuffer_1 = (byte *)calloc(1, ds_ce74_framebuffer_size);

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

void cs_c07c_set_front_buffer_as_active_framebuffer() {
	ds_dbda_framebuffer_active = ds_dbd6_framebuffer_1;
}

// void cs_c085_set_back_buffer_as_active_framebuffer() {
// 	TODO;
// 	// ds_dbda_framebuffer_active = ds_dc32;
// }

void cs_c08e_set_screen_as_active_framebuffer()
{
	ds_dbda_framebuffer_active = ds_dbd8_framebuffer_screen;
}

void cs_c097_gfx_call_with_front_buffer_as_screen(void (*fn)(void)) {
	cs_c07c_set_front_buffer_as_active_framebuffer();
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

void cs_c07c_set_frontbuffer_as_active_framebuffer() {
	ds_dbda_framebuffer_active = ds_dbd6_framebuffer_1;
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

		ds_d844_resource_ptr[index] = ptr_offset_t(buf);
	} else {
		if (res_ptr.peekle16() >= 2) {
			cs_c1aa_apply_bank_palette(res_ptr.begin());
		}
	}
}

void cs_c108_transition(int transition_type, void (*fn)())
{
	ds_dce6_in_transition = 0x80;
	cs_c097_gfx_call_with_front_buffer_as_screen(fn);

	vga_25e7_transition(ds_ce7a_pit_timer_counter, transition_type, ds_dbdc_framebuffer_2, ds_dbd8_framebuffer_screen, ds_dbd6_framebuffer_1);

	cs_c4cd_gfx_copy_framebuf_to_screen();
	vga_0b0c();

	ds_dce6_in_transition = 0x00;
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
	for (;;) {
		uint16_t v;

		for (;;) {
			v = readle16(p);
			p += 2;

			if (v != 255) {
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

		const byte *pal = p;
		p += count;
		vga_09e2_set_palette_unapplied(pal, offset, count);
	}
}

void cs_c4cd_gfx_copy_framebuf_to_screen() {
	vga_1b7c_copy_framebuffer(ds_dbd8_framebuffer_screen, ds_dbd6_framebuffer_1);

	// dump_framebuffer_ppm("cs_c4cd_gfx_copy_framebuf_to_screen", ds_dbd8_framebuffer_screen, g_screen_palette);
	g_frame++;
}

Resource *cs_c921_hnm_get_resource_by_id(uint16_t id)
{
	return ds_31ff_resources[id + 210];
}

bool cs_c92b_hnm_open_and_load_palette(uint16_t id)
{
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
	ptr_offset_t r = ds_dc10_hnm_read_buffer;
	uint16_t ax = r.readle16();

	ds_dc1a_hnm_read_offset -= ax;

	uint16_t old_offset = r.offset();
	r += ax;
	if (r.offset() < old_offset || r.offset() > ds_ce74_framebuffer_size) {
		ds_dc10_hnm_read_buffer.set_offset(ax - 2);
		ax = 0;
	}

	ds_dc10_hnm_read_buffer += ax;

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
	int old_hnm_fd = 0;
	std::swap(ds_35a6_hnm_fd, old_hnm_fd);

	if (old_hnm_fd && old_hnm_fd != ds_dbba_dat_fd) {
		cs_ce01_hnm_reset_frame_counters();
		close(old_hnm_fd);
	}
}

void cs_ca1b_hnm_load(uint16_t id)
{
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
	}

	// Tail-call by fall-through
	cs_ca59();
}

void cs_ca59() {
	ds_dc22_hnm_load_time = ds_ce7a_pit_timer_counter;
}

void cs_ca60_hnm_do_frame()
{
restart:

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

	if (tag != HNM_TAG_MM && len > read_offset) {
		return false;
	}

	byte *dst = ds_dbd6_framebuffer_1;
	if (ds_dbfe_hnm_resource_flag & 0x40) {
		dst = ds_dbda_framebuffer_active;
	}
	cs_ccf4_hnm_decode_frame(c, len, dst);

	return true;
}

bool cs_cad4_wait_for_frame()
{
	if (ds_dc1c_hnm_sd_block_ofs == 0xffff) {
		uint16_t time_elapsed = ds_ce7a_pit_timer_counter - ds_dc22_hnm_load_time;
		if (time_elapsed < hi(ds_dbfe_hnm_resource_flag)) {
			return false;
		}
		cs_ca59();
		return true;
	} else {
		TODO;
		return true;
	}
}

void cs_cb1a(/*ptr_offset_t &c*/)
{
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
				cs_c13e_open_resource_by_index(ds_dc00_hnm_id + 0x61);
				TODO; exit(0);
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
	return !(ds_dbe7_hnm_finished_flag == 0 || ds_dbe7_hnm_finished_flag == 1);
}

void *cs_cc94_blt_func;

void cs_cc96_decode_video_block()
{
	// cc96
	// cs_cc94_blt_func = x_vtable_func_17_copy_fbuf_explode_and_center

	// cc9d
	ptr_offset_t src = ds_dc14_hnm_decode_buffer;
	ds_dc14_hnm_decode_buffer = ptr_offset_t();

	if (!src.begin()) {
		return;
	}

	// ccab
	uint16_t flag = ds_dbfe_hnm_resource_flag;
	if ((ds_dbfe_hnm_resource_flag & 0x30) == 0) {
		if (ds_dc24_hnm_decode_frame_flags & 0x400) {
			return;
		}

		// ccb9
		byte *dst = ds_dbda_framebuffer_active;

		uint16_t di = (src.readle16() & 0xf9ff);
		uint8_t flags = hi(di & 0xf800);
		uint16_t width = di & 0x01ff;

		uint16_t cx = src.readle16();
		uint16_t mode   = hi(cx);
		uint16_t height = lo(cx);

		if (height == 0) {
			return;
		}

		uint16_t dst_x = src.readle16();
		uint16_t dst_y = src.readle16();

		if (ds_dc00_hnm_id < 25) {
			vga_0f5b_blit(dst, dst_x, dst_y, src, width, height, flags, mode);
		} else {
			vga_1bca_copy_interlaced(dst, dst_x, dst_y, src, width, height);
		}
		return;
	}
	if ((ds_dbfe_hnm_resource_flag & 0x20) == 0) {
		printf("jmp cs:4AFD");
	} else {
		printf("jmp cs:4AEB");
	}
}

void cs_ccf4_hnm_decode_frame(ptr_offset_t src, uint16_t len, byte *dst_ptr)
{
	ds_dc1c_hnm_sd_block_ofs = -1;
	ds_dc1e_hnm_pl_block_ofs = -1;

	if (src.offset() + len > UINT16_MAX || src.offset() + len > 64000) {
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
	if (tag & 0x400) {
		dst_ptr = ds_dbda_framebuffer_active;
	}
	ds_dc14_hnm_decode_buffer = dst_ptr;
	ds_dc24_hnm_decode_frame_flags = tag;

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
		}
	}

	cs_f403_unpack_hsq_skip_header(src, dst);
}

bool cs_cd8f_hnm_read_header_size(uint16_t *size, ptr_offset_t *buf)
{
	if (!cs_cdbf_hnm_read_bytes(2)) {
		return false;
	}

	*buf = ds_dc0c_hnm_read_buffer;
	*size = (*buf - 2).readle16();
	return true;
}

bool cs_cda0_hnm_open_at_body()
{
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
	ds_dbe8_hnm_frame_counter = 0;

	// Tail-call by fall-through
	cs_ce07_hnm_reset_other_frame_counters();
}

void cs_ce07_hnm_reset_other_frame_counters()
{
	ds_dbea_hnm_frame_counter_2 = 0;
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
	ptr_offset_t pal_ptr(ds_dc0c_hnm_read_buffer.begin(), ds_dc1e_hnm_pl_block_ofs);
	cs_c1ba_apply_palette(pal_ptr.ptr());
	vga_0b0c();
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

void  cs_cefc_load_irulan_hnm()
{
	TODO; // Load subtitles
	vga_0c06_set_y_offset(0);
	cs_c0ad_gfx_clear_active_framebuffer();
	cs_ca1b_hnm_load(25);
}

void  cs_cf1b_play_irulan_hnm()
{
	byte *old_active_frame_buffer = ds_dbda_framebuffer_active;
	cs_c08e_set_screen_as_active_framebuffer();

	do {
		if (!cs_c9e8_hnm_do_frame_skippable()) {
			return;
		}
	} while (!cs_cc85_hnm_is_complete());

	ds_dbda_framebuffer_active = old_active_frame_buffer;
}

bool cs_dd63_has_user_input()
{
	// TODO;
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

	// printf("%s: %04x %s\n", __func__, res->alloc_size, res->name);
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
	// printf("%s: %2x %2x\n", __func__, ds_ce78_dat_resource, ds_ce70_res_index_needs_extended_memory);
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
	// printf("cs_f1fb_res_open: \e[7m%s\e[m\n", filename);
	if (cs_f2a7_dat_seek_to_name(filename, fd, size, offset)) {
		if (dump_resources) {
			byte *buffer = (byte *)calloc(1, *size);
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

			// printf("%d + %d = %d\n", dst.offset(), offset, dst.offset() + offset);
			// assert(dst.offset() + offset >= 0);
			for (int i = 0; i != count + 2; ++i) {
				dst.writebyte(dst.ptr()[offset]);
			}
		}
	}
}

void vga_0967_set_graphics_mode() {}

void vga_0975(bool monochrome)
{
	vga_0b0c();
}

void vga_09d9_get_screen_buffer(byte **buffer, uint16_t *size)
{
	*size = 320 * 200;
	*buffer = (byte *)calloc(1, *size);
	// const auto &screenBuffer = app.screenBuffer();
	// *buffer = screenBuffer.pixels;
	// *size = screenBuffer.size.w * screenBuffer.size.h;
}

void vga_09e2_set_palette_unapplied(const byte *src, int byte_offset, int byte_count)
{
	byte *dst = vga_05bf_palette_unapplied + byte_offset;

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

void vga_0a58_copy_pal_1_to_pal_2() {
	memcpy(vga_02bf_palette_unapplied, vga_05bf_palette_unapplied, 3 * 256);
}

void vga_0a68_copy_pal_1_to_pal_2()
{
	vga_0a58_copy_pal_1_to_pal_2();
}

void vga_0b0c()
{
	if (!vga_01be_pal_is_dirty) {
		return;
	}

	vga_01be_pal_is_dirty = false;

	for (int i = 0; i != 256; ++i) {
		if (vga_01bf_palette_dirty_entries[i]) {
			memcpy(&g_screen_palette[3*i], &vga_05bf_palette_unapplied[3*i], 3);
		}
	}
	g_app->update_palette(g_screen_palette);

	memset(vga_01bf_palette_dirty_entries, 0, 256);
}

void vga_0b68_set_palette_to_screen(byte *pal, int start, int entries)
{
	// g_app->wait_for_vsync();
	// if (!vga_01bd_is_monochrome) {
		memcpy(&g_screen_palette[3 * start], &pal[3 * start], 3 * entries);
		g_app->update_palette(g_screen_palette);
		return;
	// }

	// for (int i = 0; i != entries; ++i) {
	// 	RGB c = pal[i];
	// 	uint16_t gray = 5 * (c.r & 0x3f) + 9 * (c.g & 0x3f) + 2 * (c.b & 0x3f);
	// 	gray /= 16;
	// 	c = {(uint8_t)gray, (uint8_t)gray, (uint8_t)gray};
	// 	// app.set_palette(&c, start + i, 1);
	// }
}

void vga_0c06_set_y_offset(uint8_t y)
{
	vga_01a3_y_offset = y;
}

void vga_1bca_copy_interlaced(byte *dst, int dst_x, int dst_y, ptr_offset_t src, int width, int height)
{
	dst_y += vga_01a3_y_offset;

	for (int y = 0; y != height; ++y) {
		for (int x = 0; x < width; ++x) {
			dst[320 * (2*y + dst_y) + 2*(x + dst_x)] = src.ptr()[width * y + x];
		}
	}
}

void vga_1b7c_copy_framebuffer(byte *dst, byte *src) {
	memcpy(dst, src, 320 * 200);
	if (dst == ds_dbd8_framebuffer_screen) {
		g_app->update_screen(ds_dbd8_framebuffer_screen);
	}
}

void vga_0f5b_blit(byte *dst, int dst_x, int dst_y, ptr_offset_t src, int width, int height, uint8_t flags, uint8_t mode)
{
	bool flip_x = flags & 0x20;
	bool flip_y = flags & 0x40;

	if (mode < 254) {
		if ((flags & 0x80) == 0) {
			printf("\tdraw_4bpp\n");
			exit(0);
			// draw_4bpp(fb, flip_x, flip_y, r, src_y, src_y, dst_x, dst_y, w, h, src_pitch, mode);
		} else {
			printf("\tdraw_4bpp_rle\n");
			exit(0);
			// draw_4bpp_rle(fb, flip_x, flip_y, r, src_y, src_y, dst_x, dst_y, w, h, src_pitch, mode);
		}
	} else {
		if ((flags & 0x80) == 0) {
			draw_8bpp(dst, flip_x, flip_y, src, dst_x, dst_y, width, height, mode);
		} else {
			draw_8bpp_rle(dst, flip_x, flip_y, src, dst_x, dst_y, width, height, mode);
		}
	}
}

inline
void write_pixel(byte *dst, int x, int y, uint8_t v)
{
	dst[320 * y + x] = v;
}

#define ADVANCE(p) do { p += (!flip_x ? 1 : -1); } while (0)

void draw_8bpp(byte *dst, bool flip_x, bool flip_y, ptr_offset_t &src, int dst_x, int dst_y, int w, int h, uint8_t mode)
{
	for (int y = 0; y != h; ++y) {
		int x = !flip_x ? 0 : w - 1;

		// mode 255 means that 0 is transparent.
		if (mode == 255) {
			for (int i = 0; i != w; ++i) {
				byte value = src.readbyte();
				if (value) {
					write_pixel(dst, dst_x + x, dst_y + y, value);
				}
				ADVANCE(x);
			}
		} else {
			for (int i = 0; i != w; ++i) {
				byte value = src.readbyte();
				write_pixel(dst, dst_x + x, dst_y + y, value);
				ADVANCE(x);
			}
		}
	}
}

void draw_8bpp_rle(byte *dst, bool flip_x, bool flip_y, ptr_offset_t &src, int dst_x, int dst_y, int w, int h, uint8_t mode)
{
	for (int y = 0; y != h; ++y) {
		int line_remain = w;
		int x = !flip_x ? 0 : w - 1;

		do {
			byte cmd = src.readbyte();
			if (cmd & 0x80) {
				int count = 257 - cmd;
				byte value = src.readbyte();
				if (mode == 255 && value == 0) {
					x += flip_x ? -count : count;
				} else {
					for (int i = 0; i != count; ++i) {
						write_pixel(dst, dst_x + x, dst_y + y, value);
						ADVANCE(x);
					}
				}
				line_remain -= count;
			} else {
				int count = cmd + 1;
				for (int i = 0; i != count; ++i) {
					byte value = src.readbyte();
					if (!(mode == 255 && value == 0)) {
						write_pixel(dst, dst_x + x, dst_y + y, value);
					}
					ADVANCE(x);
				}
				line_remain -= count;
			}
		} while (line_remain > 0);
	}
}

#undef ADVANCE

void vga_2572_wait_frame(std::atomic_uint16_t &timer, uint16_t start)
{
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

	switch (type) {
	case 0x10:
		vga_2dc3_transition_effect_0x10(timer);
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
	vga_1b7c_copy_framebuffer(vga_2539_fb, vga_2537_fb);

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
	vga_2572_wait_frame(timer, start);
}

void vga_272e_transition_effect_0x3a(std::atomic_uint16_t &timer)
{
	for (int i = 1; i != 3*255; ++i) {
		std::swap(vga_02bf_palette_unapplied[i], vga_05bf_palette_unapplied[i]);
	}
	vga_26e3(timer,  85, 3, 22);
	vga_264d(timer, 255, 3, 22);
}

void vga_2dc3_transition_effect_0x10(std::atomic_uint16_t &timer)
{
	uint16_t *vga_2fd7_transition_offsets;

	uint16_t start = timer.load();

	for (int i = 0; i != 16; ++i) {
		for (int y = 0; y != 200 / 4; ++y) {
			for (int x = 0; x != 320 / 4; ++x) {
				int offset = 320 * y + x + vga_2fd7_transition_offsets[i] + vga_01a3_y_offset;
				// dst[offset] = 0;
			}
		}
		// app.update_screen();
	}

	vga_2572_wait_frame(timer, start);
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
