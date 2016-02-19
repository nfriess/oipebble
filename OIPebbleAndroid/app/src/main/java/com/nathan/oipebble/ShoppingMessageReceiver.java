package com.nathan.oipebble;

import android.content.ContentResolver;
import android.content.Context;
import android.util.Log;

import com.getpebble.android.kit.PebbleKit;
import com.getpebble.android.kit.util.PebbleDictionary;

import java.util.ArrayList;
import java.util.Collections;
import java.util.Comparator;
import java.util.ListIterator;
import java.util.Timer;
import java.util.TimerTask;
import java.util.UUID;

/**
 * The main class that is called by PebbleKit when a message is received
 * from the shopping list app on a Pebble watch.
 */
public class ShoppingMessageReceiver extends PebbleKit.PebbleDataReceiver implements MessageReceiverInterface {

    private static final String LOG_CLASS = "ShoppingMessageReceiver";

    public static final UUID PEBBLE_OISHOPPING_UUID = UUID.fromString("d7362019-70ac-4b7c-879d-22f07fecce3a");

    /** The assumed buffer size in the pebble app. */
    private static final int PEBBLE_BUFFER_SIZE = 2048;

    // From pebble.h:
    //   buffer_size = 1 + (n * 7) + D1 + ... + Dn
    //   where n is number of items, and Dn is the size data for each item

    // Our item packets contain: int32, int8, and a string of max 128 bytes
    // ==> (7+4) + (7+1) + (7+1+strlen)

    /** Size of one shopping item when serialized, excluding the string length of the name. */
    private static final int SIZE_PER_SHOPPING_ITEM = (7+4) + (7+1) + (7+1);

    private static final int CMD_GET_LIST = 101;
    private static final int CMD_SET_BOUGHT = 102;
    private static final int CMD_SET_TOBUY = 103;
    private static final int CMD_GET_ALL_LISTS = 104;
    private static final int CMD_ADD_ITEM = 105;

    private static final int RES_ERROR = 201;
    private static final int RES_ITEM_LIST_START = 202;
    private static final int RES_ITEM_LIST_CONTINUE = 203;
    private static final int RES_ITEM_BOUGHT = 204;
    private static final int RES_ITEM_TOBUY = 205;
    private static final int RES_ALL_LISTS_START = 206;
    private static final int RES_ITEM_ADDED = 207;
    private static final int RES_FOUND_ITEMS = 208;
    private static final int RES_ITEM_ALREADY_EXISTS = 209;

    private static final int DATA_ITEM_START_KEY = 100;

    private final Context applicationContext;
    private final ContentResolver resolver;

    private ArrayList<ShoppingItem> pendingItems;
    private Timer pendingItemsTimer;

    public ShoppingMessageReceiver(Context applicationContext, ContentResolver resolver) {
        super(PEBBLE_OISHOPPING_UUID);
        this.applicationContext = applicationContext;
        this.resolver = resolver;
        this.pendingItems = null;
        this.pendingItemsTimer = null;
    }


    @Override
    public void receiveData(Context context, int transactionId, PebbleDictionary data) {

        PebbleKit.sendAckToPebble(applicationContext, transactionId);

        int cmd = data.getInteger(0).intValue();

        try {

            switch (cmd) {

                case CMD_GET_LIST:
                    cmdGetItemList(data);
                    break;

                case CMD_SET_BOUGHT:
                    cmdSetBought(data);
                    break;

                case CMD_SET_TOBUY:
                    cmdSetToBuy(data);
                    break;

                case CMD_GET_ALL_LISTS:
                    cmdGetAllLists(data);
                    break;

                case CMD_ADD_ITEM:
                    cmdAddItem(data);
                    break;

                default:
                    Log.e(LOG_CLASS, "Unknown command/response: " + cmd);
            }

        }
        catch (Exception ex) {
            Log.e(LOG_CLASS, "Exception", ex);
            try {
                PebbleDictionary respData = new PebbleDictionary();
                respData.addInt32(0, RES_ERROR);
                PebbleKit.sendDataToPebble(applicationContext, PEBBLE_OISHOPPING_UUID, respData);
            }
            catch (Exception exx) { }
        }

    }

    public void receiveNack(Context context, int transactionId) {

        Log.d(LOG_CLASS, "Got NACK for: " + transactionId);

        if (pendingItemsTimer != null) {
            pendingItemsTimer.cancel();
            pendingItemsTimer = null;
        }

        if (pendingItems != null) {
            pendingItems = null;
        }

    }

