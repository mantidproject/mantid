import stresstesting
import math
import mantid
from mantid.simpleapi import *
from reduction_workflow.instruments.sans.sns_command_interface import *
from mantid.api import *

import os

def do_cleanup():
    Files = ["EQSANS_4061_event_reduction.log",
    "EQSANS_1466_event_reduction.log"]
    for file in Files:
        absfile = FileFinder.getFullPath(file)
        if os.path.exists(absfile):
            os.remove(absfile)
    return True

class EQSANSIQOutput(stresstesting.MantidStressTest):

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

    def cleanup(self):
        do_cleanup()
        return True
    """
        Analysis Tests for EQSANS
        Testing that the I(Q) output of is correct 
    """
    
    def runTest(self):
        config = ConfigService.Instance()
        config["facilityName"]='SNS'
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

    def cleanup(self):
        do_cleanup()
        return True
    """
        Analysis Tests for EQSANS
        Testing that the Q resolution output of is correct 
    """
    
    def runTest(self):
        """
            Check that the Q resolution calculation returns positive values
            even when background is larger than signal and I(q) is negative.
            (Non-physical value that's an experimental edge case)
        """
        config = ConfigService.Instance()
        config["facilityName"]='SNS'
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

    def cleanup(self):
        do_cleanup()
        return True
    """
        Analysis Tests for EQSANS
        Testing that the Q resolution output of is correct 
    """
    
    def runTest(self):
        """
            Check that the Q resolution calculation returns positive values
            even when background is larger than signal and I(q) is negative.
            (Non-physical value that's an experimental edge case)
        """
        config = ConfigService.Instance()
        config["facilityName"]='SNS'
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
        dq_ref = [0.00178823,0.0014458,0.00144805,0.00155836,0.00150908,
                  0.00163262,0.00158216,0.00160879,0.00165932,0.00164304,
                  0.00165549,0.00163676,0.00167581,0.0016957,0.00167898,
                  0.00172297,0.00169375,0.00174938,0.00173394,0.00180498,
                  0.00188825,0.00184747,0.00181396,0.00185052,0.00191187,
                  0.00192331,0.00196536,0.00196182,0.00202844,0.00205516,
                  0.00208013,0.00210195,0.00212621,0.00217228,0.00217713,
                  0.002243,0.00225329,0.00229956,0.00234733,0.00234773,
                  0.00239551,0.00243152,0.0024392,0.00248026,0.00249286,
                  0.00252012,0.00253674,0.00257043,0.00257755,0.00261695,
                  0.00263961,0.00268499,0.0026836,0.00273043,0.00272828,
                  0.00279073,0.00279924,0.00284322,0.00283794,0.00288332,
                  0.00289423,0.00291934,0.00294244,0.00295239,0.00297587,
                  0.00300671,0.00299071,0.00307836,0.00304013,0.00307726,
                  0.00312929,0.00314636,0.00315895,0.00312642,0.00322729,
                  0.00325368,0.00326916,0.00328936,0.00331894,0.00328319,
                  0.00337098,0.00335638,0.00335586,0.00340926,0.00343972,
                  0.00349148,0.003528,0.00352863,0.0035665,0.0036791,
                  0.00360243,0.00364245,0.003671,0,0,0,0.00375495,0,0,0,0]
        dq = mtd['EQSANS_1466_event_Iq'].readDx(0)
        diff = [math.fabs(dq_ref[i]-dq[i])<0.0001 for i in range(7,100)]
        output = reduce(lambda x,y:x and y, diff)
        if not output:
            for i in range(len(dq)):
                print i, dq[i], dq_ref[i], math.fabs(dq_ref[i]-dq[i])<0.0001
        return output
    
class EQSANSDQOutput_FS(stresstesting.MantidStressTest):

    def cleanup(self):
        do_cleanup()
        return True
    """
        Analysis Tests for EQSANS
        Testing that the Q resolution output of is correct 
    """
    
    def runTest(self):
        """
            Check that the Q resolution calculation returns positive values
            even when background is larger than signal and I(q) is negative.
            (Non-physical value that's an experimental edge case)
        """
        config = ConfigService.Instance()
        config["facilityName"]='SNS'
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
        dq_ref = [0.00255107356133, 0.00215833578128, 0.00208718785908, 
                  0.00258510271064, 0.00293816108702, 0.00247205866985,
                  0.00243935430286, 0.00239444669495, 0.00222146661565, 
                  0.00218605712485, 0.00219528175558, 0.0022064529384, 
                  0.00222261319274, 0.00224172877526, 0.00225796674563, 
                  0.00228220728003, 0.00230427122347, 0.00232713464119, 
                  0.00235408216185, 0.00238474827119, 0.00240595507163, 
                  0.00243366105712, 0.00246093985138, 0.00248828126962, 
                  0.00251992966389, 0.00255373215231, 0.00259127844171, 
                  0.00263727405994, 0.00268617120932, 0.00273367187508, 
                  0.00277746568962, 0.00282377112768, 0.00287707862012, 
                  0.00292488071673, 0.00297083402995, 0.00302034443396, 
                  0.00306791149356, 0.00311128530472, 0.00315886049123, 
                  0.0032012867282, 0.00324181579199, 0.00328255488894, 
                  0.00332106647848, 0.00336006110389, 0.00339953376057, 
                  0.00343507183824, 0.00347168225631, 0.00350947714109, 
                  0.00354374653283, 0.00357867641742, 0.00361759403268, 
                  0.00365056833748, 0.00368612178547, 0.00372126622111, 
                  0.00375568496126, 0.00378827338665, 0.00382102059653, 
                  0.00386208119997, 0.00389527759712, 0.00392382196507, 
                  0.00395898855656, 0.00399254216973, 0.00402263239642, 
                  0.00405571908096, 0.0040850426166, 0.004115066991, 
                  0.00414251925121, 0.00417373849783, 0.00420187672507, 
                  0.00422580041865, 0.00425450461041, 0.00428409252891, 
                  0.0043057691751, 0.00434121835718, 0.00437168838538, 
                  0.00439831287327, 0.00443009051949, 0.00446383617502, 
                  0.00448646538796, 0.00452524116438, 0.00455891945975, 
                  0.00458584606578, 0.00461675547089, 0.00465411973842, 
                  0.00468084439834, 0.00470294856029, 0.0047424262336, 
                  0.00478414058644, 0.00481411031777, 0.00482401661572, 
                  0.00486137558128, 0.0049171158478, 0.00494417232844, 
                  0.00496567444129, 0.0049866092171, 0.00500861857974, 
                  0.00503217184255, 0.0, 0.0, 0.0, 0.0]
        


        dq = mtd['EQSANS_4061_event_frame1_Iq'].readDx(0)
        diff = [math.fabs(dq_ref[i]-dq[i])<0.0001 for i in range(7,100)]
        output = reduce(lambda x,y:x and y, diff)
        
        if not output:
            for i in range(len(dq)):
                print i, dq[i], dq_ref[i], math.fabs(dq_ref[i]-dq[i])<0.0001
        return output