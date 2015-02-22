#pylint: disable=no-init
import mantid.simpleapi as api
from mantid.api import *
from mantid.kernel import *

_OUTPUTLEVEL = "NOOUTPUT"

class ExaminePowderDiffProfile(PythonAlgorithm):
    """ Create the input TableWorkspaces for LeBail Fitting
    """
    def category(self):
        """
        """
        return "Diffraction;Utility"

    def name(self):
        """
        """
        return "ExaminePowderDiffProfile"

    def summmary(self):
        return "Examine peak profile parameters by Le Bail fit."

    def PyInit(self):
        """ Declare properties
        """
        # Data file
        self.declareProperty(MatrixWorkspaceProperty("InputWorkspace", "", Direction.Input, PropertyMode.Optional),\
                "Name of data workspace containing the diffraction pattern in .prf file. ")
        self.declareProperty(FileProperty("DataFilename","", FileAction.OptionalLoad, ['.dat']),\
                "Name of input data file.")
        self.declareProperty("LoadData", False, "Option to load data other than reading from an existing data workspace.")

        self.declareProperty("StartX", -0.0, "Minimum x value (TOF) to do the pattern calculation.")
        self.declareProperty("EndX", -0.0, "Maximum x value (TOF) to do the pattern calculation.")

        # Peak profile type
        profiletypes = ["Back-to-back exponential convoluted with PseudoVoigt", \
            "Thermal Neutron Back-to-back exponential convoluted with PseudoVoigt"]
        self.declareProperty("ProfileType", "Back-to-back exponential convoluted with PseudoVoigt",\
            StringListValidator(profiletypes), "Type of peak profile.")

        # Table workspaces
        self.declareProperty(ITableWorkspaceProperty("ProfileWorkspace", "", Direction.InOut),\
                "Name of table workspace containing peak parameters as input.")
        self.declareProperty(ITableWorkspaceProperty("BraggPeakWorkspace", "", Direction.InOut),\
                "Name of table workspace containing reflections (bragg peaks) in form of Miller index.")
        self.declareProperty(FileProperty("ProfileFilename","", FileAction.OptionalLoad, ['.irf']),\
                "Name of input data file.")
        self.declareProperty("Lattice", -0.0, "Lattice size of the cubic unit cell.")
        self.declareProperty("GenerateInformationWS", False, "Optional to genearte profile table workspace and Bragg peak table. ")

        # Background
        self.declareProperty(ITableWorkspaceProperty("BackgroundParameterWorkspace", "", Direction.InOut),\
                "Name of table workspace containing background parameters.")
        self.declareProperty("ProcessBackground", False, "Option to process background from input data file.")
        backgroundtypes = ["Polynomial", "Chebyshev", "FullprofPolynomial"]
        self.declareProperty("BackgroundType", "Polynomial", StringListValidator(backgroundtypes), "Type of background.")
        arrvalidator = FloatArrayBoundedValidator()
        arrvalidator.setLower(0.)
        self.declareProperty(FloatArrayProperty("BackgroundPoints", values=[], validator=arrvalidator, direction=Direction.Input),\
                "User specified X/TOF values of the data points to calculate background.")
        self.declareProperty(MatrixWorkspaceProperty("BackgroundWorkspace", "", Direction.Output, PropertyMode.Optional),\
                "Name of data workspace containing the background data. ")

        # Output
        self.declareProperty(MatrixWorkspaceProperty("OutputWorkspace", "", Direction.Output),\
                "Name of data workspace containing the diffraction pattern in .prf file. ")


        return


    def PyExec(self):
        """ Main Execution Body
        """
        # Process properties
        self.loaddata = self.getProperty("LoadData").value
        if self.loaddata is False:
            self.dataws = self.getProperty("InputWorkspace").value
            self.datawsname = str(self.dataws)
            self.datafilename = ""
        else:
            self.dataws = None
            self.datafilename = self.getProperty("DataFilename")

        profiletype = self.getProperty("ProfileType").value
        if profiletype == "Back-to-back exponential convoluted with PseudoVoigt":
            self.profiletype = "NeutronBk2BkExpConvPVoigt"
        else:
            self.profiletype = "ThermalNeutronBk2BkExpConvPVoigt"

        self.loadinfofile = self.getProperty("GenerateInformationWS").value
        if self.loadinfofile is True:
            self.irffilename = self.getProperty("ProfileFilename").value
            self.inputparamws = None
            self.inputbraggws = None
            self.latticesize = self.getProperty("Lattice").value
            if self.latticesize < 0.:
                self.latticesize = None
        else:
            self.irffilename = ""
            self.inputparamws = self.getProperty("ProfileWorkspace").value
            self.inputbraggws = self.getProperty("BraggPeakWorkspace").value

        self.process_bkgd = self.getProperty("ProcessBackground").value
        if self.process_bkgd is True:
            self.backgroundtype = self.getProperty("BackgroundType").value
            self.usrbkgdpoints = self.getProperty("BackgroundPoints").value
            self.bkgdwsname = self.getProperty("BackgroundWorkspace").value
        else:
            self.bkgdtablews = self.getProperty("BackgroundParameterWorkspace").value
            self.backgroundtype = self.getProperty("BackgroundType").value

        self.startx = self.getProperty("StartX").value
        self.endx = self.getProperty("EndX").value

        self.outwsname = self.getPropertyValue("OutputWorkspace")

        # Main execution
        self.mainExec()

        # 3. Export
        self.setProperty("BraggPeakWorkspace", str(self.inputbraggws))
        self.setProperty("ProfileWorkspace", str(self.inputparamws))
        self.setProperty("OutputWorkspace", self.outwsname)

        return


    def mainExec(self):
        """ Main execution body
        """
        # Load data optionally
        if self.loaddata is True:
            # Load data file
            api.LoadAscii(\
                    Filename        = self.datafilename,\
                    OutputWorkspace = self.datawsname,\
                    Unit            = 'TOF'\
                    )

        # Load .irf file and .hkl file optionally
        if self.loadinfofile is True:
            if dir(self).count('latticesize') == 0 or self.latticesize is None:
                raise NotImplementedError("Lattice size is not defined.  Unable to use option 'LoadInfo'")

            api.CreateLeBailFitInput(\
                    FullprofParameterFile   = self.irffilename,\
                    MaxHKL                  = [13, 13, 13],\
                    LatticeConstant         = float(self.latticesize),\
                    Bank                    = self.bankid,\
                    GenerateBraggReflections        =  True,\
                    InstrumentParameterWorkspace    =  str(self.inputparamws),\
                    BraggPeakParameterWorkspace     =  str(self.inputbraggws)\
                    )

        # Process background optionally
        if self.process_bkgd is True:
            # [Background]
            # Remove peaks and get pure background (hopefully)
            api.ProcessBackground(\
                    Options         =   'SelectBackgroundPoints',\
                    InputWorkspace  =   self.dataws,\
                    OutputWorkspace =   self.bkgdwsname,\
                    LowerBound      =   self.startx,\
                    UpperBound      =   self.endx,\
                    BackgroundType  =   self.backgroundtype,\
                    BackgroundPoints=   self.usrbkgdpoints,\
                    NoiseTolerance  =   '0.10000000000000001')

            # Fit background points
            functionstr = "name=%s,n=%d" % (self.backgroundtype, self.backgroundorder)
            for iborder in xrange(self.backgroundorder+1):
                functionstr = "%s,A%d=%.5f" % (functionstr, iborder, 0.0)
            api.Fit(\
                    Function        =   functionstr,\
                    InputWorkspace  =   self.bkgdwsname,\
                    Output          =   self.bkgdwsname,\
                    MaxIterations   =   '1000',\
                    Minimizer       =   'Levenberg-MarquardtMD',\
                    CreateOutput    =   '1',\
                    StartX          =   self.startx,\
                    EndX            =   self.endx)

        # [Le Bail calculation]
        self.log().debug("Fit range: %f , %f, Outputworkspace = %s" % (self.startx, self.endx, self.outwsname))
        api.LeBailFit(\
                Function                =   'Calculation',\
                InputWorkspace          =   self.dataws,\
                OutputWorkspace         =   self.outwsname,\
                InputParameterWorkspace =   self.inputparamws,\
                OutputParameterWorkspace=   str(self.inputparamws),\
                InputHKLWorkspace       =   self.inputbraggws,\
                OutputPeaksWorkspace    =   str(self.inputbraggws),\
                FitRegion               =   '%f, %f' % (self.startx, self.endx),\
                BackgroundType          =  self.backgroundtype,\
                UseInputPeakHeights     =   False,\
                PeakRadius              =   '7',\
                BackgroundParametersWorkspace   =   self.bkgdtablews,\
                PeakType                = self.profiletype,\
                )

        return


# Register algorithm with Mantid
AlgorithmFactory.subscribe(ExaminePowderDiffProfile)
