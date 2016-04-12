
#include <pebble.h>

#include "oishopping.h"

#define CMD_GET_LIST      101
#define CMD_SET_BOUGHT    102
#define CMD_SET_TOBUY     103
#define CMD_GET_ALL_LISTS 104
#define CMD_ADD_ITEM      105

#define RES_ERROR               201
#define RES_ITEM_LIST_START     202
#define RES_ITEM_LIST_CONTINUE  203
#define RES_ITEM_BOUGHT         204
#define RES_ITEM_TOBUY          205
#define RES_ALL_LISTS_START     206
#define RES_ITEM_ADDED          207
#define RES_FOUND_ITEMS         208
#define RES_ITEM_ALREADY_EXISTS 209

#define DATA_ITEM_START_KEY 100

#define COMMAND_ITEMOUT 20000
#define RECEIVE_ITEMS_ITEMOUT 10000

extern int32_t activeShoppingListID;

extern ShoppingItem *activeList;
extern uint8_t activeListCount;

extern ShoppingItem *addItemList;
extern uint8_t addItemListCount;

extern ShoppingList *allLists;
extern uint8_t allListsCount;

extern uint8_t doneInit;

extern void ui_refreshItems(int selectPos);
extern void ui_replaceWithMessage(const char *message);
extern void listsmenu_refreshItems();
extern void msgwin_show(char *message);
extern void additem_init();

static int numItemsExpected;
static int numItemsReceived;
static AppTimer *commandTimer;
static AppTimer *receiveItemsTimer;

static void onCommandTimeout(void *data) {
  commandTimer = NULL;
  msgwin_show("Timed out while sending command to phone");
}

void comms_getShoppingLists() {

  DictionaryIterator *iterator;
  app_message_outbox_begin(&iterator);

  int value = CMD_GET_ALL_LISTS;
  dict_write_int(iterator, 0, &value, sizeof(int), true /* signed */);

  if (commandTimer != NULL)
    app_timer_cancel(commandTimer);
  commandTimer = app_timer_register(COMMAND_ITEMOUT, onCommandTimeout, NULL);

  app_message_outbox_send();

  APP_LOG(APP_LOG_LEVEL_DEBUG, "Sent getShoppingLists");
}

void comms_getItemList(int32_t listID) {

  DictionaryIterator *iterator;
  app_message_outbox_begin(&iterator);

  int value = CMD_GET_LIST;
  dict_write_int(iterator, 0, &value, sizeof(int), true /* signed */);

  dict_write_int(iterator, 1, &listID, sizeof(int32_t), true /* signed */);

  if (commandTimer != NULL)
    app_timer_cancel(commandTimer);
  commandTimer = app_timer_register(COMMAND_ITEMOUT, onCommandTimeout, NULL);

  app_message_outbox_send();

  APP_LOG(APP_LOG_LEVEL_DEBUG, "Sent getItemList");
}

void comms_setItemBought(int32_t listID, int32_t itemID) {

  DictionaryIterator *iterator;
  app_message_outbox_begin(&iterator);

  int value = CMD_SET_BOUGHT;
  dict_write_int(iterator, 0, &value, sizeof(int), true /* signed */);

  dict_write_int(iterator, 1, &listID, sizeof(int32_t), true /* signed */);
  dict_write_int(iterator, 2, &itemID, sizeof(int32_t), true /* signed */);

  if (commandTimer != NULL)
    app_timer_cancel(commandTimer);
  commandTimer = app_timer_register(COMMAND_ITEMOUT, onCommandTimeout, NULL);

  app_message_outbox_send();

  APP_LOG(APP_LOG_LEVEL_DEBUG, "Sent setItemBought");
}

void comms_setItemToBuy(int32_t listID, int32_t itemID) {

  DictionaryIterator *iterator;
  app_message_outbox_begin(&iterator);

  int value = CMD_SET_TOBUY;
  dict_write_int(iterator, 0, &value, sizeof(int), true /* signed */);

  dict_write_int(iterator, 1, &listID, sizeof(int32_t), true /* signed */);
  dict_write_int(iterator, 2, &itemID, sizeof(int32_t), true /* signed */);

  if (commandTimer != NULL)
    app_timer_cancel(commandTimer);
  commandTimer = app_timer_register(COMMAND_ITEMOUT, onCommandTimeout, NULL);

  app_message_outbox_send();

  APP_LOG(APP_LOG_LEVEL_DEBUG, "Sent setItemToBuy");
}

