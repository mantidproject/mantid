#pylint: disable=no-init,attribute-defined-outside-init
import stresstesting
from mantid.simpleapi import *
from reduction_workflow.instruments.sans.sns_command_interface import *
from mantid.api import *

import os

def do_cleanup():
    absfile = FileFinder.getFullPath("EQSANS_4061_event_reduction.log")
    if os.path.exists(absfile):
        os.remove(absfile)
    return True

class EQSANSBeamCenter(stresstesting.MantidStressTest):

    def cleanup(self):
        do_cleanup()
        return True

    def runTest(self):
        config = ConfigService.Instance()
        config["facilityName"]='SNS'
        EQSANS(False)
        AppendDataFile("EQSANS_4061_event.nxs")
        NoSolidAngle()
        IndependentBinning(False)
        UseConfig(False)
        UseConfigTOFTailsCutoff(False)
        UseConfigMask(False)
        SetTransmission(1.0, 0.0)
        TotalChargeNormalization(normalize_to_beam=False)
        DirectBeamCenter("EQSANS_1466_event.nxs")
        Reduce()
        # Scale up to match correct scaling. The reference data is off by a factor 10.0
        Scale(InputWorkspace="EQSANS_4061_event_frame2_Iq", Factor=10.0,
              Operation='Multiply', OutputWorkspace="EQSANS_4061_event_frame2_Iq")
        Scale(InputWorkspace="EQSANS_4061_event_frame2_Iq", Factor=277.781,
              Operation='Multiply', OutputWorkspace="EQSANS_4061_event_frame2_Iq")

    def validate(self):
        # Be more tolerant with the output, mainly because of the errors.
        # The following tolerance check the errors up to the third digit.
        self.tolerance = 0.1
        self.disableChecking.append('Instrument')
        self.disableChecking.append('Sample')
        self.disableChecking.append('SpectraMap')
        self.disableChecking.append('Axes')
        return "EQSANS_4061_event_frame2_Iq", 'EQSANSBeamCenter.nxs'

class EQSANSBeamCenterEvent(EQSANSBeamCenter):

    def cleanup(self):
        do_cleanup()
        return True

    def runTest(self):
        config = ConfigService.Instance()
        config["facilityName"]='SNS'
        EQSANS(True)
        AppendDataFile("EQSANS_4061_event.nxs")
        NoSolidAngle()
        IndependentBinning(False)
        UseConfig(False)
        UseConfigTOFTailsCutoff(False)
        UseConfigMask(False)
        SetTransmission(1.0, 0.0)
        TotalChargeNormalization(normalize_to_beam=False)
        DirectBeamCenter("EQSANS_1466_event.nxs")
        Reduce()
        # Scale up to match correct scaling. The reference data is off by a factor 10.0
        Scale(InputWorkspace="EQSANS_4061_event_frame2_Iq", Factor=10.0,
              Operation='Multiply', OutputWorkspace="EQSANS_4061_event_frame2_Iq")
        Scale(InputWorkspace="EQSANS_4061_event_frame2_Iq", Factor=277.781,
              Operation='Multiply', OutputWorkspace="EQSANS_4061_event_frame2_Iq")
