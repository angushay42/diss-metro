import unittest
import random

# test obj
from server import UART, UARTException, MockSerial, BytesManager


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

class TestMockSerial(unittest.TestCase):
    def test_read(self):
        data = [12, 34, 89, 101]
        self.stream.io.write(bytearray(data))
        ans = []
        for i in range(len(data)):
            ans.append(int.from_bytes(self.stream.read(1), 'little')) 
        self.assertEqual(data,ans)


    def test_write(self):
        pass
    
    def setUp(self):
        super().setUp()
        self.stream = MockSerial()

class TestBytesManager(unittest.TestCase):
    manager: BytesManager = None

    def test_convert_to_bytes(self):      
        # test exceptions
        ## invalid byte sizes
        with self.assertRaises(UARTException):
            self.manager.convert_to_bytes(1, 5)
        with self.assertRaises(UARTException):
            self.manager.convert_to_bytes(1, -1)
        with self.assertRaises(UARTException):
            self.manager.convert_to_bytes(1, 3)

        ## incompatible byte sizes
        with self.assertRaises(UARTException):
            data = int(0x6e4fa)
            self.manager.convert_to_bytes(data, 1)

        with self.assertRaises(UARTException):
            data = -3930300002
            self.manager.convert_to_bytes(data, 4)

    
        # test values
        ## unsigned
        data = int(0x6e4fa)
        size = 4

        ans = data.to_bytes(4, 'little', signed=False)
        self.assertEqual(self.manager.convert_to_bytes(data, size), ans)

        data = 14329
        size = 2

        ans = data.to_bytes(2, 'little', signed=False)
        self.assertEqual(self.manager.convert_to_bytes(data, size), ans)

        ## signed
        data = -30901
        size = 4

        ans = data.to_bytes(4, 'little', signed=True)
        self.assertEqual(self.manager.convert_to_bytes(data, size), ans)

        data = -404040404
        ans = data.to_bytes(4, 'little', signed=True)
        self.assertEqual(self.manager.convert_to_bytes(data, size), ans)

        # test list of ints
        ## exceptions 

        ### empty list
        with self.assertRaises(UARTException):
            self.manager.convert_to_bytes([]) 

        ### inconsistent sizes
        with self.assertRaises(UARTException):
            self.manager.convert_to_bytes([1, 1 << 32])



        


    def test_get_size(self):
        self.assertEqual(self.manager.get_size(4), 1)

        self.assertEqual(self.manager.get_size(254), 1)

        self.assertEqual(self.manager.get_size(6550303), 4)

        self.assertEqual(self.manager.get_size(-1), 1)

        self.assertEqual(self.manager.get_size(-257), 2)

    @classmethod
    def setUpClass(cls):
        super().setUpClass()
        cls.manager = BytesManager()


class TestUART(unittest.TestCase):
    server: UART = None

    def test_send(self):
        pass

    def test_recv(self):
        """ recv will expect data to come from a serial port, how can we mock that?"""

        # empty buffer 
        with self.assertRaises(UARTException):
            self.server.recv()
        
        # invalid flag
        ## invalid bits
        flag = 1 << (UART.signed + 1)
        flag |= 1 << (UART.signed + 2)

        with self.assertRaises(UARTException, msg=f"Invalid bits set."):
            self.server.stream.write(bytearray([flag, 1, 3]))
            self.server.recv()

        ## bit count either 1 or 2.
        input = [0, 1, 3]
        with self.assertRaises(UARTException, msg=f"Invalid bit count with input: {input}"):
            self.server.stream.write(bytearray())
            self.server.recv()

        input = [7, 1, 3]
        with self.assertRaises(UARTException, msg=f"Invalid bit count with input: {input}"):
            self.server.stream.write(bytearray(input))
            self.server.recv()

        # test data integrity
        ## single bytes
        data = [2, 3, 4, 5, 90, 140, 57, 30]
        flag = 1 << UART.byte

        self.server.stream.write(bytearray([flag, len(data)] + data))
        values, s = self.server.recv()
        
        # unsigned bytes can be converted to string
        self.assertIsNotNone(s)
        self.assertEqual(data, values)

        ## words (4 bytes)
        data = [32, 5454, 11010, 20202, 34040, 54400, (1 << 30) + 435]
        flag = 1 << UART.word
        
        # byte array only accepts 0-255, must be split up first.
        d_bytes = bytearray()
        for d in data:
            d_bytes += d.to_bytes(1 << UART.word, 'little')


        self.server.stream.write(bytearray([flag, len(data)]) + d_bytes)
        values, s = self.server.recv()

        self.assertIsNone(s)
        self.assertEqual(data, values)


        ## double words (8 bytes)
        data = []
        flag = 1 << UART.double
        for _ in range(8):
            data.append(random.randint(1 << 32, (1 << 64) - 1))

        d_bytes = bytearray()
        for d in data:
            d_bytes += d.to_bytes(1 << UART.double, 'little')

        self.server.stream.write(bytearray([flag, len(data)]) + d_bytes)
        values, s = self.server.recv()

        self.assertIsNone(s)
        self.assertEqual(data, values)

        # BUG don't know how big floats are.
        ## floats   #todo


        # todo
        ## double floating point words (8 bytes)
        data = [1.4322234, 103.3030, 100000.20 ,203030.39393, 0.991]
    
    @classmethod
    def setUpClass(cls):
        super().setUpClass()
        # timeout of 5
        cls.server = UART(5, True)
        cls.server.stream = MockSerial()
        


if __name__ == "__main__":
    suite = unittest.suite.TestSuite()

    suite.addTest(unittest.defaultTestLoader.loadTestsFromTestCase(TestBytesManager))
    suite.addTest(unittest.defaultTestLoader.loadTestsFromTestCase(TestMockSerial))
    suite.addTest(unittest.defaultTestLoader.loadTestsFromTestCase(TestUART))
    unittest.TextTestRunner(verbosity=2, failfast=True).run(suite)