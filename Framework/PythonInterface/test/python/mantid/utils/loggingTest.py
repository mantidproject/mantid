# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

# package imports
from mantid.kernel import ConfigService, logger
from mantid.utils.logging import capture_logs

# standard imports
from pathlib import Path
import os
import unittest


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


if __name__ == '__main__':
    unittest.main()
