#!/usr/bin/env python

import sys
import os
import math
import unittest
from ctypes import *

argvs = sys.argv
argc = len(argvs)
sofile = os.path.join(os.path.dirname(__file__), os.pardir, "libcalcp.so")
dll = cdll.LoadLibrary(sofile)

class test_calc(unittest.TestCase):
    def setUp(self):
        pass

    def input(self, expr):
        dll.input.restype = c_char_p
        dll.input.argtypes = [ c_char_p, c_uint ]
        retval = dll.input(expr, len(expr))
        return retval

    def get_strlen(self, n, fmt):
        dll.test_get_strlen.restype = c_int
        dll.test_get_strlen.argtypes = [ c_double, c_char_p ]
        retval = dll.test_get_strlen(n, fmt)
        return retval

    def test_input(self):
        """input() """
        # メモリリークする
        #self.assertEqual(self.input('(105+312)+2*(5-3)'), '421')
        #self.assertEqual(self.input('(105+312)+2/(5-3)'), '418')
        #self.assertEqual(self.input('1+2*(5-3)'), '5')
        #self.assertEqual(self.input('1+2/(5-3)'), '2')
        #self.assertEqual(self.input('pi'), '3.14159265359')
        #self.assertEqual(self.input('e'), '2.71828182846')
        #self.assertEqual(self.input('abs(-2)'), '2')
        #self.assertEqual(self.input('sqrt(2)'), '1.41421356237')
        #self.assertEqual(self.input('sin(2)'), '0.909297426826')
        #self.assertEqual(self.input('cos(2)'), '-0.416146836547')
        #self.assertEqual(self.input('tan(2)'), '-2.18503986326')
        #self.assertEqual(self.input('asin(0.5)'), '0.523598775598')
        #self.assertEqual(self.input('acos(0.5)'), '1.0471975512')
        #self.assertEqual(self.input('atan(0.5)'), '0.463647609001')
        #self.assertEqual(self.input('exp(2)'), '7.38905609893')
        #self.assertEqual(self.input('ln(2)'), '0.69314718056')
        #self.assertEqual(self.input('log(2)'), '0.301029995664')
        #self.assertEqual(self.input('deg(2)'), '114.591559026')
        #self.assertEqual(self.input('rad(2)'), '0.0349065850399')
        #self.assertEqual(self.input('n(10)'), '3628800')
        #self.assertEqual(self.input('nPr(5,2)'), '20')
        #self.assertEqual(self.input('nCr(5,2)'), '10')

    def test_get_strlen(self):
        """get_strlen() """
        fmt = '%.15g'
        self.assertEqual(self.get_strlen(0, fmt), len('0'))

        self.assertEqual(self.get_strlen(123456789012345, fmt),
                         len('123456789012345'))

        self.assertEqual(self.get_strlen(1234567.89012345, fmt),
                         len('1234567.89012345'))

if __name__ == '__main__':
#    unittest.main()
    suite = unittest.TestLoader().loadTestsFromTestCase(test_calc)
    unittest.TextTestRunner(verbosity=2).run(suite)

