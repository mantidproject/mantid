#pylint: disable=no-init,invalid-name
from mantid.api import *
from mantid.kernel import *
import sys
import numpy as np

class SANSWideAngleCorrection(PythonAlgorithm):
    """ Calculate the Wide Angle correction for SANS transmissions.
    """

    def category(self):
        return "CorrectionFunctions\\TransmissionCorrections"

    def name(self):
        return "SANSWideAngleCorrection"

    def summary(self):
        return "Calculate the Wide Angle correction for SANS transmissions."

    def PyInit(self):
        self.declareProperty(MatrixWorkspaceProperty("SampleData", "", direction = Direction.Input),\
    	"A workspace cropped to the detector to be reduced (the SAME as the input to [[Q1D]]); used to verify the solid angle. The workspace is not modified, just inspected.")
        self.declareProperty(MatrixWorkspaceProperty("TransmissionData","",direction=Direction.Input),\
                                      "The transmission data calculated, referred to as <math>T_0</math> in equations in discussion section")
        self.declareProperty(MatrixWorkspaceProperty("OutputWorkspace","",direction=Direction.Output),\
                                      "The transmission corrected SANS data, normalised (divided) by <math>T_0</math>, see discussion section")

    def PyExec(self):
        """ Main body of execution
        """
        # 1. get parameter values
        wd = self.getProperty("SampleData").value
        trans = self.getProperty("TransmissionData").value

        # check transmission input workspace
        if len(trans.dataX(0)) != len(wd.dataX(0)):
            raise RuntimeError("Uncompatible sizes. Transmission must have the same bins of sample values")
        if min(trans.dataY(0)) < 0:
            raise RuntimeError("Invalid workspace for transmission, it does not accept negative values.")

        #check input sample has associated instrument
        if not wd.getInstrument().getSample():
            raise RuntimeError("You can not apply this correction for workspace not associated to instrument")

        #initialization of progress bar:
        #
        endrange = 5+wd.getNumberHistograms()
        prog_reporter = Progress(self,start=0.0,end=1.0,nreports=endrange)

        # creating the workspace for the output
        prog_reporter.reportIncrement(5,"Preparing Wide Angle")
        trans_wc = WorkspaceFactory.create(wd)

        # get the values of transmission (data, error, binning)
        to = np.array(trans.dataY(0))
        to_e = np.array(trans.dataE(0))
        x_bins = np.array(wd.dataX(0))

        # get the position of the sample and the instrument
        sample_pos = wd.getInstrument().getSample().getPos()
        inst_pos = sample_pos - wd.getInstrument().getSource().getPos()

        # for each spectrum (i,j) calculate the correction factor for the transmission.
        for i in range(wd.getNumberHistograms()):
            try:
                prog_reporter.report("Correcting Wide Angle")
                # calculation of A
                twoTheta = wd.getDetector(i).getTwoTheta(sample_pos, inst_pos)
                A = 1/np.cos(twoTheta) - 1
                # calculation of factor for transmission
                to_l = (np.power(to,A)-1)/(np.log(to)*A)
                to_err_l = (np.power(to_e, A) - 1)/ (np.log(to_e) * A)
                # applying the data to the workspace
                trans_wc.setY(i,to_l)
                trans_wc.setE(i,to_err_l)
                trans_wc.setX(i,x_bins)
            except:
                self.getLogger().warning("WideAngleCorrection error: " + str(sys.exc_info()[2]))

        self.setProperty("OutputWorkspace", trans_wc)

#############################################################################################
AlgorithmFactory.subscribe(SANSWideAngleCorrection)
