# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

# package imports
from mantid.kernel import ConfigService, logger
from mantid.utils.logging import capture_logs, log_to_python

# standard imports
from contextlib import contextmanager
import logging
import unittest


class CaptureHandler(logging.Handler):
    def __init__(self):
        super().__init__(level=logging.DEBUG)
        self.records = []

    def emit(self, record: logging.LogRecord):
        self.records.append(record)


@contextmanager
def backup_config():
    try:
        config = ConfigService.Instance()
        backup = {key: config[key] for key in config.keys() if 'logging' in key}
        yield
    finally:
        for key, val in backup.items():
            config[key] = val


class loggingTest(unittest.TestCase):
    def test_capture_logs(self):

        with capture_logs() as logs:
            logger.error('Error message')
            self.assertTrue('Error message' in logs.getvalue())

        config = ConfigService.Instance()
        config['logging.loggers.root.level'] = 'information'
        with capture_logs(level='error') as logs:
            self.assertTrue(config['logging.loggers.root.level'] == 'error')
            logger.error('Error-message')
            logger.debug('Debug-message')
            self.assertTrue('Error-message' in logs.getvalue())
            self.assertFalse('Debug-message' in logs.getvalue())

        self.assertTrue(config['logging.loggers.root.level'] == 'information')

    def test_log_to_python(self):
        py_logger = logging.getLogger('Mantid')
        py_logger.setLevel(logging.INFO)
        handler = CaptureHandler()
        for hdlr in py_logger.handlers:
            py_logger.removeHandler(hdlr)
        py_logger.addHandler(handler)

        with backup_config():
            log_to_python()
            logger.information('[[info]]')
            logger.warning('[[warning]]')
            logger.error('[[error]]')
            logger.fatal('[[fatal]]')

        self.assertListEqual([record.msg for record in handler.records],
                             ['[[info]]', '[[warning]]', '[[error]]', '[[fatal]]'])
        self.assertListEqual([record.levelname for record in handler.records],
                             ['INFO', 'WARNING', 'ERROR', 'CRITICAL'])

        py_logger.removeHandler(handler)


if __name__ == '__main__':
    unittest.main()
