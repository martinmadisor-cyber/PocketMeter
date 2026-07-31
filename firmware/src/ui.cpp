#include "ui.h"
#include "splash.h"
#include "codex_splash.h"
#include "web_server.h"
#include <lvgl.h>
#include "logo.h"
#include "icons.h"
#include "codex_icon.h"
#include "ohiggins_icon.h"
#include "display_cfg.h"

// Custom fonts (scaled for 314 PPI, ~1.9x from original 165 PPI)
LV_FONT_DECLARE(font_tiempos_56);
LV_FONT_DECLARE(font_styrene_48);
LV_FONT_DECLARE(font_styrene_28);
LV_FONT_DECLARE(font_styrene_24);
LV_FONT_DECLARE(font_styrene_20);
LV_FONT_DECLARE(font_mono_32);
LV_FONT_DECLARE(font_styrene_clock);
LV_FONT_DECLARE(font_styrene_temp);

// Anthropic brand palette — design tokens live in theme.h
#include "theme.h"
#define COL_BG        THEME_BG
#define COL_PANEL     THEME_PANEL
#define COL_TEXT      THEME_TEXT
#define COL_DIM       THEME_DIM
#define COL_ACCENT    THEME_ACCENT
#define COL_GREEN     THEME_GREEN
#define COL_AMBER     THEME_AMBER
#define COL_RED       THEME_RED
#define COL_BAR_BG    THEME_BAR_BG

// ---- Layout constants for 480x480 (scaled for 2.16" high-DPI + rounded corners) ----
#define SCR_W         480
#define SCR_H         480
#define MARGIN        20    // wider margin for rounded display corners
#define TITLE_Y       30
#define CONTENT_W     (SCR_W - 2 * MARGIN)   // 440

// Hero clock + date block (shared, sits below the title on every screen)
#define HDR_DIV_Y     95     // divider under the title row
#define CLOCK_Y       100
#define CLOCK_DIV_Y   186    // divider under the giant clock
#define DATE_Y        196
#define DATE_DIV_Y    234    // divider under the date line
#define CONTENT_Y     244
#define CONTENT_END_Y 420    // bottom of the stat-row area (leaves room for anim text)

// ---- Usage screen widgets ----
static lv_obj_t* usage_container;
static lv_obj_t* lbl_title;
static lv_obj_t* bar_session;
static lv_obj_t* lbl_session_pct;
static lv_obj_t* lbl_session_label;
static lv_obj_t* lbl_session_reset;
static lv_obj_t* bar_weekly;
static lv_obj_t* lbl_weekly_pct;
static lv_obj_t* lbl_weekly_label;
static lv_obj_t* lbl_weekly_reset;
static lv_obj_t* lbl_anim;
static lv_obj_t* panel_session;
static lv_obj_t* panel_weekly;
static lv_obj_t* panel_credit;
static lv_obj_t* lbl_credit_balance;
static lv_obj_t* lbl_credit_plan;

// ---- Codex screen widgets ----
static lv_obj_t* codex_container;
static lv_obj_t* lbl_codex_session_pct;
static lv_obj_t* lbl_codex_session_label;
static lv_obj_t* bar_codex_session;
static lv_obj_t* lbl_codex_session_reset;
static lv_obj_t* lbl_codex_weekly_pct;
static lv_obj_t* lbl_codex_weekly_label;
static lv_obj_t* bar_codex_weekly;
static lv_obj_t* lbl_codex_weekly_reset;
static lv_obj_t* codex_icon_img;
static lv_image_dsc_t codex_icon_dsc;
static bool codex_available = false;
static lv_obj_t* panel_codex_session;
static lv_obj_t* panel_codex_weekly;
static lv_obj_t* panel_codex_credit;
static lv_obj_t* lbl_codex_credit_balance;
static lv_obj_t* lbl_codex_credit_plan;

// ---- Generic provider screen widgets ----
static lv_obj_t* provider_container;
static lv_obj_t* lbl_provider_title;
static lv_obj_t* lbl_provider_session_pct;
static lv_obj_t* bar_provider_session;
static lv_obj_t* lbl_provider_session_reset;
static lv_obj_t* lbl_provider_weekly_pct;
static lv_obj_t* bar_provider_weekly;
static lv_obj_t* lbl_provider_weekly_reset;
static bool      generic_provider_available = false;
static ProviderData generic_provider_data   = {};
static lv_obj_t* panel_provider_session;
static lv_obj_t* panel_provider_weekly;
static lv_obj_t* panel_provider_credit;
static lv_obj_t* lbl_provider_credit_balance;
static lv_obj_t* lbl_provider_credit_plan;

// ---- Network screen widgets ----
static lv_obj_t* net_container;
static lv_obj_t* lbl_net_status;
static lv_obj_t* lbl_net_ssid;
static lv_obj_t* lbl_net_ip;
static lv_obj_t* lbl_net_rssi;

// ---- O'Higgins fan-theme screen widgets ----
static lv_obj_t* ohiggins_container;
static lv_obj_t* ohiggins_icon_img;
static lv_image_dsc_t ohiggins_icon_dsc;
static lv_obj_t* lbl_oh_date;
static lv_obj_t* lbl_oh_temp;
static lv_obj_t* lbl_oh_desc;
static lv_obj_t* lbl_oh_humidity_wind;
static lv_obj_t* lbl_oh_current_pct;
static lv_obj_t* bar_oh_current;
static lv_obj_t* lbl_oh_weekly_pct;
static lv_obj_t* bar_oh_weekly;

// ---- Battery indicator (shared, on top) ----
static lv_obj_t* battery_img;
static lv_obj_t* logo_img;
static lv_image_dsc_t battery_dscs[5];  // empty, low, medium, full, charging

// ---- Hero clock + date (shared, below the title, on top of all containers) ----
static lv_obj_t* lbl_clock;
static lv_obj_t* lbl_date;
static lv_obj_t* div_header;
static lv_obj_t* div_clock;
static lv_obj_t* div_date;

// ---- Shared ----
static lv_image_dsc_t logo_dsc;
static screen_t current_screen = SCREEN_USAGE;

// Animation state
static uint32_t anim_last_ms = 0;
static uint8_t anim_spinner_idx = 0;
static uint8_t anim_phase = 0;
static uint8_t anim_msg_idx = 0;
static uint32_t anim_msg_start = 0;
#define ANIM_MSG_MS     4000

static const char* const spinner_frames[] = {
    "\xC2\xB7", "\xE2\x9C\xBB", "\xE2\x9C\xBD",
    "\xE2\x9C\xB6", "\xE2\x9C\xB3", "\xE2\x9C\xA2",
};
#define SPINNER_COUNT 6
#define SPINNER_PHASES (2 * (SPINNER_COUNT - 1))  // 10: ping-pong 0..5..0

// Per-frame hold time. Modeled on Claude Code's spinner (Cavalry triangle
// oscillator, range 0..5, period 5s) — turn-around frames (0 and 5) appear
// once per cycle, middle frames twice, so 0/5 read as held longer.
static const uint16_t spinner_ms[SPINNER_COUNT] = {
    260, 130, 130, 130, 130, 260,
};

