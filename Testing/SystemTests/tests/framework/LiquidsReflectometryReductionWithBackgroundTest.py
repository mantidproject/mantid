# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=no-init
import systemtesting
from mantid.api import FileFinder
from mantid.simpleapi import LiquidsReflectometryReduction


class LiquidsReflectometryReductionWithBackgroundPreciseTest(systemtesting.MantidSystemTest):
    """
    This test checks that the new liquids reflectometer reduction code
    always produces the same results.
    """

    def runTest(self):
        # TODO: The reduction algorithm should not require an absolute path
        scaling_factor_file = FileFinder.getFullPath("directBeamDatabaseFall2014_IPTS_11601_2.cfg")

        LiquidsReflectometryReduction(
            RunNumbers=[119816],
            NormalizationRunNumber=119692,
            SignalPeakPixelRange=[155, 165],
            SubtractSignalBackground=True,
            SignalBackgroundPixelRange=[146, 165],
            NormFlag=True,
            NormPeakPixelRange=[154, 162],
            NormBackgroundPixelRange=[151, 165],
            SubtractNormBackground=True,
            ErrorWeighting=True,
            LowResDataAxisPixelRangeFlag=True,
            LowResDataAxisPixelRange=[99, 158],
            LowResNormAxisPixelRangeFlag=True,
            LowResNormAxisPixelRange=[118, 137],
            TOFRange=[9610, 22425],
            IncidentMediumSelected="2InDiamSi",
            GeometryCorrectionFlag=False,
            QMin=0.005,
            QStep=0.01,
            AngleOffset=0.009,
            AngleOffsetError=0.001,
            ScalingFactorFile=scaling_factor_file,
            SlitsWidthFlag=True,
            CropFirstAndLastPoints=False,
            OutputWorkspace="reflectivity_precise_119816",
        )

    def validate(self):
        self.disableChecking.append("Instrument")
        self.disableChecking.append("Sample")
        self.disableChecking.append("SpectraMap")
        self.disableChecking.append("Axes")
        return "reflectivity_precise_119816", "LiquidsReflectometryReductionTestWithBackground.nxs"


class NoNormalizationTest(systemtesting.MantidSystemTest):
    def runTest(self):
        # TODO: The reduction algorithm should not require an absolute path
        scaling_factor_file = FileFinder.getFullPath("directBeamDatabaseFall2014_IPTS_11601_2.cfg")

        LiquidsReflectometryReduction(
            RunNumbers=[119816],
            NormalizationRunNumber=119692,
            SignalPeakPixelRange=[155, 165],
            SubtractSignalBackground=True,
            SignalBackgroundPixelRange=[146, 165],
            ErrorWeighting=True,
            NormFlag=False,
            NormPeakPixelRange=[154, 162],
            NormBackgroundPixelRange=[151, 165],
            SubtractNormBackground=True,
            LowResDataAxisPixelRangeFlag=True,
            LowResDataAxisPixelRange=[99, 158],
            LowResNormAxisPixelRangeFlag=True,
            LowResNormAxisPixelRange=[118, 137],
            TOFRange=[9610, 22425],
            IncidentMediumSelected="2InDiamSi",
            GeometryCorrectionFlag=False,
            QMin=0.005,
            QStep=0.01,
            AngleOffset=0.009,
            AngleOffsetError=0.001,
            ScalingFactorFile=scaling_factor_file,
            SlitsWidthFlag=True,
            CropFirstAndLastPoints=False,
            OutputWorkspace="reflectivity_119816",
        )

    def validate(self):
        self.disableChecking.append("Instrument")
        self.disableChecking.append("Sample")
        self.disableChecking.append("SpectraMap")
        self.disableChecking.append("Axes")
        return "reflectivity_119816", "REFL_NoNormalizationTest.nxs"


