# pylint: disable=no-init,invalid-name,attribute-defined-outside-init
"""
This system test verifies that OFFSPEC data is processed correctly by
ReflectometryReductionOneAuto
"""

import stresstesting
from mantid.simpleapi import *


class OFFSPECReflRedOneAuto(stresstesting.MantidStressTest):
    def runTest(self):
        offspec75 = Load("OFFSPEC00027575.raw") #th=0.35
        offspec76 = Load("OFFSPEC00027576.raw") #th=1.00
        offspec78 = Load("OFFSPEC00027578.raw") #th=1.70
        offspec85 = Load("OFFSPEC00027585.raw") #transmission run

        #Process using ReflectometryReductionOneAuto
        ivq_75, __, __ = ReflectometryReductionOneAuto(offspec75,
                                                       ThetaIn=0.70,#2*th
                                                       MomentumTransferStep=1e-3,
                                                       FirstTransmissionRun=offspec85,
                                                       Version=1)

        ivq_76, __, __ = ReflectometryReductionOneAuto(offspec76,
                                                       ThetaIn=2.00,#2*th
                                                       MomentumTransferStep=1e-3,
                                                       FirstTransmissionRun=offspec85,
                                                       Version=1)

        ivq_78, __, __ = ReflectometryReductionOneAuto(offspec78,
                                                       ThetaIn=3.40,#2*th
                                                       MomentumTransferStep=1e-3,
                                                       FirstTransmissionRun=offspec85,
                                                       Version=1)

        ivq_75_76, __ = Stitch1D(ivq_75, ivq_76, Params="1e-3")
        #pylint: disable=unused-variable
        ivq_75_76_78, __ = Stitch1D(ivq_75_76, ivq_78, Params="0,1e-3,0.25")
        return True

    def validate(self):
        '''
        we only wish to check the Q-range in this system test. It is not necessary
        to check the Instrument definition or Instrument Parameters
        '''
        self.disableChecking = ["Instrument"]
        return ("ivq_75_76_78","OFFSPECReflRedOneAuto_good_v3.nxs")

    def requiredFiles(self):
        return ["OFFSPEC00027575.raw",
                "OFFSPEC00027576.raw",
                "OFFSPEC00027578.raw",
                "OFFSPEC00027585.raw",
                "OFFSPECReflRedOneAuto_good_v3.nxs"]