    public void receiveAck(Context context, int transactionId) {

        // Seems that we don't get our own transaction IDs back...
        Log.d(LOG_CLASS, "Got ACK for: " + transactionId);

        if (pendingItemsTimer != null) {
            pendingItemsTimer.cancel();
            pendingItemsTimer = null;
        }


        if (pendingItems != null) {

            Log.d(LOG_CLASS, "Sending more items for: " + transactionId);

            PebbleDictionary respData = new PebbleDictionary();

            respData.addInt32(0, RES_ITEM_LIST_CONTINUE);

            ShoppingItem lastKeyItem = pendingItems.get(0);
            pendingItems.remove(0);

            int lastKey = copyItemsToDict(pendingItems, lastKeyItem.id, respData);

            if (pendingItems.size() > 0) {
                Log.d(LOG_CLASS, "Queue " + pendingItems.size() + " more items");
                ShoppingItem itm = new ShoppingItem();
                itm.id = lastKey;
                pendingItems.add(0, itm);

                pendingItemsTimer = new Timer();
                pendingItemsTimer.schedule(new TimerTask() {
                                               @Override
                                               public void run() {
                                                   pendingItems = null;
                                                   pendingItemsTimer = null;
                                               }
                                           },
                        10000);

            }
            else {
                pendingItems = null;
            }

            PebbleKit.sendDataToPebble(applicationContext, PEBBLE_OISHOPPING_UUID, respData);

        }

    }

    private void cmdGetItemList(final PebbleDictionary data) {

        int listID = data.getInteger(1).intValue();

        Log.d(LOG_CLASS, "getItemList for ID " + listID);

        PebbleDictionary respData = new PebbleDictionary();

        respData.addInt32(0, RES_ITEM_LIST_START);

        ArrayList<ShoppingItem> itemList = OIShoppingConsumer.getItemList(resolver, listID);

        Collections.sort(itemList,
                new Comparator<ShoppingItem>() {

                    @Override
                    public int compare(ShoppingItem lhs, ShoppingItem rhs) {
                        return lhs.name.compareToIgnoreCase(rhs.name);
                    }

                });

        int lastKey = copyItemsToDict(itemList, DATA_ITEM_START_KEY, respData);

        if (itemList.size() > 0) {
            Log.d(LOG_CLASS, "Queue " + itemList.size() + " items");
            ShoppingItem itm = new ShoppingItem();
            itm.id = lastKey;
            itemList.add(0, itm);
            pendingItems = itemList;

            pendingItemsTimer = new Timer();
            pendingItemsTimer.schedule(new TimerTask() {
                @Override
                public void run() {
                    pendingItems = null;
                    pendingItemsTimer = null;
                }
            },
                10000);

        }

        PebbleKit.sendDataToPebble(applicationContext, PEBBLE_OISHOPPING_UUID, respData);

    }

    private int copyItemsToDict(ArrayList<ShoppingItem> itemList, int startKey, PebbleDictionary respData) {

        // Set an upper limit on returned size based on PebbleDictionary's limit of 255
        if (itemList.size() > 200)
            itemList = new ArrayList<>(itemList.subList(0, 201));

        respData.addInt32(1, itemList.size());

        int dictKey = startKey;

        int dictSizeBytes = 0;

        for (ListIterator<ShoppingItem> itmIt = itemList.listIterator(); itmIt.hasNext(); ) {

            ShoppingItem itm = itmIt.next();

            respData.addInt32(dictKey, itm.id);
            dictKey++;

            String name = itm.name;
            if (name.length() > 127)
                name = name.substring(0, 127);

            respData.addString(dictKey, name);
            dictKey++;

            respData.addUint8(dictKey, (byte) (itm.status % 128));
            dictKey++;

            // Spare position...
            dictKey++;

            dictSizeBytes += SIZE_PER_SHOPPING_ITEM + name.length();
            if (dictSizeBytes >= PEBBLE_BUFFER_SIZE) {

                // Undo last item
                dictKey--;
                dictKey--;
                respData.remove(dictKey);
                dictKey--;
                respData.remove(dictKey);
                dictKey--;
                respData.remove(dictKey);

                return dictKey;
            }

            itmIt.remove();

        }

        return dictKey;

    }

    private void cmdSetBought(final PebbleDictionary data) {

        int listID = data.getInteger(1).intValue();
        int itemID = data.getInteger(2).intValue();

        Log.d(LOG_CLASS, "CMD_SET_BOUGHT for ID " + listID + " / " + itemID);

        OIShoppingConsumer.updateStatus(resolver,
                listID, itemID, OIShoppingConsumer.BOUGHT);

        PebbleDictionary respData = new PebbleDictionary();

        respData.addInt32(0, RES_ITEM_BOUGHT);
        respData.addInt32(1, listID);
        respData.addInt32(2, itemID);

        PebbleKit.sendDataToPebble(applicationContext, PEBBLE_OISHOPPING_UUID, respData);
    }

    private void cmdSetToBuy(final PebbleDictionary data) {

        int listID = data.getInteger(1).intValue();
        int itemID = data.getInteger(2).intValue();

        Log.d(LOG_CLASS, "CMD_SET_TOBUY for ID " + listID + " / " + itemID);

        OIShoppingConsumer.updateStatus(resolver,
                listID, itemID, OIShoppingConsumer.WANT_TO_BUY);

        PebbleDictionary respData = new PebbleDictionary();

        respData.addInt32(0, RES_ITEM_TOBUY);
        respData.addInt32(1, listID);
        respData.addInt32(2, itemID);

        PebbleKit.sendDataToPebble(applicationContext, PEBBLE_OISHOPPING_UUID, respData);
    }

