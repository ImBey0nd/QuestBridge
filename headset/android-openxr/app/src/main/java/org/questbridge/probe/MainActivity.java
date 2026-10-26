package org.questbridge.probe;
import android.app.Activity;
import android.os.Bundle;
import android.util.Log;
import android.widget.TextView;
public final class MainActivity extends Activity {
 static { System.loadLibrary("questbridge_probe"); }
 private native String probeRuntime();
 @Override public void onCreate(Bundle state) {
  super.onCreate(state);
  String result = probeRuntime();
  Log.i("QuestBridge", result);
  TextView text = new TextView(this);
  text.setText("QuestBridge runtime probe\n\n" + result +
   "\n\nTracking, rendering, video decoding and PS5 connection are not implemented.");
  text.setTextSize(20); text.setPadding(32,32,32,32); setContentView(text);
 }
}
