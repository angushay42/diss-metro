from unittest import TestCase
import json
import math

# test obj
from server import Client, Server, ServerException


"""
the application will want to send integers of varying length, polarity and also strings. 

so we need to tell the person what we are going to send, can we do that in 1 byte?

we will send a byte 

format:
byte: bitmask of data to follow
word: length of data to follow
data: data

"""

class TestServer(TestCase):
    client = None
    server = None

    def test_convert_to_bytes(self):
        # init vars
        data = int(0x6e4fa)
        size = 4
        
        # test exceptions
        with self.assertRaises(ServerException):
            self.client.convert_to_bytes(data, 5)
        with self.assertRaises(ServerException):
            self.client.convert_to_bytes(data, -1)
        with self.assertRaises(ServerException):
            self.client.convert_to_bytes(data, 3)
    
        # test values
        ## unsigned
        ans = bytearray([self.client.word])
        ans += data.to_bytes(4, 'little')

        self.assertEqual(self.client.convert_to_bytes(data, size), ans)

        ## signed

    def test_get_size(self):
        self.assertEqual(self.client.get_size(4), 1)

        self.assertEqual(self.client.get_size(254), 1)

        self.assertEqual(self.client.get_size(6550303), 4)

        self.assertEqual(self.client.get_size(-1), 1)

        self.assertEqual(self.client.get_size(-257), 2)

    def test_send_string(self):
        pass

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
        cls.client = Client(True)
        cls.server = Server(True)

if __name__ == "__main__":
    pass# todo