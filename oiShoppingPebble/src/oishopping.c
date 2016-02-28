#include <pebble.h>

#include "oishopping.h"

int32_t activeShoppingListID;

ShoppingItem *activeList;
uint8_t activeListCount;

ShoppingItem *addItemList;
uint8_t addItemListCount;


ShoppingList *allLists;
uint8_t allListsCount;

uint8_t isRound;

extern void ui_init();
extern void ui_deinit();

extern void msgwin_deinit();

extern void comms_init();
extern void comms_deinit();
extern void comms_getItemList();

static void onInitTimer(void *data) {
  comms_getItemList(activeShoppingListID);
}

static void init(void) {

  ui_init();

  comms_init();

  app_timer_register(100/*ms*/, onInitTimer, NULL);

}

static void deinit(void) {
  comms_deinit();
  ui_deinit();
  msgwin_deinit();
}

int main(void) {

  if (persist_exists(STORAGE_ACTIVELISTID)) {
    activeShoppingListID = persist_read_int(STORAGE_ACTIVELISTID);
  }
  else {
    // Seems to be default app list
    activeShoppingListID = 1;
  }


  allLists = NULL;
  allListsCount = 0;

  activeList = calloc(1, sizeof(ShoppingItem));
  memset(activeList, 0, 1*sizeof(ShoppingItem));

  strcpy(activeList[0].name, "Loading...");

  activeListCount = 1;

  isRound = 0;
  if (watch_info_get_model() == WATCH_INFO_MODEL_PEBBLE_TIME_ROUND_14 ||
	watch_info_get_model() == WATCH_INFO_MODEL_PEBBLE_TIME_ROUND_20) {

	isRound = 1;
  }

  init();

  APP_LOG(APP_LOG_LEVEL_DEBUG, "Done initializing");

  app_event_loop();

  deinit();

  free(activeList);
}
