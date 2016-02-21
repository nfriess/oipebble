#include <pebble.h>

#include "oinotepad.h"

#define CMD_GET_LIST      101
#define CMD_GET_NOTE      102

#define RES_ERROR               201
#define RES_NOTE_LIST_START     202
#define RES_NOTE_LIST_CONTINUE  203
#define RES_NOTE_DATA           204

#define DATA_ITEM_START_KEY 100

#define COMMAND_ITEMOUT 10000
#define RECEIVE_ITEMS_ITEMOUT 10000

extern NoteItem *notes;
extern uint8_t noteCount;

extern void ui_refreshItems(int selectPos);
extern void ui_replaceWithMessage(const char *message);
extern void msgwin_show(char *message);

extern void viewnote_init(char *noteStr);

static int numItemsExpected;
static int numItemsReceived;
static AppTimer *commandTimer;
static AppTimer *receiveItemsTimer;

static void onCommandTimeout(void *data) {
  commandTimer = NULL;
  msgwin_show("Timed out while sending command to phone");
}

void comms_getList() {

  DictionaryIterator *iterator;
  app_message_outbox_begin(&iterator);

  int value = CMD_GET_LIST;
  dict_write_int(iterator, 0, &value, sizeof(int), true /* signed */);

  if (commandTimer != NULL)
    app_timer_cancel(commandTimer);
  commandTimer = app_timer_register(COMMAND_ITEMOUT, onCommandTimeout, NULL);

  app_message_outbox_send();

  APP_LOG(APP_LOG_LEVEL_DEBUG, "Sent getList");
}

void comms_getNote(int32_t noteID) {

  DictionaryIterator *iterator;
  app_message_outbox_begin(&iterator);

  int value = CMD_GET_NOTE;
  dict_write_int(iterator, 0, &value, sizeof(int), true /* signed */);

  dict_write_int(iterator, 1, &noteID, sizeof(int32_t), true /* signed */);

  if (commandTimer != NULL)
    app_timer_cancel(commandTimer);
  commandTimer = app_timer_register(COMMAND_ITEMOUT, onCommandTimeout, NULL);

  app_message_outbox_send();

  APP_LOG(APP_LOG_LEVEL_DEBUG, "Sent getNote");
}

static void onReceiveItemsTimeout(void *data) {
  receiveItemsTimer = NULL;
  msgwin_show("Timed out while downloading items");
}

static void copyItemsToActive(DictionaryIterator *iterator) {

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

      notes[position].id = itemTuple->value->int32;

    }
    else if ((itemTuple->key % 2) == 1) {

      if (itemTuple->type != TUPLE_CSTRING) {
	APP_LOG(APP_LOG_LEVEL_ERROR, "Data item with key %lu is not a string", itemTuple->key);
	itemTuple = dict_read_next(iterator);
	continue;
      }

      strncpy(notes[position].title, itemTuple->value->cstring, MAX_TITLE_LEN);

    }

    keysUsed++;

    itemTuple = dict_read_next(iterator);
  }

  if ((keysUsed % 2) != 0) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Not all items were filled in");
  }

  numItemsReceived += (keysUsed / 2);

  APP_LOG(APP_LOG_LEVEL_DEBUG, "Got %d out of %d items so far", numItemsReceived, numItemsExpected);

  if (numItemsReceived >= numItemsExpected) {

    if (receiveItemsTimer != NULL) {
      app_timer_cancel(receiveItemsTimer);
      receiveItemsTimer = NULL;
    }

    noteCount = numItemsReceived;
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Updated item list with %d items", noteCount);

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

static void inboxReceived(DictionaryIterator *iterator, void *context) {

  if (commandTimer != NULL) {
    app_timer_cancel(commandTimer);
    commandTimer = NULL;
  }

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

  if (cmd == RES_NOTE_LIST_START) {

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
    else if (itemsToAlloc > MAX_ITEMS)
      itemsToAlloc = MAX_ITEMS;

    noteCount = 0;
    free(notes);
    
    notes = calloc(itemsToAlloc, sizeof(NoteItem));
    memset(notes, 0, itemsToAlloc*sizeof(NoteItem));

    numItemsReceived = 0;
    numItemsExpected = countTuple->value->int32;

    APP_LOG(APP_LOG_LEVEL_DEBUG, "Receive note list, expecting %d items", numItemsExpected);

    copyItemsToActive(iterator);

    if (numItemsReceived >= numItemsExpected)
      ui_refreshItems(0);

  }
  else if (cmd == RES_NOTE_LIST_CONTINUE) {

    copyItemsToActive(iterator);

    if (numItemsReceived >= numItemsExpected)
      ui_refreshItems(0);
  }
  else if (cmd == RES_NOTE_DATA) {

    Tuple *idTuple = dict_find(iterator, 1);

    if (!idTuple || idTuple->type != TUPLE_INT) {
      APP_LOG(APP_LOG_LEVEL_ERROR, "Received note without id");
      return;
    }

    int32_t noteID  = idTuple->value->int32;

    Tuple *dataTuple = dict_find(iterator, 2);

    if (!dataTuple || dataTuple->type != TUPLE_CSTRING) {
      APP_LOG(APP_LOG_LEVEL_ERROR, "Received note without data");
      return;
    }

    int len = strlen(dataTuple->value->cstring);
    if (len > MAX_NOTE_LEN)
      len = MAX_NOTE_LEN;

    char *noteStr = calloc(len + 1, sizeof(char));
    if (noteStr == NULL) {
      APP_LOG(APP_LOG_LEVEL_ERROR, "Malloc returned null, length %d", (len+1));
      return;
    }

    strncpy(noteStr, dataTuple->value->cstring, len);
    noteStr[len] = 0;

    viewnote_init(noteStr);

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
  app_message_open(4096, 256);

}

void comms_deinit() {
  if (receiveItemsTimer != NULL) {
    app_timer_cancel(receiveItemsTimer);
    receiveItemsTimer = NULL;
  }
  app_message_deregister_callbacks();
}
