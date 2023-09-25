
package ESPlorer;

import jssc.SerialPortEvent;
import jssc.SerialPortEventListener;
import jssc.SerialPortException;

/**
 *
 * @author spy
 */
public class Port {
    private class PortTurboReader implements SerialPortEventListener {

        @Override
        public void serialEvent(SerialPortEvent event) {
            if (event.isRXCHAR() && event.getEventValue() > 0) {
                String data = "";
                try {
                    data = serialPort.readString(event.getEventValue());
                } catch (SerialPortException ex) {
                    log(ex.toString());
                }
                rcvBuf = rcvBuf + data;
                String l = data.replace("\r", "<CR>");
                l = l.replace("\n", "<LF>");
                l = l.replace("`", "<OK>");
                log("recv:" + l);
                TerminalAdd(data);
                if (rcvBuf.contains("> ")) {
                    try {
                        timeout.stop(); // first, reset watchdog timer
                    } catch (Exception e) {
                    }
                    rcvBuf = "";
                    if (j < sendBuf.size() - 1) {
                        if (timer.isRunning() || sendPending) {
                            // waiting
                        } else {
                            inc_j();
                            sendPending = true;
                            int div = sendBuf.size() - 1;
                            if (div == 0) {
                                div = 1;
                            }
                            ProgressBar.setValue((j * 100) / div);
                            timer.start();
                        }
                    } else { // send done
                        StopSend();
                    }
                }
            } else if (event.isCTS()) {
                UpdateLedCTS();
            } else if (event.isERR()) {
                log("FileManager: Unknown serial port error received.");
            }
        }
    }
    
    private class PortReader implements SerialPortEventListener {

        @Override
        public void serialEvent(SerialPortEvent event) {
            if (event.isRXCHAR() && event.getEventValue() > 0) {
                try {
                    String data = serialPort.readString(event.getEventValue());
                    if (portJustOpen) {
                        TerminalAdd("Got answer! Communication with MCU established.\r\nAutoDetect firmware...\r\n");
                        portJustOpen = false;
                        try {
                            openTimeout.stop();
                        } catch (Exception e) {
                            log(e.toString());
                        }
                        UpdateButtons();
                        if (data.contains("\r\n>>>")) {
                            TerminalAdd("\r\nMicroPython firmware detected, try get version...\r\n\r\n");
                            btnSend("import sys; print(\"MicroPython ver:\",sys.version_info)");
                            LeftTab.setSelectedIndex(0);
                            SetFirmwareType(FIRMWARE_MPYTHON);
                        } else {
                            TerminalAdd("\r\nCan't autodetect firmware, because proper answer not received (may be unknown firmware). \r\nPlease, reset module or continue.\r\n");
                        }
                    } else if (LocalEcho) {
                        TerminalAdd(data);
                        System.out.println(data);
                        TextEditorList.get(0).setText(data.replace("'", "").replace("\\n","\n"));
                    } else if (data.contains("\r")) {
                        LocalEcho = true;
                        TerminalAdd(data.substring(data.indexOf("\r")));
                    }
                } catch (SerialPortException ex) {
                    log(ex.toString());
                }
            } else if (event.isCTS()) {
                UpdateLedCTS();
            } else if (event.isERR()) {
                log("FileManager: Unknown serial port error received.");
            }
        }
    }

    private class PortExtraReader implements SerialPortEventListener {

