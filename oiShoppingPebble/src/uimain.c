#include <pebble.h>

#include "oishopping.h"

GFont font;

static Window *window;
static MenuLayer *menu;
static GPath *checkboxPath;

static const GPathInfo CHECKBOX_PTS = {
  .num_points = 3,
  .points = (GPoint []) {{4, 9}, {7, 11}, {12, 4}}
};

extern int32_t activeShoppingListID;
extern ShoppingItem *activeList;
extern uint8_t activeListCount;

extern void comms_getItemList(int32_t listID);
extern void comms_setItemBought(int32_t listID, int32_t itemID);
extern void comms_setItemToBuy(int32_t listID, int32_t itemID);

extern void actmenu_init();

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

  if (activeListCount > 0) {

    int needReload = 0;

    if (activeListCount > 1) {
      activeListCount = 1;
      needReload = 1;
    }

    if (strcmp(message, activeList[0].name)) {
      strncpy(activeList[0].name, message, MAX_NAME_LEN);
      needReload = 1;
    }

    activeList[0].status = STATUS_WANT_TO_BUY;

    if (needReload) {
      ui_refreshItems(0);
    }

  }

}

// -----------------------------------------------
// Internal functions

static uint16_t menuGetNumRows(struct MenuLayer *menu_layer, uint16_t section_index, void *callback_context) {

  return activeListCount;
}

static int16_t menuGetCellHeight(struct MenuLayer *menu_layer, MenuIndex *cell_index, void *callback_context) {

  if (cell_index->row >= activeListCount)
    return 10;

  GRect menu_bounds = layer_get_bounds(menu_layer_get_layer(menu_layer));

  // Subtract off 20px for checkbox
  if (activeList[cell_index->row].status == STATUS_BOUGHT) {
    menu_bounds.origin.x += 20;
    menu_bounds.size.w -= 20;
  }

  GSize sz = graphics_text_layout_get_content_size(activeList[cell_index->row].name,
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

  if (cell_index->row >= activeListCount)
    return;

  GRect cell_bounds = layer_get_bounds(cell_layer);

  if (activeList[cell_index->row].status == STATUS_BOUGHT) {

    graphics_context_set_stroke_width(ctx, 3);

    if (menu_cell_layer_is_highlighted(cell_layer)) {
      graphics_context_set_stroke_color(ctx, GColorWhite);
    }
    else {
      graphics_context_set_stroke_color(ctx, GColorBlack);
    }

    GPoint pt;

    pt.x = cell_bounds.origin.x + 2;
    pt.y = cell_bounds.size.h / 2 - 8;

    gpath_move_to(checkboxPath, pt);
    gpath_draw_outline_open(ctx, checkboxPath);

    cell_bounds.origin.x += 20;
    cell_bounds.size.w -= 20;
  }

  graphics_draw_text(ctx, activeList[cell_index->row].name,
		     font, cell_bounds,
		     GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);

}

static void menuOnSelect(struct MenuLayer *menu_layer, MenuIndex *cell_index, void *callback_context) {

  if (cell_index->row >= activeListCount)
    return;

  if (activeList[cell_index->row].status == STATUS_WANT_TO_BUY) {
    comms_setItemBought(activeShoppingListID, activeList[cell_index->row].id);

    // TODO: Do something to signal that item will be changed...
  }
  else {
    comms_setItemToBuy(activeShoppingListID, activeList[cell_index->row].id);

    // TODO: Do something to signal that item will be changed...
  }

}

static void menuOnSelectLong(struct MenuLayer *menu_layer, MenuIndex *cell_index, void *callback_context) {

  actmenu_init();

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

  checkboxPath = gpath_create(&CHECKBOX_PTS);

  window = window_create();
  window_set_window_handlers(window, (WindowHandlers) {
    .load = windowLoad,
    .unload = windowUnload
  });

  window_stack_push(window, true);

}

void ui_deinit(void) {
  window_destroy(window);
  gpath_destroy(checkboxPath);
}
