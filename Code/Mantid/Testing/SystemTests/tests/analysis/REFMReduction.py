#pylint: disable=no-init,attribute-defined-outside-init
import stresstesting
from mantid import *

from mantid.simpleapi import *

class REFMReduction(stresstesting.MantidStressTest):
    def runTest(self):
        RefReduction(DataRun=str(9709),
                     NormalizationRun=str(9684),
                     SignalPeakPixelRange=[216, 224],
                     SubtractSignalBackground=True,
                     SignalBackgroundPixelRange=[172, 197],
                     PerformNormalization=True,
                     NormPeakPixelRange=[226, 238],
                     NormBackgroundPixelRange=[130, 183],
                     SubtractNormBackground=False,
                     CropLowResDataAxis=True,
                     CropLowResNormAxis=False,
                     LowResDataAxisPixelRange = [86, 159],
                     NBins=40,
                     Theta=0.086,
                     PolarizedData=True,
                     Instrument="REF_M",
                     OutputWorkspacePrefix='reflectivity')

    def validate(self):
        # Be more tolerant with the output, mainly because of the errors.
        # The following tolerance check the errors up to the third digit.
        self.tolerance = 0.25
        self.disableChecking.append('Instrument')
        self.disableChecking.append('Sample')
        self.disableChecking.append('SpectraMap')
        self.disableChecking.append('Axes')
        return "reflectivity-Off_Off", 'REFMReduction_off_off.nxs'