void comms_addItem(int32_t listID, char *name) {

  DictionaryIterator *iterator;
  app_message_outbox_begin(&iterator);

  int value = CMD_ADD_ITEM;
  dict_write_int(iterator, 0, &value, sizeof(int), true /* signed */);

  dict_write_int(iterator, 1, &listID, sizeof(int32_t), true /* signed */);
  dict_write_cstring(iterator, 2, name);

  if (commandTimer != NULL)
    app_timer_cancel(commandTimer);
  commandTimer = app_timer_register(COMMAND_ITEMOUT, onCommandTimeout, NULL);

  app_message_outbox_send();

  APP_LOG(APP_LOG_LEVEL_DEBUG, "Sent addItem");
}

void comms_addItemById(int32_t listID, int itemID) {

  DictionaryIterator *iterator;
  app_message_outbox_begin(&iterator);

  int value = CMD_ADD_ITEM;
  dict_write_int(iterator, 0, &value, sizeof(int), true /* signed */);

  dict_write_int(iterator, 1, &listID, sizeof(int32_t), true /* signed */);
  dict_write_int(iterator, 3, &itemID, sizeof(int32_t), true /* signed */);

  if (commandTimer != NULL)
    app_timer_cancel(commandTimer);
  commandTimer = app_timer_register(COMMAND_ITEMOUT, onCommandTimeout, NULL);

  app_message_outbox_send();

  APP_LOG(APP_LOG_LEVEL_DEBUG, "Sent addItemById");
}

static void copyLists(DictionaryIterator *iterator) {

  APP_LOG(APP_LOG_LEVEL_DEBUG, "copyLists");

  int keysUsed = 0;

  Tuple *itemTuple = dict_read_first(iterator);
  while (itemTuple != NULL) {

    // Data items start at DATA_ITEM_START_KEY onwards
    if (itemTuple->key < DATA_ITEM_START_KEY) {
      APP_LOG(APP_LOG_LEVEL_DEBUG, "Ignore key %lu", itemTuple->key);
      itemTuple = dict_read_next(iterator);
      continue;
    }

    int position = (itemTuple->key - DATA_ITEM_START_KEY) / 2;
      
    if (position >= numItemsExpected) {
      APP_LOG(APP_LOG_LEVEL_INFO, "Ignoring too many data items: %lu", itemTuple->key);
      itemTuple = dict_read_next(iterator);
      continue;
    }

    if ((itemTuple->key % 2) == 0) {

      if (itemTuple->type != TUPLE_INT) {
	APP_LOG(APP_LOG_LEVEL_ERROR, "Data item with key %lu is not an integer", itemTuple->key);
	itemTuple = dict_read_next(iterator);
	continue;
      }

      allLists[position].id = itemTuple->value->int32;

    }
    else if ((itemTuple->key % 2) == 1) {

      if (itemTuple->type != TUPLE_CSTRING) {
	APP_LOG(APP_LOG_LEVEL_ERROR, "Data item with key %lu is not a string", itemTuple->key);
	itemTuple = dict_read_next(iterator);
	continue;
      }

      strncpy(allLists[position].name, itemTuple->value->cstring, MAX_NAME_LEN);

    }

    keysUsed++;

    itemTuple = dict_read_next(iterator);
  }

  if ((keysUsed % 2) != 0) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Not all items were filled in");
  }

  numItemsReceived += (keysUsed / 2);

  APP_LOG(APP_LOG_LEVEL_DEBUG, "Got %d out of %d items so far", numItemsReceived, numItemsExpected);

  if (numItemsReceived == numItemsExpected) {

    allListsCount = numItemsReceived;
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Updated all lists with %d items", allListsCount);

    listsmenu_refreshItems();

  }

}

static void onReceiveItemsTimeout(void *data) {
  receiveItemsTimer = NULL;
  msgwin_show("Timed out while downloading items");
}

