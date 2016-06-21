#pylint: disable=no-init,invalid-name,attribute-defined-outside-init
import stresstesting
import math
import os
from mantid.simpleapi import *
from reduction_workflow.instruments.sans.sns_command_interface import *
from mantid.api import *

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
        dq_ref = [0.00257311, 0.00215675, 0.00211189, 0.00231458, 0.00235361, 0.00237129,
                  0.00237906, 0.00234513, 0.00246818, 0.0026219, 0.0026901, 0.00260711,
                  0.00256038, 0.00263135, 0.00274395, 0.00282673, 0.00291431, 0.00301193,
                  0.00320518, 0.00356485, 0.00398606, 0.0040173, 0.00393339, 0.00412487,
                  0.00443998, 0.00432954, 0.00434995, 0.00449034, 0.00469441, 0.00494388,
                  0.00511436, 0.00531818, 0.0055303, 0.00569932, 0.00588584, 0.00610104,
                  0.00629266, 0.00654906, 0.0067967, 0.00706741, 0.00730637, 0.00764848,
                  0.0079755, 0.00829142, 0.00848771, 0.0086293, 0.00882697, 0.00903599,
                  0.00928009, 0.00966566, 0.00990059, 0.0102351, 0.0105241, 0.0108211,
                  0.0110498, 0.0113518, 0.011664, 0.0119412, 0.0122505, 0.0125243,
                  0.0128738, 0.0132604, 0.0134728, 0.0137881, 0.0141329, 0.0144213,
                  0.0147295, 0.0151151, 0.0154013, 0.0155917, 0.0158488, 0.0159876,
                  0.0163143, 0.0166233, 0.0169437, 0.0172724, 0.0175846, 0.0179733,
                  0.0182939, 0.018649, 0.0189382, 0.0193906, 0.0196348, 0.0201058,
                  0.0204372, 0.020852, 0.0211258, 0.0216753, 0.0220237, 0.0224449,
                  0.0226736, 0.0231082, 0.0236576, 0.0239633, 0.0243179, 0.0246104,
                  0.0249195, 0.0251601, 0.0254076, 0.0256563, 0]

        dq = mtd['EQSANS_1466_event_Iq'].readDx(0)
        diff = [math.fabs(dq_ref[i]-dq[i])<0.0001 for i in range(7,100)]
        output = reduce(lambda x,y:x and y, diff)
        if not output:
            for i,dqi in enumerate(dq):
                print i, dqi, dq_ref[i], math.fabs(dq_ref[i]-dqi)<0.0001
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
        dq_ref = [0.00343638, 0.00292027, 0.00284172, 0.00356216, 0.00410584, 0.00350177,
                  0.00352453, 0.0035258, 0.0033252, 0.00333963, 0.00342883, 0.00352433,
                  0.00363203, 0.00374885, 0.00386253, 0.00399503, 0.00412582, 0.00425981,
                  0.00440508, 0.00456234, 0.00469688, 0.00484763, 0.00499924, 0.00515091,
                  0.00531474, 0.0054862, 0.00567197, 0.0058903, 0.00612266, 0.00636194,
                  0.00659338, 0.00683826, 0.00713157, 0.00741236, 0.00769484, 0.00800949,
                  0.00832757, 0.00863584, 0.00898523, 0.009315, 0.00964807, 0.00999386,
                  0.0103381, 0.010696, 0.0110678, 0.0114199, 0.0117903, 0.012165,
                  0.0125308,0.0128986, 0.0133072, 0.013671, 0.0140613, 0.0144513,
                  0.0148358, 0.0152126, 0.0155984, 0.0160445, 0.0164337, 0.0167855,
                  0.0171935, 0.0175954, 0.017969, 0.0183745, 0.0187386, 0.0191184,
                  0.0194819, 0.0198701, 0.0202481, 0.0205821, 0.0209512, 0.0213435,
                  0.0216502, 0.0221041, 0.0225051, 0.0228566, 0.0232738, 0.0237153,
                  0.0240277, 0.0245176, 0.0249624, 0.0253241, 0.0257324, 0.0262129,
                  0.02659, 0.0269067, 0.0274065, 0.0279431, 0.028349, 0.0285522,
                  0.0290219, 0.0297083, 0.030095, 0.0304317, 0.0307289, 0.0310205,
                  0.0313329, 0, 0, 0, 0]

        dq = mtd['EQSANS_4061_event_frame1_Iq'].readDx(0)
        diff = [math.fabs(dq_ref[i]-dq[i])<0.0001 for i in range(7,100)]
        output = reduce(lambda x,y:x and y, diff)

        if not output:
            for i,dqi in enumerate(dq):
                print i, dqi, dq_ref[i], math.fabs(dq_ref[i]-dqi)<0.0001
        return output
