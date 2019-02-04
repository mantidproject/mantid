# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
import sys
import sys
import os
import unittest
import time

import mantidqtpython

from mantidqtpython import StdRuntimeError, StdInvalidArgument


class QwtTypesPythonInterfaceTest(unittest.TestCase):
    """Tests for accessing Qwt library types
    from Python"""

    def test_QwtDoubleInterval(self):
        # TODO: How can I create the type?
        pass

if __name__ == '__main__':
    unittest.main()
