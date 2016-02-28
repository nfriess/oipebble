#include <pebble.h>

#include "oishopping.h"

static Window *window = NULL;
static MenuLayer *menu = NULL;

extern int32_t activeShoppingListID;
extern ShoppingList *allLists;
extern uint8_t allListsCount;
extern uint8_t isRound;
extern GFont font;

extern void comms_getItemList(uint32_t listID);
extern void comms_getShoppingLists();
extern void ui_replaceWithMessage(const char *message);

// -----------------------------------------------
// Public API (other than init/deinit)

void listsmenu_refreshItems() {
  if (menu != NULL) {
    menu_layer_reload_data(menu);
  }
}

// -----------------------------------------------
// Internal functions

static uint16_t menuGetNumRows(struct MenuLayer *menu_layer, uint16_t section_index, void *callback_context) {

  return allListsCount;
}

static int16_t menuGetCellHeight(struct MenuLayer *menu_layer, MenuIndex *cell_index, void *callback_context) {

  if (cell_index->row >= allListsCount)
    return 10;

  GRect menu_bounds = layer_get_bounds(menu_layer_get_layer(menu_layer));

  GSize sz = graphics_text_layout_get_content_size(allLists[cell_index->row].name,
						   font, menu_bounds,
						   GTextOverflowModeTrailingEllipsis,
						   GTextAlignmentLeft);

  // Leave some space around the menu item and truncate text if needed
  if (sz.h >= (menu_bounds.size.h - 40)) {
    sz.h = menu_bounds.size.h - 40;
  }
  // And ensure there is enough space for the checkbox at minimum
  else if (sz.h < 10) {
    // We add another 10 below...
    sz.h = 10;
  }

  // sz seems to leave 10 px on the top of letters but not enough space
  // for letters that are below the baseline.  So we add 10 balance this out.
  return sz.h + 10;
}

static int16_t menuGetHeaderHeight(struct MenuLayer *menu_layer, uint16_t section_index, void *callback_context) {

  // A guess...
  return 16;
}

static void menuDrawRow(GContext *ctx, const Layer *cell_layer, MenuIndex *cell_index, void *callback_context) {

  if (cell_index->row >= allListsCount)
    return;

  GRect cell_bounds = layer_get_bounds(cell_layer);

  if (isRound) {
    // Add some padding so text is not clipped
    cell_bounds.origin.x += 6;
    cell_bounds.size.w -= 12;
  }

  graphics_draw_text(ctx, allLists[cell_index->row].name,
		     font, cell_bounds,
		     GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);

}

static void menuDrawHeader(GContext *ctx, const Layer *cell_layer, uint16_t section_index, void *callback_context) {

  menu_cell_basic_header_draw(ctx, cell_layer, "Shopping Lists");

}

static void menuOnSelect(struct MenuLayer *menu_layer, MenuIndex *cell_index, void *callback_context) {

  if (cell_index->row >= allListsCount)
    return;

  APP_LOG(APP_LOG_LEVEL_DEBUG, "Shopping list %i selected", cell_index->row);

  activeShoppingListID = allLists[cell_index->row].id;

  persist_write_int(STORAGE_ACTIVELISTID, activeShoppingListID);

  window_stack_pop(true);
  window_destroy(window);

  allListsCount = 0;
  free(allLists);
  allLists = NULL;

  ui_replaceWithMessage("Loading...");
  comms_getItemList(activeShoppingListID);

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
      .get_header_height = menuGetHeaderHeight,
      .draw_row = menuDrawRow,
      .draw_header = menuDrawHeader,
      .select_click = menuOnSelect
  });

  menu_layer_set_highlight_colors(menu, GColorChromeYellow, GColorWhite);

  layer_add_child(window_layer, menu_layer_get_layer(menu));

  menu_layer_set_click_config_onto_window(menu, window);

}

static void windowUnload(Window *window) {
  menu_layer_destroy(menu);
  menu = NULL;
}

static void getListsTimeout(void *data) {
  comms_getShoppingLists();
}

// -----------------------------------------------
// init/deinit

void listsmenu_init(void) {

  window = window_create();
  window_set_window_handlers(window, (WindowHandlers) {
    .load = windowLoad,
    .unload = windowUnload
  });

  window_stack_push(window, true);

  app_timer_register(100/*ms*/, getListsTimeout, NULL);

}

/*void listsmenu_deinit(void) {
  window_destroy(window);
  }*/
