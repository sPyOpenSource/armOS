/**
 *
 * @author 4refr0nt
 */

package ESPlorer;

import static ESPlorer.ESPlorer.sendBuf;
import java.util.ArrayList;
import jssc.SerialPortEvent;
import jssc.SerialPortEventListener;
import jssc.SerialPortException;

public class pyFiler {

    private static final String dir = "";
    
    public static final int OK = 0;
    public static final int ERROR_COMMUNICATION = 1;

    public String ListDir() {
        return "";
    }
    
    private void PyListDirActionPerformed(java.awt.event.ActionEvent evt) {//GEN-FIRST:event_PyListDirActionPerformed
        PyListFiles();
    }//GEN-LAST:event_PyListDirActionPerformed


    private void PyFileAsButton1ActionPerformed(java.awt.event.ActionEvent evt) {//GEN-FIRST:event_PyFileAsButton1ActionPerformed
        String fn = evt.getActionCommand();
        if (fn.endsWith(".py") || fn.endsWith(".pyc")) {
            String cmd = "f=open(\"" + fn + "\")";
            btnSend(cmd);
            cmd = "f.read()";
            btnSend(cmd);
            cmd = "f.close()";
            btnSend(cmd);
        } else if (fn.endsWith(".bin") || fn.endsWith(".dat")) {
            //HexDump(fn);
        } else {
            //ViewFile(fn);
        }
    }//GEN-LAST:event_PyFileAsButton1ActionPerformed
    
    public boolean Put(String ft, String[] s) {

        boolean success = true;
        sendBuf = new ArrayList<>();

        sendBuf.add("f=open('" + escape(ft) + "','w')");
        for (String subs : s) {
            sendBuf.add("f.write('" + escape(subs) + "\\n')");
        }
        sendBuf.add("f.close()");

        return success;
    }

    public boolean Get() {
        return false;
    }

    public boolean Rename() {
        return false;
    }

    public int Length() {
        return 0;
    }

    public String cd() {
        return dir;
    }

    public String pwd() {
        return dir;
    }

    public String GetParent() {
        return "";
    }

    public boolean isExist() {
        return false;
    }
    
    private byte[] concatArray(byte[] a, byte[] b) {
        if (a == null) {
            return b;
        }
        if (b == null) {
            return a;
        }
        byte[] r = new byte[a.length + b.length];
        System.arraycopy(a, 0, r, 0, a.length);
        System.arraycopy(b, 0, r, a.length, b.length);
        return r;
    }

    private class PortPyFilesReader implements SerialPortEventListener {
        @Override
        public void serialEvent(SerialPortEvent event) {
            String data;
            if (event.isRXCHAR() && event.getEventValue() > 0) {
                try {
                    data = serialPort.readString(event.getEventValue());
                    rcvBuf = rcvBuf + data;
                    rx_data = rx_data + data;
                    TerminalAdd(data);
                } catch (SerialPortException e) {
                    log(e.toString());
                }
                if (rx_data.contains("']\r\n>>>")) {
                    try {
                        timeout.stop();
                    } catch (Exception e) {
                        log(e.toString());
                    }
                    log("FileManager: File list found! Do parsing...");
                    try {
                        int start = rx_data.indexOf("[");
                        rx_data = rx_data.substring(start + 1, rx_data.indexOf("]"));
                        rx_data = rx_data.replace("'", "");
                        s = rx_data.split(", ");
                        Arrays.sort(s);
                        TerminalAdd("\r\n----------------------------");
                        for (String subs : s) {
                            TerminalAdd("\r\n" + subs);
                            if (subs.trim().length() > 0) {
                                AddPyFileButton(subs);
                                log("FileManager found file " + subs);
                            }
                        }
                        if (PyFileAsButton.isEmpty()) {
                            TerminalAdd("No files found.");
                        }
                        TerminalAdd("\r\n----------------------------\r\n> ");
                        PyFileManagerPane.invalidate();
                        PyFileManagerPane.doLayout();
                        PyFileManagerPane.repaint();
                        PyFileManagerPane.requestFocusInWindow();
                        log("pyFileManager: File list parsing done, found " + PyFileAsButton.size() + " file(s).");
                    } catch (Exception e) {
                        log(e.toString());
                    }
                    try {
                        serialPort.removeEventListener();
                        serialPort.addEventListener(new ESPlorer.PortReader(), portMask);
                    } catch (SerialPortException e) {
                        log(e.toString());
                    }
    //                    SendUnLock();
                }
            } else if (event.isCTS()) {
                UpdateLedCTS();
            } else if (event.isERR()) {
                log("FileManager: Unknown serial port error received.");
            }
        } // serialEvent
    } // PortPyFilesReader

