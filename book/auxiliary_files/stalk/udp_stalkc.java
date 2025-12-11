 // simplex-talk CLIENT in java, UDP version

 import java.net.*;    
 import java.io.*;

 public class stalkc {

     static public BufferedReader bin;
     static public int destport = 5432;
     static public int bufsize = 512;

     static public void main(String args[]) {
         String desthost = "localhost";
         if (args.length >= 1) desthost = args[0];

         bin = new BufferedReader(new InputStreamReader(System.in));

         InetAddress dest;
         System.err.print("Looking up address of " + desthost + "...");
         try {
             dest = InetAddress.getByName(desthost);        // DNS query
         }
         catch (UnknownHostException uhe) {
             System.err.println("unknown host: " + desthost);
             return;
         }
         System.err.println(" got it!");
 
         DatagramSocket s;
         try {
             s = new DatagramSocket();
         }
         catch(IOException ioe) {
             System.err.println("socket could not be created");
             return;
         }

         System.err.println("Our own port is " + s.getLocalPort());

         DatagramPacket msg = new DatagramPacket(new byte[0], 0, dest, destport);

         while (true) {
             String buf;
             int slen;
             try { 
                 buf = bin.readLine();
             }
             catch (IOException ioe) {
                 System.err.println("readLine() failed");
                 return;
             }
 
             if (buf == null) break;      // user typed EOF character
 
             buf = buf + "\n";	   	  // append newline character
             slen = buf.length();
             byte[] bbuf = buf.getBytes();
 
             msg.setData(bbuf);
             msg.setLength(slen);
 
             try {
                 s.send(msg);
             }
             catch (IOException ioe) {
                 System.err.println("send() failed");
                 return;
             }
         } // while
         s.close();
     }
 }

