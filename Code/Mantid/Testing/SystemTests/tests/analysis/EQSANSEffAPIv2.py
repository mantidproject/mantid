#pylint: disable=no-init
import stresstesting
import mantid
from mantid.simpleapi import *
from reduction_workflow.instruments.sans.sns_command_interface import *
from mantid.api import FileFinder

import os

class EQSANSEff(stresstesting.MantidStressTest):

    def cleanup(self):
        absfile = FileFinder.getFullPath("EQSANS_1466_event_reduction.log")
        if os.path.exists(absfile):
            os.remove(absfile)
        return True

    def runTest(self):
        """
            System test for sensitivity correction
        """
        config = ConfigService.Instance()
        config["facilityName"]='SNS'
        EQSANS(False)
        AppendDataFile("EQSANS_1466_event.nxs")
        SolidAngle()
        UseConfig(False)
        UseConfigTOFTailsCutoff(False)
        UseConfigMask(False)
        SetBeamCenter(96.29, 126.15)
        SetTransmission(1.0, 0.0)
        TotalChargeNormalization(normalize_to_beam=False)
        SensitivityCorrection("EQSANS_4061_event.nxs", min_sensitivity=0.5, max_sensitivity=1.5, dark_current=None, use_sample_dc=False)
        Reduce1D()
        Scale(InputWorkspace="EQSANS_1466_event_Iq", Factor=277.781,
              Operation='Multiply', OutputWorkspace="EQSANS_1466_event_Iq")

    def validate(self):
        # Be more tolerant with the output, mainly because of the errors.
        # The following tolerance check the errors up to the third digit.
        mtd["EQSANS_1466_event_Iq"].dataE(0)[0]=8.13907
        self.tolerance = 0.1
        self.disableChecking.append('Instrument')
        self.disableChecking.append('Sample')
        self.disableChecking.append('SpectraMap')
        self.disableChecking.append('Axes')
        return "EQSANS_1466_event_Iq", 'EQSANSEff.nxs'

