#include <cstdint>
#include <optional>

typedef uint8_t byte;

uint8_t hi(uint16_t v)
{
	return (v >> 8);
}

uint8_t lo(uint16_t v)
{
	return (v & 0xff);
}

uint16_t msb(uint16_t v)
{
	return (v >> 7) & 1;
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
	int16_t a;
	stepfn b;
	int16_t c;
	int16_t d;
	stepfn e;
	int16_t f;
};

struct ui_element_flag_and_sprite_t {
	int16_t flags;
	int16_t id;
};

struct ui_element_t {
	int16_t x0;
	int16_t y0;
	int16_t x1;
	int16_t y1;
	int16_t flags;
	int16_t id;
	uint16_t unk6;
};

struct sprite_position {
	uint16_t flags_and_id;
	int16_t x;
	int16_t y;
};

struct pos_t {
	int16_t x;
	int16_t y;
};

struct rect_t {
	int16_t x0;
	int16_t y0;
	int16_t x1;
	int16_t y1;

	bool contains(int16_t x, int16_t y) { return x0 <= x && x < x1 && y0 <= y && y < y1; }
};

struct string_def_t {
	int16_t id;
	uint16_t x;
	uint16_t y;
	uint16_t color;
};

typedef void (*frame_task_fn_t)(void);

struct frame_task_t {
	uint16_t interval;
	uint16_t ticks_since_last;
	frame_task_fn_t fn;
};

struct cursor_t {
	int16_t hotspot_x;
	int16_t hotspot_y;
	uint16_t black_mask[16];
	uint16_t white_mask[16];
};

struct ds_4856_t {
	int8_t ds_4856_offset_0;
	int8_t ds_4857_offset_1;
	int16_t ds_4858_offset_2;
	int16_t ds_485a_offset_4;
	int8_t ds_485c_offset_6;
	uint8_t ds_485d_offset_7;
};

struct Particle {
	rect_t bounding_box;      // offset +0
    uint16_t type;            // offset +8
    uint16_t subtype;         // offset +10 (0xA)
    uint8_t flags;            // offset +12 (0xC)
    uint16_t data;            // offset +13 (0xD)
    // Total size: 17 bytes (0x11)
};

extern uint16_t vga_2fd7_transition_offsets[16];

void cs_0000_start();
void cs_0083();
void cs_0086();
void cs_00b0_initialize_resources();
void cs_00d1_initialize_resources();
void cs_0169_initialize_map();
void cs_02e3();
void cs_0309_play_credits(bool play);
void cs_0579_clear_global_y_offset();
void cs_0580_play_intro();
void cs_061c_load_virgin_hnm();
void cs_0625_play_virgin_hnm();
void cs_064d_load_cryo_hnm();
void cs_0658_load_cryo2_hnm();
void cs_0661_play_cryo_hnm();
void cs_0678_load_present_hnm();
void cs_0684_play_present_hnm();
void cs_069e_load_intro_hnm();
void cs_06aa_play_intro_hnm();
void cs_06bd_play_hnm_skippable();
void cs_06ce();
void cs_06d3();
void cs_06d8();
void cs_06ea();
void cs_06fc();
void cs_06f3(uint16_t ax);
void cs_0704();
void cs_070c();
void cs_0711();
void cs_071d();
void cs_0737();
void cs_073c();
void cs_0740();
void cs_074b();
void cs_0747();
void cs_0752();
void cs_0756();
void cs_075a();
void cs_075e();
void cs_0762();
void cs_0766();
void cs_076a();
void cs_0771();
void cs_077c();
void cs_0788();
void cs_078d();
void cs_0798();
void cs_07a3();
void cs_07c6();
void cs_07e0();
void cs_07ee();
void cs_07fd();
void cs_0802(uint8_t bl);
byte *cs_0820(uint8_t bl);
void cs_085d();
void cs_0868();
void cs_087b();
void cs_0886();
void cs_08b6();
void cs_08f0(uint16_t dx, uint16_t bx);
void cs_0911();
void cs_0945_intro_script_set_current_scene(Scene *scene);
void cs_0960();
void cs_0972();
void cs_0978(uint16_t dx);
void cs_097e(uint16_t dx);
void cs_0981(uint16_t dx, uint16_t bx);
void cs_098a();
void cs_0995();
void cs_099d(byte al);
void cs_09a5();
void cs_09ad();
void cs_09c7(byte al, uint16_t dx);
void cs_09ef_load_credits_hnm();
void cs_0a16();
void cs_0a23();
void cs_0a44(uint16_t cx);
void cs_0acd_intro_28_night_attack_start();
void cs_0b45();
void cs_0c3b(ds_4856_t *ds_4856);
void cs_0d0d();
void cs_0f66_nullsub();
void cs_1797_ui_head_draw();
void cs_17e6_ui_head_animate_up();
void cs_1860();
void cs_189a(int transition_type);
void cs_1a34_ui_draw_date_and_time_indicator();
void cs_1a9b_ui_draw_date_and_time(pos_t *&p, uint16_t sprite_id);
uint16_t cs_1ad1_game_time_get_day();
void cs_274e();
void cs_275e(byte al);
void cs_2db1();
void cs_2d74_open_sal_resource();
ptr_offset_t cs_3978(uint8_t al, uint8_t bl);
void cs_37b2();
void cs_37b5();
void cs_3916();
void cs_391d();
void cs_39ec_room_draw(byte al);
void cs_3a7c();
void cs_3b59_room_draw(byte al);
void cs_3be9_room_draw_polygon(ptr_offset_t &si);
void cs_3d83_room_read_position_markers(ptr_offset_t &r, int8_t *position_markers);
void cs_3e13_room_add_polygon_section(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t start_y,
                                      uint16_t *&xs, const char *s, uint16_t *xs0);