static const char* const anim_messages[] = {
    "Accomplishing", "Elucidating", "Perusing",
    "Actioning", "Enchanting", "Philosophising",
    "Actualizing", "Envisioning", "Pondering",
    "Baking", "Finagling", "Pontificating",
    "Booping", "Flibbertigibbeting", "Processing",
    "Brewing", "Forging", "Puttering",
    "Calculating", "Forming", "Puzzling",
    "Cerebrating", "Frolicking", "Reticulating",
    "Channelling", "Generating", "Ruminating",
    "Churning", "Germinating", "Scheming",
    "Clauding", "Hatching", "Schlepping",
    "Coalescing", "Herding", "Shimmying",
    "Cogitating", "Honking", "Shucking",
    "Combobulating", "Hustling", "Simmering",
    "Computing", "Ideating", "Smooshing",
    "Concocting", "Imagining", "Spelunking",
    "Conjuring", "Incubating", "Spinning",
    "Considering", "Inferring", "Stewing",
    "Contemplating", "Jiving", "Sussing",
    "Cooking", "Manifesting", "Synthesizing",
    "Crafting", "Marinating", "Thinking",
    "Creating", "Meandering", "Tinkering",
    "Crunching", "Moseying", "Transmuting",
    "Deciphering", "Mulling", "Unfurling",
    "Deliberating", "Mustering", "Unravelling",
    "Determining", "Musing", "Vibing",
    "Discombobulating", "Noodling", "Wandering",
    "Divining", "Percolating", "Whirring",
    "Doing", "Wibbling",
    "Effecting", "Wizarding",
    "Working", "Wrangling",
};
#define ANIM_MSG_COUNT (sizeof(anim_messages) / sizeof(anim_messages[0]))

static lv_color_t pct_color(float pct) {
    if (pct >= 80.0f) return COL_RED;
    if (pct >= 50.0f) return COL_AMBER;
    return COL_GREEN;
}

static lv_color_t status_color(const char* status) {
    if (strcmp(status, "limited") == 0) return COL_RED;
    if (strcmp(status, "approaching") == 0) return COL_AMBER;
    return COL_GREEN;
}

static void format_reset_time(int mins, char* buf, size_t len) {
    if (mins < 0) {
        snprintf(buf, len, "---");
    } else if (mins < 60) {
        snprintf(buf, len, "Resets in %dm", mins);
    } else if (mins < 1440) {
        snprintf(buf, len, "Resets in %dh %dm", mins / 60, mins % 60);
    } else {
        snprintf(buf, len, "Resets in %dd %dh", mins / 1440, (mins % 1440) / 60);
    }
}

// Forward decls — callbacks defined near ui_show_screen below
static void global_click_cb(lv_event_t* e);

