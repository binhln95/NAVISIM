package com.navis.navisim;

import android.content.Intent;
import android.os.Bundle;
import android.support.design.widget.FloatingActionButton;
import android.support.design.widget.Snackbar;
import android.support.v7.app.AppCompatActivity;
import android.support.v7.widget.Toolbar;
import android.util.Log;
import android.view.View;
import android.widget.EditText;
import android.widget.LinearLayout;
import android.widget.RadioButton;
import android.widget.TextView;
import android.widget.Toast;

import java.util.List;

public class Setting extends AppCompatActivity {
    public static String CSV = "circle.csv";
    public static String TXT = "triumphv3.txt";
    public static String trajectory = "brdc3540.14n";
    public static double Lat = 30.286502;
    public static double Lon = 120.032669;
    public static double Hgt = 100;
    public static int Time = 6;
    public static int checkSection = 0; // 0: CSV, 1: TXT, 2: Tọa độ

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_setting);
        Toolbar toolbar = (Toolbar) findViewById(R.id.toolbar);
        setSupportActionBar(toolbar);
        SqlController sqlController = new SqlController(this);
        List<String> list;
        list = sqlController.selectData(SqlController.TIME, SqlController.SETTING, 2);
        EditText e = (EditText) findViewById(R.id.time);
        e.setText(list.get(0), TextView.BufferType.EDITABLE);
        LinearLayout linearLayout = (LinearLayout) findViewById(R.id.viewCoor);
        linearLayout.setVisibility(View.GONE);
        LinearLayout viewTXT = (LinearLayout) findViewById(R.id.viewTXT);
        viewTXT.setVisibility(View.GONE);

        EditText editTextLat = (EditText)findViewById(R.id.editLat);
        list = sqlController.selectData(SqlController.LAT, SqlController.SETTING, 2);
        editTextLat.setText(list.get(0), TextView.BufferType.EDITABLE);
        EditText editTextLon = (EditText)findViewById(R.id.editLon);
        list = sqlController.selectData(SqlController.LON, SqlController.SETTING, 2);
        editTextLon.setText(list.get(0), TextView.BufferType.EDITABLE);
        EditText editTextHgt = (EditText)findViewById(R.id.editHgt);
        list = sqlController.selectData(SqlController.HGT, SqlController.SETTING, 2);
        editTextHgt.setText(list.get(0), TextView.BufferType.EDITABLE);

        EditText csv = (EditText)findViewById(R.id.openFileCSV);
        list = sqlController.selectData(SqlController.CSV, SqlController.SETTING, 2);
        csv.setText(list.get(0));
        EditText txt = (EditText)findViewById(R.id.openFileTXT);
        list = sqlController.selectData(SqlController.TXT, SqlController.SETTING, 2);
        txt.setText(list.get(0));

        TextView textView = (TextView) findViewById(R.id.trajectory);
        textView.setText(trajectory);

        final RadioButton radioCoor = (RadioButton) findViewById(R.id.coor);
        RadioButton radioCsv = (RadioButton) findViewById(R.id.csv);
        RadioButton radioTxt = (RadioButton) findViewById(R.id.txt);
        if (checkSection == 0) {
            radioCsv.setChecked(true);
            radioTxt.setChecked(false);
            radioCoor.setChecked(false);
        } else if (checkSection == 1) {
            radioCsv.setChecked(false);
            radioTxt.setChecked(true);
            radioCoor.setChecked(false);
        } else if (checkSection == 2) {
            radioCsv.setChecked(false);
            radioTxt.setChecked(false);
            radioCoor.setChecked(true);
        } else {
            Toast.makeText(this, "" + checkSection, Toast.LENGTH_LONG).show();
        }
        View v = (View) findViewById(R.id.viewSetting);
        ShowText(v);

        // test
    }

    public void Save(View view) {
        RadioButton coor = (RadioButton) findViewById(R.id.coor);
        RadioButton csv = (RadioButton) findViewById(R.id.csv);
        RadioButton txt = (RadioButton) findViewById(R.id.txt);
        EditText e = (EditText) findViewById(R.id.time);
        SqlController sqlController = new SqlController(this);
        String timeView = String.valueOf(e.getText());
        int timeViewint = Integer.parseInt(timeView);
        Time = timeViewint;
        sqlController.updateDB(String.valueOf(e.getText()), SqlController.TIME);
        TextView traj = (TextView) findViewById(R.id.trajectory);
        sqlController.updateDB(SqlController.TRAJECTORY, String.valueOf(traj.getText()));
        if (coor.isChecked()) {
            checkSection = 2;
            EditText editLat = (EditText)findViewById(R.id.editLat);
            sqlController.updateDB(String.valueOf(editLat.getText()), SqlController.LAT);
            EditText editLon = (EditText)findViewById(R.id.editLon);
            sqlController.updateDB(String.valueOf(editLon.getText()), SqlController.LON);
            EditText editHgt = (EditText)findViewById(R.id.editHgt);
            sqlController.updateDB(String.valueOf(editHgt.getText()), SqlController.HGT);
            sqlController.updateDB(String.valueOf(3), SqlController.CHECKSECTION);
        } else if (csv.isChecked()) {
            checkSection = 0;
            sqlController.updateDB(String.valueOf(1), SqlController.CHECKSECTION);
        } else if (txt.isChecked()) {
            checkSection = 1;
            sqlController.updateDB(String.valueOf(2), SqlController.CHECKSECTION);
        }
        Toast.makeText(this, "Update success", Toast.LENGTH_LONG).show();
    }

    public void ShowText(View view) {
        LinearLayout viewCoor = (LinearLayout) findViewById(R.id.viewCoor);
        LinearLayout viewTXT = (LinearLayout) findViewById(R.id.viewTXT);
        LinearLayout viewCSV = (LinearLayout) findViewById(R.id.viewCSV);
        RadioButton coor = (RadioButton) findViewById(R.id.coor);
        RadioButton txt = (RadioButton) findViewById(R.id.txt);
        RadioButton csv = (RadioButton) findViewById(R.id.csv);
        if (coor.isChecked()) {
            viewCoor.setVisibility(View.VISIBLE);
            viewCSV.setVisibility(View.GONE);
            viewTXT.setVisibility(View.GONE);
        } else {
            viewCoor.setVisibility(View.GONE);
            if (csv.isChecked()) {
                viewCSV.setVisibility(View.VISIBLE);
                viewTXT.setVisibility(View.GONE);
            } else {
                viewCSV.setVisibility(View.GONE);
                if (txt.isChecked()) {
                    viewTXT.setVisibility(View.VISIBLE);
                } else {
                    viewTXT.setVisibility(View.GONE);
                }
            }
        }

    }

    public void openCSV(View view) {
        Intent intent = new Intent(Intent.ACTION_PICK);
        intent.setType("*/*");
        startActivityForResult(intent.createChooser(intent, "Select csv"), 1);
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data)
    {
        super.onActivityResult(requestCode, resultCode, data);
        SqlController sqlController = new SqlController(this);
        // check if the request code is same as what is passed  here it is 2
        if (data != null) {
            if (requestCode == 1) {
                if (data.getDataString() != null) {
                    String message = data.getDataString().substring(7);
                    if (message.substring(message.lastIndexOf(".")).equals(".csv")) {
                        Toast.makeText(this, message.substring(message.lastIndexOf(".")), Toast.LENGTH_LONG).show();
                        EditText csv = (EditText) findViewById(R.id.openFileCSV);
                        csv.setText(message);
                        sqlController.updateDB(message, SqlController.CSV);
                    } else {
                        Toast.makeText(this, "Phải chọn file có phần mở rộng là csv", Toast.LENGTH_LONG).show();
                    }
                }
            } else if (requestCode == 2) {
                if (data.getDataString() != null) {
                    String message = data.getDataString().substring(7);
                    if (message.substring(message.lastIndexOf(".")).equals(".txt")) {
                        Toast.makeText(this, message.substring(message.lastIndexOf(".")), Toast.LENGTH_LONG).show();
                        EditText txt = (EditText) findViewById(R.id.openFileTXT);
                        txt.setText(message);
                        sqlController.updateDB(message, SqlController.TXT);
                    } else {
                        Toast.makeText(this, "Phải chọn file có phần mở rộng là txt", Toast.LENGTH_LONG).show();
                    }
                }
            } else if (requestCode == 3) {
                if (data.getDataString() != null) {
                    String message = data.getDataString().substring(7);
                    if (message.substring(message.lastIndexOf(".")).equals(".14n")) {
                        TextView traj = (TextView) findViewById(R.id.trajectory);
                        traj.setText(message);
                        sqlController.updateDB(message, SqlController.TRAJECTORY);
                    } else {
                        Toast.makeText(this, "Phải chọn file có phần mở rộng là 14n", Toast.LENGTH_LONG).show();
                    }
                }
            }
        }
    }

    public void openTXT(View view) {
        Intent intent = new Intent(Intent.ACTION_PICK);
        intent.setType("*/*");
        startActivityForResult(intent.createChooser(intent, "Select txt"), 2);
    }

    public void openTraj(View view) {
        Intent intent = new Intent(Intent.ACTION_PICK);
        intent.setType("*/*");
        startActivityForResult(intent.createChooser(intent, "Select 14n"), 3);
    }
}
