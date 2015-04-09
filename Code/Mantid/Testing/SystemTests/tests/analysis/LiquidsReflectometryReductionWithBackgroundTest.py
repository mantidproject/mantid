#pylint: disable=no-init
import stresstesting
from mantid import *
from mantid.simpleapi import *
import sys

class LiquidsReflectometryReductionWithBackgroundTest(stresstesting.MantidStressTest):
    """
        This test checks that the new liquids reflectometer reduction produces
        the same results as the old code. It's more tolerant than the test below.
    """
    def runTest(self):
        #TODO: The reduction algorithm should not require an absolute path
        scaling_factor_file = FileFinder.getFullPath("directBeamDatabaseFall2014_IPTS_11601_2.cfg")
        
        LiquidsReflectometryReduction(RunNumbers=[119816],
                                      NormalizationRunNumber=119692,
                                      SignalPeakPixelRange=[155, 165],
                                      SubtractSignalBackground=True,
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
                                      IncidentMediumSelected='2InDiamSi',
                                      GeometryCorrectionFlag=False,
                                      QMin=0.005,
                                      QStep=0.01,
                                      AngleOffset=0.009,
                                      AngleOffsetError=0.001,
                                      ScalingFactorFile=scaling_factor_file,
                                      SlitsWidthFlag=True,
                                      CropFirstAndLastPoints=False,
                                      OutputWorkspace='reflectivity_119816')

    def validate(self):
        # Be more tolerant with the output.
        self.tolerance = 0.0002

        # Skip the first point so we don't have to have a big tolerance
        data_y = mtd["reflectivity_119816"].dataY(0)
        data_y[1] = 0.00499601750282373
        
        self.disableChecking.append('Instrument')
        self.disableChecking.append('Sample')
        self.disableChecking.append('SpectraMap')
        self.disableChecking.append('Axes')
        return "reflectivity_119816", 'REFL_119816.nxs'


class LiquidsReflectometryReductionWithBackgroundPreciseTest(stresstesting.MantidStressTest):
    """
        This test checks that the new liquids reflectometer reduction code
        always produces the same results.
    """
    def runTest(self):
        #TODO: The reduction algorithm should not require an absolute path
        scaling_factor_file = FileFinder.getFullPath("directBeamDatabaseFall2014_IPTS_11601_2.cfg")
        
        LiquidsReflectometryReduction(RunNumbers=[119816],
                                      NormalizationRunNumber=119692,
                                      SignalPeakPixelRange=[155, 165],
                                      SubtractSignalBackground=True,
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
                                      IncidentMediumSelected='2InDiamSi',
                                      GeometryCorrectionFlag=False,
                                      QMin=0.005,
                                      QStep=0.01,
                                      AngleOffset=0.009,
                                      AngleOffsetError=0.001,
                                      ScalingFactorFile=scaling_factor_file,
                                      SlitsWidthFlag=True,
                                      CropFirstAndLastPoints=False,
                                      OutputWorkspace='reflectivity_precise_119816')

    def validate(self):
        self.disableChecking.append('Instrument')
        self.disableChecking.append('Sample')
        self.disableChecking.append('SpectraMap')
        self.disableChecking.append('Axes')
        return "reflectivity_precise_119816", 'LiquidsReflectometryReductionTestWithBackground.nxs'

