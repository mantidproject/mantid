"""*WIKI* 


*WIKI*"""
################################################################################
#
#  Calculate g(r) and PDF for NOMAD
#
################################################################################
from MantidFramework import *
from mantidsimple import *

class NomadPDFCalculator(PythonAlgorithm):
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
        return "Algorithm"

    def name(self):
        """ Mantid required
        """
        return "NomadPDFCalculator"

    def PyInit(self):
        """ Python init
        (Mantid required)
        """
        self.declareWorkspaceProperty("InputSofQWorkspace", "", Direction=Direction.Input,
                Description="S(Q) Workspace")
        self.declareWorkspaceProperty("OutputPDFWorkspace", "", Direction=Direction.Output,
                Description="G(r), i.e., PDF Workspace")
        self.declareWorkspaceProperty("OutputGofRWorkspace", "", Direction=Direction.Output,
                Description="g(r) Workspace")
        self.declareProperty("FitQmin", 25.0, 
                Description="Min(Q) to fit S(Q) = a + b Q^2 + c Q^4")
        self.declareProperty("FitQmax", 25.0, 
                Description="Max(Q) to fit S(Q) = a + b Q^2 + c Q^4")
        self.declareProperty("SampleSigma", 1.0, 
                Description="Sample's scattering cross-section")
        self.declareProperty("SampleSb2", 1.0,
                Description="Square of sample's summed scattering length, i.e., (sum_i(b_i))^2")
        self.declareProperty("SampleSbs", 1.0,
                Description="Summation of square of sample's scattering length, i.e., sum_i b_i^2")
        self.declareProperty("SampleDensity", 1.0,
                Description="Sample's (number) density")
        self.declareProperty("SampleRadius", 1.0,
                Description="Sample's radius")
        self.declareProperty("SampleHeight", 1.0,
                Description="Sample's height")
        self.declareProperty("VanadiumRadius", 1.0,
                Description="Sample's radius")
        self.declareProperty("VanadiumHeight", 1.0,
                Description="Vanadium rod's height")
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

        # S(Q) optimization parameters
        qminfit = self.getProperty("FitQmin")
        qmaxfit = self.getProperty("FitQmax")

        # Sample crystal information
        self.sigma_sample = self.getProperty("SampleSigma")
        self.rho_sample = self.getProperty("SampleDensity")
        self.sbs = self.getProperty("SampleSbs")
        self.sb2 = self.getProperty("SampleSb2")

        # Sample/Vanadium experimental data
        r_sample = self.getProperty("SampleRadius")
        h_sample = self.getProperty("SampleHeight")
        r_vanadium = self.getProperty("VanadiumRadius")
        h_vanadium = self.getProperty("VanadiumHeight")

        # Constants of Vanadium
        sigma_vanadium = 5.08
        rho_vanadium = 0.0722

        # G(r) Calculation
        qmaxft = self.getProperty("Qmax")
        qminft = self.getProperty("Qmin")
        rmax = self.getProperty("Rmax")
        deltaR = self.getProperty("DeltaR")

        # Output
        pdfwsname = self.getPropertyValue("OutputPDFWorkspace")
        gofrwsname = self.getPropertyValue("OutputGofRWorkspace")

        # Others
        isHydrogen = False

        # 2. Fit for high Q: a + b x^2 + c x^4
        Fit(InputWorkspace=soqws, StartX=qminfit, EndX=qmaxfit, 
                Function="name=UserFunction, Formula=a+b*x*x+c*x*x*x*x, a=0, b=1, c=1",
                Output="FitResult", Minimizer="Levenberg-Marquardt", CostFunction="Least squares")
        
        #  parws is TableWorkspace: parws.getColumnCount(), parws.getColumnNames()
        parws = mtd["FitResult_Parameters"]
        if parws is None:
            raise NotImplementedError("Fit result Workspace cannot be located")
        a = parws.getDouble("Value", 0)
        b = parws.getDouble("Value", 1)
        c = parws.getDouble("Value", 2)

        # DeleteWorkspace(Workspace=fitwsname)

        ConvertToPointData(InputWorkspace=soqws, OutputWorkspace=soqws)

        # 3. Shift S(Q) by the fit function
        self.shiftSofQ(soqws, a, b, c)

        # 4. Multiplied by factor: sigma = scattering Xsection, rho = density
        factor = (self.sigma_sample*self.rho_sample*r_sample**2)/(sigma_vanadium*rho_vanadium*r_vanadium**2)
        print "S(Q) scattering factor = %f" % (factor)
        soqws *= 1.0/factor

        # 5. Optional hydrogen
        if isHydrogen:
            raise NotImplementedError("Hydrogen case is not implemetned")

        # 6. Fourier transform
        self.fourierTransform(soqws, gofrwsname, pdfwsname, rmax, deltaR, qminft, qmaxft)
        gofrws = mtd[gofrwsname]
        pdfws = mtd[pdfwsname]

        # 7. Set output 
        self.setProperty("OutputPDFWorkspace", pdfws)
        self.setProperty("OutputGofRWorkspace", gofrws)

        return

    def shiftSofQ(self, soqws, a, b, c):
        """ Shift S(Q) by a + b*Q**2 + c*Q**4
        """
        dataq = soqws.dataX(0)
        datas = soqws.dataY(0)

        for iq in xrange(len(dataq)):
            q = dataq[iq]
            q2 = q*q
            q4 = q2*q2
            shift = a+b*q2+c*q4
            datas[iq] -= shift
        # ENDFOR

        return


    def fourierTransform(self, soqws, gorwsname, pdfwsname, rmax, dr, qmin, qmax):
        """ Calculate g(r), G(r), and etc. 
        """
        import math

        print "NOMAD PDF Fourier Transform: Qmax = %f, Rmax = %f, dR = %f" % (qmax, rmax, dr)

        # 0. 
        sbs = self.sbs
        sb2 = self.sb2

        soqws += 1

        # 1. Call C++ for PDF, i.e., G(r)
        PDFFourierTransform(InputWorkspace=soqws, OutputGorRWorkspace=pdfwsname,
                OutputQSQm1Workspace="QSQm1", RMax=rmax, DeltaR=dr,
                Qmin=qmin, Qmax=qmax)
        pdfws = mtd[pdfwsname]

        # 2. Calculate g(r)-1
        CloneWorkspace(InputWorkspace=pdfwsname, OutputWorkspace=gorwsname)
        gorws = mtd[gorwsname]

        # 2. Post process
        factorft = 4.0*math.pi*self.rho_sample*self.sbs/self.sb2
        print "PDF to g(r) factor = %f"  % (factorft)

        gorws *= 1/factorft

        return

mtd.registerPyAlgorithm(NomadPDFCalculator())
