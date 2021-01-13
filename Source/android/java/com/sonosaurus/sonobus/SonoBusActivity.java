// SPDX-License-Identifier: GPLv3-or-later
// Copyright (C) 2020 Jesse Chappell


package com.sonosaurus.sonobus;

import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.content.ServiceConnection;
import android.content.ComponentName;
import android.os.IBinder;
import android.app.Activity;

import android.content.res.Configuration;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.net.Uri;
import android.os.Bundle;
import android.os.Looper;
import android.os.Handler;
import android.os.ParcelUuid;
import android.os.Environment;
import android.os.Build;
import android.view.*;
import android.graphics.*;
import java.util.*;
import java.io.*;

import  android.content.BroadcastReceiver;
import  android.content.IntentFilter;

import android.util.Log;
import java.lang.ref.WeakReference;



//==============================================================================
public class SonoBusActivity   extends Activity
{
    //==============================================================================
    private native void appNewIntent (Intent intent);

    private final static String TAG = "SonoBus";
 
    SonoBusService sbService = null;

    //==============================================================================
    private native void constructNativeClass();
    private native void destroyNativeClass();

    private long cppCounterpartInstance;

    private boolean inForeground = true;

    @Override
    public final void onCreate (Bundle savedInstanceState)
    {
        super.onCreate (savedInstanceState);

        // call the native C++ class constructor
        constructNativeClass();

        startService(new Intent(this, SonoBusService.class));
        // bind to the service.
        bindService(new Intent(this, SonoBusService.class), mConnection, Context.BIND_AUTO_CREATE);
        
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP) {
            Window window = getWindow();
            window.addFlags(WindowManager.LayoutParams.FLAG_DRAWS_SYSTEM_BAR_BACKGROUNDS);
            window.setNavigationBarColor(Color.BLACK);
        }
    }

    @Override
    protected final void onDestroy()
    {
        if (BuildConfig.DEBUG) Log.d(TAG, "onDestroy");

        // call the native C++ class destructor
        destroyNativeClass();

        super.onDestroy();

        if (sbService != null) {
            // Detach the service connection.
            unbindService(mConnection);
        }

    }


    private ServiceConnection mConnection = new ServiceConnection() {
       @Override
       public void onServiceConnected(ComponentName componentName, IBinder iBinder) {
               SonoBusService.MyBinder myBinder = (SonoBusService.MyBinder) iBinder;
               sbService = myBinder.getService();
               if (BuildConfig.DEBUG) Log.v(TAG, "Got SonoBusService Connection");
               // now you have the instance of service.
        }

        @Override
        public void onServiceDisconnected(ComponentName componentName) {
           sbService = null;
        }
    };


    public final void setForegroundServiceActive(boolean flag) 
    {
        if (sbService != null) {
            sbService.makeForegroundActive(flag);
        }
    }


    @Override
    protected void onNewIntent (Intent intent)
    {
        super.onNewIntent(intent);
        setIntent(intent);

        appNewIntent (intent);
    }
}