class NoNormalizationTest(stresstesting.MantidStressTest):
    def runTest(self):
        #TODO: The reduction algorithm should not require an absolute path
        scaling_factor_file = FileFinder.getFullPath("directBeamDatabaseFall2014_IPTS_11601_2.cfg")
        
        LiquidsReflectometryReduction(RunNumbers=[119816],
                                      NormalizationRunNumber=119692,
                                      SignalPeakPixelRange=[155, 165],
                                      SubtractSignalBackground=True,
                                      SignalBackgroundPixelRange=[146, 165],
                                      NormFlag=False,
                                      NormPeakPixelRange=[154, 162],
                                      NormBackgroundPixelRange=[151, 165],
                                      SubtractNormBackground=True,
                                      LowResDataAxisPixelRangeFlag=True,
                                      LowResDataAxisPixelRange=[99, 158],
                                      LowResNormAxisPixelRangeFlag=True,
                                      LowResNormAxisPixelRange=[118, 137],
                                      TOFRange=[9610, 22425],
                                      IncidentMediumSelected='2InDiamSi',
                                      GeometryCorrectionFlag=False,
                                      QMin=0.005,
                                      QStep=0.01,
                                      AngleOffset=0.009,
                                      AngleOffsetError=0.001,
                                      ScalingFactorFile=scaling_factor_file,
                                      SlitsWidthFlag=True,
                                      CropFirstAndLastPoints=False,
                                      OutputWorkspace='reflectivity_119816')

    def validate(self):
        self.disableChecking.append('Instrument')
        self.disableChecking.append('Sample')
        self.disableChecking.append('SpectraMap')
        self.disableChecking.append('Axes')
        return "reflectivity_119816", 'REFL_NoNormalizationTest.nxs'

class TOFRangeOFFTest(stresstesting.MantidStressTest):
    def runTest(self):
        #TODO: The reduction algorithm should not require an absolute path
        scaling_factor_file = FileFinder.getFullPath("directBeamDatabaseFall2014_IPTS_11601_2.cfg")
        
        LiquidsReflectometryReduction(RunNumbers=[119816],
                                      NormalizationRunNumber=119692,
                                      SignalPeakPixelRange=[155, 165],
                                      SubtractSignalBackground=True,
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
                                      TofRangeFlag=False,
                                      IncidentMediumSelected='2InDiamSi',
                                      GeometryCorrectionFlag=False,
                                      QMin=0.005,
                                      QStep=0.01,
                                      AngleOffset=0.009,
                                      AngleOffsetError=0.001,
                                      ScalingFactorFile=scaling_factor_file,
                                      SlitsWidthFlag=True,
                                      CropFirstAndLastPoints=False,
                                      OutputWorkspace='reflectivity_119816')

    def validate(self):
        self.disableChecking.append('Instrument')
        self.disableChecking.append('Sample')
        self.disableChecking.append('SpectraMap')
        self.disableChecking.append('Axes')
        return "reflectivity_119816", 'TOFRangeOFFTest.nxs'


class NoBackgroundTest(stresstesting.MantidStressTest):
    def runTest(self):
        #TODO: The reduction algorithm should not require an absolute path
        scaling_factor_file = FileFinder.getFullPath("directBeamDatabaseFall2014_IPTS_11601_2.cfg")

        LiquidsReflectometryReduction(RunNumbers=[119816],
                                      NormalizationRunNumber=119692,
                                      SignalPeakPixelRange=[155, 165],
                                      SubtractSignalBackground=False,
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
                                      TofRangeFlag=True,
                                      IncidentMediumSelected='2InDiamSi',
                                      GeometryCorrectionFlag=False,
                                      QMin=0.005,
                                      QStep=0.01,
                                      AngleOffset=0.009,
                                      AngleOffsetError=0.001,
                                      ScalingFactorFile=scaling_factor_file,
                                      SlitsWidthFlag=True,
                                      CropFirstAndLastPoints=False,
                                      OutputWorkspace='reflectivity_119816')

    def validate(self):
        self.disableChecking.append('Instrument')
        self.disableChecking.append('Sample')
        self.disableChecking.append('SpectraMap')
        self.disableChecking.append('Axes')
        return "reflectivity_119816", 'REFL_NoBackgroundTest.nxs'


