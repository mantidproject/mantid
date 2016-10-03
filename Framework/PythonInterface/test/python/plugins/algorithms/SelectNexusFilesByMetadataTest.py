#pylint: disable=unused-import
from __future__ import (absolute_import, division, print_function)

import unittest
from mantid.simpleapi import *

class SelectNexusFilesByMetadataTest(unittest.TestCase):

    _fileslist = 'INTER00013460,13463,13464.nxs'
    _sumfileslist = 'INTER00013460+13463+13464.nxs'

    def test_happy_case(self):

        criteria = '$raw_data_1/duration$ > 1000 or $raw_data_1/good_frames$ > 10000'
        res = SelectNexusFilesByMetadata(FileList=self._fileslist,NexusCriteria=criteria)
        outfiles = res.split(',')
        self.assertEqual(len(outfiles), 2, "Only 1st and 3rd files satisfy.")
        self.assertTrue(outfiles[0].endswith('INTER00013460.nxs'),'Should be first file name')
        self.assertTrue(outfiles[1].endswith('INTER00013464.nxs'),'Should be second file name')

    def test_sum(self):
        criteria = '$raw_data_1/duration$ > 1000 or $raw_data_1/good_frames$ > 10000'
        res = SelectNexusFilesByMetadata(FileList=self._sumfileslist, NexusCriteria=criteria)
        outfiles = res.split('+')
        self.assertEqual(len(outfiles), 2, "Only 1st and 3rd files satisfy.")
        self.assertTrue(outfiles[0].endswith('INTER00013460.nxs'), 'Should be first file name')
        self.assertTrue(outfiles[1].endswith('INTER00013464.nxs'), 'Should be second file name')

    def test_invalid_syntax(self):

        criteria = '$raw_data_1/duration$ += 1000'
        throws = False
        try:
            SelectNexusFilesByMetadata(FileList=self._fileslist, NexusCriteria=criteria)
        except RuntimeError:
            throws = True
        self.assertTrue(throws,"Should raise a runtime error")
        # asserRaises does not work with python 2.6 so fails on RHEL 6

    def test_wrong_nexus_entry(self):

        criteria = '$raw_data_1/duration$ > 1000 or $raw_data_1/good_Grames$ > 10000'
        res = SelectNexusFilesByMetadata(FileList=self._fileslist, NexusCriteria=criteria)
        self.assertFalse(res, "Output should be empty since 2nd nexus entry name does not exist")

    def test_unsatisfying_criteria(self):

        criteria = '$raw_data_1/duration$ > 1000 and $raw_data_1/good_frames$ < 10000'
        res = SelectNexusFilesByMetadata(FileList=self._fileslist, NexusCriteria=criteria)
        self.assertFalse(res, "Output should be empty since no file satisfies this criteria")

    def test_cross_criteria(self):

        criteria = '10 * $raw_data_1/duration$ < $raw_data_1/good_frames$'
        res = SelectNexusFilesByMetadata(FileList=self._fileslist,NexusCriteria=criteria)
        outfiles = res.split(',')
        self.assertEqual(len(outfiles), 1, "Only 1st file satisfies.")
        self.assertTrue(outfiles[0].endswith('INTER00013460.nxs'),'Should be 1st first file name')

    def test_string_criteria(self):

        filelist = 'ILLD33_001030.nxs'
        criteria = '$entry0/D33/name$ == "D33"'
        res = SelectNexusFilesByMetadata(FileList=filelist,NexusCriteria=criteria)
        outfiles = res.split(',')
        self.assertTrue(outfiles[0].endswith('ILLD33_001030.nxs'),'Should be the file name')

if __name__=="__main__":
    # run the test if only if the required package is present
    try:
        import h5py
        unittest.main()
    except ImportError:
        pass

