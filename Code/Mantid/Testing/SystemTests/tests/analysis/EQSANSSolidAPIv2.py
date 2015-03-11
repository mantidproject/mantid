#pylint: disable=no-init
import stresstesting
from mantid.simpleapi import *
from reduction_workflow.instruments.sans.sns_command_interface import *
from mantid.api import *

import os

def do_cleanup():
    absfile = FileFinder.getFullPath("EQSANS_1466_event_reduction.log")
    if os.path.exists(absfile):
        os.remove(absfile)
        print "cleaned"
    return True

class EQSANSSolid(stresstesting.MantidStressTest):

    def cleanup(self):
        do_cleanup()
        return True
    """
        Analysis Tests for EQSANS
        Testing that the I(Q) output of is correct 
    """

    def runTest(self):
        """
            Check that EQSANSTofStructure returns the correct workspace
        """
        config = ConfigService.Instance()
        config["facilityName"]='SNS'
        EQSANS(False)
        AppendDataFile("EQSANS_1466_event.nxs")
        SolidAngle()
        UseConfig(False)
        UseConfigTOFTailsCutoff(False)
        UseConfigMask(False)
        TotalChargeNormalization(normalize_to_beam=False)
        SetBeamCenter(96.29, 126.15)
        SetTransmission(1.0,0.0, False)
        Reduce1D()
         # Scale up to match correct scaling.
        Scale(InputWorkspace="EQSANS_1466_event_Iq", Factor=2777.81,
              Operation='Multiply', OutputWorkspace="EQSANS_1466_event_Iq")

    def validate(self):
        self.tolerance = 0.2
        mtd["EQSANS_1466_event_Iq"].dataY(0)[0] = 269.688
        mtd["EQSANS_1466_event_Iq"].dataE(0)[0] = 13.8013
        mtd["EQSANS_1466_event_Iq"].dataY(0)[2] = 11.3167

        self.disableChecking.append('Instrument')
        self.disableChecking.append('Sample')
        self.disableChecking.append('SpectraMap')
        self.disableChecking.append('Axes')

        return "EQSANS_1466_event_Iq", 'EQSANSSolid.nxs'

class EQSANSSolidEvent(EQSANSSolid):

    def cleanup(self):
        do_cleanup()
        return True
    """
        Analysis Tests for EQSANS
        Testing that the I(Q) output of is correct 
    """
    def runTest(self):
        """
            Check that EQSANSTofStructure returns the correct workspace
        """
        config = ConfigService.Instance()
        config["facilityName"]='SNS'
        EQSANS(True)
        AppendDataFile("EQSANS_1466_event.nxs")
        SolidAngle()
        UseConfig(False)
        UseConfigTOFTailsCutoff(False)
        UseConfigMask(False)
        TotalChargeNormalization(normalize_to_beam=False)
        SetBeamCenter(96.29, 126.15)
        SetTransmission(1.0,0.0, False)
        Reduce1D()
        # Scale up to match correct scaling.
        Scale(InputWorkspace="EQSANS_1466_event_Iq", Factor=2777.81,
              Operation='Multiply', OutputWorkspace="EQSANS_1466_event_Iq")
