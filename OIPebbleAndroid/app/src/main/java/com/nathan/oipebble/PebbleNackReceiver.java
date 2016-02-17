package com.nathan.oipebble;

import android.content.Context;

import com.getpebble.android.kit.PebbleKit;

/**
 * Helper class to wrap PebbleKit.PebbleNackReceiver and pass to
 * our message receiver.
 */
public class PebbleNackReceiver extends PebbleKit.PebbleNackReceiver {

    private PebbleMessageReceiver msgReceiver;

    public PebbleNackReceiver(PebbleMessageReceiver msgReceiver) {
        super(PebbleMessageReceiver.PEBBLE_OISHOPPING_UUID);
        this.msgReceiver = msgReceiver;
    }

    @Override
    public void receiveNack(Context context, int transactionId) {
        msgReceiver.receiveNack(context, transactionId);
    }
}
