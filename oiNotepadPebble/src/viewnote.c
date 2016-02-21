#include <pebble.h>

#include "oinotepad.h"

static Window *window;
static ScrollLayer *scroll;
static TextLayer *text;
static char *windowNoteContents;

extern GFont font;

static void windowLoad(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  scroll = scroll_layer_create((GRect) {
      .origin = { 0, 0 },
      .size = { bounds.size.w, bounds.size.h }
    });

  // Allow text layer to be up to 2000 px high
  text = text_layer_create((GRect) {
      .origin = { 0, 0 },
      .size = { bounds.size.w, 2000 }
    });

  text_layer_set_text(text, windowNoteContents);

  text_layer_set_font(text, font);
  text_layer_set_overflow_mode(text, GTextOverflowModeWordWrap);

  scroll_layer_add_child(scroll, text_layer_get_layer(text));

  // Resize text layer to minimum needed, including 10 px padding for text
  // that goes below the last baseline
  GSize max_size = text_layer_get_content_size(text);
  text_layer_set_size(text, max_size);
  scroll_layer_set_content_size(scroll, GSize(bounds.size.w, max_size.h + 10));

  layer_add_child(window_layer, scroll_layer_get_layer(scroll));

  text_layer_enable_screen_text_flow_and_paging(text, 5);

  scroll_layer_set_click_config_onto_window(scroll, window);

}

static void windowUnload(Window *window) {
  text_layer_destroy(text);
  scroll_layer_destroy(scroll);
  if (windowNoteContents != NULL) {
    free(windowNoteContents);
    windowNoteContents = NULL;
  }
}

// -----------------------------------------------
// init/deinit

void viewnote_init(char *noteStr) {

  windowNoteContents = noteStr;

  window = window_create();
  window_set_window_handlers(window, (WindowHandlers) {
    .load = windowLoad,
    .unload = windowUnload
  });

  window_stack_push(window, true);

}

void viewnote_deinit(void) {
  window_destroy(window);
}
