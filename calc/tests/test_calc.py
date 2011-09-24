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

    def answer(self, expr):
        dll.init_calc.argtypes = [ c_char_p ]
        dll.init_calc(expr)
        dll.answer.restype = c_char_p
        retval = dll.answer()
        return retval

    def get_strlen(self, n, fmt):
        dll._test_get_strlen.restype = c_int
        dll._test_get_strlen.argtypes = [ c_double, c_char_p ]
        retval = dll._test_get_strlen(n, fmt)
        return retval

    def test_answer(self):
        """answer() memory leak"""
        pass
        #self.assertEqual(self.answer('(105+312)+2*(5-3)'), '421')
        #dll.destroy_calc()
        #self.assertEqual(self.answer('(105+312)+2/(5-3)'), '418')
        #dll.destroy_calc()
        #self.assertEqual(self.answer('1+2*(5-3)'), '5')
        #dll.destroy_calc()
        #self.assertEqual(self.answer('1+2/(5-3)'), '2')
        #dll.destroy_calc()
        #self.assertEqual(self.answer('pi'), '3.14159265359')
        #dll.destroy_calc()
        #self.assertEqual(self.answer('e'), '2.71828182846')
        #dll.destroy_calc()
        #self.assertEqual(self.answer('abs(-2)'), '2')
        #dll.destroy_calc()
        #self.assertEqual(self.answer('sqrt(2)'), '1.41421356237')
        #dll.destroy_calc()
        #self.assertEqual(self.answer('sin(2)'), '0.909297426826')
        #dll.destroy_calc()
        #self.assertEqual(self.answer('cos(2)'), '-0.416146836547')
        #dll.destroy_calc()
        #self.assertEqual(self.answer('tan(2)'), '-2.18503986326')
        #dll.destroy_calc()
        #self.assertEqual(self.answer('asin(0.5)'), '0.523598775598')
        #dll.destroy_calc()
        #self.assertEqual(self.answer('acos(0.5)'), '1.0471975512')
        #dll.destroy_calc()
        #self.assertEqual(self.answer('atan(0.5)'), '0.463647609001')
        #dll.destroy_calc()
        #self.assertEqual(self.answer('exp(2)'), '7.38905609893')
        #dll.destroy_calc()
        #self.assertEqual(self.answer('ln(2)'), '0.69314718056')
        #dll.destroy_calc()
        #self.assertEqual(self.answer('log(2)'), '0.301029995664')
        #dll.destroy_calc()
        #self.assertEqual(self.answer('deg(2)'), '114.591559026')
        #dll.destroy_calc()
        #self.assertEqual(self.answer('rad(2)'), '0.0349065850399')
        #dll.destroy_calc()
        #self.assertEqual(self.answer('n(10)'), '3628800')
        #dll.destroy_calc()
        #self.assertEqual(self.answer('nPr(5,2)'), '20')
        #dll.destroy_calc()
        #self.assertEqual(self.answer('nCr(5,2)'), '10')
        #dll.destroy_calc()

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