    private void ClearPyFileManager() {
        if (!MenuItemViewFileManager.isSelected()) {
            return;
        }
        PyFileManagerPane.removeAll();
        PyFileManagerPane.add(PyListDir);
        PyFileManagerPane.repaint();
        PyFileAsButton = new ArrayList<>();
    } // ClearPyFileManager

    private void PyListFiles() {
        if (portJustOpen) {
            log("ERROR: Communication with MCU not yet established.");
            return;
        }
        try {
            serialPort.removeEventListener();
        } catch (SerialPortException e) {
            log(e.toString());
            return;
        }
        try {
            serialPort.addEventListener(new PortPyFilesReader(), portMask);
            log("pyFileManager: Add EventListener: Success.");
        } catch (SerialPortException e) {
            log("pyFileManager: Add EventListener Error. Canceled.");
            return;
        }
        ClearPyFileManager();
        rx_data = "";
        rcvBuf = "";
        log("pyFileManager: Starting...");
        String cmd = "import os;os.listdir('" + PYFILER.pwd() + "')";
        btnSend(cmd);
        WatchDogPyListDir();
    } // PyListFiles
    
    private boolean pySaveFileESP(String ft) {
        boolean success = false;
        log("pyFileSaveESP: Starting...");
        String[] content = TextEditorList.get(iTab).getText().split("\r?\n");
        if (PYFILER.Put(ft, content)) {
            //pasteMode(true);
            success = SendTimerStart();
        }
        return success;
    } // pySaveFileESP
    
    private class PortFilesUploader implements SerialPortEventListener {

        @Override
        public void serialEvent(SerialPortEvent event) {
            String data, crc_parsed;
            boolean gotProperAnswer = false;
            if (event.isRXCHAR() && event.getEventValue() > 0) {
                try {
                    data = serialPort.readString(event.getEventValue());
                    rcvBuf = rcvBuf + data;
                    rx_data = rx_data + data;
                    //log("rcv:"+data);
                } catch (SerialPortException e) {
                    data = "";
                    log(e.toString());
                }
                if (rcvBuf.contains("> ") && j < sendBuf.size()) {
                    //log("got intepreter answer, j="+Integer.toString(j));
                    rcvBuf = "";
                    gotProperAnswer = true;
                }
                if (rx_data.contains("~~~CRC-END~~~")) {
                    gotProperAnswer = true;
                    //log("Uploader: receiving packet checksum " + Integer.toString( j-sendBuf.size()  +1) + "/"
                    //                                           + Integer.toString( sendPackets.size() ) );
                    // parsing answer
                    int start = rx_data.indexOf("~~~CRC-START~~~");
                    //log("Before CRC parsing:"+rx_data);
                    crc_parsed = rx_data.substring(start + 15, rx_data.indexOf("~~~CRC-END~~~"));
                    rx_data = rx_data.substring(rx_data.indexOf("~~~CRC-END~~~") + 13);
                    //log("After  CRC parsing:"+crc_parsed);
                    int crc_received = Integer.parseInt(crc_parsed);
                    int crc_expected = CRC(sendPackets.get(j - sendBuf.size()));
                    if (crc_expected == crc_received) {
                        log("Uploader: receiving checksum " + Integer.toString(j - sendBuf.size() + 1) + "/"
                                + Integer.toString(sendPackets.size())
                                + " check: Success");
                        sendPacketsCRC.add(true);
                    } else {
                        log("Uploader: receiving checksum " + Integer.toString(j - sendBuf.size() + 1) + "/"
                                + Integer.toString(sendPackets.size())
                                + " check: Fail. Expected: " + Integer.toString(crc_expected)
                                + ", but received: " + Integer.toString(crc_received));
                        sendPacketsCRC.add(false);
                    }
                }
                if (gotProperAnswer) {
                    try {
                        timeout.restart();
                    } catch (Exception e) {
                        log(e.toString());
                    }
                    ProgressBar.setValue(j * 100 / (sendBuf.size() + sendPackets.size() - 1));
                    if (j < (sendBuf.size() + sendPackets.size())) {
                        if (timer.isRunning() || sendPending) {
                            //
                        } else {
                            inc_j();
                            sendPending = true;
                            timer.start();
                        }
                    } else {
                        try {
                            timer.stop();
                        } catch (Exception e) {
                        }
                    }
                }
                if (j >= (sendBuf.size() + sendPackets.size())) {
                    LocalEcho = false;
                    send(addCR("_up=nil"), false);
                    try {
                        timer.stop();
                    } catch (Exception e) {
                    }
                    try {
                        timeout.stop();
                    } catch (Exception e) {
                        log(e.toString());
                    }
                    //log("Uploader: send all data, finishing...");
                    boolean success = true;
                    for (int i = 0; i < sendPacketsCRC.size(); i++) {
                        if (!sendPacketsCRC.get(i)) {
                            success = false;
                        }
                    }
                    if (success && (sendPacketsCRC.size() == sendPackets.size())) {
                        TerminalAdd("Success\r\n");
                        log("Uploader: Success");
                    } else {
                        TerminalAdd("Fail\r\n");
                        log("Uploader: Fail");
                    }
                    try {
                        serialPort.removeEventListener();
                    } catch (SerialPortException e) {
                        log(e.toString());
                    }
                    try {
                        serialPort.addEventListener(new PortReader(), portMask);
                    } catch (SerialPortException e) {
                        log(e.toString());
                    }
                    StopSend();
                    if (mFileIndex != -1 && mFileIndex++ < mFile.size()) {
                        UploadFilesStart();
                    }
                }
            } else if (event.isCTS()) {
                UpdateLedCTS();
            } else if (event.isERR()) {
                log("FileManager: Unknown serial port error received.");
            }
        }
    }

