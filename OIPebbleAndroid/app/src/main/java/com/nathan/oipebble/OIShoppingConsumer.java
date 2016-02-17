package com.nathan.oipebble;

import android.content.ContentResolver;
import android.content.ContentValues;
import android.database.Cursor;
import android.net.Uri;

import java.util.ArrayList;

/**
 * Methods for communicating with OI Shopping List provider.
 */
public class OIShoppingConsumer {

    public static final Uri CONTENT_URI_CONTAINS = Uri.parse("content://org.openintents.shopping/contains");
    public static final Uri CONTENT_URI_ITEMS = Uri.parse("content://org.openintents.shopping/items");
    public static final Uri CONTENT_URI_LISTS = Uri.parse("content://org.openintents.shopping/lists");

    // Status values
    public static final long WANT_TO_BUY = 1;
    public static final long BOUGHT = 2;
    public static final long REMOVED_FROMLIST = 3;

    public static ArrayList<ShoppingItem> getItemList(ContentResolver resolver, int listID) {

        Cursor curContains = resolver.query(CONTENT_URI_CONTAINS,
                new String[]{"item_id","status"},
                "list_id = ? AND (status = ? OR status = ?)", new String[]{"" + listID, "" + WANT_TO_BUY, "" + BOUGHT},
                null);

        if (curContains == null)
            throw new RuntimeException("query error");

        ArrayList<ShoppingItem> retData = new ArrayList<ShoppingItem>();

        if (curContains.getCount() < 1) {
            curContains.close();
            return retData;
        }

        while (curContains.moveToNext()) {

            int itemID = curContains.getInt(0);

            Cursor curItem = resolver.query(Uri.withAppendedPath(CONTENT_URI_ITEMS, "" + itemID),
                    new String[] {"name"},
                    null, null,
                    null);

            if (curItem == null || curItem.getCount() < 1)
                throw new RuntimeException("Item " + itemID + " is in contains list but does not exist?");

            curItem.moveToFirst();

            ShoppingItem sItem = new ShoppingItem();

            sItem.id = itemID;
            sItem.name = curItem.getString(0);
            sItem.status = curContains.getInt(1);

            retData.add(sItem);

            curItem.close();

        }

        curContains.close();

        return retData;
    }

    public static void updateStatus(ContentResolver resolver, int listID, int itemID, long newStatus) {

        ContentValues newValues = new ContentValues();
        newValues.put("status", newStatus);

        resolver.update(CONTENT_URI_CONTAINS,
                newValues,
                "list_id = ? AND item_id = ?",
                new String[]{"" + listID, "" + itemID});

    }

    public static ArrayList<ShoppingList> getAllLists(ContentResolver resolver) {

        Cursor cur = resolver.query(CONTENT_URI_LISTS,
                new String[]{"_id","name"},
                null, null,
                null);

        if (cur == null)
            throw new RuntimeException("query error");


        ArrayList<ShoppingList> retData = new ArrayList<ShoppingList>();

        if (cur.getCount() < 1) {
            cur.close();
            return retData;
        }

        while (cur.moveToNext()) {

            int listID = cur.getInt(0);
            String name = cur.getString(1);

            ShoppingList sItem = new ShoppingList();

            sItem.id = listID;
            sItem.name = name;

            retData.add(sItem);
        }

        cur.close();

        return retData;
    }

    public static ArrayList<ShoppingItem> searchForItem(ContentResolver resolver, int listID, String name) {

        Cursor curItem = resolver.query(CONTENT_URI_ITEMS,
                new String[] {"_id", "name"},
                "name = ?", new String[] {name},
                null);

        if (curItem == null)
            throw new RuntimeException("query error");

        ArrayList<ShoppingItem> retData = new ArrayList<ShoppingItem>();

        getItemsFromCursor(resolver, listID, curItem, retData);

        curItem.close();

        if (retData.size() > 0)
            return retData;

        curItem = resolver.query(CONTENT_URI_ITEMS,
                new String[] {"_id", "name"},
                "name LIKE ?", new String[] {"%"+name+"%"},
                null);

        if (curItem == null)
            throw new RuntimeException("query error");

        getItemsFromCursor(resolver, listID, curItem, retData);

        curItem.close();

        return retData;

    }

    private static void getItemsFromCursor(ContentResolver resolver, int listID, Cursor curItem, ArrayList<ShoppingItem> retData) {

        while (curItem.moveToNext()) {

            int itemID = curItem.getInt(0);

            Cursor curContains = resolver.query(CONTENT_URI_CONTAINS,
                    new String[]{"_id", "status"},
                    "list_id = ? AND item_id = ?", new String[] {"" + listID, "" + itemID},
                    null);

            if (curContains == null)
                throw new RuntimeException("query error");
            else if (curContains.getCount() < 1) {
                curContains.close();
                continue;
            }

            curContains.moveToFirst();

            ShoppingItem itm = new ShoppingItem();

            itm.id = itemID;
            itm.name = curItem.getString(1);
            itm.status = curContains.getInt(1);

            retData.add(itm);

            curContains.close();

        }

    }

    public static ShoppingItem getItemDetails(ContentResolver resolver, int listID, int itemID) {

        Cursor curItem = resolver.query(Uri.withAppendedPath(CONTENT_URI_ITEMS, "" + itemID),
                new String[] {"name"},
                null, null,
                null);

        if (curItem == null || curItem.getCount() < 1)
            throw new RuntimeException("Item " + itemID + " is in contains list but does not exist?");

        curItem.moveToFirst();

        ShoppingItem sItem = new ShoppingItem();

        sItem.id = itemID;
        sItem.name = curItem.getString(0);

        curItem.close();

        Cursor curContains = resolver.query(CONTENT_URI_CONTAINS,
                new String[]{"status"},
                "list_id = ? AND item_id = ?", new String[]{"" + listID, "" + itemID},
                null);

        if (curContains == null)
            throw new RuntimeException("query error");

        curContains.moveToFirst();

        sItem.status = curContains.getInt(0);

        curContains.close();

        return sItem;
    }

}