byte *cs_3efe(uint16_t dx);
void cs_4afd();
void cs_4b16();
void cs_4bb9();
void cs_4d00();
void cs_5adf();
void cs_5b99_copy_rect(rect_t *dst, rect_t *src);
void cs_5ba8_copy_game_area_rect_to_sprite_clip_rect();
uint16_t cs_5e4f(uint16_t si, uint16_t ax);
void cs_5f79();
void cs_79de();
void cs_7b36();
void cs_8770();
byte cs_9123(uint8_t al);
void cs_91a0_setup_lip_sync_data_from_sprite_sheet(uint16_t ax);
void cs_920f_open_talking_head_resource(uint16_t ax);
void cs_978e();
bool cs_a2ef_is_pcm_enabled();
void cs_a3f9_settings_ui_draw();
void cs_aa0f_decode_sound_block();
void cs_ad57_play_music_morning();
void cs_b827_draw_globe_and_ui_to_front_buffer();
void cs_b84a_draw_globe_with_blue_background();
void cs_b85a_draw_globe_with_atmosphere();
void cs_b8a7_setup_draw_globe();
void cs_b87e_draw_globe_frescos();
void cs_b8ea();
void cs_b8f3_globe_slide_decorations_open();
void cs_b977_draw_globe();
void cs_b9ae();
void cs_b9e0_globe_rotate(int delta_rotation);
void cs_b9f6_precalculate_globe_rotation_table();
void cs_ba75_set_globe_tilt_and_rotation(int16_t tilt, uint16_t rotation);
void cs_bdfa_ui_stats_draw_ingame_day_and_charisma();
void cs_be1d_fresk_draw_results_text_and_icons();
void cs_be57();
uint16_t cs_e3b7_rand(uint16_t mask);
uint16_t cs_e3cc_rand();
void cs_e594_initialize_system();
void cs_c07c_set_fb1_as_active_framebuffer();
void cs_c085_set_hnm_buffer_as_active_framebuffer();
void cs_c08e_set_screen_as_active_framebuffer();
void cs_c097_gfx_call_with_fb1_as_screen(void (*fn)(void));
void cs_c0ad_gfx_clear_active_framebuffer();
void cs_c0f4();
void cs_c102_copy_pal_and_transition(void (*fn)());
void cs_c108_transition(int transition_type, void (*fn)());
void cs_c137_load_icones();
void cs_c13b_open_onmap_resource();
void cs_c13e_open_sprite_bank_resource_by_index(int16_t index);
void cs_c108_transition(int transition_type, void (*fn)());
void cs_c1aa_apply_bank_palette(const byte *p);
void cs_c1ba_apply_palette(const byte *p);
ptr_offset_t cs_c1f4(uint16_t ax);
void cs_c21b_draw_sprite_list(sprite_position list[]);
void cs_c22f_draw_sprite(uint16_t flags_and_id, uint16_t x, uint16_t y);
void cs_c2a1_decompress_4bpp_rle_image(ptr_offset_t &src, int w, int h, byte *dst);
void cs_c2f2(byte resource_index);
void cs_c2fd_draw_sprite(uint16_t flags_and_id, uint16_t x, uint16_t y);
void cs_c305_draw_sprite_clipped(uint16_t flags_and_id, int16_t x, int16_t y);
void cs_c30d_draw_sprite_clipped(uint16_t flags_and_id, int16_t x, int16_t y);
void cs_c370_blit_repeated_x(byte *fb, rect_t *rect, byte sprite_id);
void cs_c412_copy_active_framebuffer_to_fb2();
void cs_c432_clear_game_area_rect();
void cs_c43e_copy_game_area_rect_fb2_to_fb1();
void cs_c446_copy_rect_fb2_to_fb1(rect_t *rect);
void cs_c474_copy_game_area_rect_fb1_to_fb2();
void cs_c477_copy_rect_fb1_to_fb2(rect_t *rect);
void cs_c4cd_copy_fb1_to_screen();
void cs_c60b(uint16_t ax);
bool cs_c92b_hnm_open_and_load_palette(uint16_t id);
bool cs_c93c_hnm_read_header();
bool cs_c9e8_hnm_do_frame_skippable();
bool cs_c9f4_hnm_do_frame_and_check_if_frame_advanced();
void cs_ca01_hnm_close_file();
void cs_ca1b_hnm_load(uint16_t id);
void cs_ca59_hnm_set_load_time();
void cs_ca60_hnm_do_frame();
void cs_ca8f();
void cs_ca9a_hnm_clear_flag();
bool cs_caa0();
bool cs_cad4_wait_for_frame();
void cs_cb1a(/*ptr_offset_t &dst*/);
void cs_cc0c(ptr_offset_t &dst, uint16_t);
bool cs_cc2b(uint16_t block_size);
uint16_t cs_cc4e();
bool cs_cc85_hnm_is_complete();
void cs_cc96_decode_video_block();
void cs_ccf4_hnm_demux_frame(ptr_offset_t c, uint16_t len, byte *dst);
bool cs_cd8f_hnm_read_header_size(uint16_t *size, ptr_offset_t *buf);
bool cs_cda0_hnm_open_at_body();
bool cs_cdbf_hnm_read_bytes(int count);
void cs_cdf7_hnm_advance_read_ptr(uint16_t n);
void cs_ce01_hnm_reset_frame_counters();
void cs_ce07_hnm_reset_other_frame_counters();
void cs_ce1a_hnm_reset_buffers();
void cs_ce3b_hnm_handle_pal_chunk();
void cs_ce6c_hnm_initialize();
void cs_ceb0_hnm(uint16_t id);
void cs_cefc_load_irulan_hnm();
void cs_cf1b_play_irulan_hnm();
void cs_cf4b_subtitle_draw_or_clear();
void cs_cfb9();
char *cs_cf70_get_phrase_or_command_string(uint16_t id);
void cs_d00f_load_phrase_file();
void cs_d04e_font_set_draw_position(uint16_t x, uint16_t y);
void cs_d05f_font_get_draw_position(uint16_t *x, uint16_t *y);
void cs_d068_font_select_tall_font();
void cs_d075_font_select_small_font();
void cs_d096_font_glyph_draw_tall(byte c);
void cs_d12f_font_glyph_draw_small(byte c);
void cs_d1ef_draw_all_ui_elements();
void cs_d19b_font_draw_phrase_or_command_string(uint16_t id);
void cs_d1a6_font_draw_phrase_list(string_def_t *p);
void cs_d1bb_font_draw_string(const char *str);
void cs_d1f2_draw_ui_element_list(ui_element_t *list, int count);
void cs_d200_draw_ui_element(ui_element_t *e);
void cs_d2bd();
void cs_d72b(ui_element_t *controls);
void cs_d741();
void cs_d75a_ui_set_and_draw_frieze_sides_closed_book();
void cs_d792_ui_set_and_draw_frieze_sides_map();
void cs_d795_ui_set_and_draw_frieze_sides(const ui_element_flag_and_sprite_t *updates);
void cs_d7ad_ui_set_and_draw_frieze_sides_open_book();
void cs_d7b2_ui_set_and_draw_frieze_sides_globe();
void cs_d815_game_loop();
void cs_d9d2_process_frame_tasks();
void cs_da25_add_frame_task(uint8_t interval, frame_task_fn_t fn);
void cs_da53_remove_all_frame_tasks();
void cs_da5f_remove_frame_task(frame_task_fn_t fn);
void cs_daa3_clear_some_mouse_rect();
void cs_daaa_set_some_mouse_rect(rect_t *rect);
void cs_dc20_draw_mouse();
bool cs_dd63_has_user_input();
bool cs_ddb0_wait_interruptable(uint16_t max_wait_ticks);
bool cs_ddf0_wait_voice_interruptable(uint16_t fallback_wait_ticks);
void cs_de0c(uint16_t bx);
void cs_e290_font_draw_number_at_position_right_aligned(int x, int y, uint16_t v);
void cs_e297_font_draw_number_right_aligned(uint16_t v);
void cs_e387_wait(int n);
bool cs_e675_dat_open();
byte *cs_e741_dat_read_toc();
void cs_e75b_res_store_in_lookup_table(uint16_t res_id, byte dl, byte *, byte *);
void cs_ea7b_init_extended_memory_allocator();
byte *cs_f0b9_res_read_by_index_into_buffer(uint16_t index, uint8_t *buffer = nullptr);
uint32_t cs_f0d6_res_read(byte *buffer, uint32_t size);
byte *cs_f0f6_alloc_get_addr();
void cs_f0ff_alloc_take(uint16_t bytes);
byte *cs_f11c_alloc_check(uint16_t bytes);
void cs_f131_out_of_standard_memory();
bool cs_f1fb_res_open(const char *filename, int *fd, uint32_t *size, uint32_t *offset);
void cs_f229_res_open_or_die(const char *filename, int *fd, uint32_t *size, uint32_t *offset);
uint16_t cs_f244_res_read_to_buffer(const char *filename, byte *buffer, uint32_t *size);
bool cs_f2a7_dat_seek_to_name(const char *filename, int *fd, uint32_t *size, uint32_t *offset);
void cs_f2d6_dat_seek(int *fd, uint32_t offset);
uint16_t cs_f2ea_dat_read(int bytes, byte *p);
std::optional<uint16_t> cs_f314_dat_get_res_index_by_name(const char *filename, uint8_t &dl);
bool cs_f3a7_dat_locate_map_entry(uint16_t index, uint8_t dl, uint16_t &di);
uint16_t cs_f3d3_maybe_uncompress_hsq(byte *buffer);
void cs_f403_unpack_hsq_skip_header(ptr_offset_t &src, ptr_offset_t &dst);
void cs_f40d_unpack_hsq(ptr_offset_t src, uint16_t unpacked_size);
void cs_f435_hsq_unpack_no_header(ptr_offset_t &src, ptr_offset_t &dst);

