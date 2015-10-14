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