#include <cstdint>
#include <optional>

typedef uint8_t byte;

uint8_t hi(uint16_t v) {
	return (v >> 8);
}

uint8_t lo(uint16_t v) {
	return (v & 0xff);
}

typedef void (*stepfn)();

struct RGB {
	union {
		struct {
			uint8_t r;
			uint8_t g;
			uint8_t b;
		};
		uint8_t v[3];
	};
};

struct Scene {
	int32_t a;
	stepfn  b;
	int32_t c;
	int32_t d;
	stepfn  e;
	int32_t f;
};

void  cs_0000_start();
void  cs_00b0_initialize_resources();
void  cs_00d1_initialize_resources();
void  cs_0169_initialize_map();
void  cs_0580_play_intro();
void  cs_061c_load_virgin_hnm();
void  cs_0625_play_virgin_hnm();
void  cs_064d_load_cryo_hnm();
void  cs_0658_load_cryo2_hnm();
void  cs_0661_play_cryo_hnm();
void  cs_0945_intro_script_set_current_scene(Scene *scene);
void  cs_0f66_nullsub();
void  cs_3a7c();
bool  cs_a2ef_is_pcm_enabled();
void  cs_aa0f_decode_sound_block();
void  cs_ad57_play_music_morning();
void  cs_e594_initialize_system();
void  cs_c07c_set_front_buffer_as_active_framebuffer();
void  cs_c08e_set_screen_as_active_framebuffer();
void  cs_c097_gfx_call_with_front_buffer_as_screen(void (*fn)(void));
void  cs_c0ad_gfx_clear_active_framebuffer();
void  cs_c07c_set_frontbuffer_as_active_framebuffer();
void  cs_c0f4();
void  cs_c13e_open_resource_by_index(int16_t index);
void  cs_c108_transition(int transition_type, void (*fn)());
void  cs_c1aa_apply_bank_palette(const byte *p);
void  cs_c1ba_apply_palette(const byte *p);
void  cs_c4cd_gfx_copy_framebuf_to_screen();
bool  cs_c92b_hnm_open_and_load_palette(uint16_t id);
bool  cs_c93c_hnm_read_header();
bool  cs_c9f4_hnm_do_frame_and_check_if_frame_advanced();
void  cs_ca01_hnm_close_file();
void  cs_ca1b_hnm_load(uint16_t id);
void  cs_ca59();
void  cs_ca60_hnm_do_frame();
void  cs_ca8f();
void  cs_ca9a_hnm_clear_flag();
bool  cs_caa0();
bool  cs_cad4_wait_for_frame();
void  cs_cb1a(/*ptr_offset_t &dst*/);
void  cs_cc0c(ptr_offset_t &dst, uint16_t);
bool  cs_cc2b(uint16_t block_size);
uint16_t cs_cc4e();
bool  cs_cc85_hnm_is_complete();
void  cs_cc96_decode_video_block();
void  cs_ccf4_hnm_decode_frame(ptr_offset_t c, uint16_t len, byte *dst);
bool  cs_cd8f_hnm_read_header_size(uint16_t *size, ptr_offset_t *buf);
bool  cs_cda0_hnm_open_at_body();
bool  cs_cdbf_hnm_read_bytes(int count);
void  cs_cdf7_hnm_advance_read_ptr(uint16_t n);
void  cs_ce01_hnm_reset_frame_counters();
void  cs_ce07_hnm_reset_other_frame_counters();
void  cs_ce1a_hnm_reset_buffers();
void  cs_ce3b_hnm_handle_pal_chunk();
void  cs_ce6c_hnm_initialize();
void  cs_ceb0_hnm(uint16_t id);
bool  cs_dd63_has_user_input();
bool  cs_e675_dat_open();
byte *cs_e741_dat_read_toc();
void  cs_e75b_res_store_in_lookup_table(uint16_t res_id, byte dl, byte *, byte *);
void  cs_ea7b_init_extended_memory_allocator();
byte *cs_f0b9_res_read_by_index_into_buffer(uint16_t index, uint8_t *buffer = nullptr);
uint16_t cs_f0d6_res_read(byte* buffer, uint16_t size);
byte *cs_f0f6_alloc_get_addr();
void  cs_f0ff_alloc_take(uint16_t bytes);
byte *cs_f11c_alloc_check(uint16_t bytes);
void  cs_f131_out_of_standard_memory();
bool  cs_f1fb_res_open(const char *filename, int *fd, uint32_t *size, uint32_t *offset);
void  cs_f229_res_open_or_die(const char *filename, int *fd, uint32_t *size, uint32_t *offset);
uint16_t cs_f244_res_read_to_buffer(const char *filename, byte *buffer, uint16_t size);
bool  cs_f2a7_dat_seek_to_name(const char *filename, int *fd, uint32_t *size, uint32_t *offset);
void  cs_f2d6_dat_seek(int *fd, uint32_t offset);
uint16_t cs_f2ea_dat_read(int bytes, byte *p);
std::optional<uint16_t> cs_f314_dat_get_res_index_by_name(const char *filename, uint8_t &dl);
bool cs_f3a7_dat_locate_map_entry(uint16_t index, uint8_t dl, uint16_t &di);
uint16_t cs_f3d3_maybe_uncompress_hsq(byte *buffer);
void cs_f403_unpack_hsq_skip_header(ptr_offset_t &src, ptr_offset_t &dst);
void cs_f40d_unpack_hsq(byte *buffer, uint16_t unpacked_size);
void cs_f435_unpack_no_header(ptr_offset_t &src, ptr_offset_t &dst);

void vga_0967_set_graphics_mode();
void vga_0975(bool monochrome);
void vga_09d9_get_screen_buffer(byte **buffer, uint16_t *size);
void vga_09e2_set_palette_unapplied(const byte *src, int byte_start, int len);
void vga_0a21_pal_byte_offset_and_byte_count_to_index(uint16_t *offset, uint16_t *count);
void vga_0a58_copy_pal_1_to_pal_2();
void vga_0a68_copy_pal_1_to_pal_2();
void vga_0b0c();
void vga_0b68_set_palette_to_screen(byte *pal, int start, int entries);
void vga_0c06_set_y_offset(uint8_t y);
void vga_1b7c_copy_framebuffer(byte *dst, byte *src);
void vga_2572_wait_frame(std::atomic_uint16_t &timer, uint16_t start);
void vga_25e7_transition(std::atomic_uint16_t &timer, uint8_t type, uint8_t *si, uint8_t *es, uint8_t *ds);
void vga_261d_maybe_wait_frame(std::atomic_uint16_t &timer, uint16_t start);
void vga_2dc3_transition_effect_0x10(std::atomic_uint16_t &timer);
void vga_272e_transition_effect_0x3a(std::atomic_uint16_t &timer);
