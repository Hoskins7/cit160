 /* simplex-talk server, UDP version */

 import java.net.*;
 import java.io.*; 

 public class stalks {

     static public int destport = 5432;
     static public int bufsize = 512;
     static public final int timeout = 15000; 

     static public void main(String args[]) {
         DatagramSocket s;               // UDP uses DatagramSockets

         try {
             s = new DatagramSocket(destport);
         }
         catch (SocketException se) {
             System.err.println("cannot create socket with port " + destport);
             return;
         }
         try {
             s.setSoTimeout(timeout);       // set timeout in milliseconds
         } catch (SocketException se) {
             System.err.println("socket exception: timeout not set!");
         }

         // create DatagramPacket object for receiving data:
         DatagramPacket msg = new DatagramPacket(new byte[bufsize], bufsize);

         while(true) { // read loop
             try {
                 msg.setLength(bufsize);  // max received packet size
                 s.receive(msg);          // the actual receive operation
                 System.err.println("message from <" +
                    msg.getAddress().getHostAddress() + "," + msg.getPort() + ">");
             } catch (SocketTimeoutException ste) {    // receive() timed out
                 System.err.println("Response timed out!");
                 continue;
             } catch (IOException ioe) {                // should never happen!
                 System.err.println("Bad receive");
                 break;
             }

             String str = new String(msg.getData(), 0, msg.getLength());
             System.out.print(str);        // newline must be part of str
         }
         s.close();
     } // end of main
 }

