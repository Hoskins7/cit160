 
 /* THREADED simplex-talk TCP server */
 /* can handle multiple CONCURRENT client connections */
 /* newline is to be included at client side */
 
 import java.net.*;
 import java.io.*;
 
 public class tstalks {
 
     static public int destport = 5431;
     static public int bufsize = 512;
     static public boolean THREADING = true;
 
     static public void main(String args[]) {
         ServerSocket ss;
         Socket s;
         try {
             ss = new ServerSocket(destport);
         } catch (IOException ioe) {
             System.err.println("can't create server socket");
             return;
         }
         System.err.println("server starting on port " + ss.getLocalPort());
     
         while(true) { // accept loop
             try {
                 s = ss.accept();
             } catch (IOException ioe) {
                 System.err.println("Can't accept");
                 break;
             }
     
             if (THREADING) {
                 Talker talk = new Talker(s);
                 (new Thread(talk)).start();
             } else {
                 line_talker(s);
             }
         } // accept loop
     } // end of main
 
     public static void line_talker(Socket s) {
 	int port = s.getPort();
         InputStream istr;
         try { istr = s.getInputStream(); }
         catch (IOException ioe) {
             System.err.println("cannot get input stream");        // most likely cause: s was closed
             return;
         }
         System.err.println("New connection from <" + 
             s.getInetAddress().getHostAddress() + "," + s.getPort() + ">");
         byte[] buf = new byte[bufsize];
         int len;
 
         while (true) {		// while not done reading the socket
             try {
                 len = istr.read(buf, 0, bufsize);
             }
             catch (SocketTimeoutException ste) {
                 System.out.println("socket timeout");
                 continue;
             }
             catch (IOException ioe) {
                 System.err.println("bad read");
                 break;        // probably a socket ABORT; treat as a close
             }
             if (len == -1) break;		// other end closed gracefully
             String str = new String(buf, 0, len);
             System.out.print("" + port + ": " + str);  // str should contain newline
         } //while reading from s
 
 
         try {istr.close();}
         catch (IOException ioe) {System.err.println("bad stream close");return;}
         try {s.close();} 
         catch (IOException ioe) {System.err.println("bad socket close");return;}
         System.err.println("socket to port " + port + " closed");
     } // line_talker
 
     static class Talker implements Runnable {
         private Socket _s;
 
         public Talker (Socket s) {
             _s = s;
         }
 
         public void run() {
             line_talker(_s);
         } // run
     }  // class Talker
 }