static void copyItemsToActive(DictionaryIterator *iterator, ShoppingItem *retItemList,
			      uint8_t *retItemListCount) {

  int keysUsed = 0;

  Tuple *itemTuple = dict_read_first(iterator);
  while (itemTuple != NULL) {

    // Data items start at DATA_ITEM_START_KEY onwards
    if (itemTuple->key < DATA_ITEM_START_KEY) {
      APP_LOG(APP_LOG_LEVEL_DEBUG, "Ignore key %lu", itemTuple->key);
      itemTuple = dict_read_next(iterator);
      continue;
    }

    int position = (itemTuple->key - DATA_ITEM_START_KEY) / 4;
      
    if (position >= numItemsExpected) {
      APP_LOG(APP_LOG_LEVEL_INFO, "Ignoring too many data items: %lu", itemTuple->key);
      itemTuple = dict_read_next(iterator);
      continue;
    }

    if ((itemTuple->key % 4) == 0) {

      if (itemTuple->type != TUPLE_INT) {
	APP_LOG(APP_LOG_LEVEL_ERROR, "Data item with key %lu is not an integer", itemTuple->key);
	itemTuple = dict_read_next(iterator);
	continue;
      }

      retItemList[position].id = itemTuple->value->int32;

    }
    else if ((itemTuple->key % 4) == 1) {

      if (itemTuple->type != TUPLE_CSTRING) {
	APP_LOG(APP_LOG_LEVEL_ERROR, "Data item with key %lu is not a string", itemTuple->key);
	itemTuple = dict_read_next(iterator);
	continue;
      }

      strncpy(retItemList[position].name, itemTuple->value->cstring, MAX_NAME_LEN);

    }
    else if ((itemTuple->key % 4) == 2) {

      if (itemTuple->type != TUPLE_UINT) {
	APP_LOG(APP_LOG_LEVEL_ERROR, "Data item with key %lu is not a uint", itemTuple->key);
	itemTuple = dict_read_next(iterator);
	continue;
      }

      retItemList[position].status = itemTuple->value->uint8;

    }

    keysUsed++;

    itemTuple = dict_read_next(iterator);
  }

  if ((keysUsed % 3) != 0) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Not all items were filled in");
  }

  numItemsReceived += (keysUsed / 3);

  APP_LOG(APP_LOG_LEVEL_DEBUG, "Got %d out of %d items so far", numItemsReceived, numItemsExpected);

  if (numItemsReceived >= numItemsExpected) {

    if (receiveItemsTimer != NULL) {
      app_timer_cancel(receiveItemsTimer);
      receiveItemsTimer = NULL;
    }

    (*retItemListCount) = numItemsReceived;
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Updated item list with %d items", *retItemListCount);

  }
  else {
    if (receiveItemsTimer != NULL) {
      app_timer_reschedule(receiveItemsTimer, RECEIVE_ITEMS_ITEMOUT);
    }
    else {
      receiveItemsTimer = app_timer_register(RECEIVE_ITEMS_ITEMOUT, onReceiveItemsTimeout, NULL);
    }
  }

}

static int tolower(int c) {
  if (c >= 'A' && c <= 'Z') {
    c += 'a' - 'A';
  }
  return c;
}

static int stricmp(char *str1, char *str2) {

  char c1 = tolower(*str1);
  char c2 = tolower(*str2);

  while (*str1 && *str2 && c1 == c2) {
    str1++;
    str2++;
    c1 = tolower(*str1);
    c2 = tolower(*str2);
  }

  return c2 - c1;
}

