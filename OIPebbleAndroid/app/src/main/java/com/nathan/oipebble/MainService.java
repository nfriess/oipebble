package com.nathan.oipebble;

import android.app.Service;
import android.content.BroadcastReceiver;
import android.content.Intent;
import android.os.IBinder;
import android.support.annotation.Nullable;
import android.util.Log;

import com.getpebble.android.kit.PebbleKit;

/**
 * Background service that listens for messages from a Pebble watch.
 */
public class MainService extends Service {

    private BroadcastReceiver shoppingDataReceiver = null;
    private BroadcastReceiver shoppingAckReceiver = null;
    private BroadcastReceiver shoppingNackReceiver = null;
    private BroadcastReceiver notepadDataReceiver = null;
    private BroadcastReceiver notepadAckReceiver = null;
    private BroadcastReceiver notepadNackReceiver = null;

    @Nullable
    @Override
    public IBinder onBind(Intent intent) {
        return null;
    }

    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {

        Log.d("OIPebbleService", "Start service, didReceiver=" + (shoppingDataReceiver != null));

        if (shoppingDataReceiver == null) {

            ShoppingMessageReceiver rcv = new ShoppingMessageReceiver(this, getContentResolver());

            shoppingAckReceiver = PebbleKit.registerReceivedAckHandler(this, new PebbleAckReceiver(rcv, ShoppingMessageReceiver.PEBBLE_OISHOPPING_UUID));
            shoppingNackReceiver = PebbleKit.registerReceivedNackHandler(this, new PebbleNackReceiver(rcv, ShoppingMessageReceiver.PEBBLE_OISHOPPING_UUID));

            shoppingDataReceiver = PebbleKit.registerReceivedDataHandler(this, rcv);

        }

        if (notepadDataReceiver == null) {

            NotePadMessageReceiver rcv = new NotePadMessageReceiver(this, getContentResolver());

            notepadAckReceiver = PebbleKit.registerReceivedAckHandler(this, new PebbleAckReceiver(rcv, NotePadMessageReceiver.PEBBLE_OINOTEPAD_UUID));
            notepadNackReceiver = PebbleKit.registerReceivedNackHandler(this, new PebbleNackReceiver(rcv, NotePadMessageReceiver.PEBBLE_OINOTEPAD_UUID));

            notepadDataReceiver = PebbleKit.registerReceivedDataHandler(this, rcv);

        }

        return START_STICKY;
    }

    @Override
    public void onDestroy() {

        if (shoppingDataReceiver != null) {
            unregisterReceiver(shoppingDataReceiver);
            unregisterReceiver(shoppingNackReceiver);
            unregisterReceiver(shoppingAckReceiver);
            shoppingDataReceiver = null;
            shoppingNackReceiver = null;
            shoppingAckReceiver = null;
        }

        if (notepadDataReceiver != null) {
            unregisterReceiver(notepadDataReceiver);
            unregisterReceiver(notepadNackReceiver);
            unregisterReceiver(notepadAckReceiver);
            notepadDataReceiver = null;
            notepadNackReceiver = null;
            notepadAckReceiver = null;
        }

        super.onDestroy();
    }
}