    private void cmdGetAllLists(final PebbleDictionary data) {

        Log.d(LOG_CLASS, "CMD_GET_ALL_LISTS");

        PebbleDictionary respData = new PebbleDictionary();

        respData.addInt32(0, RES_ALL_LISTS_START);

        ArrayList<ShoppingList> itemList = OIShoppingConsumer.getAllLists(resolver);

        Collections.sort(itemList,
                new Comparator<ShoppingList>() {

                    @Override
                    public int compare(ShoppingList lhs, ShoppingList rhs) {
                        return lhs.name.compareToIgnoreCase(rhs.name);
                    }

                });

        respData.addInt32(1, itemList.size());

        int dictKey = DATA_ITEM_START_KEY;

        for (ShoppingList itm : itemList) {

            respData.addInt32(dictKey, itm.id);
            dictKey++;

            String name = itm.name;
            if (name.length() > 127)
                name = name.substring(0, 127);

            respData.addString(dictKey, name);
            dictKey++;

        }

        PebbleKit.sendDataToPebble(applicationContext, PEBBLE_OISHOPPING_UUID, respData);
    }

    private void cmdAddItem(final PebbleDictionary data) {

        int listID = data.getInteger(1).intValue();

        PebbleDictionary respData = new PebbleDictionary();

        // Direct ID number
        if (data.contains(3)) {

            int itemId = data.getInteger(3).intValue();

            Log.d(LOG_CLASS, "addItem to list " + listID + " with ID " + itemId);

            ShoppingItem itm = OIShoppingConsumer.getItemDetails(resolver, listID, itemId);

            if (itm.status != OIShoppingConsumer.REMOVED_FROMLIST) {
                respData.addInt32(0, RES_ITEM_ALREADY_EXISTS);
                PebbleKit.sendDataToPebble(applicationContext, PEBBLE_OISHOPPING_UUID, respData);
                return;
            }

            OIShoppingConsumer.updateStatus(resolver, listID, itemId,
                    OIShoppingConsumer.WANT_TO_BUY);

            respData.addInt32(0, RES_ITEM_ADDED);
            respData.addInt32(1, itemId);

            String name = itm.name;
            if (name.length() > 127)
                name = name.substring(0, 127);

            respData.addString(2, name);

            PebbleKit.sendDataToPebble(applicationContext, PEBBLE_OISHOPPING_UUID, respData);
            return;
        }

        String itemName = data.getString(2);

        Log.d(LOG_CLASS, "addItem to list " + listID + " called " + itemName);

        ArrayList<ShoppingItem> itemList = OIShoppingConsumer.searchForItem(resolver, listID, itemName);

        if (itemList.size() > 1) {

            // See if the list contains only one item that is not shown yet
            ArrayList<ShoppingItem> tempList = new ArrayList<ShoppingItem>(itemList);

            for (ListIterator<ShoppingItem> it = tempList.listIterator(); it.hasNext(); ) {

                ShoppingItem itm = it.next();

                if (itm.status != OIShoppingConsumer.REMOVED_FROMLIST)
                    it.remove();

            }

            // It does, so use that item as the list
            if (tempList.size() == 1)
                itemList = tempList;

        }

        // No items found...
        if (itemList.size() == 0) {
            // TODO... create a new item? return error?
            respData.addInt32(0, RES_ITEM_ALREADY_EXISTS);
            PebbleKit.sendDataToPebble(applicationContext, PEBBLE_OISHOPPING_UUID, respData);
            return;
        }

        // If there is exactly one item, check it off and return success
        if (itemList.size() == 1) {

            ShoppingItem itm = itemList.get(0);

            if (itm.status != OIShoppingConsumer.REMOVED_FROMLIST) {
                respData.addInt32(0, RES_ITEM_ALREADY_EXISTS);
                PebbleKit.sendDataToPebble(applicationContext, PEBBLE_OISHOPPING_UUID, respData);
                return;
            }

            OIShoppingConsumer.updateStatus(resolver, listID, itm.id,
                    OIShoppingConsumer.WANT_TO_BUY);

            respData.addInt32(0, RES_ITEM_ADDED);
            respData.addInt32(1, itm.id);

            String name = itm.name;
            if (name.length() > 127)
                name = name.substring(0, 127);

            respData.addString(2, name);

            PebbleKit.sendDataToPebble(applicationContext, PEBBLE_OISHOPPING_UUID, respData);
            return;
        }

        // There is more than one possible match, so return the items

        respData.addInt32(0, RES_FOUND_ITEMS);

        Collections.sort(itemList,
                new Comparator<ShoppingItem>() {

                    @Override
                    public int compare(ShoppingItem lhs, ShoppingItem rhs) {
                        return lhs.name.compareToIgnoreCase(rhs.name);
                    }

                });

        // Limit to 10 results
        if (itemList.size() > 10)
            itemList = new ArrayList<ShoppingItem>(itemList.subList(0, 10));

        copyItemsToDict(itemList, DATA_ITEM_START_KEY, respData);

        PebbleKit.sendDataToPebble(applicationContext, PEBBLE_OISHOPPING_UUID, respData);

        if (itemList.size() > 0) {
            Log.d(LOG_CLASS, "TODO: Queue " + itemList.size() + " items");
        }

    }

}