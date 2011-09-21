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
        self.val = c_double(50000)
        self.fmt = '%.18g'

    def test_get_digit(self):
        """test first"""
        retval = dll.test_get_digit(self.val, self.fmt)
        self.assertEqual(retval, 5)

        n = 500000000000000
        retval = dll.test_get_digit(c_double(n), self.fmt)
        self.assertEqual(retval, int(math.log10(n) + 1))

if __name__ == '__main__':
#    unittest.main()
    suite = unittest.TestLoader().loadTestsFromTestCase(test_calc)
    unittest.TextTestRunner(verbosity=2).run(suite)

