"""Tests for the migration with simple one-line string replacements
"""
import unittest
import os

from MigrationTest import MigrationTest

__INPUTSTRING__ = r"""
from MantidFramework import *
import MantidFramework
mtd.initialize()
mtd.initialise()
from mantidsimple import *
import mantidsimple
from mantidsimple import Load

MantidFramework.mtd.sendLogMessage("Started")
mtd.sendLogMessage("Started")
mantid.sendLogMessage("Started")
w.getSampleDetails()
mantidsimple.Load
"""

__EXPECTEDSTRING__ = r"""
from mantid import *
import mantid


from mantid.simpleapi import *
import mantid.simpleapi as simpleapi
from mantid.simpleapi import Load

mantid.logger.notice("Started")
logger.notice("Started")
logger.notice("Started")
w.getRun()
simpleapi.Load
"""

class SimpleStringReplaceMigrationTest(MigrationTest):

    _test_filename = None
    _test_backupname = None
    _backup_ext = '.mantidbackup'

    def tearDown(self):
        """Clean up after a test"""
        self.remove_test_files()

    def test_simple_string_replace_gets_expected_string(self):
        self.do_migration(__INPUTSTRING__)
        self.check_outcome(__INPUTSTRING__, __EXPECTEDSTRING__)

if __name__ == "__main__":
    unittest.main()
