#pylint: disable=no-init,invalid-name,attribute-defined-outside-init
import stresstesting
import math
from mantid.simpleapi import *
from reduction_workflow.instruments.sans.sns_command_interface import *
from mantid.api import *

import os

def do_cleanup():
    Files = ["EQSANS_4061_event_reduction.log",
             "EQSANS_1466_event_reduction.log"]
    for filename in Files:
        absfile = FileFinder.getFullPath(filename)
        if os.path.exists(absfile):
            os.remove(absfile)
    return True

class EQSANSIQOutput(stresstesting.MantidStressTest):
    """
        Analysis Tests for EQSANS
        Testing that the I(Q) output of is correct
    """

    def cleanup(self):
        do_cleanup()
        return True

    def runTest(self):
        """
            Check that EQSANSTofStructure returns the correct workspace
        """
        configI = ConfigService.Instance()
        configI["facilityName"]='SNS'
        EQSANS()
        SetBeamCenter(96.29, 126.15)
        AppendDataFile("EQSANS_1466_event.nxs")
        NoSolidAngle()
        UseConfig(False)
        UseConfigTOFTailsCutoff(False)
        UseConfigMask(False)
        TotalChargeNormalization(normalize_to_beam=False)
        Reduce1D()
        # Scale up to match correct scaling.
        Scale(InputWorkspace="EQSANS_1466_event_Iq", Factor=2777.81,
              Operation='Multiply', OutputWorkspace="EQSANS_1466_event_Iq")

    def validate(self):
        self.tolerance = 0.2
        mtd["EQSANS_1466_event_Iq"].dataY(0)[0] = 269.687
        mtd["EQSANS_1466_event_Iq"].dataE(0)[0] = 16.4977
        mtd["EQSANS_1466_event_Iq"].dataE(0)[1] = 6.78
        mtd["EQSANS_1466_event_Iq"].dataY(0)[2] = 11.3157
        mtd["EQSANS_1466_event_Iq"].dataE(0)[2] = 1.23419
        self.disableChecking.append('Instrument')
        self.disableChecking.append('Sample')
        self.disableChecking.append('SpectraMap')
        self.disableChecking.append('Axes')
        return "EQSANS_1466_event_Iq", 'EQSANSIQOutput.nxs'

class EQSANSBeamMonitor(stresstesting.MantidStressTest):
    """
        Analysis Tests for EQSANS
        Testing that the I(Q) output of is correct
    """

    def cleanup(self):
        do_cleanup()
        return True

    def runTest(self):
        configI = ConfigService.Instance()
        configI["facilityName"]='SNS'
        EQSANS()
        SetBeamCenter(96.29, 126.15)
        AppendDataFile("EQSANS_1466_event.nxs")
        NoSolidAngle()
        UseConfig(False)
        UseConfigTOFTailsCutoff(False)
        UseConfigMask(False)
        BeamMonitorNormalization('SANSBeamFluxCorrectionMonitor.nxs')
        Reduce1D()

    def validate(self):
        self.disableChecking.append('Instrument')
        self.disableChecking.append('Sample')
        self.disableChecking.append('SpectraMap')
        self.disableChecking.append('Axes')
        return "EQSANS_1466_event_Iq", 'EQSANSBeamMonitor.nxs'

class EQSANSDQPositiveOutput(stresstesting.MantidStressTest):
    """
        Analysis Tests for EQSANS
        Testing that the Q resolution output of is correct
    """

    def cleanup(self):
        do_cleanup()
        return True

    def runTest(self):
        """
            Check that the Q resolution calculation returns positive values
            even when background is larger than signal and I(q) is negative.
            (Non-physical value that's an experimental edge case)
        """
        configI = ConfigService.Instance()
        configI["facilityName"]='SNS'
        EQSANS()
        SetBeamCenter(96.29, 126.15)
        AppendDataFile("EQSANS_1466_event.nxs")
        UseConfig(False)
        UseConfigTOFTailsCutoff(False)
        UseConfigMask(False)
        TotalChargeNormalization(normalize_to_beam=False)
        SetTransmission(1.0,0.0, False)
        Background("EQSANS_4061_event.nxs")
        Resolution()
        Reduce1D()

    def validate(self):
        dq = mtd['EQSANS_1466_event_Iq'].dataDx(0)
        for x in dq:
            if x<0:
                return False
        return True

