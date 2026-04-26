from unittest import TestCase
from io import BytesIO
import json
import math

# test obj
from server import UART, UARTException, MockSerial


"""
the application will want to send integers of varying length, polarity and also strings. 

so we need to tell the person what we are going to send, can we do that in 1 byte?

we will send a byte 

format:
byte: bitmask of data to follow
byte: length of data to follow      range is 0-255 then
data: data
"""
"""
Dependency Injection
UART relies on serial connection, it depends on it existing
At runtime, if it is testing, we should be able to swap out where it reads from 
so that it uses a test buffer.

"""



class TestServer(TestCase):
    server: UART = None

    def test_convert_to_bytes(self):
        # init vars
        data = int(0x6e4fa)
        size = 4
        
        # test exceptions
        with self.assertRaises(UARTException):
            self.server.convert_to_bytes(data, 5)
        with self.assertRaises(UARTException):
            self.server.convert_to_bytes(data, -1)
        with self.assertRaises(UARTException):
            self.server.convert_to_bytes(data, 3)
    
        # test values
        ## unsigned
        ans = bytearray([1 << self.server.word])
        ans += data.to_bytes(4, 'little')

        self.assertEqual(self.server.convert_to_bytes(data, size), ans)

        ## signed

    def test_get_size(self):
        self.assertEqual(self.server.get_size(4), 1)

        self.assertEqual(self.server.get_size(254), 1)

        self.assertEqual(self.server.get_size(6550303), 4)

        self.assertEqual(self.server.get_size(-1), 1)

        self.assertEqual(self.server.get_size(-257), 2)

    def test_send(self):
        pass

    def test_recv(self):
        """ recv will expect data to come from a serial port, how can we mock that?"""

        # empty buffer 
        with self.assertRaises(UARTException):
            self.server.recv()
        
        # invalid flag
        ## invalid bits
        flag = 1 << 6
        flag |= 1 << 7      

        with self.assertRaises(UARTException, msg=f"Invalid bits set."):
            self.server.stream.write(bytearray([flag, 1, 3]))
            self.server.recv()

        ## bit count either 1 or 2.

        with self.assertRaises(UARTException, msg="Invalid bit count"):
            self.server.stream.write(bytearray([0, 1, 3]))
            self.server.recv()



    def setUp(self):
        return super().setUp()
    
    def tearDown(self):
        return super().tearDown()
    
    @classmethod
    def tearDownClass(cls):
        super().tearDownClass()
        

    @classmethod
    def setUpClass(cls):
        super().setUpClass()
        # timeout of 5
        cls.server = UART(5, True)
        cls.server.stream = MockSerial()


if __name__ == "__main__":
    pass# todo