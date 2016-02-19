package com.nathan.oipebble;

import android.content.Context;

/**
 * Interface for both MessageReceiver classes, so that PebbleAckReceiver
 * and PebbleNackReceiver can pass along acks/nacks.
 */
public interface MessageReceiverInterface {
    void receiveNack(Context context, int transactionId);

    void receiveAck(Context context, int transactionId);
}