static void inboxReceived(DictionaryIterator *iterator, void *context) {

  if (commandTimer != NULL) {
    app_timer_cancel(commandTimer);
    commandTimer = NULL;
  }

  doneInit = 1;

  Tuple *cmdTuple = dict_find(iterator, 0);

  if (!cmdTuple) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Recieved data without command code");
    return;
  }
  else if (cmdTuple->type != TUPLE_INT) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Recieved command code is not an integer");
    return;
  }

  int cmd = cmdTuple->value->int32;

  APP_LOG(APP_LOG_LEVEL_DEBUG, "Got response: %d", cmd);
  
  if (cmd == RES_ERROR) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Host app sent back error");
    return;
  }

  if (cmd == RES_ITEM_LIST_START) {

    Tuple *countTuple = dict_find(iterator, 1);

    if (!countTuple || countTuple->type != TUPLE_INT) {
      APP_LOG(APP_LOG_LEVEL_ERROR, "Received item list without size");
      return;
    }

    int itemsToAlloc = countTuple->value->int32;

    if (itemsToAlloc < 1) {
      ui_replaceWithMessage("No items in list");
      return;
    }
    else if (itemsToAlloc > MAX_ACTIVE_ITEMS)
      itemsToAlloc = MAX_ACTIVE_ITEMS;

    activeListCount = 0;
    free(activeList);
    
    activeList = calloc(itemsToAlloc, sizeof(ShoppingItem));
    memset(activeList, 0, itemsToAlloc*sizeof(ShoppingItem));

    numItemsReceived = 0;
    numItemsExpected = countTuple->value->int32;

    APP_LOG(APP_LOG_LEVEL_DEBUG, "Receive item list, expecting %d items", numItemsExpected);

    copyItemsToActive(iterator, activeList, &activeListCount);

    if (numItemsReceived >= numItemsExpected)
      ui_refreshItems(0);

  }
  else if (cmd == RES_ITEM_LIST_CONTINUE) {

    copyItemsToActive(iterator, activeList, &activeListCount);

    if (numItemsReceived >= numItemsExpected)
      ui_refreshItems(0);
  }
  else if (cmd == RES_ITEM_BOUGHT || cmd == RES_ITEM_TOBUY) {

    Tuple *listIDTuple = dict_find(iterator, 1);

    if (!listIDTuple || listIDTuple->type != TUPLE_INT) {
      APP_LOG(APP_LOG_LEVEL_ERROR, "Missing or incorrect list ID type");
      return;
    }

    Tuple *itemIDTuple = dict_find(iterator, 2);

    if (!itemIDTuple || itemIDTuple->type != TUPLE_INT) {
      APP_LOG(APP_LOG_LEVEL_ERROR, "Missing or incorrect item ID type");
      return;
    }
    
    if (listIDTuple->value->int32 != activeShoppingListID) {
      APP_LOG(APP_LOG_LEVEL_INFO, "Ignoring item update for list that is not active: %ld vs %ld",
	      listIDTuple->value->int32, activeShoppingListID);
      return;
    }

    int found = 0;
    for (int i = 0; i < activeListCount; i++) {
      if (activeList[i].id == itemIDTuple->value->int32) {

	if (cmd == RES_ITEM_BOUGHT) {
	  activeList[i].status = STATUS_BOUGHT;
	}
	else {
	  activeList[i].status = STATUS_WANT_TO_BUY;
	}

	found = 1;
	break;
      }
    }

    if (!found) {
      APP_LOG(APP_LOG_LEVEL_INFO, "Item with id %ld not in list", itemIDTuple->value->int32);
      return;
    }

    ui_refreshItems(-1);

  }
  else if (cmd == RES_ALL_LISTS_START) {

    Tuple *countTuple = dict_find(iterator, 1);

    if (!countTuple || countTuple->type != TUPLE_INT) {
      APP_LOG(APP_LOG_LEVEL_ERROR, "Received list without size");
      return;
    }

    int itemsToAlloc = countTuple->value->int32;
    if (itemsToAlloc > MAX_SHOPPING_LISTS)
      itemsToAlloc = MAX_SHOPPING_LISTS;

    allListsCount = 0;
    if (allLists != NULL)
      free(allLists);
    
    allLists = calloc(itemsToAlloc, sizeof(ShoppingList));
    memset(allLists, 0, itemsToAlloc*sizeof(ShoppingList));

    numItemsReceived = 0;
    numItemsExpected = countTuple->value->int32;

    APP_LOG(APP_LOG_LEVEL_DEBUG, "Receive list, expecting %d items", numItemsExpected);

    copyLists(iterator);

  }
  else if (cmd == RES_ITEM_ADDED) {

    Tuple *idTuple = dict_find(iterator, 1);

    if (!idTuple || idTuple->type != TUPLE_INT) {
      APP_LOG(APP_LOG_LEVEL_ERROR, "Received add response without ID");
      return;
    }

    int itemId = idTuple->value->int32;

    Tuple *nameTuple = dict_find(iterator, 2);

    if (!nameTuple || nameTuple->type != TUPLE_CSTRING) {
      APP_LOG(APP_LOG_LEVEL_ERROR, "Received add response without name");
      return;
    }

    char *itemName = nameTuple->value->cstring;

    int insertPos = 0;

    while (insertPos < activeListCount &&
	   stricmp(itemName, activeList[insertPos].name) < 0) {
      insertPos++;
    }

    APP_LOG(APP_LOG_LEVEL_DEBUG, "Insert at pos %d", insertPos);

    ShoppingItem *tempList = calloc(activeListCount+1, sizeof(ShoppingItem));

    if (insertPos > 0)
      memcpy(tempList, activeList, insertPos*sizeof(ShoppingItem));

    memset(&(tempList[insertPos]), 0, sizeof(ShoppingItem));

    tempList[insertPos].id = itemId;
    strncpy(tempList[insertPos].name, itemName, MAX_NAME_LEN);
    tempList[insertPos].status = STATUS_WANT_TO_BUY;

    if (insertPos < activeListCount)
      memcpy(&(tempList[insertPos + 1]), &(activeList[insertPos]), (activeListCount - insertPos)*sizeof(ShoppingItem));

    free(activeList);

    activeList = tempList;
    activeListCount++;

    ui_refreshItems(insertPos);

  }
  else if (cmd == RES_FOUND_ITEMS) {

    Tuple *countTuple = dict_find(iterator, 1);

    if (!countTuple || countTuple->type != TUPLE_INT) {
      APP_LOG(APP_LOG_LEVEL_ERROR, "Received item list without size");
      return;
    }

    int itemsToAlloc = countTuple->value->int32;

    if (itemsToAlloc < 1) {
      msgwin_show("Phone returned invalid response");
      return;
    }
    else if (itemsToAlloc > MAX_ACTIVE_ITEMS)
      itemsToAlloc = MAX_ACTIVE_ITEMS;

    addItemListCount = 0;
    if (addItemList != NULL)
      free(addItemList);
    
    addItemList = calloc(itemsToAlloc, sizeof(ShoppingItem));
    memset(addItemList, 0, itemsToAlloc*sizeof(ShoppingItem));

    numItemsReceived = 0;
    numItemsExpected = countTuple->value->int32;

    APP_LOG(APP_LOG_LEVEL_DEBUG, "Receive item list, expecting %d items", numItemsExpected);

    copyItemsToActive(iterator, addItemList, &addItemListCount);

    if (numItemsReceived < numItemsExpected) {
      msgwin_show("Phone sent too many items");
      return;
    }

    additem_init();

  }
  else if (cmd == RES_ITEM_ALREADY_EXISTS) {

    msgwin_show("Item is already on the list");

  }

}