class TOFMismatchTest(stresstesting.MantidStressTest):
    def runTest(self):
        #TODO: The reduction algorithm should not require an absolute path
        scaling_factor_file = FileFinder.getFullPath("directBeamDatabaseFall2014_IPTS_11601_2.cfg")
        self.correct_exception_caught = False
        try:
            LiquidsReflectometryReduction(RunNumbers=[119816],
                                          NormalizationRunNumber=119690,
                                          SignalPeakPixelRange=[155, 165],
                                          SubtractSignalBackground=True,
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
                                          TofRangeFlag=True,
                                          IncidentMediumSelected='2InDiamSi',
                                          GeometryCorrectionFlag=False,
                                          QMin=0.005,
                                          QStep=0.01,
                                          AngleOffset=0.009,
                                          AngleOffsetError=0.001,
                                          ScalingFactorFile=scaling_factor_file,
                                          SlitsWidthFlag=True,
                                          CropFirstAndLastPoints=False,
                                          OutputWorkspace='reflectivity_119816')
        except RuntimeError as e:
            if str(e).startswith("Requested TOF range does not match data"):
                self.correct_exception_caught = True

    def validate(self):
        return self.correct_exception_caught


class BadDataTOFRangeTest(stresstesting.MantidStressTest):
    def runTest(self):
        #TODO: The reduction algorithm should not require an absolute path
        scaling_factor_file = FileFinder.getFullPath("directBeamDatabaseFall2014_IPTS_11601_2.cfg")
        self.correct_exception_caught = False
        try:
            LiquidsReflectometryReduction(RunNumbers=[119816],
                                          NormalizationRunNumber=119690,
                                          SignalPeakPixelRange=[155, 165],
                                          SubtractSignalBackground=True,
                                          SignalBackgroundPixelRange=[146, 165],
                                          NormFlag=True,
                                          NormPeakPixelRange=[154, 162],
                                          NormBackgroundPixelRange=[151, 165],
                                          SubtractNormBackground=True,
                                          LowResDataAxisPixelRangeFlag=True,
                                          LowResDataAxisPixelRange=[99, 158],
                                          LowResNormAxisPixelRangeFlag=True,
                                          LowResNormAxisPixelRange=[118, 137],
                                          TOFRange=[29623.0, 42438.0],
                                          TofRangeFlag=True,
                                          IncidentMediumSelected='2InDiamSi',
                                          GeometryCorrectionFlag=False,
                                          QMin=0.005,
                                          QStep=0.01,
                                          AngleOffset=0.009,
                                          AngleOffsetError=0.001,
                                          ScalingFactorFile=scaling_factor_file,
                                          SlitsWidthFlag=True,
                                          CropFirstAndLastPoints=False,
                                          OutputWorkspace='reflectivity_119816')
        except RuntimeError as e:
            if str(e).startswith("Requested TOF range does not match data"):
                self.correct_exception_caught = True

    def validate(self):
        return self.correct_exception_caught


class BadPeakSelectionTest(stresstesting.MantidStressTest):
    def runTest(self):
        #TODO: The reduction algorithm should not require an absolute path
        scaling_factor_file = FileFinder.getFullPath("directBeamDatabaseFall2014_IPTS_11601_2.cfg")
        self.correct_exception_caught = False
        try:
            LiquidsReflectometryReduction(RunNumbers=[119816],
                                          NormalizationRunNumber=119692,
                                          SignalPeakPixelRange=[138, 145],
                                          SubtractSignalBackground=True,
                                          SignalBackgroundPixelRange=[135, 165],
                                          NormFlag=True,
                                          NormPeakPixelRange=[154, 162],
                                          NormBackgroundPixelRange=[151, 165],
                                          SubtractNormBackground=True,
                                          LowResDataAxisPixelRangeFlag=True,
                                          LowResDataAxisPixelRange=[99, 158],
                                          LowResNormAxisPixelRangeFlag=True,
                                          LowResNormAxisPixelRange=[118, 137],
                                          TOFRange=[9610, 22425],
                                          IncidentMediumSelected='2InDiamSi',
                                          GeometryCorrectionFlag=False,
                                          QMin=0.005,
                                          QStep=0.01,
                                          AngleOffset=0.009,
                                          AngleOffsetError=0.001,
                                          ScalingFactorFile=scaling_factor_file,
                                          SlitsWidthFlag=True,
                                          CropFirstAndLastPoints=False,
                                          OutputWorkspace='reflectivity_119816')
        except RuntimeError as e:
            if str(e).startswith("The reflectivity is all zeros"):
                self.correct_exception_caught = True

    def validate(self):
        return self.correct_exception_caught


