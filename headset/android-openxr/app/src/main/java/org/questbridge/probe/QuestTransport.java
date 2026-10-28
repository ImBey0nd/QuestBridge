// SPDX-License-Identifier: GPL-3.0-or-later
package org.questbridge.probe;

import java.net.DatagramPacket;
import java.net.DatagramSocket;
import java.net.InetAddress;
import java.net.SocketTimeoutException;
import java.net.PortUnreachableException;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.security.MessageDigest;
import java.security.SecureRandom;
import java.util.Arrays;
import java.util.concurrent.atomic.AtomicReference;
import java.util.function.Consumer;
import javax.crypto.Mac;
import javax.crypto.spec.SecretKeySpec;

/** Session-paired v1 sender. Keys and destinations are never persisted or logged. */
public final class QuestTransport implements AutoCloseable {
 private static final int HEADER = 40, TAG = 32, MAX_PACKET = 1200;
 private static final long STALE_NS = 200_000_000L, REPLY_TIMEOUT_NS = 2_000_000_000L;
 private static final class Sample {
  final byte[] bytes; final long receivedNs;
  Sample(byte[] data) { bytes = data.clone(); receivedNs = System.nanoTime(); }
 }
 private final AtomicReference<Sample> latest = new AtomicReference<>();
 private final byte[] key;
 private final InetAddress address;
 private final int port;
 private final Consumer<String> status;
 private final Thread worker;
 private volatile boolean closed;
 private volatile DatagramSocket socket;
 private final byte[] neutral = new byte[156];
 public QuestTransport(String ip, int destinationPort, String hexKey, Consumer<String> listener) throws Exception {
  if (hexKey == null || !hexKey.matches("[0-9a-fA-F]{64}")) throw new IllegalArgumentException("Use a 64-digit pairing key");
  if (destinationPort < 1 || destinationPort > 65535) throw new IllegalArgumentException("Invalid receiver port");
  String[] parts = ip == null ? new String[0] : ip.split("\\.", -1);
  if (parts.length != 4) throw new IllegalArgumentException("Use the receiver's IPv4 address");
  byte[] raw = new byte[4];
  for (int i = 0; i < 4; i++) {
   if (!parts[i].matches("[0-9]{1,3}") || Integer.parseInt(parts[i]) > 255) throw new IllegalArgumentException("Invalid IPv4 address");
   raw[i] = (byte) Integer.parseInt(parts[i]);
  }
  address = InetAddress.getByAddress(raw);
  if (address.isAnyLocalAddress() || address.isMulticastAddress() || Arrays.equals(raw, new byte[]{-1,-1,-1,-1})) throw new IllegalArgumentException("Use a unicast receiver address");
  port = destinationPort; key = new byte[32];
  for (int i = 0; i < key.length; i++) key[i] = (byte) Integer.parseInt(hexKey.substring(2*i, 2*i+2), 16);
  status = listener;
  ByteBuffer identity = ByteBuffer.wrap(neutral).order(ByteOrder.BIG_ENDIAN);
  for (int i = 0; i < 3; i++) identity.putFloat(i*52+24, 1);
  worker = new Thread(this::run, "QuestBridge UDP");
 }
 public void start() { worker.start(); }
 public void publish(byte[] tracking) {
  validateTracking(tracking);
  latest.set(new Sample(tracking));
 }
 public void invalidate() { latest.set(null); }
 private static void validateTracking(byte[] bytes) {
  if (bytes == null || bytes.length != 156) throw new IllegalArgumentException("Invalid tracking payload");
  ByteBuffer data = ByteBuffer.wrap(bytes).order(ByteOrder.BIG_ENDIAN);
  for (int i=0;i<3;i++) {
   float[] values = new float[11];
   for (int j=0;j<11;j++) { values[j]=data.getFloat(i*52+j*4); if(!Float.isFinite(values[j]))throw new IllegalArgumentException("Nonfinite tracking"); }
   for(int j=0;j<3;j++)if(Math.abs(values[j])>1000)throw new IllegalArgumentException("Tracking position range");
   double norm=0;for(int j=3;j<7;j++)norm+=(double)values[j]*values[j];
   if(Math.abs(norm-1)>0.02)throw new IllegalArgumentException("Tracking orientation range");
   if(Math.abs(values[7])>1||Math.abs(values[8])>1||values[9]<0||values[9]>1||values[10]<0||values[10]>1)throw new IllegalArgumentException("Tracking input range");
   if((data.getInt(i*52+44)&~255)!=0||(data.getInt(i*52+48)&~15)!=0)throw new IllegalArgumentException("Tracking flags");
  }
 }
 private byte[] packet(Mac mac,int type,long session,long sequence,byte[] payload) {
  ByteBuffer out=ByteBuffer.allocate(HEADER+payload.length+TAG).order(ByteOrder.BIG_ENDIAN);
  out.putInt(0x51425247).putShort((short)1).putShort((short)type).putShort((short)payload.length).putShort((short)0);
  out.putLong(session).putLong(sequence).putLong(System.nanoTime()/1000).putInt(0).put(payload);
  mac.update(out.array(),0,HEADER+payload.length);out.put(mac.doFinal());return out.array();
 }
 private void send(Mac mac,int type,long session,long sequence,byte[] payload) throws Exception {
  byte[] bytes=packet(mac,type,session,sequence,payload);
  try { socket.send(new DatagramPacket(bytes,bytes.length)); }
  catch (PortUnreachableException unavailable) { /* Retry; receiver may be restarting. */ }
 }
 private void run() {
  try (DatagramSocket opened = new DatagramSocket()) {
   socket=opened;if(closed)return;
   opened.connect(address,port);opened.setSoTimeout(10);
   Mac mac=Mac.getInstance("HmacSHA256");mac.init(new SecretKeySpec(key,"HmacSHA256"));
   byte[] nonce=new byte[16], incoming=new byte[MAX_PACKET+1];SecureRandom random=new SecureRandom();random.nextBytes(nonce);
   long session=0,rx=0,tx=0,lastReply=System.nanoTime(),lastHello=0,lastPing=0,lastTracking=0,ping=0;
   status.accept("Pairing with receiver");
   while(!closed) {
    long now=System.nanoTime();
    if(session!=0 && now-lastReply>REPLY_TIMEOUT_NS){session=0;rx=0;tx=0;lastHello=0;random.nextBytes(nonce);status.accept("Receiver reply timeout; reconnecting");}
    if(session==0){if(lastHello==0||now-lastHello>=500_000_000L){send(mac,1,0,0,nonce);lastHello=now;}}
    else {
     if(lastPing==0||now-lastPing>=400_000_000L){ping=now/1000;send(mac,4,session,++tx,ByteBuffer.allocate(8).putLong(ping).array());lastPing=now;}
     if(lastTracking==0||now-lastTracking>=11_000_000L){Sample sample=latest.get();byte[] bytes=sample!=null&&now-sample.receivedNs<=STALE_NS?sample.bytes:neutral;send(mac,3,session,++tx,bytes);lastTracking=now;}
    }
    DatagramPacket datagram=new DatagramPacket(incoming,incoming.length);
    try{opened.receive(datagram);}catch(SocketTimeoutException | PortUnreachableException timeout){continue;}
    int length=datagram.getLength();
    if(length<HEADER+TAG||length>MAX_PACKET)continue;
    ByteBuffer header=ByteBuffer.wrap(incoming,0,length).order(ByteOrder.BIG_ENDIAN);
    if(header.getInt()!=0x51425247||header.getShort()!=1)continue;
    int type=Short.toUnsignedInt(header.getShort()), payloadLength=Short.toUnsignedInt(header.getShort());
    if(header.getShort()!=0||payloadLength!=length-HEADER-TAG)continue;
    long incomingSession=header.getLong(),sequence=header.getLong();header.getLong();if(header.getInt()!=0)continue;
    mac.update(incoming,0,length-TAG);byte[] signature=mac.doFinal();
    if(!MessageDigest.isEqual(signature,Arrays.copyOfRange(incoming,length-TAG,length)))continue;
    byte[] payload=Arrays.copyOfRange(incoming,HEADER,length-TAG);
    if(type==2&&session==0&&incomingSession!=0&&sequence!=0&&Arrays.equals(nonce,payload)){
     session=incomingSession;rx=sequence;lastReply=System.nanoTime();lastPing=0;status.accept("Receiver paired; tracking transport active");continue;
    }
    if(session==0||incomingSession!=session||Long.compareUnsigned(sequence,rx)<=0)continue;
    if(type!=5||payloadLength!=24||ByteBuffer.wrap(payload).getLong()!=ping)continue;
    rx=sequence;lastReply=System.nanoTime();
   }
  } catch (Exception error) {
   if(!closed)status.accept("Transport stopped; check receiver and network");
  } finally { socket=null; Arrays.fill(key,(byte)0); }
 }
 @Override public void close() {
  closed=true;DatagramSocket opened=socket;if(opened!=null)opened.close();
  if(Thread.currentThread()!=worker)try{worker.join(1500);}catch(InterruptedException interrupted){Thread.currentThread().interrupt();}
  // The worker must stop before key memory is overwritten.
  if(!worker.isAlive())Arrays.fill(key,(byte)0);
  latest.set(null);
 }
}
