#pragma once

#include <stdint.h>

struct rr_game;
struct rr_ui_element;
struct rr_game_squad_client;

struct rr_ui_choose_element_metadata
{
    uint8_t (*choose)(struct rr_ui_element *, struct rr_game *);
    struct rr_ui_element *a;
    struct rr_ui_element *b;
    void *data;
};

struct rr_ui_container_metadata
{
    uint8_t type;
    uint8_t width; // for 2D containers
    uint8_t height;
    float outer_spacing;
    float inner_spacing;
    uint32_t size;
    uint32_t capacity;
    struct rr_ui_element **start;
};

struct rr_ui_element
{
    struct rr_ui_element *container;
    void *data;
    uint8_t *hidden;
    uint8_t (*should_show)(struct rr_ui_element *, struct rr_game *);
    uint8_t (*animate)(struct rr_ui_element *, struct rr_game *);
    void (*on_render)(struct rr_ui_element *, struct rr_game *);
    void (*on_event)(struct rr_ui_element *, struct rr_game *);
    void (*poll_events)(struct rr_ui_element *, struct rr_game *);
    float x;
    float y;
    float width;
    float abs_width;
    float height;
    float abs_height;
    float abs_x;
    float abs_y;
    float animation;
    uint32_t fill;
    int8_t h_justify;
    int8_t v_justify;
    uint8_t resizeable;
    uint8_t completely_hidden;
    uint8_t first_frame;
};

void rr_ui_render_element(struct rr_ui_element *, struct rr_game *);
uint8_t rr_ui_mouse_over(struct rr_ui_element *, struct rr_game *);
void rr_ui_element_check_if_focused(struct rr_ui_element *, struct rr_game *);
void rr_ui_no_focus(struct rr_ui_element *, struct rr_game *);
struct rr_ui_element *rr_ui_link_toggle(struct rr_ui_element *, uint8_t *);

struct rr_ui_element *rr_ui_element_init();
struct rr_ui_element *rr_ui_static_space_init(float);

struct rr_ui_element *rr_ui_container_init();
struct rr_ui_element *rr_ui_flex_container_init();
struct rr_ui_element *rr_ui_2d_container_init(uint8_t, uint8_t, float, float);
struct rr_ui_element *rr_ui_container_add_element(struct rr_ui_element *,
                                                  struct rr_ui_element *);

struct rr_ui_element *rr_ui_text_init(char const *, float, uint32_t);

struct rr_ui_element *rr_ui_labeled_button_init(char const *, float, uint8_t *);

struct rr_ui_element *rr_ui_choose_element_init(
    struct rr_ui_element *, struct rr_ui_element *,
    uint8_t (*)(struct rr_ui_element *, struct rr_game *));

// custom
struct rr_ui_element *
rr_ui_squad_player_container_init(struct rr_game_squad_client *);
struct rr_ui_element *rr_ui_countdown_init(struct rr_game *);
struct rr_ui_element *rr_ui_info_init();

struct rr_ui_element *rr_ui_inventory_container_init();

struct rr_ui_element *rr_ui_title_screen_loadout_button_init(uint8_t);
struct rr_ui_element *rr_ui_loadout_button_init(uint8_t);

struct rr_ui_element *rr_ui_scroll_container_init(struct rr_ui_element *,
                                                  float);