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
        dq_ref = [0.00179027, 0.00145976, 0.00146710, 0.00163422, 0.00161492, 0.00179164,
                  0.00179029, 0.00188995, 0.00201170, 0.00207360, 0.00218609, 0.00220714,
                  0.00234202, 0.00248183, 0.00254135, 0.00267204, 0.00268951, 0.00289434,
                  0.00296832, 0.00318395, 0.00347480, 0.00342855, 0.00342605, 0.00359649,
                  0.00384836, 0.00394975, 0.00408187, 0.00422229, 0.00447594, 0.00461118,
                  0.00472718, 0.00493621, 0.00495764, 0.00535326, 0.00529209, 0.00567744,
                  0.00583839, 0.00605891, 0.00645220, 0.00644125, 0.00672271, 0.00694243,
                  0.00726187, 0.00733947, 0.00769454, 0.00790483, 0.00801236, 0.00836713,
                  0.00841945, 0.00873702, 0.00915623, 0.00944367, 0.00963511, 0.00978777,
                  0.01014052, 0.01041332, 0.01089778, 0.01102930, 0.01131622, 0.01175365,
                  0.01178201, 0.01185351, 0.01225765, 0.01291915, 0.01266411, 0.0131045,
                  0.01327450, 0.01340715, 0.01373935, 0.01437759, 0.01474689, 0.01423396,
                  0.01505104, 0.01543748, 0.01574309, 0.01584045, 0.01635975, 0.01723819,
                  0.01683236, 0.01687807, 0.01705414, 0.01736111, 0.01773676, 0.01863491,
                  0.01872719, 0.01930282, 0.01960495, 0.01965400, 0.02058695, 0.02073798,
                  0.02093445, 0.02090090, 0.02137465, 0., 0., 0.,
                  0.02225058, 0., 0., 0., 0.]

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
        dq_ref = [0.0025562401875382367, 0.0021790962062932595, 0.0021290959068076083,
                  0.0026802124939521062, 0.0031074321602349289, 0.0026774405911485217,
                  0.0027207082270666681, 0.0027487677707096962, 0.0026247953341494452,
                  0.0026651032275206979, 0.0027635552536132684, 0.0028676821934358675,
                  0.0029818100223198852, 0.0031035596823376366, 0.0032232077253371329,
                  0.0033576194755771654, 0.0034907516138855572, 0.0036265816355667713,
                  0.0037710626911531238, 0.0039250441580203372, 0.0040609475263300582,
                  0.0042096897493604209, 0.0043588342800656822, 0.0045079887072827212,
                  0.0046667831858807959, 0.0048317029230020786, 0.0050076609393374626,
                  0.0052096495474734219, 0.0054232998460007533, 0.005641697184990838,
                  0.0058532801279732871, 0.0060757844127261465, 0.0063365609784989552,
                  0.006587022381459454, 0.0068386196975272822, 0.0071160446125018472,
                  0.0073962666023466806, 0.0076683067760816501, 0.0079737228854276504,
                  0.0082634135825662806, 0.0085556257575542059, 0.0088584657638246869,
                  0.0091601703141331713, 0.0094733500518526109, 0.0097983865929968557,
                  0.01010745525673251, 0.010431878268358239, 0.010760933631508998,
                  0.01108229480294976, 0.011406132095524922, 0.011764424130191145,
                  0.012085507458974829, 0.01242913230492846, 0.012772948956576465,
                  0.013112594274109792, 0.013445656575060812, 0.01378640824830157,
                  0.014179748618504842, 0.014524468855731137, 0.014837067717344261,
                  0.015198005009872338, 0.015554082381810538, 0.015886081122916094,
                  0.01624516716243159, 0.01656915276248308, 0.016906720665017649,
                  0.017229976003919293, 0.017575054680507338, 0.017910950643762438,
                  0.01820892047464711, 0.018537501040326795, 0.018886196930150295,
                  0.019160712001214622, 0.019562906334057995, 0.019919424059868483,
                  0.0202332025818662, 0.020603789727188158, 0.020995732565035612,
                  0.021275592193502255, 0.021709848093393911, 0.022104923572868702,
                  0.022427716772188584, 0.022791105771053317, 0.023217797545887896,
                  0.023554197370525821, 0.023837707894543755, 0.024281298934845711,
                  0.024757280918840573, 0.025119051654789667, 0.025303540846777578,
                  0.025720732183297266, 0.026328472612552979, 0.02667374336275298,
                  0.026975736979349105, 0.027241340711120433, 0.027501260220704871,
                  0.027779626404565765, 0.0, 0.0, 0.0, 0.0]

        dq = mtd['EQSANS_4061_event_frame1_Iq'].readDx(0)
        diff = [math.fabs(dq_ref[i]-dq[i])<0.0001 for i in range(7,100)]
        output = reduce(lambda x,y:x and y, diff)

        if not output:
            for i in range(len(dq)):
                print i, dq[i], dq_ref[i], math.fabs(dq_ref[i]-dq[i])<0.0001
        return output