class TOFRangeOFFTest(systemtesting.MantidSystemTest):
    def runTest(self):
        # TODO: The reduction algorithm should not require an absolute path
        scaling_factor_file = FileFinder.getFullPath("directBeamDatabaseFall2014_IPTS_11601_2.cfg")

        LiquidsReflectometryReduction(
            RunNumbers=[119816],
            NormalizationRunNumber=119692,
            SignalPeakPixelRange=[155, 165],
            SubtractSignalBackground=True,
            ErrorWeighting=True,
            SignalBackgroundPixelRange=[146, 165],
            NormFlag=True,
            NormPeakPixelRange=[154, 162],
            NormBackgroundPixelRange=[151, 165],
            SubtractNormBackground=True,
            LowResDataAxisPixelRangeFlag=True,
            LowResDataAxisPixelRange=[99, 158],
            LowResNormAxisPixelRangeFlag=True,
            LowResNormAxisPixelRange=[118, 137],
            TOFRange=[9610, 22425],
            TOFRangeFlag=False,
            IncidentMediumSelected="2InDiamSi",
            GeometryCorrectionFlag=False,
            QMin=0.005,
            QStep=0.01,
            AngleOffset=0.009,
            AngleOffsetError=0.001,
            ScalingFactorFile=scaling_factor_file,
            SlitsWidthFlag=True,
            CropFirstAndLastPoints=False,
            OutputWorkspace="reflectivity_119816",
        )

    def validate(self):
        self.disableChecking.append("Instrument")
        self.disableChecking.append("Sample")
        self.disableChecking.append("SpectraMap")
        self.disableChecking.append("Axes")
        return "reflectivity_119816", "TOFRangeOFFTest.nxs"


class NoBackgroundTest(systemtesting.MantidSystemTest):
    def runTest(self):
        # TODO: The reduction algorithm should not require an absolute path
        scaling_factor_file = FileFinder.getFullPath("directBeamDatabaseFall2014_IPTS_11601_2.cfg")

        LiquidsReflectometryReduction(
            RunNumbers=[119816],
            NormalizationRunNumber=119692,
            SignalPeakPixelRange=[155, 165],
            SubtractSignalBackground=False,
            ErrorWeighting=True,
            SignalBackgroundPixelRange=[146, 165],
            NormFlag=True,
            NormPeakPixelRange=[154, 162],
            NormBackgroundPixelRange=[151, 165],
            SubtractNormBackground=False,
            LowResDataAxisPixelRangeFlag=True,
            LowResDataAxisPixelRange=[99, 158],
            LowResNormAxisPixelRangeFlag=True,
            LowResNormAxisPixelRange=[118, 137],
            TOFRange=[9610, 22425],
            TOFRangeFlag=True,
            IncidentMediumSelected="2InDiamSi",
            GeometryCorrectionFlag=False,
            QMin=0.005,
            QStep=0.01,
            AngleOffset=0.009,
            AngleOffsetError=0.001,
            ScalingFactorFile=scaling_factor_file,
            SlitsWidthFlag=True,
            CropFirstAndLastPoints=False,
            OutputWorkspace="reflectivity_119816",
        )

    def validate(self):
        self.disableChecking.append("Instrument")
        self.disableChecking.append("Sample")
        self.disableChecking.append("SpectraMap")
        self.disableChecking.append("Axes")
        return "reflectivity_119816", "REFL_NoBackgroundTest.nxs"


class TOFMismatchTest(systemtesting.MantidSystemTest):
    correct_exception_caught = False

    def runTest(self):
        # TODO: The reduction algorithm should not require an absolute path
        scaling_factor_file = FileFinder.getFullPath("directBeamDatabaseFall2014_IPTS_11601_2.cfg")
        self.correct_exception_caught = False
        try:
            LiquidsReflectometryReduction(
                RunNumbers=[119816],
                NormalizationRunNumber=119690,
                SignalPeakPixelRange=[155, 165],
                SubtractSignalBackground=True,
                SignalBackgroundPixelRange=[146, 165],
                ErrorWeighting=True,
                NormFlag=True,
                NormPeakPixelRange=[154, 162],
                NormBackgroundPixelRange=[151, 165],
                SubtractNormBackground=True,
                LowResDataAxisPixelRangeFlag=True,
                LowResDataAxisPixelRange=[99, 158],
                LowResNormAxisPixelRangeFlag=True,
                LowResNormAxisPixelRange=[118, 137],
                TOFRange=[9610, 22425],
                TOFRangeFlag=True,
                IncidentMediumSelected="2InDiamSi",
                GeometryCorrectionFlag=False,
                QMin=0.005,
                QStep=0.01,
                AngleOffset=0.009,
                AngleOffsetError=0.001,
                ScalingFactorFile=scaling_factor_file,
                SlitsWidthFlag=True,
                CropFirstAndLastPoints=False,
                OutputWorkspace="reflectivity_119816",
            )
        except RuntimeError as err:
            msg_exp = "LiquidsReflectometryReduction-v1: Requested TOF range does not match data"
            if str(err).startswith(msg_exp):
                self.correct_exception_caught = True
            else:
                print("EXPECTED ERROR:", msg_exp)
                print("OBSERVED ERROR:", str(err))

    def validate(self):
        return self.correct_exception_caught


