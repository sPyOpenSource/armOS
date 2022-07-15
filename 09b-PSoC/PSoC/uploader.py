#!/usr/bin/env python2.7
import time
import os
from struct import pack
from builtins import bytes
from serial import Serial

class Avr109(object):
    """AVR109 firmware upload protocol.  This currently just implements the
    limited subset of AVR109 (butterfly) protocol necessary to load the
    Atmega 328P used on the Mobilinkd TNC.
    """
    
    def __init__(self, reader):
        self.start()
        
    def start(self):
        os.system("stty -f /dev/cu.usbmodem101 1200")
        print("starting")
        time.sleep(2.5)
        self.sio = Serial('/dev/cu.usbmodem101', 115200)
    
    def send_address(self, address):
        address //= 2 # convert from byte to word address
        ah = (address & 0xFF00) >> 8
        al = address & 0xFF
        self.sio.write(bytes(pack('cBB', b'A', ah, al)))
        self.sio.flush()
        self.verify_command_sent("Set address to: %04x" % address)
    
    def send_block(self, memtype, data):
        """Send a block of memory to the bootloader.  This command should
        be preceeded by a call to send_address() to set the address that
        the block will be written to.
        
        @note The block must be in multiple of 2 bytes (word size).
        
        @param memtype is the type of memory to write. 'E' is for EEPROM,
            'F' is for flash.
        @data is a block of data to be written."""
        
        assert(len(data) % 2 == 0)
        
        ah = 0
        al = len(data)
        
        self.sio.write(bytes(pack('cBBc', b'B', ah, al, memtype)))
        self.sio.write(data)
        self.sio.flush()
        self.verify_command_sent("Block load: %d" % len(data))
    
    def read_block(self, memtype, size):
        """Read a block of memory from the bootloader.  This command should
        be preceeded by a call to send_address() to set the address that
        the block will be written to.
        
        @note The block must be in multiple of 2 bytes (word size).
        
        @param memtype is the type of memory to write. 'E' is for EEPROM,
            'F' is for flash.
        @param size is the size of the block to read."""
                
        self.sio.write(bytes(pack('cBBc', b'g', 0, size, memtype)))
        self.sio.flush()
        self.sio.timeout = 1
        result = self.sio.read(size)
        return result
        
    def write_block(self, address, memtype, data):
        pass
    
    def do_upload(self):
        pass
    
    def verify_command_sent(self, cmd):
        timeout = self.sio.timeout
        self.sio.timeout = 1
        c = self.sio.read(1)
        self.sio.timeout = timeout
        if c != b'\r':
            # Do not report c because it could be None
            raise IOError("programmer did not respond to command: %s" % cmd)
        else:
            pass
            # print "programmer success: %s" % cmd
    
    def chip_erase(self):
        time.sleep(.1)
        self.sio.write(b'e')
        self.sio.flush()
        self.verify_command_sent("chip erase")
    
    def enter_program_mode(self):
        time.sleep(.1)
        self.sio.write(b'P')
        self.sio.flush()
        #self.verify_command_sent("enter program mode")
        
    def leave_program_mode(self):
        time.sleep(.1)
        self.sio.write(b'L')
        self.sio.flush()
        self.verify_command_sent("leave program mode")
   
    def exit_bootloader(self):
        time.sleep(.1)
        self.sio.write(b'E')
        self.sio.flush()
        self.verify_command_sent("exit bootloader")
        
    def supports_auto_increment(self):
        # Auto-increment support
        time.sleep(.1)
        self.sio.write(b'a')
        self.sio.flush()
        return (self.sio.read(1) == b'Y')
    
    def get_block_size(self):
        time.sleep(.1)
        self.sio.write(b'b')
        self.sio.flush()
        if self.sio.read(1) == b'Y':
            tmp = bytearray(self.sio.read(2))
            return tmp[0] * 256 + tmp[1]
        return 0
   
    def get_bootloader_signature(self):
        self.sio.timeout = 1
        junk = self.sio.read()
        self.sio.timeout = .1
        for i in range(10):
            self.start()
            # Bootloader
            self.sio.write(b'S')
            self.sio.flush()
            loader = self.sio.read(7)
            if loader == b'AVRBOOT':
                return loader
            time.sleep(.1)
        raise RuntimeError("Invalid bootloader: {}".format(loader))
    
    def send_expect(self, cmd, expected, retries = 5):
        expected_len = len(expected)
        self.sio.timeout = .1
        junk = self.sio.read()
        for i in range(retries):
            self.sio.write(cmd)
            self.sio.flush()
            received = bytes(self.sio.read(expected_len))
            if received == expected:
                return True
            time.sleep(.1)
        return False
        
    
    def get_software_version(self):
        """Return the bootloader software version as a string with the format
        MAJOR.MINOR (e.g. "1.7")."""

        time.sleep(.1)
        self.sio.write(b'V')
        self.sio.flush()
        self.sio.timeout = .1
        sw_version = bytes(self.sio.read(2))
        if len(sw_version) < 2: return "unknown"
        return "%d.%d" % (sw_version[0] - 48, sw_version[1] - 48)
    
    def get_programmer_type(self):
        return b'S' if self.send_expect(b'p', b'S') else None
        
        time.sleep(.1)
        self.sio_writer.write(b'p')
        self.sio_writer.flush()
        self.sio_reader.timeout = .1
        return self.sio_reader.read(1)
    
    def get_device_list(self):
        # Device Code
        self.sio.timeout = .1
        self.sio.write(b't')
        self.sio.flush()
        device_list = []
        while True:
            device = self.sio.read(1)
            if device[0] == 0: break
            device_list.append(device)
        
        return device_list
        
    def get_device_signature(self):
        # Read Signature
        self.sio.write(b's')
        self.sio.flush()
        return self.sio.read(3)

#os.system("make upload")
programmer = Avr109(open("build/psoc.hex", "r"))
#programmer.enter_program_mode()
print(programmer.get_bootloader_signature())
print(programmer.get_device_signature())
print(programmer.get_software_version())
print(programmer.get_programmer_type())
print(programmer.get_device_list())
print(programmer.get_block_size())
