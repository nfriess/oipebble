package com.nathan.oipebble;

import android.content.Context;

import com.getpebble.android.kit.PebbleKit;

import java.util.UUID;

/**
 * Helper class to wrap PebbleKit.PebbleAckReceiver and pass to
 * our message receiver.
 */
public class PebbleAckReceiver extends PebbleKit.PebbleAckReceiver {

    private MessageReceiverInterface msgReceiver;

    public PebbleAckReceiver(MessageReceiverInterface msgReceiver, UUID uuid) {
        super(uuid);
        this.msgReceiver = msgReceiver;
    }

    @Override
    public void receiveAck(Context context, int transactionId) {
        msgReceiver.receiveAck(context, transactionId);
    }
}
