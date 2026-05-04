import unittest
import random

# test obj
from server import UART, UARTException, MockSerial, BytesManager


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

    def test_get_flag(self):
        with self.assertRaises(UARTException):
            self.manager.get_flag(0)
        
        with self.assertRaises(UARTException):
            self.manager.get_flag(3)

        with self.assertRaises(UARTException):
            self.manager.get_flag(-100)

        self.assertEqual(self.manager.get_flag(1), 1 << BytesManager.byte)
        self.assertEqual(self.manager.get_flag(2), 1 << BytesManager.half)
        self.assertEqual(self.manager.get_flag(4), 1 << BytesManager.word)
        self.assertEqual(self.manager.get_flag(8), 1 << BytesManager.double)

    def test_validate_packet(self):
        # malformed
        ## just random
        packet = bytearray([1,2,3,4,5,6])
        self.assertFalse(self.manager.validate_packet(packet))
        ## incorrect sizes

        packet = bytearray([ord('{'), 1, *list(range(240)), ord('}')])
        self.assertFalse(self.manager.validate_packet(packet))

        ## missing flag
        data = [1, 45, 2, 0]
        packet = bytearray([ord('{'), len(data), *data, ord('}')])
        self.assertFalse(self.manager.validate_packet(packet))

        ## incorrect size
        packet = bytearray([ord('{'), 1 << self.manager.double, len(data), *data, ord('}')])
        self.assertFalse(self.manager.validate_packet(packet))

        ## missing stop
        packet = bytearray([ord('{'), 1 << self.manager.double, len(data), *data])
        self.assertFalse(self.manager.validate_packet(packet))

        # well formed
        ## single bytes
        data = [1,2,3,4]
        packet = bytearray([ord('{'), 1 << self.manager.byte, len(data), *data, ord('}')])
        self.assertTrue(self.manager.validate_packet(packet))

        data = [1,2,3,4, 212, 39000, 10002, 405, 109]
        size = 2
        startb, stopb = bytes('{', encoding="ascii"), bytes('}', encoding="ascii")
        packet = bytearray() + startb
        packet += bytearray([1 << self.manager.half, len(data)])
        for d in data:
            packet += int.to_bytes(d, size, 'little')
        packet += stopb


    def test_get_packet(self):
        with self.assertRaises(UARTException):
            self.manager.get_packet(0, 1)

        self.assertEqual(
            # 1 unsigned byte
            bytearray(
                # start    flag                    len d  stop
                [ord('{'), 1 << BytesManager.byte, 1,  1, ord('}')]
            ),
            self.manager.get_packet(1, 1)
        )

        self.assertEqual(
            # many unsigned bytes
            bytearray([ord('{'), 1 << BytesManager.byte, 4, 1, 2, 3, 4, ord('}')]),
            self.manager.get_packet(1, [1,2,3,4])
        )

        data = [1,2,3,4,5]
        ans = bytearray([ord('{'), 1 << BytesManager.half, len(data)])
        for d in data:
            ans += d.to_bytes(2, 'little')  # half words
        ans += ord('}').to_bytes(1, 'little')
        self.assertEqual(
            ans,
            self.manager.get_packet(2, data)
        )

    def test_get_packet_from_bytes(self):
        self.manager.get_packet_from_bytes()

    def test_convert_many(self):
        # exceptions 
        ## empty list
        with self.assertRaises(UARTException):
            self.manager.convert_to_bytes([]) 

        ## inconsistent sizes
        with self.assertRaises(UARTException):
            self.manager.convert_to_bytes([1, 1 << 32])

        ## mixed polarity
        with self.assertRaises(UARTException) as e:
            self.manager.convert_to_bytes([1, (1 << 16) - 1, -100, -(1 << 5)])

        # values
        data = [490, 2390, -2, 123, 96, 1900, 16000, -20000]
        size = 4
        signed = True
        b = bytearray()
        for d in data:
            b += d.to_bytes(size, 'little', signed=signed)
        
        ans = self.manager.convert_to_bytes(data, size, signed)
        self.assertEqual(ans, b)

    def test_convert_once(self):
        # unsigned
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

        ## invalid input types
        with self.assertRaises(UARTException):
            self.manager.convert_to_bytes(1.20)
        
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

    def test_detect_packet(self):
        # timeout #todo
        # self.server = UART(5, True)
        # self.server.stream = MockSerial()
        # self.assertFalse(self.server.detect_packet())

        # todo
        # random bytes (no packet)

        # todo
        # random bytes (partial packet)

        # only send the packet
        packet = BytesManager.get_packet(2, [1,2,3,4,5])
        print(packet)
        self.server.stream.write(packet)
        self.assertTrue(self.server.detect_packet())

        # send random bytes in between
        packet = BytesManager.get_packet(2, [1,2,3,4,5])

        randidx = random.randint(0, 7)
        for i in range(8):
            if i == randidx:
                self.server.stream.write(packet)
            else:
                self.server.stream.write(random.randint(0,255).to_bytes(1, 'little'))

        self.assertTrue(self.server.detect_packet())

        # send random bytes (some are start/stop)

    
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