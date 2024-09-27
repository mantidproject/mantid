# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=invalid-name,no-init
# pylint: disable=too-many-public-methods
"""
Check that file manipulation works fine
"""

import unittest
import systemtesting
from ISIS.SANS.isis_sans_system_test import ISISSansSystemTest
from mantid.simpleapi import *
import SANSUtility as su
import os
from ISISCommandInterface import *
from sans_core.common.enums import SANSInstrument


def get_full_path_SANS_system_test(filename):
    """
    Check if the file is in one of the directories or not
    """

    def _useDir(direc):
        """Only allow directories that aren't test output or
        reference results."""
        if "reference" in direc:
            return False
        if config["defaultsave.directory"] == direc:
            return False
        return "Data" in direc

    dirs = config["datasearch.directories"].split(";")
    for directory in dirs:
        full_path = os.path.join(directory, filename)
        if _useDir(directory) and os.path.isfile(full_path):
            return True, full_path
    return False, ""


class SANSFileCheckingTest(unittest.TestCase):
    def _do_test(self, file_name, expected_time):
        exists, full_path = get_full_path_SANS_system_test(file_name)
        if exists:
            measurement_time = su.get_measurement_time_from_file(full_path)
            self.assertEqual(measurement_time, expected_time)
        else:
            print("Missing data files. Path to system test data needs to be set.")
            self.fail()

    def test_that_sans2D_nexus_file_with_date_is_evaluated_correctly(self):
        file_name = "SANS2D00022048.nxs"
        expected_time = "2013-10-25T19:37:36"
        self._do_test(file_name, expected_time)

        file_name_2 = "SANS2D00000808.nxs"
        expected_time_2 = "2009-08-06T05:31:12"
        self._do_test(file_name_2, expected_time_2)

        file_name_3 = "SANS2D00029089.nxs"
        expected_time_3 = "2015-06-02T17:46:06"
        self._do_test(file_name_3, expected_time_3)

    def test_that_LOQ_nexus_file_with_date_is_evaluated_correctly(self):
        file_name = "LOQ74014.nxs"
        expected_time = "2012-10-22T11:14:54"
        self._do_test(file_name, expected_time)

    def test_that_SANS2D_RAW_file_with_date_is_evaluated_correctly(self):
        file_name = "SANS2D00005546.raw"
        expected_time = "2010-05-26T12:12:31"
        self._do_test(file_name, expected_time)

        file_name_2 = "SANS2D00000808.raw"
        expected_time_2 = "2009-08-06T05:31:12"
        self._do_test(file_name_2, expected_time_2)

    def test_that_LOQ_RAW_file_with_date_is_evaluated_correctly(self):
        file_name = "LOQ54431.raw"
        expected_time = "2009-11-11T15:13:57"
        self._do_test(file_name, expected_time)

        file_name = "LOQ99618.RAW"
        expected_time = "2009-09-15T15:31:43"
        self._do_test(file_name, expected_time)


