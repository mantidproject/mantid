"""*WIKI* 

Algorithm to calculate the solid angle correction transmission.
.

*WIKI*"""


from MantidFramework import *
from mantidsimple import *
import os
import numpy as np

class SANSSolidAngleCorrection(PythonAlgorithm):
    """ Calculate the Solid Angle correction for SANS transmissions.
    """
   
    def category(self):
        return "CorrectionFunctions\\TransmissionCorrections"

    def name(self):
        return "SANSSolidAngleCorrection"

    def PyInit(self):
        self.declareWorkspaceProperty("SampleData","", Direction=Direction.Input, Type=Workspace,
                Optional=False,
                Description=
                    "The Detector Bank data, used to verify the solid angle.")
        self.declareWorkspaceProperty("TransmissionData","",Direction=Direction.Input,
                                      Type=Workspace, Optional = False, 
                                      Description = "The transmission data calculated")
        self.declareWorkspaceProperty("OutputWorkspace","",Direction=Direction.Output,
                                      Type = Workspace, Optional = False,
                                      Description = "The transmission corrected")
      
    def PyExec(self):
        """ Main body of execution
        """
        # 1. get parameter values
        wsSample = self.getPropertyValue("SampleData")
        wsTrans = self.getPropertyValue("TransmissionData")
        wsOutputName = self.getPropertyValue("OutputWorkspace")
        
        CloneWorkspace(InputWorkspace=wsSample, OutputWorkspace=wsOutputName)
        trans_wc = mtd[wsOutputName]

        trans = mtd[wsTrans]
        to = np.array(trans.dataY(0))
        to_e = np.array(trans.dataE(0))
        
        wd = mtd[wsSample]
        sample_pos = wd.getInstrument().getSample().getPos()
        inst_pos = sample_pos - wd.getInstrument().getSource().getPos()
        for i in range(wd.getNumberHistograms()):
            try:
                twoTheta = wd.getDetector(i).getTwoTheta(sample_pos, inst_pos)
                A = 1/np.cos(twoTheta) - 1
                to_l = (np.power(to,A)-1)/(np.log(to)*A)
                to_err_l = (np.power(to_e, A) - 1)/ (np.log(to_e) * A)
                for l in range(len(trans_wc.dataY(i))):
                    trans_wc.dataY(i)[l] = to_l[l]
                    trans_wc.dataE(i)[l] = to_err_l[l]
            except:
                mantid.sendWarningMessage("SolidAngleCorrection error: " + str(sys.exc_info()))

        self.setProperty("OutputWorkspace",trans_wc)

        return
        
        
mtd.registerPyAlgorithm(SANSSolidAngleCorrection())
