"""*WIKI* 


*WIKI*"""
################################################################################
#
#  Calculate g(r) and PDF for NOMAD
#
################################################################################
from MantidFramework import *
from mantidsimple import *

class PowgenPDFCalculator(PythonAlgorithm):
    """ Class to calculate PDF, g(r) for NOMAD data
    """
    def __init__(self):
        """ Intialization
        """
        PythonAlgorithm.__init__(self)

        return

    def category(self):
        """ Mantid required
        """
        return "Diffraction"

    def name(self):
        """ Mantid required
        """
        return "PowgenPDFCalculator"

    def PyInit(self):
        """ Python init
        (Mantid required)
        """
        self.declareWorkspaceProperty("InputSofQWorkspace", "", Direction=Direction.Input,
                Description="S(Q) Workspace")
        self.declareWorkspaceProperty("OutputPDFWorkspace", "", Direction=Direction.Output,
                Description="G(r), i.e., PDF Workspace")
        self.declareProperty("SampleDensity", 1.0,
                Description="Sample's density")
        self.declareProperty("SampleEffectiveDensity", 1.0,
                Description="Sample's effective density")
        self.declareProperty("Qmax", 25.0,
                Description="Max(Q) of S(Q) for Fourier Transform to G(r)/PDF")
        self.declareProperty("Qmin", 0.28,
                Description="Min(Q) of S(Q) for Fourier Transform to G(r)/PDF")
        self.declareProperty("Rmax", 20.0,
                Description="Max(r) for G(r)")
        self.declareProperty("DeltaR", 0.01,
                Description="Step of r for G(r)")

        return

    def PyExec(self):
        """ generic stuff for running
        (Mantid required)
        """
        # 1. Get user input
        soqws = self.getProperty("InputSofQWorkspace")

        # Sample crystal information
        rho_sample = self.getProperty("SampleDensity")
        effrho_sample = self.getProperty("SampleEffectiveDensity")

        # G(r) Calculation
        qmaxft = self.getProperty("Qmax")
        qminft = self.getProperty("Qmin")
        rmax = self.getProperty("Rmax")
        deltaR = self.getProperty("DeltaR")

        # Output
        pdfwsname = self.getPropertyValue("OutputPDFWorkspace")

        # 2. Adjust S(Q) by sample's effective density
        soqws *= (rho_sample*0.6)
        soqws /= effrho_sample
        
        # 6. Fourier transform
        gofrwsname  = "gofR_temp"
        self.fourierTransform(soqws, gofrwsname, pdfwsname, rmax, deltaR, qminft, qmaxft)
        pdfws = mtd[pdfwsname]

        # 7. Set output 
        self.setProperty("OutputPDFWorkspace", pdfws)

        return


    def fourierTransform(self, soqws, gorwsname, pdfwsname, rmax, dr, qmin, qmax):
        """ Calculate g(r), G(r), and etc. 
        """
        import math

        print "POWGEN PDF Fourier Transform: Qmax = %f, Rmax = %f, dR = %f" % (qmax, rmax, dr)

        # 1. Call C++ for PDF, i.e., G(r)
        PDFFourierTransform(InputWorkspace=soqws, OutputPDFWorkspace=pdfwsname,
                InputSofQType="S(Q)", RMax=rmax, DeltaR=dr,
                Qmin=qmin, Qmax=qmax,
                PDFType="G(r)")

        return

mtd.registerPyAlgorithm(PowgenPDFCalculator())
