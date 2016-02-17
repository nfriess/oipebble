package com.nathan.oipebble;

import android.content.Intent;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.util.Log;

/**
 * A dummy activity for when the app is launched by the user.
 * This will start the background service if not running.
 */
public class MainActivity extends AppCompatActivity {

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        Log.d("MainActivity", "Starting service...");
        Intent svcIntent = new Intent(this, MainService.class);
        startService(svcIntent);
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
    }
}
