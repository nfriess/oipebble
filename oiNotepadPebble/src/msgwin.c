#include <pebble.h>

extern uint8_t doneInit;

static Window *window = NULL;
static TextLayer *textLayer;
static AppTimer *hideTimer;

void msgwin_hide();

#define MSGWIN_TIMEOUT 6000

// -----------------------------------------------
// Internal functions

static void windowLoad(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  bounds.origin.x += 10;
  bounds.origin.y += 20;
  bounds.size.w -= 20;
  bounds.size.h -= 40;

  textLayer = text_layer_create(bounds);

  GFont font = fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD);

  text_layer_set_font(textLayer, font);

  text_layer_set_text_alignment(textLayer, GTextAlignmentLeft);

  layer_add_child(window_layer, text_layer_get_layer(textLayer));

}

static void windowUnload(Window *window) {
  text_layer_destroy(textLayer);

  // If this was the initial loading message... then quit now
  if (!doneInit)
    window_stack_pop_all(false);
}

static void onHideTimeout(void *data) {
  hideTimer = NULL;
  msgwin_hide();
}

// -----------------------------------------------
// init/deinit

static void msgwin_init(void) {

  window = window_create();
  window_set_window_handlers(window, (WindowHandlers) {
    .load = windowLoad,
    .unload = windowUnload
  });

  window_stack_push(window, true);

}

void msgwin_deinit(void) {
  if (window != NULL) {
    window_destroy(window);
  }
}


// -----------------------------------------------
// Public API

void msgwin_show(char *message) {

  if (window == NULL) {
    msgwin_init();
  }

  text_layer_set_text(textLayer, message);

  window_stack_push(window, true);

  if (hideTimer != NULL) {
    app_timer_reschedule(hideTimer, MSGWIN_TIMEOUT);
  }
  else {
    hideTimer = app_timer_register(MSGWIN_TIMEOUT, onHideTimeout, NULL);
  }

}

void msgwin_hide() {

  if (hideTimer != NULL) {
    app_timer_cancel(hideTimer);
    hideTimer = NULL;
  }

  window_stack_remove(window, true);

}
