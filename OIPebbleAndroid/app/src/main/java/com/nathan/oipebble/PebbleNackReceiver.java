package com.nathan.oipebble;

import android.content.Context;

import com.getpebble.android.kit.PebbleKit;

import java.util.UUID;

/**
 * Helper class to wrap PebbleKit.PebbleNackReceiver and pass to
 * our message receiver.
 */
public class PebbleNackReceiver extends PebbleKit.PebbleNackReceiver {

    private MessageReceiverInterface msgReceiver;

    public PebbleNackReceiver(MessageReceiverInterface msgReceiver, UUID uuid) {
        super(uuid);
        this.msgReceiver = msgReceiver;
    }

    @Override
    public void receiveNack(Context context, int transactionId) {
        msgReceiver.receiveNack(context, transactionId);
    }
}
