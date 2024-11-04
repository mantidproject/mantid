# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

from isis_reflectometry import quick

from mantid.api import mtd
from mantid.simpleapi import CreateSingleValuedWorkspace, CreateWorkspace, DeleteWorkspace, LoadISISNexus
from mantid.kernel import ConfigService


class ReflectometryQuickAuxiliaryTest(unittest.TestCase):
    __wsName = None

    def __init__(self, methodName="runTest"):
        self.__wsName = "TestWorkspace"
        super(ReflectometryQuickAuxiliaryTest, self).__init__(methodName)

    def setUp(self):
        ConfigService.Instance().setString("default.facility", "ISIS")
        LoadISISNexus(Filename="POLREF00004699", OutputWorkspace=self.__wsName)
        ConfigService.Instance().setString("default.facility", " ")

    def tearDown(self):
        DeleteWorkspace(mtd[self.__wsName])

    def test_cleanup(self):
        numObjectsOriginal = len(mtd.getObjectNames())
        CreateSingleValuedWorkspace(OutputWorkspace="_toremove", DataValue=1, ErrorValue=1)
        tokeep = CreateSingleValuedWorkspace(OutputWorkspace="tokeep", DataValue=1, ErrorValue=1)
        self.assertEqual(numObjectsOriginal + 2, len(mtd.getObjectNames()))
        # Should remove workspaces starting with _
        quick.cleanup()
        cleaned_object_names = mtd.getObjectNames()
        self.assertEqual(numObjectsOriginal + 1, len(cleaned_object_names))
        self.assertEqual(True, ("tokeep" in cleaned_object_names))

        DeleteWorkspace(tokeep)

    def test_groupGet_instrument(self):
        expectedInstrument = "POLREF"

        # Test with group workspace as input
        instrument = quick.groupGet(self.__wsName, "inst")
        self.assertEqual(expectedInstrument, instrument.getName(), "Did not fetch the instrument from ws group")

        # Test with single workspace as input
        instrument = quick.groupGet(mtd[self.__wsName][0].name(), "inst")
        self.assertEqual(expectedInstrument, instrument.getName(), "Did not fetch the instrument from ws")

    def test_groupGet_histogram_count(self):
        expectedNHistograms = mtd[self.__wsName][0].getNumberHistograms()

        # Test with group workspace as input
        nHistograms = quick.groupGet(self.__wsName, "wksp")
        self.assertEqual(expectedNHistograms, nHistograms, "Did not fetch the n histograms from ws group")

        # Test with single workspace as input
        nHistograms = quick.groupGet(mtd[self.__wsName][0].name(), "wksp")
        self.assertEqual(expectedNHistograms, nHistograms, "Did not fetch the n histograms from ws")

    def test_groupGet_log_single_value(self):
        expectedNPeriods = 2

        # Test with group workspace as input
        nPeriods = quick.groupGet(self.__wsName, "samp", "nperiods")
        self.assertEqual(expectedNPeriods, nPeriods, "Did not fetch the number of periods from ws group")

        # Test with single workspace as input
        nPeriods = quick.groupGet(mtd[self.__wsName][0].name(), "samp", "nperiods")
        self.assertEqual(expectedNPeriods, nPeriods, "Did not fetch the number of periods from ws")

    def test_groupGet_multi_value_log(self):
        # Expected start theta, taken from the last value of the time series log.
        expectedStartTheta = 0.4903

        # Test with group workspace as input
        stheta = quick.groupGet(self.__wsName, "samp", "stheta")
        self.assertEqual(expectedStartTheta, round(float(stheta), 4))

        # Test with single workspace as input
        stheta = quick.groupGet(mtd[self.__wsName][0].name(), "samp", "stheta")
        self.assertEqual(expectedStartTheta, round(float(stheta), 4))

    def test_groupGet_unknown_log_error_code(self):
        errorCode = 0
        # Test with group workspace as input
        self.assertEqual(errorCode, quick.groupGet(self.__wsName, "samp", "MADE-UP-LOG-NAME"))

        # Test with group workspace as input
        self.assertEqual(errorCode, quick.groupGet(mtd[self.__wsName][0].name(), "samp", "MADE-UP-LOG-NAME"))

    def test_exponential_correction_strategy(self):
        test_ws = CreateWorkspace(UnitX="TOF", DataX=[0, 1, 2, 3], DataY=[1, 1, 1], NSpec=1)

        correction = quick.ExponentialCorrectionStrategy(1, 0)  # Should have no effect.
        self.assertTrue(isinstance(correction, quick.CorrectionStrategy), msg="Should be of type Correction")

        corrected = correction.apply(test_ws)

        self.assertTrue(all(test_ws.readY(0) == corrected.readY(0)), msg="Input and outputs should be identical")

        DeleteWorkspace(test_ws)
        DeleteWorkspace(corrected)

    def test_polynomial_correction_strategy(self):
        test_ws = CreateWorkspace(UnitX="TOF", DataX=[0, 1, 2, 3], DataY=[1, 1, 1], NSpec=1)

        correction = quick.PolynomialCorrectionStrategy("1, 0")  # Should have no effect.
        self.assertTrue(isinstance(correction, quick.CorrectionStrategy), msg="Should be of type Correction")

        corrected = correction.apply(test_ws)

        self.assertTrue(all(test_ws.readY(0) == corrected.readY(0)), msg="Input and outputs should be identical")

        DeleteWorkspace(test_ws)
        DeleteWorkspace(corrected)

    def test_null_correction_strategy(self):
        test_ws = CreateWorkspace(UnitX="TOF", DataX=[0, 1, 2, 3], DataY=[1, 1, 1], NSpec=1)

        correction = quick.NullCorrectionStrategy()  # Should have no effect.
        self.assertTrue(isinstance(correction, quick.CorrectionStrategy), msg="Should be of type Correction")

        corrected = correction.apply(test_ws)

        self.assertTrue(all(test_ws.readY(0) == corrected.readY(0)), msg="Input and outputs should be identical")

        DeleteWorkspace(test_ws)
        DeleteWorkspace(corrected)


if __name__ == "__main__":
    unittest.main()