        @Override
        public void serialEvent(SerialPortEvent event) {
            if (event.isRXCHAR() && event.getEventValue() > 0) {
                String data = "";
                try {
                    data = serialPort.readString(event.getEventValue());
                } catch (SerialPortException ex) {
                    log(ex.toString());
                }
                data = data.replace(">> ", "");
                data = data.replace(">>", "");
                data = data.replace("\r\n> ", "");
                data = data.replace("\r\n\r\n", "\r\n");

                rcvBuf = rcvBuf + data;
                log("recv:" + data.replace("\r\n", "<CR><LF>"));
                TerminalAdd(data);
                if (rcvBuf.contains(sendBuf.get(j).trim())) {
                    // first, reset watchdog timer
                    try {
                        timeout.stop();
                    } catch (Exception e) {
                        Logger.getLogger(ESPlorer.class.getName()).log(Level.SEVERE, null, e);
                    }
  
                    rcvBuf = "";
                    if (j < sendBuf.size() - 1) {
                        if (timer.isRunning() || sendPending) {
                            // waiting
                        } else {
                            inc_j();
                            sendPending = true;
                            int div = sendBuf.size() - 1;
                            if (div == 0) {
                                div = 1;
                            }
                            ProgressBar.setValue((j * 100) / div);
                            timer.start();
                        }
                    } else {  // send done
                        StopSend();
                    }
                }
                if (rcvBuf.contains("Type \"help()")) {
                    StopSend();
                    String msg[] = {"ESP module reboot detected!", "Event: internal MicroPython exception or power fail.", "Please, try again."};
                    JOptionPane.showMessageDialog(null, msg);
                }
            } else if (event.isCTS()) {
                UpdateLedCTS();
            } else if (event.isERR()) {
                log("FileManager: Unknown serial port error received.");
            }
        }
    }
    
    public boolean portOpen() {
        String portName = GetSerialPortName();
        nSpeed = Integer.parseInt((String) Speed.getSelectedItem());
        if (pOpen) {
            try {
                serialPort.closePort();
            } catch (SerialPortException e) {
            }
        } else {
            log("Try to open port " + portName + ", baud " + Integer.toString(nSpeed) + ", 8N1");
        }
        serialPort = new SerialPort(portName);
        pOpen = false;
        boolean success;
        try {
            success = serialPort.openPort();
            if (!success) {
                log("ERROR opening serial port " + portName);
                return success;
            }
            SetSerialPortParams();
            serialPort.addEventListener(new PortReader(), portMask);
        } catch (SerialPortException ex) {
            log(ex.toString());
            success = false;
        }
        pOpen = success;
        if (pOpen) {
            log("Open port " + portName + " - Success.");
            TerminalAdd("\r\nPORT OPEN " + Speed.getSelectedItem() + "\r\n");
            CheckComm();
        }
        return pOpen;
    }
        
    private void PortRTSActionPerformed(java.awt.event.ActionEvent evt) {//GEN-FIRST:event_PortRTSActionPerformed
        prefs.putBoolean(PORT_RTS, PortRTS.isSelected());
        try {
            serialPort.setRTS(PortRTS.isSelected());
            if (PortRTS.isSelected()) {
                log("RTS set to ON");
            } else {
                log("RTS set to OFF");
            }
        } catch (SerialPortException e) {
            PortRTS.setSelected(false);
            log(e.toString());
            log("Can't change RTS state");
        }
        UpdateLED();
    }//GEN-LAST:event_PortRTSActionPerformed
    
    private void PortDTRActionPerformed(java.awt.event.ActionEvent evt) {//GEN-FIRST:event_PortDTRActionPerformed
        prefs.putBoolean(PORT_DTR, PortDTR.isSelected());
        try {
            serialPort.setDTR(PortDTR.isSelected());
            if (PortDTR.isSelected()) {
                log("DTR set to ON");
            } else {
                log("DTR set to OFF");
            }
        } catch (SerialPortException e) {
            PortDTR.setSelected(false);
            log(e.toString());
            log("Can't change DTR state");
        }
        UpdateLED();
    }//GEN-LAST:event_PortDTRActionPerformed

    public void portClose() {
        boolean success = false;
        if (portJustOpen) {
            try {
                openTimeout.stop();
            } catch (Exception e) {
                log(e.toString());
            }
        }
        try {
            success = serialPort.closePort();
        } catch (SerialPortException ex) {
            log(ex.toString());
        }
        if (success) {
            TerminalAdd("\r\nPORT CLOSED\r\n");
            log("Close port " + Port.getSelectedItem().toString() + " - Success.");
        } else {
            log("Close port " + Port.getSelectedItem().toString() + " - unknown error.");
        }
        pOpen = false;
        if (Open.isSelected()) {
            Open.setSelected(false);
        }
        UpdateLED();
    }
}
