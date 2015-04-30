#pylint: disable=no-init,attribute-defined-outside-init
import stresstesting
from mantid import *
from mantid.simpleapi import *

class REFLWithBackground(stresstesting.MantidStressTest):
    def runTest(self):
        #TODO: The reduction algorithm should not require an absolute path
        scaling_factor_file = FileFinder.getFullPath("directBeamDatabaseFall2014_IPTS_11601_2.cfg")

        RefLReduction(RunNumbers=[119816],
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
                      OutputWorkspace='reflectivity_119816')

    def validate(self):
        # Be more tolerant with the output.
        self.tolerance = 0.0001

        self.disableChecking.append('Instrument')
        self.disableChecking.append('Sample')
        self.disableChecking.append('SpectraMap')
        self.disableChecking.append('Axes')
        return "reflectivity_119816", 'REFL_119816.nxs'

