"""*WIKI* 

This algorithm is to import Fullprof .irf file (peak parameters) and .hkl file (reflections) and 
record the information to TableWorkspaces, which serve as the inputs for algorithm LeBailFit. 

==== Format of Instrument parameter TableWorkspace ====
Instrument parameter TableWorkspace contains all the peak profile parameters imported from Fullprof .irf file.  

Presently these are the peak profiles supported
 * Thermal neutron back to back exponential convoluted with pseudo-voigt (profile No. 10 in Fullprof)

Each row in TableWorkspace corresponds to one profile parameter.  

Columns include Name, Value, FitOrTie, Min, Max and StepSize.


==== Format of reflection TableWorkspace ====
Each row of this workspace corresponds to one diffraction peak.  
The information contains the peak's Miller index and (local) peak profile parameters of this peak.  
For instance of a back-to-back exponential convoluted with Gaussian peak, 
the peak profile parameters include Alpha, Beta, Sigma, centre and height. 

== How to use algorithm with other algorithms ==
This algorithm is designed to work with other algorithms to do Le Bail fit.  The introduction can be found in the wiki page of [[Le Bail Fit]].

*WIKI*"""
from mantid.api import PythonAlgorithm, AlgorithmFactory, ITableWorkspaceProperty, WorkspaceFactory, FileProperty, FileAction
from mantid.kernel import Direction, StringListValidator, FloatBoundedValidator

import mantid.simpleapi as api

_OUTPUTLEVEL = "NOOUTPUT"

class CreateLeBailFitInput(PythonAlgorithm):
    """ Create the input TableWorkspaces for LeBail Fitting
    """
    def category(self):
        """
        """
        return "Diffraction;Utility"

    def name(self):
        """
        """
        return "CreateLeBailFitInput"
 
    def PyInit(self):
        """ Declare properties
        """
        instruments=["POWGEN", "NOMAD", "VULCAN"]
        self.setWikiSummary("""Create various input Workspaces required by algorithm LeBailFit.""")
        
        self.declareProperty("Instrument", "POWGEN", StringListValidator(instruments), "Powder diffractometer's name")

        self.declareProperty(FileProperty("ReflectionsFile","", FileAction.Load, ['.hkl']),
                "Name of [http://www.ill.eu/sites/fullprof/ Fullprof] .hkl file that contains the peaks.")

        self.declareProperty(FileProperty("FullprofParameterFile", "", FileAction.Load, ['.irf']),
                "Fullprof's .irf file containing the peak parameters.")

        self.declareProperty("Bank", 1, "Bank ID for output if there are more than one bank in .irf file.")

        self.declareProperty("LatticeConstant", -0.0, validator=FloatBoundedValidator(lower=1.0E-9), doc="Lattice constant for cubic crystal.")

        self.declareProperty(ITableWorkspaceProperty("InstrumentParameterWorkspace", "", Direction.Output), 
                "Name of Table Workspace Containing Peak Parameters From .irf File.")
 
        self.declareProperty(ITableWorkspaceProperty("BraggPeakParameterWorkspace", "", Direction.Output), 
                "Name of Table Workspace Containing Peaks' Miller Indices From .prf File.")

        return
 
    def PyExec(self):
        """ Main Execution Body
        """
        # 1. Peak profile parameter workspace
        irffilename = self.getPropertyValue("FullprofParameterFile")
        paramWS = self.createPeakParameterWorkspace(irffilename)
        self.setProperty("InstrumentParameterWorkspace", paramWS)

        hklWS = WorkspaceFactory.createTable()
        self.setProperty("BraggPeakParameterWorkspace", hklWS)

        # 2. Get Other Properties
        instrument = self.getProperty("Instrument")
        reflectionfilename = self.getPropertyValue("ReflectionsFile")

        # 3. Import reflections list
        hklws = self.importFullProfHKLFile(reflectionfilename)
        self.setProperty("BraggPeakParameterWorkspace", hklws)

        return

    def importFullProfHKLFile(self, hklfilename):
        """ Import Fullprof's .hkl file
        """
        import math
        import random

        rand = random.randint(1, 100000)
        dummywsname = "Foo%d" % (rand)
        hklwsname = self.getPropertyValue("BraggPeakParameterWorkspace")
        print hklwsname
        tempX = api.LoadFullprofFile(
                Filename=hklfilename, 
                # PeakParameterWorkspace = hklwsname,
                PeakParameterWorkspace = "TempXXX",
                OutputWorkspace = dummywsname)
    
        hklws = tempX[0]
        dummyws = tempX[1]

        api.DeleteWorkspace(Workspace=dummyws)

        return hklws


    def createPeakParameterWorkspace(self, irffilename):
        """ Create TableWorkspace by importing Fullprof .irf file

        Note:
        1. Sig-0, Sig-1 and Sig-2 in .irf file are actually the square of sig0, sig1 and sig2
           defined in the manual
    
        Input:
         - irffilename:  Resolution file (.irf)  Can be single bank or multiple bank
    
        Output:
         - tableworkspace
        """
        # 1. Import
        irfwsname = irffilename.split("/")[-1]
        irfws = api.LoadFullprofResolution(Filename=irffilename, OutputWorkspace=irfwsname)

        # 2. Create an empty workspace
        tablews = WorkspaceFactory.createTable()
        
        tablews.addColumn("str",    "Name")
        tablews.addColumn("double", "Value")
        tablews.addColumn("str",    "FitOrTie")
        tablews.addColumn("double", "Min")
        tablews.addColumn("double", "Max")
        tablews.addColumn("double", "StepSize")

        numrows = irfws.rowCount()
        for ir in xrange(numrows):
            tablews.addRow(["Parameter", 0.0, "tie", -1.0E200, 1.0E200, 1.0]) 

        # 3. Copy between 2 workspace
        for ir in xrange(numrows):
            tablews.setCell(ir, 0, irfws.cell(ir, 0))
            tablews.setCell(ir, 1, irfws.cell(ir, 1))

        # 4. Extra Lattice parameter
        latticepar = float(self.getPropertyValue("LatticeConstant"))
        tablews.addRow(["LatticeConstant", latticepar,  "tie", -1.0E200, 1.0E200, 1.0]) 

        # 5. Clean
        api.DeleteWorkspace(Workspace=irfwsname)
        
        return tablews

# Register algorithm with Mantid
AlgorithmFactory.subscribe(CreateLeBailFitInput)