class SANSMatchIDFInReducerAndWorkspaceTest(unittest.TestCase):
    def _get_idf_path_for_workspace(self, filename, instrument_name):
        exists, full_path = get_full_path_SANS_system_test(filename)
        idf_path_workspace = None
        if exists:
            measurement_time = su.get_measurement_time_from_file(full_path)
            idf_path_workspace = ExperimentInfo.getInstrumentFilename(instrument_name, measurement_time)
        else:
            print("Missing data files. Path to system test data needs to be set.")
            self.fail()
        return idf_path_workspace

    def test_that_reducer_for_SANS2D_switches_to_correct_IDF_when_outdated(self):
        # Arrange
        Clean()
        SANS2D()
        MaskFile("MASKSANS2D.091A")
        Set1D()
        instrument_name = "SANS2D"
        filename = "SANS2D00029089.nxs"
        idf_workspace = self._get_idf_path_for_workspace(filename, instrument_name)
        idf_reducer_before = ReductionSingleton().get_idf_file_path()
        # Act
        AssignSample(filename)
        # Assert
        idf_reducer_after = ReductionSingleton().get_idf_file_path()

        self.assertNotEqual(os.path.normpath(idf_workspace), os.path.normpath(idf_reducer_before))
        self.assertEqual(os.path.normpath(idf_workspace), os.path.normpath(idf_reducer_after))

    def test_that_reducer_for_SANS2D_stays_when_already_the_same_as_in_workspace(self):
        # Arrange
        Clean()
        SANS2DTUBES()
        MaskFile("MASKSANS2D.091A")
        Set1D()
        instrument_name = "SANS2D"
        filename = "SANS2D00029089.nxs"
        idf_workspace = self._get_idf_path_for_workspace(filename, instrument_name)
        idf_reducer_before = ReductionSingleton().get_idf_file_path()
        # Act
        AssignSample(filename)
        # Assert
        idf_reducer_after = ReductionSingleton().get_idf_file_path()

        self.assertEqual(os.path.normpath(idf_workspace), os.path.normpath(idf_reducer_before))
        self.assertEqual(os.path.normpath(idf_workspace), os.path.normpath(idf_reducer_after))

    def test_that_reducer_for_LARMOR_switches_to_correct_IDF_when_outdated(self):
        # Arrange
        Clean()
        LARMOR()
        MaskFile("USER_LARMOR_151B_LarmorTeam_80tubes_BenchRot1p4_M4_r3699.txt")
        Set1D()
        instrument_name = "LARMOR"
        filename = "LARMOR00000063.nxs"
        idf_workspace = self._get_idf_path_for_workspace(filename, instrument_name)
        idf_reducer_before = ReductionSingleton().get_idf_file_path()
        # Act
        AssignSample(filename)
        # Assert
        idf_reducer_after = ReductionSingleton().get_idf_file_path()
        self.assertNotEqual(os.path.normpath(idf_workspace), os.path.normpath(idf_reducer_before))
        self.assertEqual(os.path.normpath(idf_workspace), os.path.normpath(idf_reducer_after))

    def test_that_reducer_for_LARMOR_stays_when_already_the_same_as_in_workspace(self):
        # Arrange
        Clean()
        LARMOR("LARMOR_Definition_19000000-20150317.xml")
        MaskFile("USER_LARMOR_151B_LarmorTeam_80tubes_BenchRot1p4_M4_r3699.txt")
        Set1D()
        instrument_name = "LARMOR"
        filename = "LARMOR00000063.nxs"
        idf_workspace = self._get_idf_path_for_workspace(filename, instrument_name)
        idf_reducer_before = ReductionSingleton().get_idf_file_path()
        # Act
        AssignSample(filename)
        # Assert
        idf_reducer_after = ReductionSingleton().get_idf_file_path()
        self.assertEqual(os.path.normpath(idf_workspace), os.path.normpath(idf_reducer_before))
        self.assertEqual(os.path.normpath(idf_workspace), os.path.normpath(idf_reducer_after))

    def test_that_reducer_for_LARMOR_switches_to_correct_IDF_when_outdated_V2(self):
        # Arrange
        Clean()
        LARMOR()
        MaskFile("USER_LARMOR_151B_LarmorTeam_80tubes_BenchRot1p4_M4_r3699.txt")
        Set1D()
        instrument_name = "LARMOR"
        filename = "LARMOR00002260.nxs"
        idf_workspace = self._get_idf_path_for_workspace(filename, instrument_name)
        idf_reducer_before = ReductionSingleton().get_idf_file_path()
        # Act
        AssignSample(filename)
        # Assert
        idf_reducer_after = ReductionSingleton().get_idf_file_path()
        self.assertNotEqual(os.path.normpath(idf_workspace), os.path.normpath(idf_reducer_before))
        self.assertEqual(os.path.normpath(idf_workspace), os.path.normpath(idf_reducer_after))

    def test_that_reducer_for_LOQ_stays_when_already_the_same_as_in_workspace(self):
        # Arrange
        Clean()
        LOQ()
        MaskFile("MASK.094AA")
        Set1D()
        instrument_name = "LOQ"
        filename = "LOQ54431.raw"
        idf_workspace = self._get_idf_path_for_workspace(filename, instrument_name)
        idf_reducer_before = ReductionSingleton().get_idf_file_path()
        # Act
        AssignSample(filename)
        # Assert
        idf_reducer_after = ReductionSingleton().get_idf_file_path()
        self.assertEqual(os.path.normpath(idf_workspace), os.path.normpath(idf_reducer_before))
        self.assertEqual(os.path.normpath(idf_workspace), os.path.normpath(idf_reducer_after))

    def test_that_reducer_for_LOQ_switches_to_correct_IDF_when_outdated(self):
        # Arrange
        Clean()
        LOQ("LOQ_Definition_20121016-.xml")
        MaskFile("MASK.094AA")
        Set1D()
        instrument_name = "LOQ"
        filename = "LOQ54431.raw"
        idf_workspace = self._get_idf_path_for_workspace(filename, instrument_name)
        idf_reducer_before = ReductionSingleton().get_idf_file_path()
        # Act
        AssignSample(filename)
        # Assert
        idf_reducer_after = ReductionSingleton().get_idf_file_path()
        self.assertNotEqual(os.path.normpath(idf_workspace), os.path.normpath(idf_reducer_before))
        self.assertEqual(os.path.normpath(idf_workspace), os.path.normpath(idf_reducer_after))


@ISISSansSystemTest(SANSInstrument.LARMOR, SANSInstrument.LOQ, SANSInstrument.SANS2D)
class SANSSwitchIDFTestRunner(systemtesting.MantidSystemTest):
    def __init__(self):
        systemtesting.MantidSystemTest.__init__(self)
        self._success = False

    def runTest(self):
        self._success = False
        suite = unittest.TestSuite()
        suite.addTest(unittest.makeSuite(SANSFileCheckingTest, "test"))
        suite.addTest(unittest.makeSuite(SANSMatchIDFInReducerAndWorkspaceTest, "test"))
        runner = unittest.TextTestRunner()
        res = runner.run(suite)
        if res.wasSuccessful():
            self._success = True

    def validate(self):
        return self._success
