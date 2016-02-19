package com.nathan.oipebble;

import android.content.ContentResolver;
import android.database.Cursor;
import android.net.Uri;

import java.util.ArrayList;

/**
 * Methods for communicating with OI NotePad provider.
 */
public class OINotePadConsumer {

    public static final Uri CONTENT_URI_NOTES = Uri.parse("content://org.openintents.notepad/notes");

    public static ArrayList<NoteItem> getAllNotes(ContentResolver resolver) {

        Cursor curNotes = resolver.query(CONTENT_URI_NOTES,
                new String[]{"_id","title","note"},
                null, null,
                "title");

        if (curNotes == null)
            throw new RuntimeException("query error");

        ArrayList<NoteItem> retData = new ArrayList<NoteItem>();

        if (curNotes.getCount() < 1) {
            curNotes.close();
            return retData;
        }

        while (curNotes.moveToNext()) {

            NoteItem nItem = new NoteItem();

            nItem.id = curNotes.getInt(0);
            nItem.title = curNotes.getString(1);
            nItem.note = curNotes.getString(2);

            retData.add(nItem);

        }

        curNotes.close();

        return retData;
    }

    public static String getNote(ContentResolver resolver, int noteID) {

        Cursor curNote = resolver.query(Uri.withAppendedPath(CONTENT_URI_NOTES, "" + noteID),
                new String[]{"note"},
                null, null,
                null);

        if (curNote == null)
            throw new RuntimeException("query error");

        if (curNote.getCount() != 1)
            return null;


        curNote.moveToFirst();

        String note = curNote.getString(0);

        curNote.close();

        return note;

    }

}
