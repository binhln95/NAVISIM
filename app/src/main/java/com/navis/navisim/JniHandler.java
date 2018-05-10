package com.navis.navisim;
import android.os.Build;
import android.support.annotation.Keep;
import android.util.Log;


public class JniHandler {
    /*
     * Print out status to logcat
     */
    @Keep
    private void updateStatus(String msg) {
        if (msg.toLowerCase().contains("error")) {
            Log.e("JniHandler", "Native Err: " + msg);
        } else {
            Log.i("JniHandler", "Native Msg: " + msg);
        }
    }

    @Keep
    private void updateTD(String msg) {
        Log.d("BinhLN msg", msg);
    }

    /*
     * Return OS build version: a static function
     */
    @Keep
    static public String getBuildVersion() {
        return Build.VERSION.RELEASE;
    }

    /*
     * Return Java memory info
     */
    @Keep
    public long getRuntimeMemorySize() {
        return Runtime.getRuntime().freeMemory();
    }
}
