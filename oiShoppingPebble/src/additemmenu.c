#include <pebble.h>

#include "oishopping.h"

static Window *window;
static MenuLayer *menu;

extern int32_t activeShoppingListID;
extern ShoppingItem *addItemList;
extern uint8_t addItemListCount;

extern GFont font;

extern void comms_addItemById(int32_t listID, int itemID);

// -----------------------------------------------
// Internal functions

static uint16_t menuGetNumRows(struct MenuLayer *menu_layer, uint16_t section_index, void *callback_context) {

  return addItemListCount;
}

static int16_t menuGetCellHeight(struct MenuLayer *menu_layer, MenuIndex *cell_index, void *callback_context) {

  if (cell_index->row >= addItemListCount)
    return 10;

  GRect menu_bounds = layer_get_bounds(menu_layer_get_layer(menu_layer));

  GSize sz = graphics_text_layout_get_content_size(addItemList[cell_index->row].name,
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

static void menuDrawRow(GContext *ctx, const Layer *cell_layer, MenuIndex *cell_index, void *callback_context) {

  if (cell_index->row >= addItemListCount)
    return;

  GRect cell_bounds = layer_get_bounds(cell_layer);

  graphics_draw_text(ctx, addItemList[cell_index->row].name,
		     font, cell_bounds,
		     GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);

}

static void menuOnSelect(struct MenuLayer *menu_layer, MenuIndex *cell_index, void *callback_context) {

  if (cell_index->row >= addItemListCount)
    return;

  APP_LOG(APP_LOG_LEVEL_DEBUG, "Complete add item %ld", addItemList[cell_index->row].id);

  comms_addItemById(activeShoppingListID, addItemList[cell_index->row].id);

  free(addItemList);
  addItemList = NULL;
  addItemListCount = 0;

  window_stack_pop(true);
  window_destroy(window);

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
      .select_click = menuOnSelect
  });

  menu_layer_set_highlight_colors(menu, GColorScreaminGreen, GColorWhite);

  layer_add_child(window_layer, menu_layer_get_layer(menu));

  menu_layer_set_click_config_onto_window(menu, window);

}

static void windowUnload(Window *window) {
  menu_layer_destroy(menu);
}

// -----------------------------------------------
// init/deinit

void additem_init(void) {

  window = window_create();
  window_set_window_handlers(window, (WindowHandlers) {
    .load = windowLoad,
    .unload = windowUnload
  });

  window_stack_push(window, true);

}

/*void additem_deinit(void) {
  window_destroy(window);
  }*/