    private void AddPyFileButton(String FileName) {
        PyFileAsButton.add(new javax.swing.JButton());
        int i = PyFileAsButton.size() - 1;
        PyFileAsButton.get(i).setText(FileName);
        PyFileAsButton.get(i).setAlignmentX(0.5F);
        PyFileAsButton.get(i).setMargin(new java.awt.Insets(2, 2, 2, 2));
        PyFileAsButton.get(i).setMaximumSize(new java.awt.Dimension(130, 25));
        PyFileAsButton.get(i).setPreferredSize(new java.awt.Dimension(130, 25));
        PyFileAsButton.get(i).setHorizontalAlignment(javax.swing.SwingConstants.LEFT);
        PyFileAsButton.get(i).addActionListener((java.awt.event.ActionEvent evt) -> {
            PyFileAsButton1ActionPerformed(evt);
        });
        // PopUp menu
        FilePopupMenu.add(new javax.swing.JPopupMenu());
        int x = FilePopupMenu.size() - 1;
        int y;
 
        PyFileManagerPane.add(PyFileAsButton.get(i));

    } // AddPyFileButton
    
    private void WatchDogPyListDir() {
        watchDog = (ActionEvent evt) -> {
            //StopSend();
            Toolkit.getDefaultToolkit().beep();
            TerminalAdd("Waiting answer from ESP - Timeout reached. Command aborted.");
            log("Waiting answer from ESP - Timeout reached. Command aborted.");
            try {
                serialPort.removeEventListener();
                serialPort.addEventListener(new PortReader(), portMask);
            } catch (SerialPortException e) {
                log(e.toString());
            }
            SendUnLock();
        };
        int delay = AnswerDelay.getValue() * 1000;
        if (delay == 0) {
            delay = 300;
        }

        timeout = new Timer(delay, watchDog);
        timeout.setRepeats(false);
        timeout.setInitialDelay(delay);
        timeout.start();
    } // WatchDogPyListDir
    
    private byte[] copyPartArray(byte[] a, int start, int len) {
        if (a == null) {
            return null;
        }
        if (start > a.length) {
            return null;
        }
        byte[] r = new byte[len];
        try {
            System.arraycopy(a, start, r, 0, len);
        } catch (Exception e) {
            /*log(e.toString());
            log("copyPartArray exception");
            log("size a=" + Integer.toString(a.length));
            log("start =" + Integer.toString(start));
            log("len   =" + Integer.toString(len));*/
        }
        return r;
    }
    
    private void FileDownloadFinisher(boolean success) {
        try {
            serialPort.removeEventListener();
        } catch (SerialPortException e) {
            log(e.toString());
        }
        try {
            serialPort.addEventListener(new ESPlorer.PortReader(), portMask);
        } catch (SerialPortException e) {
            log("Downloader: Can't Add OldEventListener.");
        }
        //SendUnLock();
        StopSend();
        if (success) {
            TerminalAdd("Success.\r\n");
            if (DownloadCommand.startsWith("EDIT")) {
                FileNew(rcvFile);
            } else if (DownloadCommand.startsWith("DOWNLOAD")) {
                SaveDownloadedFile();
            } else {
                // nothing, reserved
            }
        } else {
            TerminalAdd("FAIL.\r\n");
        }
    }
    
    public String escape(String str) {
        char ch;
        StringBuilder buf = new StringBuilder(str.length() * 2);

        for (int i = 0, l = str.length(); i < l; ++i) {
            ch = str.charAt(i);
            if (ch == '"') {
                buf.append("\\");
            } else if (ch == '\'') {
                buf.append("\\");
            }
            buf.append(ch);
        }
        return buf.toString();
    } // escape
} // pyFiler
