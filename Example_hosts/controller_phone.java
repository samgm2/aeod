package hello;
import java.io.IOException;
import java.io.OutputStream;
import javax.bluetooth.*;
import javax.microedition.io.*;
import javax.microedition.midlet.*;
import javax.microedition.lcdui.*;

public class HelloMIDlet extends MIDlet
        implements DiscoveryListener, CommandListener {

    private Command exitCommand; // The exit command
    private Display display;     // The display for this MIDlet
        /* We need a reference to LocalDevice to do anything concerning Bluetooth */
    private LocalDevice localDev = null;
       /* If used ??? */
    private DiscoveryAgent localAgent = null;
        /* L2CAP connection for me */
    private L2CAPConnection localSocket = null;
    
    public HelloMIDlet() {
        display = Display.getDisplay(this);
        exitCommand = new Command("Exit", Command.EXIT, 0);
    }

    public void startApp() {
        byte[] recvbytes;
        int recvlen = 0;
        recvbytes = new byte[32];



        Alert a = null;
        try {
            localDev = LocalDevice.getLocalDevice();

        }catch(BluetoothStateException bse) {
            a = new Alert("Bluetooth error","Either Bluetooth must be turned on, or your device does not support JABWT",null,AlertType.ERROR);
            a.setTimeout(Alert.FOREVER);
            a.addCommand(exitCommand);
            a.setCommandListener(this);
            display.setCurrent(a);
            return;
        }
            /* If need to use discovery to show devices */
        localAgent = localDev.getDiscoveryAgent();
        

        try {
            localSocket = (L2CAPConnection)Connector.open("btl2cap://0002780256CF:1001");
        } catch (IOException ex) {
            a = new Alert("Bluetooth error","Can't connect to bt stuff",null,AlertType.ERROR);
            a.setTimeout(Alert.FOREVER);
            a.addCommand(exitCommand);
            a.setCommandListener(this);
            display.setCurrent(a);
        }

        try {
            localSocket.send("Hello".getBytes());
        } catch (IOException ex) {
            ex.printStackTrace();
        }

        for(int i = 0 ; i < recvbytes.length ; i++) {
            recvbytes[i] = 0;
        }


        try {
            recvlen = localSocket.receive(recvbytes);
        } catch (IOException ex) {
            ex.printStackTrace();
        }
        TextBox t;

        if (recvlen > 0) {
            t = new TextBox("Hello", "Got:" + new String(recvbytes), 256, 0);
        } else {
            t = new TextBox("Hello", "Recv error!", 256, 0);
        }        

        t.addCommand(exitCommand);
        t.setCommandListener(this);
        if (a == null) {
            display.setCurrent(t);
        } else {
            display.setCurrent(a,t);
        }
    }

    public void pauseApp() {
    }

    public void destroyApp(boolean unconditional) {
    }

    public void commandAction(Command c, Displayable s) {
        if (c == exitCommand) {
            destroyApp(false);
            notifyDestroyed();
        } 
    }

    public void deviceDiscovered(RemoteDevice arg0, DeviceClass arg1) {
        throw new UnsupportedOperationException("Not supported yet.");
    }

    public void inquiryCompleted(int arg0) {
        throw new UnsupportedOperationException("Not supported yet.");
    }

    public void serviceSearchCompleted(int arg0, int arg1) {
        throw new UnsupportedOperationException("Not supported yet.");
    }

    public void servicesDiscovered(int arg0, ServiceRecord[] arg1) {
        throw new UnsupportedOperationException("Not supported yet.");
    }
}
