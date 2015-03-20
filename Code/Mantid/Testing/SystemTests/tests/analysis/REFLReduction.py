#pylint: disable=no-init
import stresstesting
from mantid import *

from mantid.simpleapi import *

class REFLReduction(stresstesting.MantidStressTest):
    def runTest(self):
        #TODO: The reduction algorithm should not require an absolute path
        scaling_factor_file = FileFinder.getFullPath("directBeamDatabaseFall2014_IPTS_11601_2.cfg")
        
        #RefLReduction(RunNumbers=[119814],
        LiquidsReflectometryReduction(RunNumbers=[119814],
                      NormalizationRunNumber=119690,
                      SignalPeakPixelRange=[154, 166],
                      SubtractSignalBackground=True,
                      SignalBackgroundPixelRange=[151, 169],
                      NormFlag=True,
                      NormPeakPixelRange=[154, 160],
                      NormBackgroundPixelRange=[151, 163],
                      SubtractNormBackground=True,
                      LowResDataAxisPixelRangeFlag=True,
                      LowResDataAxisPixelRange=[99, 158],
                      LowResNormAxisPixelRangeFlag=True,
                      LowResNormAxisPixelRange=[98, 158],
                      TOFRange=[29623.0, 42438.0],
                      IncidentMediumSelected='2InDiamSi',
                      GeometryCorrectionFlag=False,
                      QMin=0.005,
                      QStep=0.01,
                      AngleOffset=0.009,
                      AngleOffsetError=0.001,
                      ScalingFactorFile=scaling_factor_file,
                      SlitsWidthFlag=True,
                      OutputWorkspace='reflectivity_119814')

    def validate(self):
        # Be more tolerant with the output.
        self.tolerance = 0.008

        self.disableChecking.append('Instrument')
        self.disableChecking.append('Sample')
        self.disableChecking.append('SpectraMap')
        self.disableChecking.append('Axes')
        return "reflectivity_119814", 'REFL_119814_combined_data.nxs'