class BadDataTOFRangeTest(systemtesting.MantidSystemTest):
    correct_exception_caught = False

    def runTest(self):
        # TODO: The reduction algorithm should not require an absolute path
        scaling_factor_file = FileFinder.getFullPath("directBeamDatabaseFall2014_IPTS_11601_2.cfg")
        self.correct_exception_caught = False
        try:
            LiquidsReflectometryReduction(
                RunNumbers=[119816],
                NormalizationRunNumber=119690,
                SignalPeakPixelRange=[155, 165],
                SubtractSignalBackground=True,
                SignalBackgroundPixelRange=[146, 165],
                ErrorWeighting=True,
                NormFlag=True,
                NormPeakPixelRange=[154, 162],
                NormBackgroundPixelRange=[151, 165],
                SubtractNormBackground=True,
                LowResDataAxisPixelRangeFlag=True,
                LowResDataAxisPixelRange=[99, 158],
                LowResNormAxisPixelRangeFlag=True,
                LowResNormAxisPixelRange=[118, 137],
                TOFRange=[29623.0, 42438.0],
                TOFRangeFlag=True,
                IncidentMediumSelected="2InDiamSi",
                GeometryCorrectionFlag=False,
                QMin=0.005,
                QStep=0.01,
                AngleOffset=0.009,
                AngleOffsetError=0.001,
                ScalingFactorFile=scaling_factor_file,
                SlitsWidthFlag=True,
                CropFirstAndLastPoints=False,
                OutputWorkspace="reflectivity_119816",
            )
        except RuntimeError as err:
            msg_exp = "LiquidsReflectometryReduction-v1: Requested TOF range does not match data"
            if str(err).startswith(msg_exp):
                self.correct_exception_caught = True
            else:
                print("EXPECTED ERROR:", msg_exp)
                print("OBSERVED ERROR:", str(err))

    def validate(self):
        return self.correct_exception_caught


class BadPeakSelectionTest(systemtesting.MantidSystemTest):
    correct_exception_caught = False

    def runTest(self):
        # TODO: The reduction algorithm should not require an absolute path
        scaling_factor_file = FileFinder.getFullPath("directBeamDatabaseFall2014_IPTS_11601_2.cfg")
        self.correct_exception_caught = False
        try:
            LiquidsReflectometryReduction(
                RunNumbers=[119816],
                NormalizationRunNumber=119692,
                SignalPeakPixelRange=[138, 145],
                SubtractSignalBackground=True,
                SignalBackgroundPixelRange=[135, 165],
                ErrorWeighting=True,
                NormFlag=True,
                NormPeakPixelRange=[154, 162],
                NormBackgroundPixelRange=[151, 165],
                SubtractNormBackground=True,
                LowResDataAxisPixelRangeFlag=True,
                LowResDataAxisPixelRange=[99, 158],
                LowResNormAxisPixelRangeFlag=True,
                LowResNormAxisPixelRange=[118, 137],
                TOFRange=[9610, 22425],
                IncidentMediumSelected="2InDiamSi",
                GeometryCorrectionFlag=False,
                QMin=0.005,
                QStep=0.01,
                AngleOffset=0.009,
                AngleOffsetError=0.001,
                ScalingFactorFile=scaling_factor_file,
                SlitsWidthFlag=True,
                CropFirstAndLastPoints=False,
                OutputWorkspace="reflectivity_119816",
            )
        except RuntimeError as err:
            msg_exp = "LiquidsReflectometryReduction-v1: The reflectivity is all zeros"
            if str(err).startswith(msg_exp):
                self.correct_exception_caught = True
            else:
                print("EXPECTED ERROR:", msg_exp)
                print("OBSERVED ERROR:", str(err))

    def validate(self):
        return self.correct_exception_caught
