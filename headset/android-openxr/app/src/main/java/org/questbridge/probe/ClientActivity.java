package org.questbridge.probe;

import android.app.NativeActivity;
import android.os.Bundle;
import android.util.Log;
import java.util.concurrent.atomic.AtomicReference;

/** Real stereo/tracking diagnostic. Optional transport uses transient launch settings. */
public final class ClientActivity extends NativeActivity {
 private static final class Settings {
  final String host, key; final int port;
  Settings(String address, String secret, int number) { host = address; key = secret; port = number; }
 }
 private static final AtomicReference<Settings> pending = new AtomicReference<>();
 static void setReceiver(String host, String key, int port) { pending.set(new Settings(host, key, port)); }
 private QuestTransport transport;
 private boolean acceptSamples;
 @Override public void onCreate(Bundle state) {
  super.onCreate(state);
  Settings settings = pending.getAndSet(null);
  if (settings != null) {
   try {
    transport = new QuestTransport(settings.host, settings.port, settings.key, text -> Log.i("QuestBridge", text));
    // Settings are process-local and consumed once; no key enters task Intent state.
    transport.start();
   } catch (Exception error) { Log.e("QuestBridge", "Invalid transport settings; running local diagnostic"); }
  }
 }
 // Called on the native OpenXR thread. Only the latest sample is retained.
 @SuppressWarnings("unused")
 public synchronized void onTracking(byte[] bytes) { if (acceptSamples && transport != null) transport.publish(bytes); }
 @Override public void onResume() { super.onResume(); synchronized (this) { acceptSamples = true; } }
 @Override public void onPause() { synchronized (this) { acceptSamples = false; if (transport != null) transport.invalidate(); } super.onPause(); }
 @Override public void onDestroy() {
  QuestTransport old;
  synchronized (this) { acceptSamples = false; old = transport; transport = null; }
  if (old != null) old.close();
  // NativeActivity may wait for the XR thread. Never hold this monitor here.
  super.onDestroy();
 }
}
