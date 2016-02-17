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

    private BroadcastReceiver dataReceiver = null;
    private BroadcastReceiver ackReceiver = null;
    private BroadcastReceiver nackReceiver = null;

    @Nullable
    @Override
    public IBinder onBind(Intent intent) {
        return null;
    }

    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {

        Log.d("OIPebbleService", "Start service, didReceiver=" + (dataReceiver != null));

        if (dataReceiver == null) {
            PebbleMessageReceiver rcv = new PebbleMessageReceiver(this, getContentResolver());

            ackReceiver = PebbleKit.registerReceivedAckHandler(this, new PebbleAckReceiver(rcv));
            nackReceiver = PebbleKit.registerReceivedNackHandler(this, new PebbleNackReceiver(rcv));

            dataReceiver = PebbleKit.registerReceivedDataHandler(this, rcv);

        }

        return START_STICKY;
    }

    @Override
    public void onDestroy() {

        unregisterReceiver(dataReceiver);
        unregisterReceiver(nackReceiver);
        unregisterReceiver(ackReceiver);
        dataReceiver = null;
        nackReceiver = null;
        ackReceiver = null;

        super.onDestroy();
    }
}
