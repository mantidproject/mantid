################################################################################
#
# Partial PDF Caclulator For NOMAD -- S(Q)
#
################################################################################
from MantidFramework import *
from mantidsimple import *

class NomadSoQCalculator(PythonAlgorithm):
    """ Class to calculate PDF for NOMAD
    The workflow and algorithm are rooted from Joerg
    
    It should inherit (PythonAlgorithm):
    """
    # def __init__(self):
    #     """ Init:  
    #     """
    #     # FIXME : Remove if it becomes a Mantid Python Algorithm
    #     return

    def category(self):
        """ Mantid required
        """
        return "Algorithm"

    def name(self):
        """ Mantid required
        """
        return "NomadSoQCalculator"

    def PyInit(self):
        """ Python init
        (Mantid required)
        """
        # self.declareListProperty("RunNumbers",[0], Validator=ArrayBoundedValidator(Lower=0))
        # self.declareWorkspaceProperty("OutputWorkspace", "", Direction=Direction.Output)
        # self.declareFileProperty("Directory", "", FileAction.OptionalDirectory)
        self.declareWorkspaceProperty("InputSampleWorkspace", "", Direction = Direction.Input, 
                Description = 'Sample Workspace.  Must been diffraction focused')
        self.declareWorkspaceProperty("InputBackgroundWorkspace", "", Direction = Direction.Input, 
                Description = 'Background Workspace.  Must been diffraction focused')
        self.declareWorkspaceProperty("InputVanadiumWorkspace", "", Direction = Direction.Input, 
                Description = 'Vanadium Workspace.  Must been diffraction focused')
        self.declareWorkspaceProperty("OutputSofQWorkspace", "", Direction = Direction.Output, Description = 'S(Q) before optimized')
        self.declareProperty("DeltaQ", 0.02,
                             Description="Delta Q for constant binning")
        self.declareProperty("RadiusVanadium", 0.627*0.5,
                             Description="Radius of Vanadium Rod")

        return

    def PyExec(self):
        """ generic stuff for running
        (Mantid required)
        """
        # 1. Get user input
        self.samwksp = self.getProperty("InputSampleWorkspace")
        self.vanwksp = self.getProperty("InputVanadiumWorkspace")
        self.bakwksp = self.getProperty("InputBackgroundWorkspace")
        self.deltaQ = self.getProperty("DeltaQ")
        self.r_vanadium = self.getProperty("RadiusVanadium")
        soqwsname = self.getPropertyValue("OutputSofQWorkspace")

        print "Input: %s, %s, %s" % (str(self.samwksp), str(self.vanwksp), str(self.bakwksp))
        print "dQ = %f, R(van) = %f" % (self.deltaQ, self.r_vanadium)
        print "Output: %s" % (soqwsname)

        # 1. Check validity and get some information
        self.processInputData()

        # 3. Rebin to constant bin
        self.rebinData()

        # 2. Process vanadium
        self.vanwksp = self.processVanadium(self.vanwksp)

        # 4. Sum spectra (blending)
        SumSpectra(InputWorkspace=self.samwksp, OutputWorkspace=self.samwksp)
        SumSpectra(InputWorkspace=self.vanwksp, OutputWorkspace=self.vanwksp)
        SumSpectra(InputWorkspace=self.bakwksp, OutputWorkspace=self.bakwksp)

        # 5. Calcualte S(Q)
        CloneWorkspace(InputWorkspace=self.samwksp, OutputWorkspace=soqwsname)
        self.soqwksp = mtd[soqwsname]
        self.calSoQ()

        # -1. Set output
        self.setProperty("OutputSofQWorkspace", self.soqwksp) 

        return


    def processInputData(self):
        """ Get some information of the workspace and do necessary unit conversion
        and rebin

        """
        import math

        # 1. Number of banks
        self.numBanks = self.samwksp.getNumberHistograms()
        numvanbanks = self.vanwksp.getNumberHistograms()
        numbakbanks = self.bakwksp.getNumberHistograms()

        if self.numBanks != numvanbanks or self.numBanks != numbakbanks:
            raise NotImplementedError("Input workspaces have different number of banks")

        workspaces = [self.samwksp, self.bakwksp, self.vanwksp]

        # 2. Check unit and convert
        for ws in workspaces:
            unit = ws.getAxis(0).getUnit().name()
            if unit != "MomentumTransfer":
                print "Workspace %s is of unit %s.  Transferring it to Q-space" % (str(ws), unit)
                ConvertUnits(InputWorkspace=str(ws), OutputWorkspace=str(ws), Target="MomentumTransfer",
                        EMode="Elastic")
            # ENDIF
        # ENDFOR

        return


    def rebinData(self):
        """ Rebin all workspace to uniform constant bin,
        """
        workspaces = [self.samwksp, self.bakwksp, self.vanwksp]

        # 1. Check data range
        qmins = []
        qmaxs = []
        for ws in workspaces:
            # For each workspace/data 
            for bid in xrange(0, self.numBanks):
                # For each bank
                qmin = ws.dataX(bid)[0]
                qmax = ws.dataX(bid)[-1]
                qmins.append(qmin)
                qmaxs.append(qmax)
            # ENDFOR
        # ENDFOR

        overall_qmin = min(qmins)
        overall_qmax = max(qmaxs)

        print "Data range: %f, %f" % (overall_qmin, overall_qmax)

        # 2. Determin rebin parameter
        qmin_rebin = 0.0+self.deltaQ*0.5
        qmax_rebin = int(overall_qmax*int(1/self.deltaQ))*1.0/int(1/self.deltaQ)+self.deltaQ*0.5
        # qmin_rebin = overall_qmin
        # qmax_rebin = overall_qmax
        rebinparam = "%f,%f,%f" % (qmin_rebin, self.deltaQ, qmax_rebin)
        print "Min(Q) = %f, Max(Q) = %f in %d Spectra;  Rebin = %s" % (qmin_rebin, qmax_rebin, len(qmins), rebinparam)
       
        # 3. Rebin
        for ws in workspaces:
            Rebin(InputWorkspace=ws, OutputWorkspace=ws, Params=rebinparam, PreserveEvents=False)

        # 4. Extended region

    
        return


    def calSoQ(self):
        """ Calculate S(Q)
        including
        (1) calcualte S-B
        (2) blending S-B
        (2) normalized S-B by vanadium (blended)
        """
        # 1. Sample - Background
        self.soqwksp -= self.bakwksp

        # 2.5 Divided by vanadium and scattering factor
        self.soqwksp /= self.vanwksp
    
        return 


    def processVanadium(self, vanws):
        """ Normalize vanadium including
        (1) peak removal
        (2) absorption correction
        """
        # 1. Peak strip
        ConvertUnits(InputWorkspace=vanws, OutputWorkspace=vanws, Target="dSpacing", Emode="Elastic")
        ConvertToMatrixWorkspace(InputWorkspace=vanws, OutputWorkspace=vanws)
        StripVanadiumPeaks(InputWorkspace=vanws, OutputWorkspace=vanws, PeakWidthPercent=5.0)
        ConvertUnits(InputWorkspace=vanws, OutputWorkspace=vanws, Target="MomentumTransfer", Emode="Elastic")

        # 2. Correction for vanadium
        MultipleScatteringCylinderAbsorption(InputWorkspace=vanws, OutputWorkspace=vanws,
                AttenuationXSection=2.8, ScatteringXSection=5.1, 
                SampleNumberDensity=0.0721, CylinderSampleRadius=self.r_vanadium)

        return vanws


""" Register
"""
mtd.registerPyAlgorithm(NomadSoQCalculator())

# If __name__=="__main__":
#     nomadpdfcal = NomadSoQCalculator()
#     nomadpdfcal.PyExec()
