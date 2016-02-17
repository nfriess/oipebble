package com.nathan.oipebble;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;

/**
 * Recieves system boot notification and starts the main service.
 */
public class SystemBootReceiver extends BroadcastReceiver {


    @Override
    public void onReceive(Context context, Intent intent) {

        Intent svcIntent = new Intent(context, MainService.class);

        context.startService(svcIntent);

    }

}