class EQSANSDQOutput(stresstesting.MantidStressTest):
    """
        Analysis Tests for EQSANS
        Testing that the Q resolution output of is correct
    """

    def cleanup(self):
        do_cleanup()
        return True

    def runTest(self):
        """
            Check that the Q resolution calculation returns positive values
            even when background is larger than signal and I(q) is negative.
            (Non-physical value that's an experimental edge case)
        """
        configI = ConfigService.Instance()
        configI["facilityName"]='SNS'
        EQSANS()
        SetBeamCenter(96.29, 126.15)
        AppendDataFile("EQSANS_1466_event.nxs")
        UseConfig(False)
        UseConfigTOFTailsCutoff(False)
        UseConfigMask(False)
        TotalChargeNormalization(normalize_to_beam=False)
        SetTransmission(1.0, 0.0, False)
        Background("EQSANS_4061_event.nxs")
        Resolution(10)
        Reduce1D()

    def validate(self):
        """
            Reference values were generate using the event-by-event method
            and are slightly different than the ones generated using
            the histogram method.
            The event-by-event method processes each event one-by-one,
            computes dQ for each of them, and averages those dQ for each
            Q bin of the I(Q) distribution.
        """
        dq_ref = [0.00179091, 0.00146174, 0.00147197, 0.00164544, 0.00163043, 0.00181878,
                  0.00182258, 0.00193342, 0.00206702, 0.00213708, 0.00226281, 0.00228791,
                  0.00243715, 0.00259161, 0.00265703, 0.00280087, 0.00281749, 0.00304302,
                  0.00312193, 0.00336326, 0.00369032, 0.00363043, 0.00361898, 0.00380932,
                  0.00409294, 0.00420369, 0.00434701, 0.00449854, 0.00478654, 0.00493388,
                  0.00505692, 0.00528792, 0.00530439, 0.00576054, 0.00567652, 0.00611692,
                  0.00629483, 0.00654051, 0.00699489, 0.00697047, 0.00729058, 0.0075435,
                  0.00790411, 0.00799244, 0.00839339, 0.00863375, 0.00875037, 0.00915707,
                  0.00920598, 0.00956547, 0.0100485, 0.010375, 0.0105826, 0.0107592,
                  0.0111573, 0.0114594, 0.0120101, 0.0121598, 0.0124813, 0.0129782,
                  0.0130015, 0.013079, 0.0135361, 0.0142822, 0.0139875, 0.0144879,
                  0.0146769, 0.0148238, 0.0151934, 0.0159136, 0.0163288, 0.0157482,
                  0.0166674, 0.0170985, 0.0174427, 0.0175502, 0.0181306, 0.0191156,
                  0.0186574, 0.0187086, 0.0189025, 0.0192468, 0.0196659, 0.0206723,
                  0.0207745, 0.0214165, 0.0217546, 0.0218078, 0.0228492, 0.023019,
                  0.0232376, 0.0231974, 0.0237268,       0,       0,       0,
                  0.0247018,       0,       0,       0,       0]

        dq = mtd['EQSANS_1466_event_Iq'].readDx(0)
        diff = [math.fabs(dq_ref[i]-dq[i])<0.0001 for i in range(7,100)]
        output = reduce(lambda x,y:x and y, diff)
        if not output:
            for i in range(len(dq)):
                print i, dq[i], dq_ref[i], math.fabs(dq_ref[i]-dq[i])<0.0001
        return output

class EQSANSDQOutput_FS(stresstesting.MantidStressTest):
    """
        Analysis Tests for EQSANS
        Testing that the Q resolution output of is correct
    """

    def cleanup(self):
        do_cleanup()
        return True

    def runTest(self):
        """
            Check that the Q resolution calculation returns positive values
            even when background is larger than signal and I(q) is negative.
            (Non-physical value that's an experimental edge case)
        """
        configI = ConfigService.Instance()
        configI["facilityName"]='SNS'
        EQSANS()
        SetBeamCenter(96.29, 126.15)
        AppendDataFile("EQSANS_4061_event.nxs")
        UseConfig(False)
        UseConfigTOFTailsCutoff(False)
        UseConfigMask(False)
        TotalChargeNormalization(normalize_to_beam=False)
        SetTransmission(1.0,0.0, False)
        Resolution(12)
        Reduce1D()

    def validate(self):
        """
            Reference values were generate using the event-by-event method
            and are slightly different than the ones generated using
            the histogram method.
            The event-by-event method processes each event one-by-one,
            computes dQ for each of them, and averages those dQ for each
            Q bin of the I(Q) distribution.
        """
        dq_ref = [0.00255737, 0.00218259, 0.00213623, 0.00270166, 0.00314686, 0.00271786,
        0.00277551, 0.0028148, 0.0026912, 0.00274053, 0.00285143, 0.00296802, 0.00309525,
        0.00323062, 0.00336323, 0.00351213, 0.0036594, 0.00380924, 0.00396875, 0.00413898,
        0.00428816, 0.0044516, 0.00461547, 0.00477897, 0.00495329, 0.00513446, 0.00532868,
        0.00555334, 0.00579135, 0.00603596, 0.00627302, 0.00652278, 0.00681888, 0.00710323,
        0.00738927, 0.0077063, 0.00802685, 0.00833806, 0.00868906, 0.00902133, 0.00935685,
        0.00970481, 0.0100514, 0.0104113, 0.0107849, 0.0111394, 0.0115118, 0.0118885,
        0.0122565, 0.0126266, 0.0130366, 0.0134029, 0.013795, 0.014187, 0.0145736,
        0.0149527, 0.0153406, 0.0157879, 0.0161792, 0.0165336, 0.0169434, 0.0173473,
        0.0177233, 0.0181306, 0.0184971, 0.0188791, 0.0192451, 0.0196353, 0.0200155,
        0.0203521, 0.0207234, 0.0211177, 0.0214272, 0.0218823, 0.0222852, 0.0226389,
        0.0230577, 0.0235006, 0.0238156, 0.0243063, 0.0247524, 0.0251161, 0.025526,
        0.0260076, 0.0263866, 0.0267056, 0.0272062, 0.0277434, 0.0281509, 0.0283573,
        0.028828, 0.0295137, 0.0299022, 0.030241, 0.0305401, 0.0308333,
        0.0311474,       0,       0,       0,       0]

        dq = mtd['EQSANS_4061_event_frame1_Iq'].readDx(0)
        diff = [math.fabs(dq_ref[i]-dq[i])<0.0001 for i in range(7,100)]
        output = reduce(lambda x,y:x and y, diff)

        if not output:
            for i in range(len(dq)):
                print i, dq[i], dq_ref[i], math.fabs(dq_ref[i]-dq[i])<0.0001
        return output
