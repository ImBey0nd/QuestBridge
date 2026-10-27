// SPDX-License-Identifier: GPL-3.0-or-later
import org.questbridge.probe.QuestTransport;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.util.Arrays;

/** JVM interoperability fixture. All poses are synthetic, not Quest measurements. */
public final class TransportHarness {
 public static void main(String[] args) throws Exception {
  ByteBuffer data=ByteBuffer.allocate(156).order(ByteOrder.BIG_ENDIAN);
  for(int i=0;i<3;i++){data.putFloat(i*52+4,1.6f);data.putFloat(i*52+24,1);data.putInt(i*52+48,15);}
  try(QuestTransport transport=new QuestTransport("127.0.0.1",Integer.parseInt(args[0]),args[1],System.out::println)){
   // Invalid application samples must never reach the wire.
   for(byte[] bad:new byte[][]{new byte[155],new byte[156]}){
    boolean rejected=false;try{transport.publish(bad);}catch(IllegalArgumentException expected){rejected=true;}
    if(!rejected)throw new AssertionError("Invalid sample accepted");
   }
   transport.start();
   long end=System.nanoTime()+1_300_000_000L;
   while(System.nanoTime()<end){transport.publish(data.array());Thread.sleep(20);}
   System.out.println("FIXTURE_STALE");
   Thread.sleep(900);
   System.out.println("FIXTURE_RESTART");
   end=System.nanoTime()+5_500_000_000L;
   while(System.nanoTime()<end){transport.publish(data.array());Thread.sleep(20);}
  }
 }
}
