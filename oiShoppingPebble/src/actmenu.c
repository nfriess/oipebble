#include <pebble.h>

#include "oishopping.h"

static Window *window = NULL;
static SimpleMenuLayer *menu = NULL;
static DictationSession *dictSession;

extern int32_t activeShoppingListID;

extern void listsmenu_init();
extern void comms_addItem(int32_t listID, char *name);
extern void msgwin_show(char *message);

// -----------------------------------------------
// Internal functions

static void onDictation(DictationSession *session, DictationSessionStatus status, char *transcription, void *context) {

  if (status == DictationSessionStatusSuccess) {

    APP_LOG(APP_LOG_LEVEL_DEBUG, "Dictation text: %s", transcription);

    comms_addItem(activeShoppingListID, transcription);

  }
  else {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Dictation result: %d", status);

    msgwin_show("Dictation failed");

  }

  dictation_session_destroy(dictSession);

}

static void onSelect(int index, void *context) {

  window_stack_pop(true);
  window_destroy(window);

  // Do action...

  if (index == 0) {
    // Add item

    dictSession = dictation_session_create(1024, onDictation, NULL);

    dictation_session_start(dictSession);

  }
  else if (index == 1) {
    // Select shopping list
    listsmenu_init();
  }
  else if (index == 2) {
    // Clean up list
  }

}

static void windowLoad(Window *window) {

  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  SimpleMenuSection *sections;
  SimpleMenuItem *items;

  items = calloc(3, sizeof(SimpleMenuItem));
  sections = calloc(1, sizeof(SimpleMenuSection));

  memset(sections, 0, 1*sizeof(SimpleMenuSection));
  sections->title = "Select an action";
  sections->num_items = 3;
  sections->items = items;

  memset(items, 0, 3*sizeof(SimpleMenuItem));

  items[0].title = "Add Item";
  items[0].callback = onSelect;

  items[1].title = "Change List";
  items[1].callback = onSelect;

  items[2].title = "Clean Up Items";
  items[2].callback = onSelect;

  menu = simple_menu_layer_create((GRect) {
      .origin = { 0, 0 },
      .size = { bounds.size.w, bounds.size.h }
    },
    window,
    sections,
    1,
    NULL
    );

  menu_layer_set_highlight_colors(simple_menu_layer_get_menu_layer(menu),
				  GColorChromeYellow, GColorWhite);

  layer_add_child(window_layer, simple_menu_layer_get_layer(menu));

}

static void windowUnload(Window *window) {
  simple_menu_layer_destroy(menu);
  menu = NULL;
}

// -----------------------------------------------
// init/deinit

void actmenu_init(void) {

  window = window_create();
  window_set_window_handlers(window, (WindowHandlers) {
    .load = windowLoad,
    .unload = windowUnload
  });

  window_stack_push(window, true);

}

/*void actmenu_deinit(void) {
  window_destroy(window);
  }*/
