
/* simpletalk-addition server, UDP version 
 * client sends a list of numbers, in binary network byte order; server prints their sum
 */

import java.net.*;
import java.io.*;

public class sadds {

    static public int destport = 5434;
    static public int bufsize = 512;

    static public void main(String args[]) {
        DatagramSocket s;
	InetAddress dest;

        try {
                s = new DatagramSocket(destport);
        }
        catch (SocketException se) {
	        System.err.println("cannot create socket with port " + destport);
                return;
        }

        DatagramPacket msg = new DatagramPacket(new byte[bufsize], bufsize);

        while(true) { // read loop
            try {
                        msg.setLength(bufsize);
                        s.receive(msg);
                        System.err.println("message from <" +
                                msg.getAddress().getHostAddress() +
                                "," + msg.getPort() + ">");
             } catch (IOException ioe) {           // should never happen!
                    System.err.println("Bad receive");
                    break;
            }

            ByteArrayInputStream bais = new ByteArrayInputStream(msg.getData(), 0, msg.getLength());
            DataInputStream dis = new DataInputStream(bais);
	    int sum = 0;
            try {
                int count = dis.readShort();
	        for (int i=0; i<count; i++) {
		    sum += dis.readInt();
	        }
            } catch (IOException ioe) { 
                System.err.println("DATA packet conversion failed");
            	continue; 
            }

            System.out.println("The sum of the numbers received is: " + sum); 
        } // end of read loop
        s.close();
    } // end of main
}