static void inboxDropped(AppMessageResult reason, void *context) {

  if (receiveItemsTimer != NULL) {
    app_timer_cancel(receiveItemsTimer);
    receiveItemsTimer = NULL;
  }

  if ((reason & APP_MSG_BUSY) == APP_MSG_BUSY) {
    msgwin_show("Phone sent responses too fast");
  }
  else if ((reason & APP_MSG_BUFFER_OVERFLOW) == APP_MSG_BUFFER_OVERFLOW) {
    msgwin_show("Recieved response that was too large to process");
  }
  else {
    const char *msg = "Message lost from phone: %d";
    char *buffer = malloc(strlen(msg) + 10);
    snprintf(buffer, strlen(msg) + 9, msg, reason);
    msgwin_show(buffer);
    free(buffer);
  }

}

static void outboxFailed(DictionaryIterator *iterator, AppMessageResult reason, void *context) {

  if ((reason & APP_MSG_NOT_CONNECTED) == APP_MSG_NOT_CONNECTED) {
    msgwin_show("Phone is not connected");
  }
  else if ((reason & APP_MSG_APP_NOT_RUNNING) == APP_MSG_APP_NOT_RUNNING) {
    msgwin_show("Pebble app is not running on the phone");
  }
  else if ((reason & APP_MSG_SEND_TIMEOUT) == APP_MSG_SEND_TIMEOUT) {
    msgwin_show("Timed out while sending command");
  }
  else if ((reason & APP_MSG_SEND_REJECTED) == APP_MSG_SEND_REJECTED) {
    msgwin_show("Phone was unable to process command");
  }
  else {
    const char *msg = "Error while sending command to phone: %d";
    char *buffer = malloc(strlen(msg) + 10);
    snprintf(buffer, strlen(msg) + 9, msg, reason);
    msgwin_show(buffer);
    free(buffer);
  }

}

/*static void outboxSent(DictionaryIterator *iterator, void *context) {
  // Send success
}*/

void comms_init() {

  receiveItemsTimer = NULL;

  app_message_register_inbox_received(inboxReceived);
  app_message_register_inbox_dropped(inboxDropped);
  app_message_register_outbox_failed(outboxFailed);
  //app_message_register_outbox_sent(outboxSent);

  // app_message_inbox_size_maximum(), app_message_outbox_size_maximum()
  app_message_open(2048, 256);

}

void comms_deinit() {
  if (receiveItemsTimer != NULL) {
    app_timer_cancel(receiveItemsTimer);
    receiveItemsTimer = NULL;
  }
  app_message_deregister_callbacks();
}

