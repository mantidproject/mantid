# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

# package imports
from mantid.kernel import ConfigService, logger
from mantid.utils.logging import to_file

# standard imports
from pathlib import Path
import os
import unittest


class loggingTest(unittest.TestCase):

    def test_to_file(self):

        with to_file() as log_file:
            logger.error('Error message')
            self.assertTrue('Error message' in open(log_file, 'r').read())
        self.assertFalse(Path(log_file).exists())

        with to_file(filename='my_one_and_only_one_log_file.log') as log_file:
            logger.error('Error message')
            self.assertTrue('Error message' in open(log_file, 'r').read())
        self.assertTrue(Path(log_file).exists())
        os.remove(log_file)

        config = ConfigService.Instance()
        config['logging.loggers.root.level'] = 'information'
        with to_file(level='error') as log_file:
            self.assertTrue(config['logging.loggers.root.level'] == 'error')
            logger.error('Error-message')
            logger.debug('Debug-message')
            self.assertTrue('Error-message' in open(log_file, 'r').read())
            self.assertFalse('Debug-message' in open(log_file, 'r').read())
        self.assertFalse(Path(log_file).exists())
        self.assertTrue(config['logging.loggers.root.level'] == 'information')
