package com.nathan.oipebble;

import android.content.Context;

import com.getpebble.android.kit.PebbleKit;

/**
 * Helper class to wrap PebbleKit.PebbleAckReceiver and pass to
 * our message receiver.
 */
public class PebbleAckReceiver extends PebbleKit.PebbleAckReceiver {

    private PebbleMessageReceiver msgReceiver;

    public PebbleAckReceiver(PebbleMessageReceiver msgReceiver) {
        super(PebbleMessageReceiver.PEBBLE_OISHOPPING_UUID);
        this.msgReceiver = msgReceiver;
    }

    @Override
    public void receiveAck(Context context, int transactionId) {
        msgReceiver.receiveAck(context, transactionId);
    }
}
