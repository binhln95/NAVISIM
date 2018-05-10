package com.navis.navisim;

import android.Manifest;
import android.app.PendingIntent;
import android.app.ProgressDialog;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.pm.PackageManager;
import android.hardware.usb.UsbDevice;
import android.hardware.usb.UsbDeviceConnection;
import android.hardware.usb.UsbEndpoint;
import android.hardware.usb.UsbInterface;
import android.hardware.usb.UsbManager;
import android.support.annotation.Keep;
import android.support.v7.widget.Toolbar;
import android.nfc.Tag;
import android.os.Build;
import android.os.Environment;
import android.support.v4.app.ActivityCompat;
import android.support.v4.content.ContextCompat;
import android.support.v7.app.AlertDialog;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.util.Log;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.widget.Button;
import android.widget.TextView;
import android.widget.Toast;

import com.google.android.gms.maps.CameraUpdateFactory;
import com.google.android.gms.maps.GoogleMap;
import com.google.android.gms.maps.MapFragment;
import com.google.android.gms.maps.OnMapReadyCallback;
import com.google.android.gms.maps.UiSettings;
import com.google.android.gms.maps.model.LatLng;
import com.google.android.gms.maps.model.MarkerOptions;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.OutputStreamWriter;
import java.util.HashMap;
import java.util.Iterator;

public class MainActivity extends AppCompatActivity implements OnMapReadyCallback {
    // Used to load the 'native-lib' library on application startup.
    static {
        System.loadLibrary("native-lib");
    }
    public GoogleMap map;
// /home/binhnl/Downloads/android-ndk-master/android-ndk/hello-jniCallback/app/build/intermediates/cmake/debug/obj/arm64-v8a
    /**
     * A native method that is implemented by the 'native-lib' native library,
     * which is packaged with this application.
     */
    public native String stringFromJNI();
    public native int runSimulator(String ExternalStoragePath, int fileDescription, String usbDevicePath, int vid, int pid);
    public native int generateData(String ExternalStoragePath,String trajectory, int flag, int time);
    private static final int REQUEST_WRITE_STORAGE = 112;
    private static final int REQUEST_USB_ = 112;
    public UsbDevice usbDevice;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        Toolbar toolbar = (Toolbar) findViewById(R.id.toolbar);
        setSupportActionBar(toolbar);

        // create db
        SqlController s = new SqlController(this);
        s.insertDataFirst();

        File folderData = new File (Environment.getExternalStorageDirectory().getPath() + "/dataNAVISIM");
        if (!folderData.exists()) {
            folderData.mkdir();
        }
        stringFromJNI();
        test();
        MapFragment m = (MapFragment) getFragmentManager().findFragmentById(R.id.myMap);
        m.getMapAsync(this);
        Toast.makeText(this, "Start load map", Toast.LENGTH_SHORT).show();
        // Example of a call to a native method
//        TextView tv = (TextView) findViewById(R.id.sample_text);
//        tv.setText(stringFromJNI());

