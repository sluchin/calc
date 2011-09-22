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

    def test_get_digit(self):
        """get_digit() """
        fmt = '%.18g'
        n = 50000
        retval = dll.test_get_digit(c_double(n), fmt)
        self.assertEqual(retval, len(str(n)))

        n = 500000000000
        retval = dll.test_get_digit(c_double(n), fmt)
        self.assertEqual(retval, len(str(n)))

if __name__ == '__main__':
#    unittest.main()
    suite = unittest.TestLoader().loadTestsFromTestCase(test_calc)
    unittest.TextTestRunner(verbosity=2).run(suite)

