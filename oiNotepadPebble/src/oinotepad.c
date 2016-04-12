#include <pebble.h>

#include "oinotepad.h"

NoteItem *notes;
uint8_t noteCount;
uint8_t doneInit;

extern void ui_init();
extern void ui_deinit();

extern void msgwin_deinit();

extern void comms_init();
extern void comms_deinit();
extern void comms_getList();

static void onInitTimer(void *data) {
  comms_getList();
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

  doneInit = 0;

  notes = calloc(1, sizeof(NoteItem));
  memset(notes, 0, 1*sizeof(NoteItem));

  strcpy(notes[0].title, "Loading...");
  
  noteCount = 1;

  init();

  APP_LOG(APP_LOG_LEVEL_DEBUG, "Done initializing");

  app_event_loop();

  deinit();

  free(notes);
}