void vga_015a_gfx_30(byte ax, byte *di, byte *ds, byte *es);
void vga_0967_set_graphics_mode();
void vga_0975(bool monochrome);
void vga_09d9_get_screen_buffer(byte **buffer, uint16_t *size);
void vga_09e2_set_palette_unapplied(const byte *src, int byte_start, int len);
void vga_0a21_pal_byte_offset_and_byte_count_to_index(uint16_t *offset, uint16_t *count);
void vga_0a40_gfx_38_set_palette_2(const byte *src, int byte_start, int len);
void vga_0a58_copy_pal_1_to_pal_2();
void vga_0a68_gfx_41_copy_pal_1_to_pal_2();
void vga_0ad7_gfx_39(uint8_t al, uint16_t offset, uint16_t count);
void vga_0b0c_gfx_32_set_screen_palette();
void vga_0b68_set_palette_to_screen(byte *pal, int start, int entries);
void vga_0c06_gfx_33_set_global_y_offset(uint8_t y);
void vga_0f5b_blit(byte *dst, int dst_x, int dst_y, ptr_offset_t src, int width, int height, uint8_t flags,
                   uint8_t mode);
void draw_4bpp(byte *dst, bool flip_x, bool flip_y, ptr_offset_t &src, int dst_x, int dst_y, int w, int h, uint8_t mode,
               rect_t clip_rect);
