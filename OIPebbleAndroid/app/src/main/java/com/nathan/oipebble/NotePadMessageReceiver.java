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
 * from the notepad app on a Pebble watch.
 */
public class NotePadMessageReceiver extends PebbleKit.PebbleDataReceiver implements MessageReceiverInterface {

    private static final String LOG_CLASS = "NotePadMessageReceiver";

    public static final UUID PEBBLE_OINOTEPAD_UUID = UUID.fromString("539015eb-e163-4988-adaa-fd0826726f18");

    /** The assumed buffer size in the pebble app. */
    private static final int PEBBLE_BUFFER_SIZE = 2048;

    // From pebble.h:
    //   buffer_size = 1 + (n * 7) + D1 + ... + Dn
    //   where n is number of items, and Dn is the size data for each item

    // Our item packets contain: int32, a string of max 128 bytes
    // ==> (7+4) + (7+1+strlen)

    /** Size of one note item when serialized, excluding the string length of the title and note. */
    private static final int SIZE_PER_NOTE_ITEM = (7+4) + (7+1) + (7+1);

    private static final int CMD_GET_LIST = 101;
    private static final int CMD_GET_NOTE = 102;

    private static final int RES_ERROR = 201;
    private static final int RES_NOTE_LIST_START = 202;
    private static final int RES_NOTE_LIST_CONTINUE = 203;
    private static final int RES_NOTE_DATA = 204;

    private static final int DATA_ITEM_START_KEY = 100;

    private final Context applicationContext;
    private final ContentResolver resolver;

    private ArrayList<NoteItem> pendingItems;
    private Timer pendingItemsTimer;

    public NotePadMessageReceiver(Context applicationContext, ContentResolver resolver) {
        super(PEBBLE_OINOTEPAD_UUID);
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
                    cmdGetList(data);
                    break;

                case CMD_GET_NOTE:
                    cmdGetNote(data);
                    break;

                default:
                    Log.e(LOG_CLASS, "Unknown command/response: " + cmd);
            }

        } catch (Exception ex) {
            Log.e(LOG_CLASS, "Exception", ex);
            try {
                PebbleDictionary respData = new PebbleDictionary();
                respData.addInt32(0, RES_ERROR);
                PebbleKit.sendDataToPebble(applicationContext, PEBBLE_OINOTEPAD_UUID, respData);
            } catch (Exception exx) {
            }
        }

    }

    @Override
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

    @Override
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

            respData.addInt32(0, RES_NOTE_LIST_CONTINUE);

            NoteItem lastKeyItem = pendingItems.get(0);
            pendingItems.remove(0);

            int lastKey = copyItemsToDict(pendingItems, lastKeyItem.id, respData);

            if (pendingItems.size() > 0) {
                Log.d(LOG_CLASS, "Queue " + pendingItems.size() + " more items");
                NoteItem itm = new NoteItem();
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

            PebbleKit.sendDataToPebble(applicationContext, PEBBLE_OINOTEPAD_UUID, respData);

        }

    }

    private void cmdGetList(final PebbleDictionary data) {

        Log.d(LOG_CLASS, "cmdGetList");

        PebbleDictionary respData = new PebbleDictionary();

        respData.addInt32(0, RES_NOTE_LIST_START);

        ArrayList<NoteItem> itemList = OINotePadConsumer.getAllNotes(resolver);

        Collections.sort(itemList,
                new Comparator<NoteItem>() {

                    @Override
                    public int compare(NoteItem lhs, NoteItem rhs) {
                        return lhs.title.compareToIgnoreCase(rhs.title);
                    }

                });

        int lastKey = copyItemsToDict(itemList, DATA_ITEM_START_KEY, respData);

        if (itemList.size() > 0) {
            Log.d(LOG_CLASS, "Queue " + itemList.size() + " items");
            NoteItem itm = new NoteItem();
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

        PebbleKit.sendDataToPebble(applicationContext, PEBBLE_OINOTEPAD_UUID, respData);

    }

    private int copyItemsToDict(ArrayList<NoteItem> itemList, int startKey, PebbleDictionary respData) {

        // Set an upper limit on returned size based on PebbleDictionary's limit of 255
        if (itemList.size() > 200)
            itemList = new ArrayList<>(itemList.subList(0, 201));

        respData.addInt32(1, itemList.size());

        int dictKey = startKey;

        int dictSizeBytes = 0;

        for (ListIterator<NoteItem> itmIt = itemList.listIterator(); itmIt.hasNext(); ) {

            NoteItem itm = itmIt.next();

            respData.addInt32(dictKey, itm.id);
            dictKey++;

            String title = itm.title;
            if (title.length() > 127)
                title = title.substring(0, 127);

            respData.addString(dictKey, title);
            dictKey++;

            dictSizeBytes += SIZE_PER_NOTE_ITEM + title.length();
            if (dictSizeBytes >= PEBBLE_BUFFER_SIZE) {

                // Undo last item
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

    private void cmdGetNote(final PebbleDictionary data) {

        Log.d(LOG_CLASS, "cmdGetNote");

        int noteID = data.getInteger(1).intValue();

        PebbleDictionary respData = new PebbleDictionary();

        respData.addInt32(0, RES_NOTE_DATA);

        String note = OINotePadConsumer.getNote(resolver, noteID);

        if (note.length() > 4000)
            note = note.substring(0, 4000);

        respData.addInt32(1, noteID);
        respData.addString(2, note);

        PebbleKit.sendDataToPebble(applicationContext, PEBBLE_OINOTEPAD_UUID, respData);

    }

}
