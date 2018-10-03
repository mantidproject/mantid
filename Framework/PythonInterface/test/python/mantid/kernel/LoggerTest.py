# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)

import unittest

from mantid.kernel import Logger

class LoggerTest(unittest.TestCase):

    def test_logger_creation_does_not_raise_an_error(self):
        logger = Logger("LoggerTest")
        self.assertTrue(isinstance(logger, Logger))
        attrs = ['fatal', 'error','warning','notice', 'information', 'debug']
        for att in attrs:
            if not hasattr(logger, att):
                self.fail("Logger object does not have the required attribute '%s'" % att)


if __name__ == '__main__':
    unittest.main()