void draw_4bpp_rle(byte *dst, bool flip_x, bool flip_y, ptr_offset_t &src, int dst_x, int dst_y, int w, int h,
                   uint8_t mode, rect_t clip_rect);
void draw_8bpp(byte *dst, bool flip_x, bool flip_y, ptr_offset_t &src, int dst_x, int dst_y, int w, int h, uint8_t mode,
               rect_t clip_rect);
void draw_8bpp_rle(byte *dst, bool flip_x, bool flip_y, ptr_offset_t &src, int dst_x, int dst_y, int w, int h,
                   uint8_t mode, rect_t clip_rect);
void draw_4bpp_scaled(byte *src, uint16_t src_pitch, int dst_x, int dst_y, int dst_w, int dst_h, byte flags,
                      byte pal_offset, uint16_t src_delta_fp);
void vga_1452_blit_clipped(byte *dst, int dst_x, int dst_y, ptr_offset_t src, int width, int height, uint8_t flags,
                           uint8_t mode, rect_t clip_rect);
void vga_1888_gfx_03_draw_cursor(int16_t x, int16_t y, cursor_t *c);
void vga_1940_gfx_04_restore_cursor_area();
void vga_1979_gfx_09_clear_rect(byte *dst, rect_t *rect);
void vga_197b_gfx_10_fill_rect(byte *dst, rect_t *rect, byte color);
void vga_19c9_gfx_31_copy_rect(byte *fb, uint16_t src_x, uint16_t src_y, uint16_t w, uint16_t h, uint16_t dst_x,
                               uint16_t dst_y);