static lv_obj_t* make_panel(lv_obj_t* parent, int x, int y, int w, int h) {
    lv_obj_t* panel = lv_obj_create(parent);
    lv_obj_set_pos(panel, x, y);
    lv_obj_set_size(panel, w, h);
    lv_obj_set_style_bg_color(panel, COL_PANEL, 0);
    lv_obj_set_style_bg_opa(panel, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(panel, 8, 0);
    lv_obj_set_style_border_width(panel, 0, 0);
    lv_obj_set_style_pad_left(panel, 16, 0);
    lv_obj_set_style_pad_right(panel, 16, 0);
    lv_obj_set_style_pad_top(panel, 12, 0);
    lv_obj_set_style_pad_bottom(panel, 12, 0);
    lv_obj_clear_flag(panel, LV_OBJ_FLAG_SCROLLABLE);
    // Bubble click events up to the screen / usage_container so a tap anywhere
    // on the panel fires the global click handler.
    lv_obj_add_flag(panel, LV_OBJ_FLAG_EVENT_BUBBLE);
    return panel;
}

static lv_obj_t* make_divider(lv_obj_t* parent, int x, int y, int w) {
    lv_obj_t* div = lv_obj_create(parent);
    lv_obj_set_pos(div, x, y);
    lv_obj_set_size(div, w, 1);
    lv_obj_set_style_bg_color(div, COL_BAR_BG, 0);
    lv_obj_set_style_bg_opa(div, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(div, 0, 0);
    lv_obj_set_style_radius(div, 0, 0);
    lv_obj_set_style_pad_all(div, 0, 0);
    lv_obj_clear_flag(div, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(div, LV_OBJ_FLAG_EVENT_BUBBLE);
    return div;
}

static lv_obj_t* make_bar(lv_obj_t* parent, int x, int y, int w, int h) {
    lv_obj_t* bar = lv_bar_create(parent);
    lv_obj_set_pos(bar, x, y);
    lv_obj_set_size(bar, w, h);
    lv_bar_set_range(bar, 0, 100);
    lv_bar_set_value(bar, 0, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(bar, COL_BAR_BG, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(bar, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_radius(bar, 6, LV_PART_MAIN);
    lv_obj_set_style_bg_color(bar, COL_GREEN, LV_PART_INDICATOR);
    lv_obj_set_style_bg_opa(bar, LV_OPA_COVER, LV_PART_INDICATOR);
    lv_obj_set_style_radius(bar, 6, LV_PART_INDICATOR);
    return bar;
}

static void init_icon_dsc(lv_image_dsc_t* dsc, int w, int h, const uint16_t* data) {
    dsc->header.w = w;
    dsc->header.h = h;
    dsc->header.cf = LV_COLOR_FORMAT_RGB565;
    dsc->header.stride = w * 2;
    dsc->data = (const uint8_t*)data;
    dsc->data_size = w * h * 2;
}

// RGB565A8: planar — w*h RGB565 pixels followed by w*h alpha bytes.
// Stride is RGB565-only (w*2); LVGL infers alpha plane location from header.
static void init_icon_dsc_rgb565a8(lv_image_dsc_t* dsc, int w, int h, const uint8_t* data) {
    dsc->header.w = w;
    dsc->header.h = h;
    dsc->header.cf = LV_COLOR_FORMAT_RGB565A8;
    dsc->header.stride = w * 2;
    dsc->data = data;
    dsc->data_size = w * h * 3;
}

static lv_obj_t* make_pill(lv_obj_t* parent, const char* text) {
    lv_obj_t* lbl = lv_label_create(parent);
    lv_label_set_text(lbl, text);
    lv_obj_set_style_text_font(lbl, &font_styrene_28, 0);
    lv_obj_set_style_text_color(lbl, COL_TEXT, 0);
    lv_obj_set_style_bg_color(lbl, COL_BAR_BG, 0);
    lv_obj_set_style_bg_opa(lbl, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(lbl, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_pad_left(lbl, 18, 0);
    lv_obj_set_style_pad_right(lbl, 18, 0);
    lv_obj_set_style_pad_top(lbl, 6, 0);
    lv_obj_set_style_pad_bottom(lbl, 6, 0);
    return lbl;
}

// ---- Battery icon initialization ----
static void init_battery_icons(void) {
    init_icon_dsc_rgb565a8(&battery_dscs[0], ICON_BATTERY_W, ICON_BATTERY_H, icon_battery_data);
    init_icon_dsc_rgb565a8(&battery_dscs[1], ICON_BATTERY_LOW_W, ICON_BATTERY_LOW_H, icon_battery_low_data);
    init_icon_dsc_rgb565a8(&battery_dscs[2], ICON_BATTERY_MEDIUM_W, ICON_BATTERY_MEDIUM_H, icon_battery_medium_data);
    init_icon_dsc_rgb565a8(&battery_dscs[3], ICON_BATTERY_FULL_W, ICON_BATTERY_FULL_H, icon_battery_full_data);
    init_icon_dsc_rgb565a8(&battery_dscs[4], ICON_BATTERY_CHARGING_W, ICON_BATTERY_CHARGING_H, icon_battery_charging_data);
}

// ======== Usage Screen (480x480) ========

#define ROW_H       90

// One Current/Weekly stat row (flat, no card): big % on the left, label at
// top-right, reset text below it at bottom-right, full-width bar underneath,
// thin divider under that.
static lv_obj_t* make_usage_panel(lv_obj_t* parent, int y, const char* label_text,
                                  lv_obj_t** out_pct, lv_obj_t** out_pill,
                                  lv_obj_t** out_bar, lv_obj_t** out_reset) {
    lv_obj_t* row = lv_obj_create(parent);
    lv_obj_set_pos(row, MARGIN, y);
    lv_obj_set_size(row, CONTENT_W, ROW_H);
    lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(row, 0, 0);
    lv_obj_set_style_pad_all(row, 0, 0);
    lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(row, LV_OBJ_FLAG_EVENT_BUBBLE);

    *out_pct = lv_label_create(row);
    lv_label_set_text(*out_pct, "---%");
    lv_obj_set_style_text_font(*out_pct, &font_styrene_48, 0);
    lv_obj_set_style_text_color(*out_pct, COL_TEXT, 0);
    lv_obj_set_pos(*out_pct, 0, 0);

    *out_pill = lv_label_create(row);
    lv_label_set_text(*out_pill, label_text);
    lv_obj_set_style_text_font(*out_pill, &font_styrene_28, 0);
    lv_obj_set_style_text_color(*out_pill, COL_TEXT, 0);
    lv_obj_align(*out_pill, LV_ALIGN_TOP_RIGHT, 0, 6);

    *out_reset = lv_label_create(row);
    lv_label_set_text(*out_reset, "---");
    lv_obj_set_style_text_font(*out_reset, &font_styrene_20, 0);
    lv_obj_set_style_text_color(*out_reset, COL_DIM, 0);
    lv_obj_align(*out_reset, LV_ALIGN_TOP_RIGHT, 0, 44);

    *out_bar = make_bar(row, 0, 70, CONTENT_W, 12);

    make_divider(row, 0, ROW_H - 4, CONTENT_W);

    return row;
}

static lv_obj_t* make_credit_panel(lv_obj_t* parent, int y, int h,
                                   lv_obj_t** out_balance, lv_obj_t** out_plan) {
    lv_obj_t* panel = make_panel(parent, MARGIN, y, CONTENT_W, h);

    *out_balance = lv_label_create(panel);
    lv_label_set_text(*out_balance, "");
    lv_obj_set_style_text_font(*out_balance, &font_styrene_48, 0);
    lv_obj_set_style_text_color(*out_balance, COL_TEXT, 0);
    lv_obj_align(*out_balance, LV_ALIGN_TOP_MID, 0, 40);

    *out_plan = make_pill(panel, "");
    lv_obj_align(*out_plan, LV_ALIGN_TOP_MID, 0, 110);

    return panel;
}

static void set_credit_mode(lv_obj_t* session_panel, lv_obj_t* weekly_panel,
                            lv_obj_t* credit_panel, bool credit_mode) {
    if (credit_mode) {
        lv_obj_add_flag(session_panel, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(weekly_panel, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(credit_panel, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_clear_flag(session_panel, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(weekly_panel, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(credit_panel, LV_OBJ_FLAG_HIDDEN);
    }
}

static void init_usage_screen(lv_obj_t* scr) {
    usage_container = lv_obj_create(scr);
    lv_obj_set_size(usage_container, SCR_W, SCR_H);
    lv_obj_set_pos(usage_container, 0, 0);
    lv_obj_set_style_bg_opa(usage_container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(usage_container, 0, 0);
    lv_obj_set_style_pad_all(usage_container, 0, 0);
    lv_obj_clear_flag(usage_container, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(usage_container, global_click_cb, LV_EVENT_CLICKED, NULL);

    lbl_title = lv_label_create(usage_container);
    lv_label_set_text(lbl_title, "Usage");
    lv_obj_set_style_text_font(lbl_title, &font_tiempos_56, 0);
    lv_obj_set_style_text_color(lbl_title, COL_TEXT, 0);
    lv_obj_align(lbl_title, LV_ALIGN_TOP_MID, 16, TITLE_Y);

    panel_session = make_usage_panel(usage_container, CONTENT_Y, "Current",
                                     &lbl_session_pct, &lbl_session_label,
                                     &bar_session, &lbl_session_reset);
    panel_weekly = make_usage_panel(usage_container, CONTENT_Y + ROW_H, "Weekly",
                                    &lbl_weekly_pct, &lbl_weekly_label,
                                    &bar_weekly, &lbl_weekly_reset);

    panel_credit = make_credit_panel(usage_container, CONTENT_Y, CONTENT_END_Y - CONTENT_Y,
                                     &lbl_credit_balance, &lbl_credit_plan);
    lv_obj_add_flag(panel_credit, LV_OBJ_FLAG_HIDDEN);

    lbl_anim = lv_label_create(usage_container);
    lv_label_set_text(lbl_anim, "");
    lv_obj_set_style_text_font(lbl_anim, &font_mono_32, 0);
    lv_obj_set_style_text_color(lbl_anim, COL_ACCENT, 0);
    lv_obj_align(lbl_anim, LV_ALIGN_BOTTOM_MID, 0, -15);
}

// ======== Codex Screen (480x480) ========

static void init_codex_screen(lv_obj_t* scr) {
    codex_container = lv_obj_create(scr);
    lv_obj_set_size(codex_container, SCR_W, SCR_H);
    lv_obj_set_pos(codex_container, 0, 0);
    lv_obj_set_style_bg_opa(codex_container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(codex_container, 0, 0);
    lv_obj_set_style_pad_all(codex_container, 0, 0);
    lv_obj_clear_flag(codex_container, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(codex_container, global_click_cb, LV_EVENT_CLICKED, NULL);

    // Title "Codex" in teal
    lv_obj_t* lbl_title = lv_label_create(codex_container);
    lv_label_set_text(lbl_title, "Codex");
    lv_obj_set_style_text_font(lbl_title, &font_tiempos_56, 0);
    lv_obj_set_style_text_color(lbl_title, THEME_CODEX, 0);
    lv_obj_align(lbl_title, LV_ALIGN_TOP_MID, 16, TITLE_Y);

    // Session/Weekly panels (same layout as usage screen)
    panel_codex_session = make_usage_panel(codex_container, CONTENT_Y, "Current",
                                           &lbl_codex_session_pct, &lbl_codex_session_label,
                                           &bar_codex_session, &lbl_codex_session_reset);
    panel_codex_weekly = make_usage_panel(codex_container, CONTENT_Y + ROW_H, "Weekly",
                                          &lbl_codex_weekly_pct, &lbl_codex_weekly_label,
                                          &bar_codex_weekly, &lbl_codex_weekly_reset);

    // Override bar indicator color to Codex teal
    lv_obj_set_style_bg_color(bar_codex_session, THEME_CODEX, LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(bar_codex_weekly,  THEME_CODEX, LV_PART_INDICATOR);

    // Credit panel
    panel_codex_credit = make_credit_panel(codex_container, CONTENT_Y, CONTENT_END_Y - CONTENT_Y,
                                           &lbl_codex_credit_balance, &lbl_codex_credit_plan);
    lv_obj_add_flag(panel_codex_credit, LV_OBJ_FLAG_HIDDEN);

    // Cloud icon (top-left, same position as logo — logo is hidden on SCREEN_CODEX)
    init_icon_dsc_rgb565a8(&codex_icon_dsc, CODEX_ICON_W, CODEX_ICON_H, codex_icon_data);
    codex_icon_img = lv_image_create(scr);
    lv_image_set_src(codex_icon_img, &codex_icon_dsc);
    lv_obj_set_pos(codex_icon_img, MARGIN, TITLE_Y - 10);
    lv_obj_add_flag(codex_icon_img, LV_OBJ_FLAG_HIDDEN);

    lv_obj_add_flag(codex_container, LV_OBJ_FLAG_HIDDEN);
}

// ======== Generic Provider Screen (480x480) ========
// Reuses the same layout as the Claude screen but without the animation bar.
// Content is populated via ui_set_generic_provider().

static lv_color_t provider_accent_color(const char* name) {
    if (strcmp(name, "gemini")   == 0) return lv_color_hex(0xAB87EA);
    if (strcmp(name, "copilot")  == 0) return lv_color_hex(0x0078D4);
    if (strcmp(name, "grok")     == 0) return lv_color_hex(0xE7E7E7);
    if (strcmp(name, "openai")   == 0) return lv_color_hex(0x74AA9C);
    if (strcmp(name, "deepseek") == 0) return lv_color_hex(0x4D6BFE);
    if (strcmp(name, "windsurf") == 0) return lv_color_hex(0x00BFA5);
    if (strcmp(name, "cursor")   == 0) return lv_color_hex(0x00BCA5);
    if (strcmp(name, "kimi")     == 0) return lv_color_hex(0xFF6B35);
    return lv_color_hex(0x7B8FA1);
}

static const char* provider_display_name(const char* name) {
    if (strcmp(name, "gemini")   == 0) return "Gemini";
    if (strcmp(name, "copilot")  == 0) return "Copilot";
    if (strcmp(name, "grok")     == 0) return "Grok";
    if (strcmp(name, "openai")   == 0) return "OpenAI";
    if (strcmp(name, "deepseek") == 0) return "DeepSeek";
    if (strcmp(name, "windsurf") == 0) return "Windsurf";
    if (strcmp(name, "cursor")   == 0) return "Cursor";
    if (strcmp(name, "kimi")     == 0) return "Kimi";
    return name;
}

static void init_provider_screen(lv_obj_t* scr) {
    provider_container = lv_obj_create(scr);
    lv_obj_set_size(provider_container, SCR_W, SCR_H);
    lv_obj_set_pos(provider_container, 0, 0);
    lv_obj_set_style_bg_color(provider_container, COL_BG, 0);
    lv_obj_set_style_bg_opa(provider_container, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(provider_container, 0, 0);
    lv_obj_set_style_pad_all(provider_container, 0, 0);
    lv_obj_clear_flag(provider_container, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(provider_container, global_click_cb, LV_EVENT_CLICKED, NULL);

    // Title — filled in by ui_set_generic_provider
    lbl_provider_title = lv_label_create(provider_container);
    lv_label_set_text(lbl_provider_title, "Provider");
    lv_obj_set_style_text_font(lbl_provider_title, &font_tiempos_56, 0);
    lv_obj_set_style_text_color(lbl_provider_title, COL_TEXT, 0);
    lv_obj_align(lbl_provider_title, LV_ALIGN_TOP_MID, 16, TITLE_Y);

    // Session/Weekly rows (same flat layout as Usage/Codex)
    lv_obj_t* lbl_provider_session_label;
    lv_obj_t* lbl_provider_weekly_label;
    panel_provider_session = make_usage_panel(provider_container, CONTENT_Y, "Session",
                                              &lbl_provider_session_pct, &lbl_provider_session_label,
                                              &bar_provider_session, &lbl_provider_session_reset);
    panel_provider_weekly = make_usage_panel(provider_container, CONTENT_Y + ROW_H, "Weekly",
                                             &lbl_provider_weekly_pct, &lbl_provider_weekly_label,
                                             &bar_provider_weekly, &lbl_provider_weekly_reset);

    // Credit panel (spans the same content area as the two rows above)
    panel_provider_credit = make_credit_panel(provider_container, CONTENT_Y, CONTENT_END_Y - CONTENT_Y,
                                              &lbl_provider_credit_balance, &lbl_provider_credit_plan);
    lv_obj_add_flag(panel_provider_credit, LV_OBJ_FLAG_HIDDEN);

    lv_obj_add_flag(provider_container, LV_OBJ_FLAG_HIDDEN);
}

// ======== Network Screen (480x480) ========

static void init_network_screen(lv_obj_t* scr) {
    net_container = lv_obj_create(scr);
    lv_obj_set_size(net_container, SCR_W, SCR_H);
    lv_obj_set_pos(net_container, 0, 0);
    lv_obj_set_style_bg_opa(net_container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(net_container, 0, 0);
    lv_obj_set_style_pad_all(net_container, 0, 0);
    lv_obj_clear_flag(net_container, LV_OBJ_FLAG_SCROLLABLE);

    // Title
    lv_obj_t* lbl_net_title = lv_label_create(net_container);
    lv_label_set_text(lbl_net_title, "Network");
    lv_obj_set_style_text_font(lbl_net_title, &font_tiempos_56, 0);
    lv_obj_set_style_text_color(lbl_net_title, COL_TEXT, 0);
    lv_obj_align(lbl_net_title, LV_ALIGN_TOP_MID, 16, TITLE_Y);

    // Info block (flat, no card — matches the rest of the app)
    lv_obj_t* p_info = lv_obj_create(net_container);
    lv_obj_set_pos(p_info, MARGIN, CONTENT_Y);
    lv_obj_set_size(p_info, CONTENT_W, CONTENT_END_Y - CONTENT_Y);
    lv_obj_set_style_bg_opa(p_info, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(p_info, 0, 0);
    lv_obj_set_style_pad_all(p_info, 0, 0);
    lv_obj_clear_flag(p_info, LV_OBJ_FLAG_SCROLLABLE);

    // WiFi label
    lv_obj_t* wifi_icon = lv_label_create(p_info);
    lv_label_set_text(wifi_icon, "WiFi");
    lv_obj_set_style_text_font(wifi_icon, &font_styrene_28, 0);
    lv_obj_set_style_text_color(wifi_icon, COL_DIM, 0);
    lv_obj_set_pos(wifi_icon, 0, 4);

    lbl_net_status = lv_label_create(p_info);
    lv_label_set_text(lbl_net_status, "Disconnected");
    lv_obj_set_style_text_font(lbl_net_status, &font_styrene_48, 0);
    lv_obj_set_style_text_color(lbl_net_status, COL_DIM, 0);
    lv_obj_set_pos(lbl_net_status, 56, 2);

    make_divider(p_info, 0, 60, CONTENT_W);

    lbl_net_ssid = lv_label_create(p_info);
    lv_label_set_text(lbl_net_ssid, "SSID: ---");
    lv_obj_set_style_text_font(lbl_net_ssid, &font_styrene_28, 0);
    lv_obj_set_style_text_color(lbl_net_ssid, COL_DIM, 0);
    lv_obj_set_pos(lbl_net_ssid, 0, 76);

    lbl_net_ip = lv_label_create(p_info);
    lv_label_set_text(lbl_net_ip, "IP: ---");
    lv_obj_set_style_text_font(lbl_net_ip, &font_styrene_28, 0);
    lv_obj_set_style_text_color(lbl_net_ip, COL_DIM, 0);
    lv_obj_set_pos(lbl_net_ip, 0, 116);

    lbl_net_rssi = lv_label_create(p_info);
    lv_label_set_text(lbl_net_rssi, "RSSI: --- dBm");
    lv_obj_set_style_text_font(lbl_net_rssi, &font_styrene_28, 0);
    lv_obj_set_style_text_color(lbl_net_rssi, COL_DIM, 0);
    lv_obj_set_pos(lbl_net_rssi, 0, 156);

    // Attribution
    lv_obj_t* lbl_credit = lv_label_create(net_container);
    lv_label_set_text(lbl_credit, "Built by @hermannbjorgvin");
    lv_obj_set_style_text_font(lbl_credit, &font_styrene_24, 0);
    lv_obj_set_style_text_color(lbl_credit, COL_DIM, 0);
    lv_obj_align(lbl_credit, LV_ALIGN_BOTTOM_MID, 0, -46);

    lv_obj_t* lbl_credit2 = lv_label_create(net_container);
    lv_label_set_text(lbl_credit2, "Clawd animation by @amaanbuilds");
    lv_obj_set_style_text_font(lbl_credit2, &font_styrene_20, 0);
    lv_obj_set_style_text_color(lbl_credit2, COL_DIM, 0);
    lv_obj_align(lbl_credit2, LV_ALIGN_BOTTOM_MID, 0, -20);

    // Start hidden
    lv_obj_add_flag(net_container, LV_OBJ_FLAG_HIDDEN);
}

// ======== O'Higgins Fan Theme Screen (480x480) ========
// Own crest icon (shown in the shared logo slot), own title, own two-line
// date (the shared clock is reused as-is), a weather box, and two compact
// boxed mini-stats mirroring Claude's Current/Weekly usage.

#define OH_DATE_Y      196
#define OH_DATE_DIV_Y  268
#define OH_CONTENT_Y   278
#define OH_BOX_H       190
#define OH_WEATHER_W   212
#define OH_GAP         16
#define OH_RIGHT_X     (MARGIN + OH_WEATHER_W + OH_GAP)
#define OH_RIGHT_W     (CONTENT_W - OH_WEATHER_W - OH_GAP)
#define OH_MINI_H      90
#define OH_MINI_GAP    10

static void init_ohiggins_screen(lv_obj_t* scr) {
    ohiggins_container = lv_obj_create(scr);
    lv_obj_set_size(ohiggins_container, SCR_W, SCR_H);
    lv_obj_set_pos(ohiggins_container, 0, 0);
    lv_obj_set_style_bg_opa(ohiggins_container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(ohiggins_container, 0, 0);
    lv_obj_set_style_pad_all(ohiggins_container, 0, 0);
    lv_obj_clear_flag(ohiggins_container, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(ohiggins_container, global_click_cb, LV_EVENT_CLICKED, NULL);

    lv_color_t oh_blue = lv_color_hex(0x3FA9F5);

    // Title, right of the crest (crest sits in the shared logo slot)
    lv_obj_t* lbl_oh_title = lv_label_create(ohiggins_container);
    lv_label_set_text(lbl_oh_title, "\xC2\xA1VAMOS O'HIGGINS!");
    lv_obj_set_style_text_font(lbl_oh_title, &font_styrene_24, 0);
    lv_obj_set_style_text_color(lbl_oh_title, oh_blue, 0);
    lv_obj_set_width(lbl_oh_title, CONTENT_W - 100);
    lv_obj_set_style_text_align(lbl_oh_title, LV_TEXT_ALIGN_RIGHT, 0);
    lv_obj_set_pos(lbl_oh_title, MARGIN + 100, TITLE_Y + 18);

    // Own two-line date (the shared clock above it is reused as-is)
    lbl_oh_date = lv_label_create(ohiggins_container);
    lv_label_set_text(lbl_oh_date, "---");
    lv_obj_set_style_text_font(lbl_oh_date, &font_styrene_24, 0);
    lv_obj_set_style_text_color(lbl_oh_date, oh_blue, 0);
    lv_obj_set_width(lbl_oh_date, CONTENT_W);
    lv_obj_set_style_text_align(lbl_oh_date, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_pos(lbl_oh_date, MARGIN, OH_DATE_Y);

    make_divider(ohiggins_container, MARGIN, OH_DATE_DIV_Y, CONTENT_W);

    // Weather box (left)
    lv_obj_t* p_weather = make_panel(ohiggins_container, MARGIN, OH_CONTENT_Y, OH_WEATHER_W, OH_BOX_H);

    lv_obj_t* lbl_weather_title = lv_label_create(p_weather);
    lv_label_set_text(lbl_weather_title, "CLIMA ACTUAL");
    lv_obj_set_style_text_font(lbl_weather_title, &font_styrene_20, 0);
    lv_obj_set_style_text_color(lbl_weather_title, COL_DIM, 0);
    lv_obj_set_pos(lbl_weather_title, 0, 0);

    lbl_oh_temp = lv_label_create(p_weather);
    lv_label_set_text(lbl_oh_temp, "--\xC2\xB0" "C");
    lv_obj_set_style_text_font(lbl_oh_temp, &font_styrene_temp, 0);
    lv_obj_set_style_text_color(lbl_oh_temp, COL_TEXT, 0);
    lv_obj_set_pos(lbl_oh_temp, 0, 34);

    lbl_oh_desc = lv_label_create(p_weather);
    lv_label_set_text(lbl_oh_desc, "---");
    lv_obj_set_style_text_font(lbl_oh_desc, &font_styrene_20, 0);
    lv_obj_set_style_text_color(lbl_oh_desc, COL_DIM, 0);
    lv_obj_set_width(lbl_oh_desc, OH_WEATHER_W - 32);
    lv_obj_set_pos(lbl_oh_desc, 0, 90);

    lbl_oh_humidity_wind = lv_label_create(p_weather);
    lv_label_set_text(lbl_oh_humidity_wind, "---");
    lv_obj_set_style_text_font(lbl_oh_humidity_wind, &font_styrene_20, 0);
    lv_obj_set_style_text_color(lbl_oh_humidity_wind, COL_DIM, 0);
    lv_obj_set_pos(lbl_oh_humidity_wind, 0, 122);

    // Right column: compact Current/Weekly boxes mirroring Claude's data
    lv_obj_t* p_current = make_panel(ohiggins_container, OH_RIGHT_X, OH_CONTENT_Y, OH_RIGHT_W, OH_MINI_H);
    lv_obj_t* lbl_current_lbl = lv_label_create(p_current);
    lv_label_set_text(lbl_current_lbl, "Actual");
    lv_obj_set_style_text_font(lbl_current_lbl, &font_styrene_20, 0);
    lv_obj_set_style_text_color(lbl_current_lbl, COL_DIM, 0);
    lv_obj_set_pos(lbl_current_lbl, 0, 0);
    lbl_oh_current_pct = lv_label_create(p_current);
    lv_label_set_text(lbl_oh_current_pct, "---%");
    lv_obj_set_style_text_font(lbl_oh_current_pct, &font_styrene_28, 0);
    lv_obj_set_style_text_color(lbl_oh_current_pct, COL_TEXT, 0);
    lv_obj_set_pos(lbl_oh_current_pct, 0, 20);
    bar_oh_current = make_bar(p_current, 0, 56, OH_RIGHT_W - 32, 10);

    lv_obj_t* p_weekly = make_panel(ohiggins_container, OH_RIGHT_X, OH_CONTENT_Y + OH_MINI_H + OH_MINI_GAP, OH_RIGHT_W, OH_MINI_H);
    lv_obj_t* lbl_weekly_lbl = lv_label_create(p_weekly);
    lv_label_set_text(lbl_weekly_lbl, "Semanal");
    lv_obj_set_style_text_font(lbl_weekly_lbl, &font_styrene_20, 0);
    lv_obj_set_style_text_color(lbl_weekly_lbl, COL_DIM, 0);
    lv_obj_set_pos(lbl_weekly_lbl, 0, 0);
    lbl_oh_weekly_pct = lv_label_create(p_weekly);
    lv_label_set_text(lbl_oh_weekly_pct, "---%");
    lv_obj_set_style_text_font(lbl_oh_weekly_pct, &font_styrene_28, 0);
    lv_obj_set_style_text_color(lbl_oh_weekly_pct, COL_TEXT, 0);
    lv_obj_set_pos(lbl_oh_weekly_pct, 0, 20);
    bar_oh_weekly = make_bar(p_weekly, 0, 56, OH_RIGHT_W - 32, 10);

    // Crest icon — reuses the shared logo slot
    init_icon_dsc_rgb565a8(&ohiggins_icon_dsc, OHIGGINS_ICON_W, OHIGGINS_ICON_H, ohiggins_icon_data);
    ohiggins_icon_img = lv_image_create(scr);
    lv_image_set_src(ohiggins_icon_img, &ohiggins_icon_dsc);
    lv_obj_set_pos(ohiggins_icon_img, MARGIN, TITLE_Y - 10);
    lv_obj_add_flag(ohiggins_icon_img, LV_OBJ_FLAG_HIDDEN);

    lv_obj_add_flag(ohiggins_container, LV_OBJ_FLAG_HIDDEN);
}

// ======== Public API ========

void ui_init(void) {
    lv_obj_t* scr = lv_screen_active();
    lv_obj_set_style_bg_color(scr, COL_BG, 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);

    // Logo (shared, always visible, on top of all containers)
    // Logo is RGB565A8 (planar: w*h RGB565 then w*h alpha) so it composites
    // cleanly against whatever bg is behind it.
    init_icon_dsc_rgb565a8(&logo_dsc, LOGO_WIDTH, LOGO_HEIGHT, logo_data);

    // Initialize battery icon descriptors
    init_battery_icons();

    init_usage_screen(scr);
    init_codex_screen(scr);
    init_provider_screen(scr);
    init_network_screen(scr);
    init_ohiggins_screen(scr);
    splash_init(scr);

    codex_splash_init(scr);

    // Tap on either splash dismisses it
    if (splash_get_root())
        lv_obj_add_event_cb(splash_get_root(), global_click_cb, LV_EVENT_CLICKED, NULL);
    if (codex_splash_get_root())
        lv_obj_add_event_cb(codex_splash_get_root(), global_click_cb, LV_EVENT_CLICKED, NULL);

    // Logo on top of all containers (inset for rounded corners)
    logo_img = lv_image_create(scr);
    lv_image_set_src(logo_img, &logo_dsc);
    lv_obj_set_pos(logo_img, MARGIN, TITLE_Y - 10);

    // Battery indicator on top of all containers (upper-right, inset)
    battery_img = lv_image_create(scr);
    lv_image_set_src(battery_img, &battery_dscs[0]);
    lv_obj_set_pos(battery_img, SCR_W - 48 - MARGIN, TITLE_Y);

    // Divider under the title row (shared — same y on every screen)
    div_header = make_divider(scr, MARGIN, HDR_DIV_Y, CONTENT_W);

    // Hero clock: huge, centered, shared across every non-splash screen
    lbl_clock = lv_label_create(scr);
    lv_label_set_text(lbl_clock, "--:--");
    lv_obj_set_style_text_font(lbl_clock, &font_styrene_clock, 0);
    lv_obj_set_style_text_color(lbl_clock, COL_TEXT, 0);
    lv_obj_set_width(lbl_clock, CONTENT_W);
    lv_obj_set_style_text_align(lbl_clock, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_pos(lbl_clock, MARGIN, CLOCK_Y);

    div_clock = make_divider(scr, MARGIN, CLOCK_DIV_Y, CONTENT_W);

    // Date line, centered, in green
    lbl_date = lv_label_create(scr);
    lv_label_set_text(lbl_date, "---");
    lv_obj_set_style_text_font(lbl_date, &font_styrene_28, 0);
    lv_obj_set_style_text_color(lbl_date, COL_GREEN, 0);
    lv_obj_set_width(lbl_date, CONTENT_W);
    lv_obj_set_style_text_align(lbl_date, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_pos(lbl_date, MARGIN, DATE_Y);

    div_date = make_divider(scr, MARGIN, DATE_DIV_Y, CONTENT_W);
}

void ui_update(const UsageData* data) {
    if (!data->valid || data->provider_count == 0) return;

    // Claude usage screen (provider 0)
    const ProviderData* pd = &data->providers[0];
    int s_pct = (int)(pd->session_pct + 0.5f);

    lv_label_set_text_fmt(lbl_session_pct, "%d%%", s_pct);
    lv_bar_set_value(bar_session, s_pct, LV_ANIM_ON);
    lv_obj_set_style_bg_color(bar_session, pct_color(pd->session_pct), LV_PART_INDICATOR);

    char buf[48];
    format_reset_time(pd->session_reset_mins, buf, sizeof(buf));
    lv_label_set_text(lbl_session_reset, buf);

    int w_pct = (int)(pd->weekly_pct + 0.5f);
    lv_label_set_text_fmt(lbl_weekly_pct, "%d%%", w_pct);
    lv_bar_set_value(bar_weekly, w_pct, LV_ANIM_ON);
    lv_obj_set_style_bg_color(bar_weekly, pct_color(pd->weekly_pct), LV_PART_INDICATOR);

    format_reset_time(pd->weekly_reset_mins, buf, sizeof(buf));
    lv_label_set_text(lbl_weekly_reset, buf);

    // Mirror the same Claude numbers on the O'Higgins screen's mini-stats
    lv_label_set_text_fmt(lbl_oh_current_pct, "%d%%", s_pct);
    lv_bar_set_value(bar_oh_current, s_pct, LV_ANIM_ON);
    lv_obj_set_style_bg_color(bar_oh_current, pct_color(pd->session_pct), LV_PART_INDICATOR);
    lv_label_set_text_fmt(lbl_oh_weekly_pct, "%d%%", w_pct);
    lv_bar_set_value(bar_oh_weekly, w_pct, LV_ANIM_ON);
    lv_obj_set_style_bg_color(bar_oh_weekly, pct_color(pd->weekly_pct), LV_PART_INDICATOR);

    // Toggle credit mode for Claude
    set_credit_mode(panel_session, panel_weekly, panel_credit, pd->has_credits);
    if (pd->has_credits) {
        static char cbuf[32];
        snprintf(cbuf, sizeof(cbuf), "$%.2f", (double)pd->credits_balance);
        lv_label_set_text(lbl_credit_balance, cbuf);
        lv_obj_set_style_text_color(lbl_credit_balance, status_color(pd->status), 0);
        lv_label_set_text(lbl_credit_plan, pd->plan_type[0] ? pd->plan_type : "");
    }

    // Codex screen (provider 1, if present and ok)
    if (data->provider_count > 1) {
        const ProviderData* cx = &data->providers[1];
        if (cx->ok) {
            int cs_pct = (int)(cx->session_pct + 0.5f);
            lv_label_set_text_fmt(lbl_codex_session_pct, "%d%%", cs_pct);
            lv_bar_set_value(bar_codex_session, cs_pct, LV_ANIM_ON);
            lv_obj_set_style_bg_color(bar_codex_session, pct_color(cx->session_pct), LV_PART_INDICATOR);
            format_reset_time(cx->session_reset_mins, buf, sizeof(buf));
            lv_label_set_text(lbl_codex_session_reset, buf);

            int cw_pct = (int)(cx->weekly_pct + 0.5f);
            lv_label_set_text_fmt(lbl_codex_weekly_pct, "%d%%", cw_pct);
            lv_bar_set_value(bar_codex_weekly, cw_pct, LV_ANIM_ON);
            lv_obj_set_style_bg_color(bar_codex_weekly, pct_color(cx->weekly_pct), LV_PART_INDICATOR);
            format_reset_time(cx->weekly_reset_mins, buf, sizeof(buf));
            lv_label_set_text(lbl_codex_weekly_reset, buf);

            // Toggle credit mode for Codex
            set_credit_mode(panel_codex_session, panel_codex_weekly, panel_codex_credit, cx->has_credits);
            if (cx->has_credits) {
                static char cbuf[32];
                snprintf(cbuf, sizeof(cbuf), "$%.2f", (double)cx->credits_balance);
                lv_label_set_text(lbl_codex_credit_balance, cbuf);
                lv_obj_set_style_text_color(lbl_codex_credit_balance, status_color(cx->status), 0);
                lv_label_set_text(lbl_codex_credit_plan, cx->plan_type[0] ? cx->plan_type : "");
            }
        }
    }
}

void ui_tick_anim(void) {
    if (current_screen != SCREEN_USAGE) return;

    uint32_t now = lv_tick_get();

    if (now - anim_msg_start >= ANIM_MSG_MS) {
        anim_msg_idx = (anim_msg_idx + 1) % ANIM_MSG_COUNT;
        anim_msg_start = now;
    }

    if (now - anim_last_ms >= spinner_ms[anim_spinner_idx]) {
        anim_last_ms = now;
        anim_phase = (anim_phase + 1) % SPINNER_PHASES;
        anim_spinner_idx = (anim_phase < SPINNER_COUNT) ? anim_phase
                                                        : (SPINNER_PHASES - anim_phase);

        static char buf[80];
        snprintf(buf, sizeof(buf), "%s %s\xE2\x80\xA6",
                 spinner_frames[anim_spinner_idx],
                 anim_messages[anim_msg_idx]);
        lv_label_set_text(lbl_anim, buf);
    }
}

static screen_t prev_non_splash_screen = SCREEN_USAGE;
// Hide the battery indicator and the hero clock/date block on the splash
// screen — they're visually noisy over the pixel-art creature animations.
// The O'Higgins screen also hides the shared date/battery (it draws its own
// two-line date and has no device-battery row) but keeps the shared clock.
static void apply_battery_visibility(void) {
    if (!battery_img) return;
    bool splash_mode = (current_screen == SCREEN_SPLASH || current_screen == SCREEN_CODEX_SPLASH);
    bool ohiggins_mode = (current_screen == SCREEN_OHIGGINS);

    lv_obj_t* clock_only_hidden_on[] = { lbl_clock, div_header, div_clock };
    for (lv_obj_t* obj : clock_only_hidden_on) {
        if (!obj) continue;
        if (splash_mode) lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
        else              lv_obj_clear_flag(obj, LV_OBJ_FLAG_HIDDEN);
    }

    lv_obj_t* hidden_on_splash_and_ohiggins[] = { battery_img, lbl_date, div_date };
    for (lv_obj_t* obj : hidden_on_splash_and_ohiggins) {
        if (!obj) continue;
        if (splash_mode || ohiggins_mode) lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
        else                              lv_obj_clear_flag(obj, LV_OBJ_FLAG_HIDDEN);
    }
}

static bool ui_claude_enabled(void) {
    return web_server_claude_visible();
}

static bool ui_codex_enabled(void) {
    return web_server_codex_visible() && codex_available;
}

static bool ui_generic_enabled(void) {
    return generic_provider_available &&
           web_server_provider_visible(generic_provider_data.name);
}

static bool ui_selected_generic_has_splash(void) {
    if (!ui_generic_enabled()) return false;
    const char* sel = web_server_selected_provider_name();
    if (!sel || strcmp(sel, "claude") == 0 || strcmp(sel, "codex") == 0) return false;
    return splash_has_custom_for(sel);
}

static bool ui_splash_enabled(void) {
    return ui_claude_enabled() || ui_selected_generic_has_splash();
}

static screen_t preferred_provider_usage_screen(void) {
    const char* sel = web_server_selected_provider_name();
    // Check if selection matches a specific provider
    if (strcmp(sel, "codex") == 0) {
        if (ui_codex_enabled())   return SCREEN_CODEX;
        if (ui_generic_enabled()) return SCREEN_PROVIDER;
        if (ui_claude_enabled())  return SCREEN_USAGE;
    } else if (strcmp(sel, "claude") == 0) {
        if (ui_claude_enabled())  return SCREEN_USAGE;
        if (ui_codex_enabled())   return SCREEN_CODEX;
        if (ui_generic_enabled()) return SCREEN_PROVIDER;
    } else {
        // selected is a generic provider
        if (ui_generic_enabled()) return SCREEN_PROVIDER;
        if (ui_claude_enabled())  return SCREEN_USAGE;
        if (ui_codex_enabled())   return SCREEN_CODEX;
    }
    return SCREEN_NETWORK;
}

static screen_t fallback_for_hidden_screen(screen_t screen) {
    switch (screen) {
    case SCREEN_SPLASH:
    case SCREEN_USAGE:
    case SCREEN_CODEX_SPLASH:
    case SCREEN_CODEX:
    case SCREEN_PROVIDER:
        return preferred_provider_usage_screen();
    case SCREEN_NETWORK:
    default:
        return SCREEN_NETWORK;
    }
}

static bool screen_is_visible(screen_t screen) {
    switch (screen) {
    case SCREEN_SPLASH:
    case SCREEN_USAGE:
        return screen == SCREEN_SPLASH ? ui_splash_enabled() : ui_claude_enabled();
    case SCREEN_CODEX_SPLASH:
    case SCREEN_CODEX:
        return ui_codex_enabled();
    case SCREEN_PROVIDER:
        return ui_generic_enabled();
    case SCREEN_NETWORK:
    case SCREEN_OHIGGINS:
        return true;
    default:
        return false;
    }
}

// LVGL handles click debouncing internally. Screen-level handler fires when
// no child consumed the event (children only consume if they have their own
// event callback, e.g. the Reset Bluetooth zone). On BT screen we skip the
// splash toggle so only the reset zone is interactive there.
static void global_click_cb(lv_event_t* e) {
    (void)e;
    if (ui_get_current_screen() == SCREEN_NETWORK) return;
    ui_toggle_splash();
}

void ui_show_screen(screen_t screen) {
    lv_obj_add_flag(usage_container,    LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(codex_container,    LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(provider_container, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(net_container,      LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(ohiggins_container, LV_OBJ_FLAG_HIDDEN);
    if (codex_icon_img)    lv_obj_add_flag(codex_icon_img, LV_OBJ_FLAG_HIDDEN);
    if (ohiggins_icon_img) lv_obj_add_flag(ohiggins_icon_img, LV_OBJ_FLAG_HIDDEN);
    splash_hide();
    codex_splash_hide();

    switch (screen) {
    case SCREEN_SPLASH:
        splash_show();
        break;
    case SCREEN_USAGE:
        lv_obj_clear_flag(usage_container, LV_OBJ_FLAG_HIDDEN);
        break;
    case SCREEN_CODEX:
        lv_obj_clear_flag(codex_container, LV_OBJ_FLAG_HIDDEN);
        if (codex_icon_img) lv_obj_clear_flag(codex_icon_img, LV_OBJ_FLAG_HIDDEN);
        break;
    case SCREEN_CODEX_SPLASH:
        codex_splash_show();
        break;
    case SCREEN_PROVIDER:
        lv_obj_clear_flag(provider_container, LV_OBJ_FLAG_HIDDEN);
        break;
    case SCREEN_NETWORK:
        lv_obj_clear_flag(net_container, LV_OBJ_FLAG_HIDDEN);
        break;
    case SCREEN_OHIGGINS:
        lv_obj_clear_flag(ohiggins_container, LV_OBJ_FLAG_HIDDEN);
        if (ohiggins_icon_img) lv_obj_clear_flag(ohiggins_icon_img, LV_OBJ_FLAG_HIDDEN);
        break;
    default:
        break;
    }

    // Logo: hidden on splash screens, Codex screen (cloud icon), generic
    // provider screen, and the O'Higgins screen (crest icon instead)
    if (logo_img) {
        bool hide_logo = (screen == SCREEN_SPLASH || screen == SCREEN_CODEX ||
                          screen == SCREEN_CODEX_SPLASH || screen == SCREEN_PROVIDER ||
                          screen == SCREEN_OHIGGINS);
        if (hide_logo) lv_obj_add_flag(logo_img, LV_OBJ_FLAG_HIDDEN);
        else           lv_obj_clear_flag(logo_img, LV_OBJ_FLAG_HIDDEN);
    }

    bool is_splash = (screen == SCREEN_SPLASH || screen == SCREEN_CODEX_SPLASH);
    if (!is_splash) prev_non_splash_screen = screen;
    current_screen = screen;
    apply_battery_visibility();
}

void ui_cycle_screen(void) {
    bool show_claude   = ui_claude_enabled();
    bool show_codex    = ui_codex_enabled();
    bool show_generic  = ui_generic_enabled();

    screen_t next;
    if (current_screen == SCREEN_USAGE) {
        if (show_codex)   next = SCREEN_CODEX;
        else if (show_generic) next = SCREEN_PROVIDER;
        else              next = SCREEN_NETWORK;
    } else if (current_screen == SCREEN_CODEX) {
        next = show_generic ? SCREEN_PROVIDER : SCREEN_NETWORK;
    } else if (current_screen == SCREEN_PROVIDER) {
        next = SCREEN_NETWORK;
    } else if (current_screen == SCREEN_NETWORK) {
        next = SCREEN_OHIGGINS;
    } else if (current_screen == SCREEN_OHIGGINS) {
        next = preferred_provider_usage_screen();
    } else {
        next = preferred_provider_usage_screen();
    }
    ui_show_screen(next);
}

void ui_set_codex_available(bool available) {
    bool changed = (codex_available != available);
    codex_available = available;
    if (changed) {
        ui_reconcile_provider_visibility(true);
    }
}

void ui_reconcile_provider_visibility(bool prefer_primary_provider) {
    screen_t target = current_screen;

    if (prefer_primary_provider) {
        target = preferred_provider_usage_screen();
    } else if (!screen_is_visible(current_screen)) {
        target = fallback_for_hidden_screen(current_screen);
    } else if (current_screen == SCREEN_NETWORK && preferred_provider_usage_screen() != SCREEN_NETWORK) {
        target = preferred_provider_usage_screen();
    }

    if (target != current_screen) {
        ui_show_screen(target);
    }
}

void ui_toggle_splash(void) {
    bool claude_on  = ui_claude_enabled();
    bool codex_on   = ui_codex_enabled();
    bool generic_on = ui_generic_enabled();

    if (current_screen == SCREEN_SPLASH) {
        if (claude_on)       ui_show_screen(SCREEN_USAGE);
        else if (generic_on) ui_show_screen(SCREEN_PROVIDER);
        else if (codex_on)   ui_show_screen(SCREEN_CODEX_SPLASH);
    } else if (current_screen == SCREEN_USAGE) {
        if (codex_on)        ui_show_screen(SCREEN_CODEX_SPLASH);
        else if (generic_on) ui_show_screen(SCREEN_PROVIDER);
        else if (claude_on)  ui_show_screen(SCREEN_SPLASH);
    } else if (current_screen == SCREEN_CODEX_SPLASH) {
        if (codex_on)        ui_show_screen(SCREEN_CODEX);
        else if (claude_on)  ui_show_screen(SCREEN_SPLASH);
        else if (generic_on) ui_show_screen(SCREEN_PROVIDER);
    } else if (current_screen == SCREEN_CODEX) {
        if (generic_on)      ui_show_screen(SCREEN_PROVIDER);
        else if (claude_on)  ui_show_screen(SCREEN_SPLASH);
        else if (codex_on)   ui_show_screen(SCREEN_CODEX_SPLASH);
    } else if (current_screen == SCREEN_PROVIDER) {
        if (ui_selected_generic_has_splash()) ui_show_screen(SCREEN_SPLASH);
        else if (claude_on)  ui_show_screen(SCREEN_SPLASH);
        else if (codex_on)   ui_show_screen(SCREEN_CODEX_SPLASH);
        else if (generic_on) ui_show_screen(SCREEN_PROVIDER);
    } else {
        screen_t preferred = preferred_provider_usage_screen();
        if      (preferred == SCREEN_CODEX    && codex_on)   ui_show_screen(SCREEN_CODEX_SPLASH);
        else if (preferred == SCREEN_USAGE    && ui_splash_enabled()) ui_show_screen(SCREEN_SPLASH);
        else if (preferred == SCREEN_PROVIDER && generic_on) ui_show_screen(SCREEN_PROVIDER);
    }
}

screen_t ui_get_current_screen(void) {
    return current_screen;
}

void ui_update_network_status(bool connected, const char* ssid, const char* ip, int rssi) {
    if (connected) {
        lv_label_set_text(lbl_net_status, "Connected");
        lv_obj_set_style_text_color(lbl_net_status, COL_GREEN, 0);
    } else {
        lv_label_set_text(lbl_net_status, "Disconnected");
        lv_obj_set_style_text_color(lbl_net_status, COL_RED, 0);
    }

    if (ssid) {
        static char sbuf[48];
        snprintf(sbuf, sizeof(sbuf), "SSID: %s", ssid);
        lv_label_set_text(lbl_net_ssid, sbuf);
    }
    if (ip) {
        static char ibuf[48];
        snprintf(ibuf, sizeof(ibuf), "IP: %s", ip);
        lv_label_set_text(lbl_net_ip, ibuf);
    }
    static char rbuf[32];
    snprintf(rbuf, sizeof(rbuf), "RSSI: %d dBm", rssi);
    lv_label_set_text(lbl_net_rssi, rbuf);
}

void ui_update_battery(int percent, bool charging) {
    int idx;
    if (charging) {
        idx = 4;  // charging icon
    } else if (percent < 0) {
        idx = 0;  // no battery / unknown
    } else if (percent <= 10) {
        idx = 0;  // empty
    } else if (percent <= 35) {
        idx = 1;  // low
    } else if (percent <= 75) {
        idx = 2;  // medium
    } else {
        idx = 3;  // full
    }
    lv_image_set_src(battery_img, &battery_dscs[idx]);
    apply_battery_visibility();
}

void ui_update_clock(const char* time_str) {
    lv_label_set_text(lbl_clock, time_str);
}

void ui_update_date(const char* date_str) {
    lv_label_set_text(lbl_date, date_str);
}

void ui_update_ohiggins_date(const char* date_str) {
    lv_label_set_text(lbl_oh_date, date_str);
}

void ui_update_weather(const WeatherData* w) {
    if (!w || !w->ok) return;
    lv_label_set_text_fmt(lbl_oh_temp, "%d\xC2\xB0" "C", (int)(w->temp_c + 0.5f));
    lv_label_set_text(lbl_oh_desc, w->description);
    lv_label_set_text_fmt(lbl_oh_humidity_wind, "%d%%  -  %d km/h",
                          w->humidity_pct, (int)(w->wind_kmh + 0.5f));
}

void ui_set_generic_provider(const ProviderData* pd) {
    bool was_available = generic_provider_available;
    if (!pd || !pd->ok) {
        generic_provider_available = false;
        memset(&generic_provider_data, 0, sizeof(generic_provider_data));
        if (was_available) ui_reconcile_provider_visibility(false);
        return;
    }

    generic_provider_available = true;
    generic_provider_data = *pd;

    // Update the screen widgets
    lv_color_t accent = provider_accent_color(pd->name);
    lv_label_set_text(lbl_provider_title, provider_display_name(pd->name));
    lv_obj_set_style_text_color(lbl_provider_title, accent, 0);

    char buf[48];
    int s_pct = (int)(pd->session_pct + 0.5f);
    lv_label_set_text_fmt(lbl_provider_session_pct, "%d%%", s_pct);
    lv_obj_set_style_text_color(lbl_provider_session_pct, pct_color(pd->session_pct), 0);
    lv_bar_set_value(bar_provider_session, s_pct, LV_ANIM_ON);
    lv_obj_set_style_bg_color(bar_provider_session, pct_color(pd->session_pct), LV_PART_INDICATOR);
    format_reset_time(pd->session_reset_mins, buf, sizeof(buf));
    lv_label_set_text(lbl_provider_session_reset, buf);

    int w_pct = (int)(pd->weekly_pct + 0.5f);
    lv_label_set_text_fmt(lbl_provider_weekly_pct, "%d%%", w_pct);
    lv_obj_set_style_text_color(lbl_provider_weekly_pct, pct_color(pd->weekly_pct), 0);
    lv_bar_set_value(bar_provider_weekly, w_pct, LV_ANIM_ON);
    lv_obj_set_style_bg_color(bar_provider_weekly, pct_color(pd->weekly_pct), LV_PART_INDICATOR);
    format_reset_time(pd->weekly_reset_mins, buf, sizeof(buf));
    lv_label_set_text(lbl_provider_weekly_reset, buf);

    bool detail_panel_mode = pd->has_credits || !pd->metrics_available;
    set_credit_mode(panel_provider_session, panel_provider_weekly, panel_provider_credit, detail_panel_mode);
    if (pd->has_credits) {
        static char cbuf[32];
        snprintf(cbuf, sizeof(cbuf), "$%.4f", (double)pd->credits_balance);
        lv_label_set_text(lbl_provider_credit_balance, cbuf);
        lv_obj_set_style_text_color(lbl_provider_credit_balance, status_color(pd->status), 0);
        lv_label_set_text(lbl_provider_credit_plan, pd->plan_type[0] ? pd->plan_type : "");
    } else if (!pd->metrics_available) {
        lv_label_set_text(lbl_provider_credit_balance, "Available");
        lv_obj_set_style_text_color(lbl_provider_credit_balance, accent, 0);
        lv_label_set_text(lbl_provider_credit_plan,
                          pd->note[0] ? pd->note : "Usage metrics unavailable");
    }

    if (!was_available) ui_reconcile_provider_visibility(false);
}
