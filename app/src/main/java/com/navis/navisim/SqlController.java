package com.navis.navisim;

import android.content.ContentValues;
import android.content.Context;
import android.database.Cursor;
import android.database.sqlite.SQLiteDatabase;
import android.database.sqlite.SQLiteOpenHelper;
import android.util.Log;

import java.util.ArrayList;
import java.util.List;

public class SqlController extends SQLiteOpenHelper {
    public static final String DATA_BASE_NAME = "binhln.db";
    public static final int VERSION = 1;
    public static final String SETTING = "setting";

    public static final String ID = "id";
    public static final String KEY = "key";
    public static final String VALUE = "value";

    public static final String CSV = "csv";
    public static final String TXT = "txt";
    public static final String LAT = "lat";
    public static final String LON = "lon";
    public static final String HGT = "hgt";
    public static final String TIME = "time";
    public static final String CHECKSECTION = "check_section";
    public static final String TRAJECTORY = "brdc3540.14n";
    public SqlController(Context context) {
        super(context, DATA_BASE_NAME, null, VERSION);
        SQLiteDatabase db = this.getWritableDatabase();
    }

    @Override
    public void onCreate(SQLiteDatabase sqLiteDatabase) {
        String query = "CREATE TABLE IF NOT EXISTS " + SETTING + "("
                + ID + " INTEGER PRIMARY KEY,"
                + KEY + " TEXT,"
                + VALUE + " TEXT" + ")";
        sqLiteDatabase.execSQL(query);
    }

    @Override
    public void onUpgrade(SQLiteDatabase sqLiteDatabase, int i, int i1) {

    }

    public boolean insert (String key, String value) {
        SQLiteDatabase db = getWritableDatabase();
//        Cursor c = db.rawQuery("SELECT name FROM sqlite_master WHERE type='table'", null);

        ContentValues contentValues = new ContentValues();
        contentValues.put(KEY, key);
        contentValues.put(VALUE, value);
        db.insert(SETTING, null, contentValues);
        return true;
    }

    public List<String> selectData(String key, String table, int order) {
        List<String> listString = new ArrayList<String>();
        SQLiteDatabase db = this.getReadableDatabase();
        Cursor cu = db.rawQuery("SELECT * FROM " + table + " WHERE " + KEY + " =? ", new String[]{key});
        if (cu.moveToFirst()) {
            while ( !cu.isAfterLast() ) {
                //Toast.makeText(this, "Table Name=> "+cu.getString(2), Toast.LENGTH_LONG).show();
                listString.add(cu.getString(order));
                cu.moveToNext();
            }
        }
        return listString;
    }

    public void insertDataFirst() {
        if (selectData(CSV, SqlController.SETTING, 2).size() == 0) {
            insert(SqlController.CSV, Setting.CSV);
        } else {
            List<String> stringList = selectData(CSV, SETTING, 2);
            Setting.CSV = stringList.get(0);
        }

        if (selectData(TXT, SqlController.SETTING, 2).size() == 0) {
            insert(SqlController.TXT, Setting.TXT);
        } else {
            List<String> stringList = selectData(TXT, SETTING, 2);
            Setting.TXT = stringList.get(0);
        }

        if (selectData(LAT, SqlController.SETTING, 2).size() == 0) {
            insert(SqlController.LAT, String.valueOf(Setting.Lat));
        } else {
            List<String> stringList = selectData(LAT, SETTING, 2);
            Setting.Lat = Double.parseDouble(stringList.get(0));
        }

        if (selectData(LON, SqlController.SETTING, 2).size() == 0) {
            insert(SqlController.LON, String.valueOf(Setting.Lon));
        } else {
            List<String> stringList = selectData(LON, SETTING, 2);
            Setting.Lon = Double.parseDouble(stringList.get(0));
        }

        if (selectData(HGT, SqlController.SETTING, 2).size() == 0) {
            insert(SqlController.HGT, String.valueOf(Setting.Hgt));
        } else {
            List<String> stringList = selectData(HGT, SETTING, 2);
            Setting.Hgt = Double.parseDouble(stringList.get(0));
        }

        if (selectData(TIME, SqlController.SETTING, 2).size() == 0) {
            insert(SqlController.TIME, String.valueOf(Setting.Time));
        } else {
            List<String> stringList = selectData(TIME, SETTING, 2);
            Setting.Time = Integer.parseInt(stringList.get(0));
        }

        if (selectData(TRAJECTORY, SqlController.SETTING, 2).size() == 0) {
            insert(SqlController.TRAJECTORY, String.valueOf(Setting.trajectory));
        } else {
            List<String> stringList = selectData(TRAJECTORY, SETTING, 2);
            Setting.trajectory = stringList.get(0);
        }
    }

    public void updateDB(String value, String key) {
        SQLiteDatabase sqLiteDatabase = this.getWritableDatabase();
        String sql = "UPDATE " + SETTING + " SET " + VALUE + " = \"" + value + "\" WHERE " + KEY + " = \"" + key + "\";";
        Log.d("BinhLN", sql);
        sqLiteDatabase.execSQL(sql);
        sqLiteDatabase.close();
    }
}
