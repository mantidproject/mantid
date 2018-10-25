# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function, unicode_literals)

import unittest

from mantid.kernel import Logger


class LoggerTest(unittest.TestCase):

    def test_str_logger(self):
        logger = Logger(str("LoggerTest"))
        self.assertTrue(isinstance(logger, Logger))
        for att in ['fatal', 'error', 'warning', 'notice', 'information', 'debug']:
            if not hasattr(logger, att):
                self.fail("Logger object does not have the required attribute '%s'" % att)

        logger.fatal(str('This is a test'))
        logger.error(str('This is a test'))
        logger.warning(str('This is a test'))
        logger.notice(str('This is a test'))
        logger.information(str('This is a test'))
        logger.debug(str('This is a test'))

    def test_unicode_logger(self):
        logger = Logger("LoggerTest")
        self.assertTrue(isinstance(logger, Logger))
        for att in ['fatal', 'error', 'warning', 'notice', 'information', 'debug']:
            if not hasattr(logger, att):
                self.fail("Logger object does not have the required attribute '%s'" % att)

        logger.fatal('This is a test')
        logger.error('This is a test')
        logger.warning('This is a test')
        logger.notice('This is a test')
        logger.information('This is a test')
        logger.debug('This is a test')


if __name__ == '__main__':
    unittest.main()
