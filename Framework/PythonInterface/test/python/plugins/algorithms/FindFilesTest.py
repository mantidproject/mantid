from __future__ import (absolute_import, division, print_function)

import unittest
from mantid.simpleapi import *

class FindFilesTest(unittest.TestCase):

    _fileslist = 'INTER00013460,13463,13464.nxs'

    def test_happy_case(self):

        criteria = '$raw_data_1/duration$ > 1000 or $raw_data_1/good_frames$ > 10000'
        res = FindFiles(FileList=self._fileslist,NexusCriteria=criteria)
        self.assertEqual(res.count(','), 1,"There should be 1 comma, since only 1st and 3rd files satisfy.")
        outfiles = res.split(',')
        self.assertTrue(outfiles[0].endswith('INTER00013460.nxs'),'Fully resolved first file name')
        self.assertTrue(outfiles[1].endswith('INTER00013464.nxs'),'Fully resolved second file name')

    def test_invalid_syntax(self):

        criteria = '$raw_data_1/duration$ += 1000'
        res = FindFiles(FileList=self._fileslist, NexusCriteria=criteria)
        self.assertFalse(res, "Output should be empty because criteria is wrong")

    def test_wrong_nexus_entry(self):

        criteria = '$raw_data_1/duration$ > 1000 or $raw_data_1/good_Grames$ > 10000'
        res = FindFiles(FileList=self._fileslist, NexusCriteria=criteria)
        self.assertFalse(res, "Output should be empty since 2nd nexus entry name does not exist")

    def test_unsatisfying_criteria(self):

        criteria = '$raw_data_1/duration$ > 1000 and $raw_data_1/good_frames$ < 10000'
        res = FindFiles(FileList=self._fileslist, NexusCriteria=criteria)
        self.assertFalse(res, "Output should be empty since no file satisfies this criteria")

if __name__=="__main__":
    unittest.main()
