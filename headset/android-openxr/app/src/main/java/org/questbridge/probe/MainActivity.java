package org.questbridge.probe;
import android.app.Activity;
import android.os.Bundle;
import android.util.Log;
import android.widget.TextView;
import android.widget.Button;
import android.widget.EditText;
import android.widget.ScrollView;
import android.widget.Toast;
import android.text.InputType;
import android.widget.LinearLayout;
import android.content.Intent;
public final class MainActivity extends Activity {
 static { System.loadLibrary("questbridge_probe"); }
 private native String probeRuntime();
 @Override public void onCreate(Bundle state) {
  super.onCreate(state);
  String result = probeRuntime();
  Log.i("QuestBridge", result);
  TextView text = new TextView(this);
  text.setText("QuestBridge runtime probe\n\n" + result +
   "\n\nStart the stereo diagnostic to test head and controller tracking. Video streaming and PS5 VR integration remain unavailable.");
  text.setTextSize(20); text.setPadding(32,32,32,32);
  LinearLayout layout = new LinearLayout(this); layout.setOrientation(LinearLayout.VERTICAL);
  layout.addView(text);
  Button start = new Button(this); start.setText("Start stereo and tracking diagnostic");
  start.setOnClickListener(view -> startActivity(new Intent(this, ClientActivity.class).addCategory("com.oculus.intent.category.VR")));
  layout.addView(start);
  EditText receiver = new EditText(this); receiver.setHint("Receiver IPv4 (optional)"); receiver.setSingleLine(true);
  receiver.setInputType(InputType.TYPE_CLASS_PHONE); receiver.setSaveEnabled(false); layout.addView(receiver);
  EditText key = new EditText(this); key.setHint("Pairing key: 64 hex digits"); key.setSingleLine(true);
  key.setInputType(InputType.TYPE_CLASS_TEXT | InputType.TYPE_TEXT_VARIATION_PASSWORD);
  key.setSaveEnabled(false); layout.addView(key);
  Button connect = new Button(this); connect.setText("Start with receiver (UDP 42420)");
  connect.setOnClickListener(view -> {
   String address = receiver.getText().toString().trim(), pairing = key.getText().toString().trim();
   if (!pairing.matches("[0-9a-fA-F]{64}") || address.isEmpty()) { Toast.makeText(this,"Enter receiver IPv4 and pairing key",Toast.LENGTH_SHORT).show(); return; }
   ClientActivity.setReceiver(address, pairing, 42420);
   Intent intent = new Intent(this, ClientActivity.class).addCategory("com.oculus.intent.category.VR");
   key.setText(""); startActivity(intent);
  });
  layout.addView(connect); ScrollView scroll = new ScrollView(this); scroll.addView(layout); setContentView(scroll);
 }
}
