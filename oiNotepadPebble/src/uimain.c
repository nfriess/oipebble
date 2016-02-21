#include <pebble.h>

#include "oinotepad.h"

GFont font;

static Window *window;
static MenuLayer *menu;

extern NoteItem *notes;
extern uint8_t noteCount;

extern void comms_getNote(int32_t noteID);

// -----------------------------------------------
// Public API (other than init/deinit)

void ui_refreshItems(int selectPos) {
  if (selectPos != -1) {
    MenuIndex idx;
    idx.section = 0;
    idx.row = selectPos;
    menu_layer_set_selected_index(menu, idx, MenuRowAlignTop, false);
  }
  menu_layer_reload_data(menu);
}

void ui_replaceWithMessage(const char *message) {

  if (noteCount > 0) {

    int needReload = 0;

    if (noteCount > 1) {
      noteCount = 1;
      needReload = 1;
    }

    if (strcmp(message, notes[0].title)) {
      strncpy(notes[0].title, message, MAX_TITLE_LEN);
      needReload = 1;
    }

    if (needReload) {
      ui_refreshItems(0);
    }

  }

}


// -----------------------------------------------
// Internal functions

static uint16_t menuGetNumRows(struct MenuLayer *menu_layer, uint16_t section_index, void *callback_context) {

  return noteCount;
}

static int16_t menuGetCellHeight(struct MenuLayer *menu_layer, MenuIndex *cell_index, void *callback_context) {

  if (cell_index->row >= noteCount)
    return 10;

  GRect menu_bounds = layer_get_bounds(menu_layer_get_layer(menu_layer));

  GSize sz = graphics_text_layout_get_content_size(notes[cell_index->row].title,
						   font, menu_bounds,
						   GTextOverflowModeTrailingEllipsis,
						   GTextAlignmentLeft);

  // Leave some space around the menu item and truncate text if needed
  if (sz.h >= (menu_bounds.size.h - 40)) {
    sz.h = menu_bounds.size.h - 40;
  }
  // And set a minimum size
  else if (sz.h < 10) {
    // We add another 10 below...
    sz.h = 10;
  }

  // sz seems to leave 10 px on the top of letters but not enough space
  // for letters that are below the baseline.  So we add 10 balance this out.
  return sz.h + 10;
}

static void menuDrawRow(GContext *ctx, const Layer *cell_layer, MenuIndex *cell_index, void *callback_context) {

  if (cell_index->row >= noteCount)
    return;

  GRect cell_bounds = layer_get_bounds(cell_layer);

  graphics_draw_text(ctx, notes[cell_index->row].title,
		     font, cell_bounds,
		     GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);

}

static void menuOnSelect(struct MenuLayer *menu_layer, MenuIndex *cell_index, void *callback_context) {

  if (cell_index->row >= noteCount)
    return;

  comms_getNote(notes[cell_index->row].id);

}

static void menuOnSelectLong(struct MenuLayer *menu_layer, MenuIndex *cell_index, void *callback_context) {

  // TODO: Bring up menu with delete option?

}

static void windowLoad(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  menu = menu_layer_create((GRect) {
      .origin = { 0, 0 },
      .size = { bounds.size.w, bounds.size.h }
  });

  menu_layer_set_callbacks(menu, NULL, (MenuLayerCallbacks) {
      .get_num_rows = menuGetNumRows,
      .get_cell_height = menuGetCellHeight,
      .draw_row = menuDrawRow,
      .select_click = menuOnSelect,
      .select_long_click = menuOnSelectLong
  });

  menu_layer_set_highlight_colors(menu, GColorBlueMoon, GColorWhite);

  layer_add_child(window_layer, menu_layer_get_layer(menu));

  menu_layer_set_click_config_onto_window(menu, window);

}

static void windowUnload(Window *window) {
  menu_layer_destroy(menu);
}

// -----------------------------------------------
// init/deinit

void ui_init(void) {

  font = fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD);

  window = window_create();
  window_set_window_handlers(window, (WindowHandlers) {
    .load = windowLoad,
    .unload = windowUnload
  });

  window_stack_push(window, true);

}

void ui_deinit(void) {
  window_destroy(window);
}
