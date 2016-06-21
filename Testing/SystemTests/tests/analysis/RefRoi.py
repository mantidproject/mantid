#pylint: disable=no-init
import stresstesting
from mantid import *
from mantid.simpleapi import *

class RefRoiTest(stresstesting.MantidStressTest):
    def runTest(self):
        workspace = Load(Filename="REF_L_119814")
        workspace = Integration(InputWorkspace=workspace)
        roi = RefRoi(InputWorkspace=workspace,
                     NXPixel=256, NYPixel=304,
                     IntegrateY=False, ConvertToQ=False)
        roi = Transpose(InputWorkspace=roi)

    def validate(self):
        self.disableChecking.append('Instrument')
        self.disableChecking.append('Sample')
        self.disableChecking.append('SpectraMap')
        self.disableChecking.append('Axes')
        return "roi", 'REFL_119814_roi_peak.nxs'