void vga_1a07_draw_line(byte *dst, uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, byte al, uint16_t bp,
                        rect_t *si);
void vga_1adc_draw_line(byte *dst, uint16_t x0, uint16_t y0, int16_t delta_x, int16_t delta_y, byte al, uint16_t bp,
                        rect_t *si);
void vga_1b7c_gfx_11_copy_framebuffer(byte *dst, byte *src);
void vga_1b8e_gfx_12_copy_framebuffer_rect(byte *dst, byte *src, rect_t *rect);
void vga_1bca_copy_interlaced(byte *dst, int dst_x, int dst_y, ptr_offset_t src, int width, int height);
void vga_1be7_copy_game_area(byte *dst, byte *src);
void vga_1bfe_gfx_7_draw_glyph(byte *dst, int x, int y, byte *src, uint8_t w, uint8_t h, uint16_t color);
void vga_1c76_gfx_26_copy_framebuffer_rect(byte *dst, byte *src, rect_t *rect);
void vga_1cb6_gfx_23(byte *dst, byte *map, byte *globdata, int dx, int ax);
void vga_2572_wait_frame(std::atomic_uint16_t &timer, uint16_t start);
void vga_25e7_transition(std::atomic_uint16_t &timer, uint8_t type, uint8_t *si, uint8_t *es, uint8_t *ds);
void vga_261d_maybe_wait_frame(std::atomic_uint16_t &timer, uint16_t start);
void vga_272e_transition_effect_0x3a(std::atomic_uint16_t &timer);
void vga_2757_transition_effect_0x30();
void vga_2dc3_transition_effect_0x10(std::atomic_uint16_t &timer, uint16_t cx);
void vga_3200_screen_effect(byte ax, byte *di, byte *ds, byte *es);
void vga_39e9_gfx_36_draw_gradient_line_with_noise(int x, int y, int w, int dir, uint16_t &bp, uint16_t si, uint16_t di,
                                                   uint16_t color);
