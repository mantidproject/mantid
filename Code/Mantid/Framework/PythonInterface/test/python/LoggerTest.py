import unittest

from mantid import Logger

class LoggerTest(unittest.TestCase):
  
    def test_logger_creation_does_not_raise_an_error(self):
        logger = Logger.get("LoggerTest")
        self.assertTrue(isinstance(logger, Logger))
        attrs = ['fatal', 'error','warning','notice', 'information', 'debug']
        for att in attrs:
            if not hasattr(logger, att):
                self.fail("Logger object does not have the required attribute '%s'" % att)
    