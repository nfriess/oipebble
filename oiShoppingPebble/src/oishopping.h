
#ifndef OISHOPPING_H
#define OISHOPPING_H


#define MAX_NAME_LEN 127



typedef struct _ShoppingItem {
  int32_t id;
  char name[MAX_NAME_LEN+1];
  uint8_t  status;
} ShoppingItem;

#define MAX_ACTIVE_ITEMS 100

#define STATUS_WANT_TO_BUY 1  
#define STATUS_BOUGHT      2
// Never used on the watch...
#define STATUS_REMOVED     3



typedef struct _ShoppingList {
  int32_t id;
  char name[MAX_NAME_LEN+1];
} ShoppingList;

#define MAX_SHOPPING_LISTS 10


#define STORAGE_ACTIVELISTID 1


#endif
