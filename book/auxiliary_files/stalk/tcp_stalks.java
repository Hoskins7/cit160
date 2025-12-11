
/* simple talk TCP server */
/* newline is to be included at server side */

import java.net.*;
import java.io.*;

public class stalks {

    static public int destport = 5431;
    static public int bufsize = 512;

    static public void main(String args[]) {
        ServerSocket ss;
        Socket s;
        try {
            ss = new ServerSocket(destport);
        } catch (IOException ioe) {
            System.err.println("can't create server socket with port=" + destport);
            return;
        }

        System.out.println("Waiting on port " + destport);

        while(true) { // accept loop
            try {
                s = ss.accept();
            } catch (IOException ioe) {
                System.err.println("Can't accept");
                break;
            }
            /* setSoTimeout example (not really needed here) 
            final int TIMEOUT = 12000;
            if (TIMEOUT != 0) try {
                s.setSoTimeout(TIMEOUT);       // time in milliseconds
            } catch (SocketException se) {
                System.err.println("socket exception: timeout not set!");
            }
            /* */
            line_talker(s);
            /* 
            InputStream istr;
            try { istr = s.getInputStream(); }
            catch (IOException ioe) {System.err.println("I/O error!"); return;}
            System.err.println("New connection from <" + 
                s.getInetAddress().getHostAddress() + "," + s.getPort() + ">");
            byte[] buf = new byte[bufsize];
            int len;
            while (true) {
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
                if (len == -1) break;
                String str = new String(buf, 0, len);
                System.out.println(str);
            } //while reading from s
            try {istr.close();}
            catch (IOException ioe) {System.err.println("bad stream close");return;}
            try {s.close();} 
            catch (IOException ioe) {System.err.println("bad socket close");return;}
            /* */
        } // accept loop
    } // end of main

    public static void line_talker(Socket s) {
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

        while (true) {
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
            if (len == -1) break;
            String str = new String(buf, 0, len);
            System.out.print(str);   // str should contain newline
        } //while reading from s

        int port = s.getPort();
        try {istr.close();}
        catch (IOException ioe) {System.err.println("bad stream close");return;}
        try {s.close();} 
        catch (IOException ioe) {System.err.println("bad socket close");return;}
        System.err.println("socket to port " + port + " closed");
    } // line_talker

}
