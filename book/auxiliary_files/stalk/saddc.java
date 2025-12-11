/* simpletalk-addition client, UDP version 
 * client sends a list of numbers, in binary network byte order; server prints their sum
 */

import java.util.*;
import java.net.*;      
import java.io.*;

//import java.io.Externalizable;

public class saddc {

    static public BufferedReader bin;
    static public int destport = 5434;
    static public int bufsize = 512;
    static public int SHORTSIZE = 2;
    static public int INTSIZE   = 4;

    //============================================================

    static public void main(String args[]) {
	String desthost = "localhost";
	if (args.length >= 1) desthost = args[0];

        bin = new BufferedReader(new InputStreamReader(System.in));

        InetAddress dest;
        System.err.print("Looking up address of " + desthost + "...");
        try {
            dest = InetAddress.getByName(desthost);	// DNS query
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
            System.err.println("cannot create socket");
            return;
        }

	System.err.println("port=" + s.getLocalPort());

        DatagramPacket msg = new DatagramPacket(new byte[0], 0, dest, destport);

        //============================================================

        while (true) {
            String buf;
            try { 
		buf = bin.readLine();
	    } catch (IOException ioe) {
                System.err.println("readLine() failed");
                return;
            }

            if (buf == null) break;	// user typed EOF character

	    // break the line up into individual numeric strings; convert the strings to ints; create list
	    String[] theTokens = buf.split("[^0-9]");
	    ArrayList<Integer> theNums = new ArrayList<Integer>();
	    for (String tok : theTokens) {
		if (tok.length() == 0) continue;
		int theNum = Integer.parseInt(tok);
		theNums.add(theNum);
	    }
	    
	    // build packet
	    int size = SHORTSIZE + INTSIZE*theNums.size();
            ByteArrayOutputStream baos = new ByteArrayOutputStream(size);	// size is optional here
            DataOutputStream dos = new DataOutputStream(baos);
	    try {
	        dos.writeShort(theNums.size());
	        for (int n : theNums) {
		    dos.writeInt(n);
	        }
            } catch (IOException ioe) {
                System.err.println("number conversion failed");
                break;
            }

	    byte[] bbuf = baos.toByteArray();

            msg.setData(bbuf);
            msg.setLength(size);

            try {
                s.send(msg);
            } catch (IOException ioe) {
                System.err.println("send() failed");
                return;
            }
        } // while
        s.close();
    }
}
