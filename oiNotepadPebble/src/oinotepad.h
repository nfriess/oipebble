
#ifndef OINOTEPAD_H
#define OINOTEPAD_H

#define MAX_TITLE_LEN 127
#define MAX_NOTE_LEN 4000

typedef struct _NoteItem {
  int32_t id;
  char title[MAX_TITLE_LEN+1];
} NoteItem;

#define MAX_ITEMS 100

#endif
