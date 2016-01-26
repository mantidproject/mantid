#pylint: disable=no-init,invalid-name,line-too-long

"""
This system test verifies that OFFSPEC data is processed correctly
by ReflectometryReductionAutoOne with PolarizationCorrection performed
as part of the workflow.
"""

import stresstesting
from mantid.simpleapi import *

class OFFSPECReflRedOneAutoPolarizationCorrection(stresstesting.MantidStressTest):
    def runTest(self):
        inputWorkspace = Load("OFFSPEC00033767.nxs")
        transmissionGroup = Load("OFFSPEC00033772.nxs")
        #set up our transmission workspace
        transWorkspace = CreateTransmissionWorkspaceAuto(transmissionGroup,
                                                         AnalysisMode="MultiDetectorAnalysis",
                                                         ProcessingInstructions="110-120",
                                                         WavelengthMin=2.0,
                                                         WavelengthMax=12.0)
        # set up our efficiency constants
        CRho=[1]
        CAlpha=[1]
        CAp=[1]
        CPp=[1]
        #run reflectometryReductionOneAuto
        __, _IvsLam_polCorr,__ = ReflectometryReductionOneAuto(inputWorkspace, AnalysisMode="MultiDetectorAnalysis",
                                                              ProcessingInstructions="110-120",
                                                              FirstTransmissionRun=transWorkspace,
                                                              ThetaIn="1.2",WavelengthMin=2.0,
                                                              WavelengthMax=12.0,CorrectionAlgorithm='None',
                                                              PolarizationAnalysis='PA',
                                                              CPp=CPp,CAp=CAp,CRho=CRho,CAlpha=CAlpha)
        return True

    def validate(self):
        return ("_IvsLam_polCorr", "OFFSPECReflRedOneAutoPolarizationCorrection_good_v2.nxs")

    def requiredFiles(self):
        return ["OFFSPEC00033767.nxs",
                "OFFSPEC00033772.nxs",
                "OFFSPECReflRedOneAutoPolarizationCorrection_good_v2.nxs"]