        if (ContextCompat.checkSelfPermission(this,
                Manifest.permission.WRITE_EXTERNAL_STORAGE)
                != PackageManager.PERMISSION_GRANTED) {

            // Permission is not granted
            // Should we show an explanation?
            if (ActivityCompat.shouldShowRequestPermissionRationale(this,
                    Manifest.permission.WRITE_EXTERNAL_STORAGE)) {

                // Show an explanation to the user *asynchronously* -- don't block
                // this thread waiting for the user's response! After the user
                // sees the explanation, try again to request the permission.

            } else {

                // No explanation needed; request the permission
                ActivityCompat.requestPermissions(this,
                        new String[]{Manifest.permission.WRITE_EXTERNAL_STORAGE},
                        REQUEST_WRITE_STORAGE);

                // MY_PERMISSIONS_REQUEST_READ_CONTACTS is an
                // app-defined int constant. The callback method gets the
                // result of the request.
            }
        } else {
            // Permission has already been granted
        }
    }

    public void messageMe() {
        Toast.makeText(this, "success", Toast.LENGTH_LONG).show();
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        // Inflate the menu; this adds items to the action bar if it is present.
        getMenuInflater().inflate(R.menu.menu_main, menu);
        return true;
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        // Handle action bar item clicks here. The action bar will
        // automatically handle clicks on the Home/Up button, so long
        // as you specify a parent activity in AndroidManifest.xml.
        int id = item.getItemId();

        //noinspection SimplifiableIfStatement
        if (id == R.id.action_settings) {
            Intent i = new Intent(this, Setting.class);
            startActivity(i);
            return true;
        }

        return super.onOptionsItemSelected(item);
    }

    private Boolean OpenUsbDevice (int VID,int PID){
        UsbManager manager = (UsbManager)getSystemService(Context.USB_SERVICE);
        int dvVid,dvPid;
        Boolean res = false;
        HashMap<String, UsbDevice> deviceList = manager.getDeviceList();
        Iterator<UsbDevice> deviceIterator = deviceList.values().iterator();
        while(deviceIterator.hasNext()) {
            UsbDevice device = deviceIterator.next();
            dvVid = device.getVendorId();
            dvPid = device.getProductId();
            if((dvVid==VID)&(dvPid==PID)){
                res = true;
                usbDevice = device;
            }
        }
        return res;
    }

    private static final String ACTION_USB_PERMISSION =
            "com.android.example.USB_PERMISSION";

    private final BroadcastReceiver mUsbReceiver = new BroadcastReceiver() {

        public void onReceive(Context context, Intent intent) {
            String action = intent.getAction();
            if (ACTION_USB_PERMISSION.equals(action)) {
                synchronized (this) {
                    UsbManager manager = (UsbManager) getSystemService(Context.USB_SERVICE);
                    UsbDevice device = (UsbDevice)intent.getParcelableExtra(UsbManager.EXTRA_DEVICE);

                    if (intent.getBooleanExtra(UsbManager.EXTRA_PERMISSION_GRANTED, false)) {
                        if (!manager.hasPermission(device)) {
                            Log.d("DEBUG","Permissions were granted but can't access the device");
                        } else {
                            Log.d("DEBUG","Permissions granted and device is accessible");
                        }
                    }
                    else {
                        Log.d("ERROR","permission denied for device " + device);
                    }
                }
            }
        }
    };


    public void generateSimData(View view){
        AlertDialog.Builder builder;
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP) {
            builder = new AlertDialog.Builder(this, android.R.style.Theme_Material_Dialog_Alert);
        } else {
            builder = new AlertDialog.Builder(this);
        }

        try {
        String pathFile = "";
        if (Setting.checkSection == 0) {
            pathFile = Setting.CSV;
        } else if (Setting.checkSection == 1) {
            pathFile = Setting.TXT;
        } else if (Setting.checkSection == 2) {
            pathFile = Setting.Lat + "," + Setting.Lon + "," + Setting.Hgt;
        } else {
            Toast.makeText(this, "Error check section: " + Setting.checkSection, Toast.LENGTH_SHORT).show();
            return;
        }

        int runTime = generateData(pathFile, Setting.trajectory, Setting.checkSection, Setting.Time);
        Toast.makeText(this, "end: " + runTime, Toast.LENGTH_SHORT).show();
        builder.setTitle("Finished")
                .setMessage("The file was generated in " + Setting.Time + "s")
                .setPositiveButton(android.R.string.yes, new DialogInterface.OnClickListener() {
                    public void onClick(DialogInterface dialog, int which) {
                        // continue with delete
                    }
                })
                .setNegativeButton(android.R.string.no, new DialogInterface.OnClickListener() {
                    public void onClick(DialogInterface dialog, int which) {
                        // do nothing
                    }
                })
                .setIcon(android.R.drawable.ic_dialog_alert)
                .show();

        }
        catch (Exception e){
            Log.d("DEBUG", e.getMessage());
        }
    }

    public void generateSignal(View view){
        int fileDescription = -1;
        String usbDevicePath = "";
        AlertDialog.Builder builder;
        boolean bulkCtrlAvailable = false;
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP) {
            builder = new AlertDialog.Builder(this, android.R.style.Theme_Material_Dialog_Alert);
        } else {
            builder = new AlertDialog.Builder(this);
        }

        try {
            boolean isUsbConnected = OpenUsbDevice(0x1D50,0x6108);
//            boolean isUsbConnected = OpenUsbDevice(0x0BDA, 0x2832);//RTL
            if (isUsbConnected) {
                UsbManager mUsbManager = (UsbManager) getSystemService(Context.USB_SERVICE);
                PendingIntent mPermissionIntent = PendingIntent.getBroadcast(this, 0, new Intent(ACTION_USB_PERMISSION), 0);
                IntentFilter filter = new IntentFilter(ACTION_USB_PERMISSION);
                registerReceiver(mUsbReceiver, filter);
                mUsbManager.requestPermission(usbDevice, mPermissionIntent);
                if (!mUsbManager.hasPermission(usbDevice)) {

                    return;
                }

                UsbDeviceConnection connection = mUsbManager.openDevice(usbDevice);
                // if we make this, kernel driver will be disconnected
                UsbInterface usbInterface = usbDevice.getInterface(0);
                connection.claimInterface(usbInterface, true);


                Log.d("DEBUG", "inserting device with id: " + usbDevice.getDeviceId() + " and file descriptor: " + connection.getFileDescriptor());
                fileDescription = connection.getFileDescriptor();
                usbDevicePath = usbDevice.getDeviceName();
                for(int i=0; i<usbInterface.getEndpointCount();i++){
                    UsbEndpoint endpoint = usbInterface.getEndpoint(i);
                    int endPointAddr = endpoint.getAddress();
                    if((endPointAddr==0x0F)||(endPointAddr==0x8F))
                        bulkCtrlAvailable = true;
                }

                if(!bulkCtrlAvailable){
                    builder.setTitle("Error")
                            .setMessage("Cannot found USB endpoint")
                            .setPositiveButton(android.R.string.yes, new DialogInterface.OnClickListener() {
                                public void onClick(DialogInterface dialog, int which) {
                                    // continue with delete
                                }
                            })
                            .setNegativeButton(android.R.string.no, new DialogInterface.OnClickListener() {
                                public void onClick(DialogInterface dialog, int which) {
                                    // do nothing
                                }
                            })
                            .setIcon(android.R.drawable.ic_dialog_alert)
                            .show();
                    return;
                }

                int runTime = runSimulator(Environment.getExternalStorageDirectory().getPath(), fileDescription,
                        usbDevicePath,usbDevice.getVendorId(),usbDevice.getProductId());

                if(runTime<0){
                    builder.setTitle("Error")
                            .setMessage("Signal generating errors")
                            .setPositiveButton(android.R.string.yes, new DialogInterface.OnClickListener() {
                                public void onClick(DialogInterface dialog, int which) {
                                    // continue with delete
                                }
                            })
                            .setNegativeButton(android.R.string.no, new DialogInterface.OnClickListener() {
                                public void onClick(DialogInterface dialog, int which) {
                                    // do nothing
                                }
                            })
                            .setIcon(android.R.drawable.ic_dialog_alert)
                            .show();
                }
                else{
                    builder.setTitle("Finished")
                            .setMessage("Generated signal in " + runTime + "s")
                            .setPositiveButton(android.R.string.yes, new DialogInterface.OnClickListener() {
                                public void onClick(DialogInterface dialog, int which) {
                                    // continue with delete
                                }
                            })
                            .setNegativeButton(android.R.string.no, new DialogInterface.OnClickListener() {
                                public void onClick(DialogInterface dialog, int which) {
                                    // do nothing
                                }
                            })
                            .setIcon(android.R.drawable.ic_dialog_alert)
                            .show();
                }
                connection.releaseInterface(usbInterface);
                connection.close();
            } else {
                Toast.makeText(this, "Not connect device", Toast.LENGTH_SHORT).show();
            }
        }
        catch (Exception e){
            Log.d("DEBUG", e.getMessage());
        }
    }

    @Override
    public void onMapReady(GoogleMap googleMap) {
        map = googleMap;
//        LatLng HN = new LatLng(21.0056183, 105.8433475);
////         googleMap.setMyLocationEnabled(true);
//        googleMap.moveCamera(CameraUpdateFactory.newLatLngZoom(HN, 13));
//        googleMap.addMarker(new MarkerOptions()
//                .title("HUST")
//                .snippet("Trường đại học Bách khoa Hà Nội")
//                .position(HN));
//        UiSettings uiSettings = googleMap.getUiSettings();
//        uiSettings.setZoomControlsEnabled(true);
    }

    public void test() {
        LatLng HN = new LatLng(21.0056183, 105.8433475);
//         googleMap.setMyLocationEnabled(true);
        if (map == null) {
            Log.d("BinhLN", "null");
            return;
        }
        map.moveCamera(CameraUpdateFactory.newLatLngZoom(HN, 13));
        map.addMarker(new MarkerOptions()
                .title("HUST")
                .snippet("Trường đại học Bách khoa Hà Nội")
                .position(HN));
        UiSettings uiSettings = map.getUiSettings();
        uiSettings.setZoomControlsEnabled(true);
    }

    public void callBack(String msg) {
        Log.d("BinhLN", msg);
    }

    public void callBack1(double msg, double msg1, double msg2) {
        Log.d("BinhLN", "" + msg);
        Log.d("BinhLN", "" + msg1);
        Log.d("BinhLN", "" + msg2);
    }
}
