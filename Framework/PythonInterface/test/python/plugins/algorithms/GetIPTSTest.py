# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from unittest import mock
from mantid.simpleapi import GetIPTS


class GetIPTSTest(unittest.TestCase):
    @mock.patch("plugins.algorithms.GetIPTS.GetIPTS.findFile")
    def test_getIPTS_bad(self, mockFindFile):
        mockFindFile.return_value = "nowhere/"
        runNumber = "123456"
        with self.assertRaises(RuntimeError) as context:
            GetIPTS(RunNumber=runNumber, Instrument="SNAP")
        assert "Failed to determine IPTS directory" in str(context.exception)

    @mock.patch("plugins.algorithms.GetIPTS.GetIPTS.findFile")
    def test_getIPTS_good(self, mockFindFile):
        mockFindFile.return_value = "somewhere/IPTS/folder"
        runNumber = "123456"
        res = GetIPTS(RunNumber=runNumber, Instrument="SNAP")
        assert res == "somewhere/IPTS/"

    @mock.patch("plugins.algorithms.GetIPTS.FileFinder")
    def test_getIPTS_cache(self, mockFileFinder):
        mockFileFinder.findRuns.return_value = ["somewhere/IPTS/folder"]
        runNumber = "123456"
        res = GetIPTS(RunNumber=runNumber, Instrument="SNAP", ClearCache=True)
        assert res == "somewhere/IPTS/"
        mockFileFinder.findRuns.assert_called_once_with(f"SNAP_{runNumber}")

        # call again -- still only called once
        res = GetIPTS(RunNumber=runNumber, Instrument="SNAP")
        mockFileFinder.findRuns.assert_called_once_with(f"SNAP_{runNumber}")

    @mock.patch("plugins.algorithms.GetIPTS.FileFinder")
    def test_getIPTS_cache_clear(self, mockFileFinder):
        mockFileFinder.findRuns.return_value = ["somewhere/IPTS/folder"]
        runNumber = "123456"
        res = GetIPTS(RunNumber=runNumber, Instrument="SNAP", ClearCache=True)
        assert res == "somewhere/IPTS/"
        mockFileFinder.findRuns.assert_called_once_with(f"SNAP_{runNumber}")

        # call again -- still only called once
        res = GetIPTS(RunNumber=runNumber, Instrument="SNAP")
        mockFileFinder.findRuns.assert_called_once_with(f"SNAP_{runNumber}")

        # clear cache
        res = GetIPTS(RunNumber=runNumber, Instrument="SNAP", ClearCache=True)
        with self.assertRaises(AssertionError):
            mockFileFinder.findRuns.assert_called_once_with(f"SNAP_{runNumber}")
        assert mockFileFinder.findRuns.call_count == 2


if __name__ == "__main__":
    unittest.main()
