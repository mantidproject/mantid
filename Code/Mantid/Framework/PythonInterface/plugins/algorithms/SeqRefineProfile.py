######################################################################
#
# This is a partial copy from LeBailFitScript.py
#
# Python Script as Step 1 of Le Bail Fitting to
# 1. Load file
# 2. Create LeBailFitInput
# 3. Fit Peaks
#
# Step 1:   Load data, model (HKL list) and starting instrument parameters values,
#           and do an initial LeBailFit/calculation to see how much the starting
#           values are off;
#
######################################################################

import mantid.simpleapi as api
from mantid.api import *
from mantid.kernel import *


#from Calibration_ImportInformation import *


#--------------------------------------------------------------------------------
class RefinePowderDiffProfileSeq(PythonAlgorithm):
    """ Refine powder diffractometer profile by Le Bail algorithm sequentially
    """
    def category(self):
	""" Category
	"""
	return "Diffraction"

    def name(self):
	""" Algorithm name
	"""
	return "RefinePowderDiffProfileSeq"

    def PyInit(self):
	""" Declare properties
	"""
	self.setWikiSummary("""Refine powder diffractomer profile parameters sequentially.""")

	self.declareProperty(ITableWorkspaceProperty("SeqControlInfoWorkspace", "", Direction.InOut, PropertyMode.Optional),
	    "Name of table workspace containing sequential refinement information.")

	self.declareProperty(ITableWorkspaceProperty("InputProfileWorkspace", "", Direction.Input, PropertyMode.Optional),
	    "Name of table workspace containing starting profile parameters.")

	self.declareProperty(ITableWorkspaceProperty("InputBraggPeaksWorkspace", "", Direction.Input, PropertyMode.Optional),
	    "Name of table workspace containing a list of reflections. ")

	self.declareProperty(ITableWorkspaceProperty("InputBackgroundParameterWorkspace", "", Direction.Input, 
	    PropertyMode.Optional), "Name of table workspace containing a list of reflections. ")

	self.declareProperty("StartX", -0., "Start X (TOF) to refine diffraction pattern.")
	self.declareProperty("EndX",   -0., "End X (TOF) to refine diffraction pattern.")

	funcoptions = ["Setup", "Refine"]
	self.declareProperty("FunctionOption", "Refine-Levenberg", StringListValidator(funcoptions), "Options of functionality")

	refoptions = ["Levenberg-Marquardt", "Random Walk", "Single Peak Fit"]
	self.declareProperty("RefinementOption", "Refine-Levenberg", StringListValidator(refoptions),
	    "Options of algorithm to refine. ")

	self.declareProperty(StringArrayProperty("Parameters2Refine", values=[], direction=Direction.Input),
	    "List of parameters to refine.")

	self.declareProperty("NumRefineCycles", 1, "Number of refinement cycles.")

	bkgdtypes = ["Polynomial", "Chebyshev", "FullprofPolynomial"]
	self.declareProperty("BackgroundType", "Polynomial", StringListValidator(bkgdtypes),
	    "Type of background function.")

	return


    def PyExec(self): 
	""" Main 
	"""
	# Process input
	self._processInputProperties()

	# Instantiaze sequential refinement 
	seqrefine = SeqRefineProfile("IDx890")

	# Execute 
	if self.functionoption == "Setup": 
	    # Set up 
	    if seqrefine.isSetup() is True:
		raise NotImplementedError("Impossible to have it set up already.")

	    seqrefine.initSetup(self.profilews, self.braggpeakws, self.bkgdtype, self.bkgdparws, 
		self.startx, self.endx)

	elif self.functionoption == "Refine": 
	    # Refine 
	    if seqrefine.isSetup() is False:
		raise NotImplementedError("Exception because sequential refinement is not set up.")
	    seqrefine.refine(self.paramstofit,  self.numcycles, self.startx, self.endx)

	else:
	    # None-support
	    raise NotImplementedError("Function is not supported.")

	return


    def _processInputProperties(self):
	""" Process input properties
	"""
	self.startx = self.getProperty("StartX").value
	self.endx = self.getProperty("EndX").value

	self.functionoption = self.getProperty("FunctionOption").value
	if self.functionoption == "Setup":
	    self.bkgdtype = self.getProperty("BackgroundType").value
	    self.bkgdparws = self.getProperty("InputBackgroundParameterWorkspace").value
	    self.profilews = self.getProperty("InputProfileWorkspace").value
	    self.braggpeakws = self.getProperty("InputBraggPeaksWorkspace").value

	elif self.functionoptions == "Refine":
	    self.paramstofit = self.getProperty("Parameters2Refine").value
	    self.numcycles = self.Property("NumRefineCycle").value

	return


#--------------------------------------------------------------------
class SeqRefineProfile:
    """ A class to do sequential refinement on peak profile

    Use case:
    1. Set class object such as : ID/StartDate/Directory
    2. Run main to refine some parameters
    3. Check result
    4. If no further instruction, only need to set up parameters to refine
       the input/starting values should be from the last
    """
    def __init__(self, ID):
	""" 
	"""
	self._ID = str(ID)
	
	self._recordwsname = "Record%sTable" % (str(ID))

	return

    def initSetup(self, profilews, braggpeakws, bkgdtype, bkgdparws, startx, endx):
	""" Set up the properties for LeBailFit as the first time
	including profilews, braggpeakws, and etc
	"""
	# Profile 
	self._profileWS = profilews
	self._braggpeakws = braggpeakws
	self._bkgdtype = bkgdtype
	self._bkgdparws = bkgdparws

	# Generate record table
	self._genRecordTable()

	# Check input parameters, i.e., verification/examine input parameters
	runner = RefineProfileParameters()

        runner.startx =  startx
        runner.endx   =  endx

	runner.inprofilewsname = self._profileWS
	runner.inreflectionwsname = self._braggpeakws

	runner.bkgdtype = self._bkgdtype
	runner.bkgdtablewsname = self._bkgdparws

	runner.outprofilewsname = self._profileWS
	runner.outreflectionwsname = self._braggpeakws

	self._recordPreRefineInfo(runner)
	runner.calculate()
	self._recordPostRefineInfo(runner)

	return

    def refine(self, parametersToFit, numcycles, startx, endx):
	""" Refine parameters
	"""
	runner = RefineProfileParameters(self._globalFilename)

	# Locate refinement record table
	profilewsname, braggpeakwsname, bkgdtype, bkgdparamwsname = self._parseRecordTable()

	# Set up runner and refine
	runner.numsteps = numcycles
	runner.paramToFit = parametersToFit

        runner.startx =  startx
        runner.endx   =  endx

	runner.inprofilewsname = profilewsname
	runner.inreflectionwsname = braggpeakwsname

	runner.bkgdtype = bkgdtype
	runner.bkgdtablewsname = bkgdparamwsname
	
	# Refine
	self._recordPreRefineInfo(runner)
	
	outwsname, outprofilewsname, outbraggpeakwsname = self._genOutputWorkspace(runner.datawsname, profilewsname, braggpeakwsname)
        runner.outwsname = outwsname
        runner.outprofilewsname = outprofilewsname
	runner.outreflectionwsname = outbraggpeakwsname

	runner.refine()
	self._recordPostRefineInfo(runner)

	return

    def _genRecordTable(self):
	""" Generate record table
	"""
	tablews = CreateEmptyTableWorkspace(OutputWorkspace=self._recordwsname)

	tablews.addColumn("int", "Step")
	tablews.addColumn("str", "OutProfile")
	tablews.addColumn("str", "OutReflection")
	tablews.addColumn("str", "OutBackgroud")
	tablews.addColumn("str", "OutBckgroundParam")
	tablews.addColumn("str", "Refine")
	tablews.addColumn("double", "RwpOut")
	tablews.addColumn("int", "LastStep")
	tablews.addColumn("double", "RwpIn")
	tablews.addColumn("str", "InProfile")
	tablews.addColumn("str", "InReflection")
	tablews.addColumn("str", "InBackgroud")
	tablews.addColumn("str", "InBckgroundParam")

	return

    def _parseRecordTable(self):
	""" Parse record table and return the last refinement result
        Notice that 'last row' in record table might not be a valid row (incomplete). 
        It might be caused by an exception raised in refinement or its setup. 
        Class variable _lastrowinvalid is used to indicate this
	"""
	rectablews = mtd[self._recordwsname]
	numrows = rectablews.rowCount()
	if numrows == 0:
	    raise NotImplementedError("Empty record table workspace. ")

	lastrow = numrows-1

        while lastrow >= 0:
	    profilewsname = rectablews.cell(lastrow, 1)
	    reflectwsname = rectablews.cell(lastrow, 2)
	    bkgdtype = rectablews.cell(lastrow, 3)
	    bkgdparamwsname = rectablews.cell(lastrow, 4)
	    
	    if profilewsname == "": 
                print "Profile workspace name is emtpy in row %d!" % (lastrow)
                lastrow -= 1
            else:
                break
        # ENDWHILE

        if profilewsname == "":
            raise NotImplementedError("Profile workspace name is empty!")

        self._lastrowinvalid = False
        if lastrow != numrows-1:
            self._lastrowinvalid = True

	return (profilewsname, reflectwsname, bkgdtype, bkgdparamwsname)


    def _recordPreRefineInfo(self, refiner):
        """ Record pre-refinement information
        """
	rectablews = mtd[self._recordwsname]
        numrows = rectablews.rowCount()

        if self._lastrowinvalid is False:
            self._currstep = numrows
            laststep = self._currstep-1
            rectablews.addRow([self._currstep, "", "", "", "", "", -1.0, laststep, -1.0, "profilews", "reflectionws", "Polynomial", "BkgdParm"])
        else:
            self._currstep = numrows-1
            laststep = self._currstep-1

        if len(refiner.paramToFit) > 0: 
            rectablews.setCell(self._currstep,  5, str(refiner.paramToFit))
        rectablews.setCell(self._currstep,  9, refiner.inprofilewsname)
        rectablews.setCell(self._currstep, 10, refiner.inreflectionwsname)
        rectablews.setCell(self._currstep, 11, refiner.bkgdtype)
        rectablews.setCell(self._currstep, 12, refiner.bkgdtablewsname)
        
	return

    def _recordPostRefineInfo(self, refiner):
        """ Record post-refinement information, i.e., refinement result
        """
        # Parse profile table workspace
        outprofilews = mtd[refiner.outprofilewsname]
        # FIXME - Use Name[0], Value[1] as default
        numpars = outprofilews.rowCount()
        rwp = None
        for i in xrange(numpars):
            parname = outprofilews.cell(i, 0)
            if parname.lower() == "rwp":
                rwp = outprofilews.cell(i, 1)
                break

        # Set the record table workspace
	rectablews = mtd[self._recordwsname]
        numrows = rectablews.rowCount()
        # currstep = numrows-1

        rectablews.setCell(self._currstep, 1, refiner.outprofilewsname)
        rectablews.setCell(self._currstep, 2, refiner.outreflectionwsname)
        rectablews.setCell(self._currstep, 3, refiner.bkgdtype)
        rectablews.setCell(self._currstep, 4, refiner.bkgdtablewsname)
        if rwp is not None: 
            rectablews.setCell(self._currstep, 6, rwp)
        
	return

    def _genOutputWorkspace(self, dataws, profilewsname, braggpeakwsname):
        """ 
        """
        outwsname = "%s_%s_Step%d" % (dataws, self._ID, self._currstep)

        if profilewsname.count(self._ID) > 0:
            outprofilewsname = profilewsname.split(self._ID)[0]
        else:
            outprofilewsname = profilewsname
	print "Input: ", profilewsname, "Output: ", type(outprofilewsname), "ID = ", self._ID, "Step = ", self._currstep
        outprofilewsname = "%s%s_Step%d" % (outprofilewsname, self._ID, self._currstep)

        if braggpeakwsname.count(str(self._ID)) > 0:
            outbpwsname = braggpeakwsname.split(self._ID)[0]
        else:
            outbpwsname = braggpeakwsname
        outbpwsname = "%s%s_Step%d"%(outbpwsname, self._ID, self._currstep)
    
        return (outwsname, outprofilewsname, outbpwsname)

#--------------------------------------------------------------------

def generateMCSetupTable(wsname):
    """ Generate a Le Bail fit Monte Carlo random walk setup table
    """
    import mantid.simpleapi as api
    
    tablews = api.CreateEmptyTableWorkspace(OutputWorkspace=str(wsname))
    
    tablews.addColumn("str", "Name")
    tablews.addColumn("double", "A0")
    tablews.addColumn("double", "A1")
    tablews.addColumn("int", "NonNegative")
    tablews.addColumn("int", "Group")
    
    group = 0
    tablews.addRow(["Dtt1"  , 5.0, 0.0, 0, group]) 
    tablews.addRow(["Dtt1t" , 5.0, 0.0, 0, group])
    tablews.addRow(["Dtt2t" , 1.0, 0.0, 0, group])
    tablews.addRow(["Zero"  , 5.0, 0.0, 0, group])
    tablews.addRow(["Zerot" , 5.0, 0.0, 0, group])
    tablews.addRow(["Width" , 0.0, 0.1, 1, group])
    tablews.addRow(["Tcross", 0.0, 1.0, 1, group])
    
    group = 1
    tablews.addRow(["Beta0" , 0.50, 1.0, 0, group]) 
    tablews.addRow(["Beta1" , 0.05, 1.0, 0, group])
    tablews.addRow(["Beta0t", 0.50, 1.0, 0, group])
    tablews.addRow(["Beta1t", 0.05, 1.0, 0, group])
    
    group = 2
    tablews.addRow(["Alph0" , 0.05, 1.0, 0, group])
    tablews.addRow(["Alph1" , 0.02, 1.0, 0, group])
    tablews.addRow(["Alph0t", 0.10, 1.0, 0, group])
    tablews.addRow(["Alph1t", 0.05, 1.0, 0, group])
    
    group = 3
    tablews.addRow(["Sig0", 2.0, 1.0, 1, group])
    tablews.addRow(["Sig1", 2.0, 1.0, 1, group])
    tablews.addRow(["Sig2", 2.0, 1.0, 1, group])
    
    group = 4
    tablews.addRow(["Gam0", 2.0, 1.0, 0, group])
    tablews.addRow(["Gam1", 2.0, 1.0, 0, group])
    tablews.addRow(["Gam2", 2.0, 1.0, 0, group])

    return tablews

def breakParametersGroups(tablews):
    """ Break the parameter groups.  Such that each parameter/row has an individual group
    """
    numrows = tablews.rowCount()
    for ir in xrange(numrows):
        tablews.setCell(ir, 4, ir)

    return

def resetParametersGroups(tablews):
    """ Set the group number to original setup
    """
    numrows = tablews.rowCount()
    for ir in xrange(numrows):
        parname = tablews.cell(ir, 0)
        if parname in ["Dtt1", "Dtt1t", "Dtt2t", "Zero", "Zerot", "Width", "Tcross"]:
            group = 0
        elif parname in ["Beta0", "Beta1", "Beta0t", "Beta1t"]:
            group = 1
        elif parname in ["Alph0", "Alph1", "Alph0t", "Alph1t"]:
            group = 2
        elif parname in ["Sig0", "Sig1", "Sig2"]:
            group = 3
        else:
            group = 4
        tablews.setCell(ir, 4, group)
        return


class RefineProfileParameters:
    """ Class to refine profile parameters ONE step
    """
    def __init__(self):
        """ Initialization
        """
        # bankid, calibDict = importCalibrationInformation(globalsfname)

        # self.bankid = int(bankid)
        # bankid = self.bankid

        # self.sample = calibDict[bankid]["Sample"]

        # self.datafilename = calibDict["DataFileDir"] + calibDict[bankid]["DataFileName"] 
        # hklfilename  = calibDict["HKLFileDir"]  + calibDict[bankid]["HKLFileName"]
        # irffilename  = calibDict["IrfFileDir"]  + calibDict[bankid]["IrfFileName"]
    
        # self.startx = float(calibDict[bankid]["LeBailFitMinTOF"])
        # self.endx   = float(calibDict[bankid]["LeBailFitMaxTOF"])
	# 
	# self.run = calibDict[bankid]["Run"]

        # # Name of workspaces
        # self.datawsname = calibDict[bankid]["DataWorkspace"]
        # instrparamwsname     = "Bank%sInstrumentParameterTable" % (bankid)
        # braggpeakparamwsname = 'BraggPeakParameterTable'

        # # Background related
        # usrbkgdpoints       = calibDict[bankid]["UserSpecifiedBkgdPts"]
        # self.bkgdwsname     = self.datawsname+"_Background"
        # backgroundtype      = calibDict["BackgroundType"]
        # backgroundorder     = int(calibDict["BackgroundOrder"])
        # self.bkgdfilename   = calibDict["WorkingDir"] + self.datawsname + "_Parameters.bak'"
        # bkgdwsname          = self.datawsname + "_Background"
        # self.bkgdtablewsname = self.datawsname + "_Background_Parameters"

        # # Other constants
        # latticesize   = calibDict[bankid]["LatticeSize"]

   
        # Output
        self.outwsname = None

        self.numsteps = 0

        # Refine
        self.paramToFit = []

	# Flags
	self._inputIsSetup = False
	self._outputIsSetup = False

        return

    def setInputs(self, datawsname, profilewsname, braggpeakwsname, bkgdtype, bkgdparwsname):
	"""
	"""
	self.datawsname = datawsname
	self.inprofilewsname = profilewsname
	self.inreflectionwsname = braggpeakwsname
	self.bkgdtype = bkgdtype
	self.bkgdtablewsname = bkgdparwsname

	self._inputIsSetup = True

	return

    def setOutputs(self, outwsname, profilewsname, braggpeakwsname, bkgdparwsname):
	""" Set up the variables for output
	"""
	self.outwsname = outwsname
	self.outprofilewsname = profilewsname
	self.outreflectionwsname = braggpeakwsname
	self.outbkgdtablewsname = bkgdparwsname

	self._outputIsSetup = True

	return


    def calculate(self, startx, endx):
        """ Do Le bail calculation
        """
	if (self._inputIsSetup and self.outputIsSetup) is False:
	    raise NotImplementedError("Either input or output is not setup.")

        LeBailFit( 
                Function                =   'Calculation',
                InputWorkspace          =   self.datawsname, 
                OutputWorkspace         =   self.outwsname,
                InputParameterWorkspace =   self.inprofilewsname,
                OutputParameterWorkspace=   self.outprofilewsname, 
                InputHKLWorkspace       =   self.inreflectionwsname,
                OutputPeaksWorkspace    =   self.outreflectionwsname, 
    	        FitRegion               =   '%f, %f' % (startx, endx),
                BackgroundType          =   self.bkgdtype,
                UseInputPeakHeights     =   False, 
                PeakRadius              =   '8',
                BackgroundParametersWorkspace   =   self.bkgdtablewsname
        )

        return


    def refine(self, numsteps, parameternames, startx, endx):
        """ Main execution body (doStep4)
        """
	# Check validity
	if (self._inputIsSetup and self.outputIsSetup) is False:
	    raise NotImplementedError("Either input or output is not setup.")

	# Update parameters' fit table
        if numsteps > 0:
            # Set up the default parameters to refine

            # Set up the parameters to refine
            UpdatePeakParameterTableValue(
                    InputWorkspace  =   self.inprofilewsname,
                    Column          =   "FitOrTie",
                    NewStringValue  =   "tie")
            UpdatePeakParameterTableValue( 
                    InputWorkspace  =   self.inprofilewsname, 
                    Column          =   "FitOrTie",
                    ParameterNames  =   parameternames,
                    NewStringValue  =   "fit")

            # Limit the range of MC 
            if parameternames.count("Width") > 0:
                #self.cwl = 1.33
                UpdatePeakParameterTableValue( 
                        InputWorkspace  =   self.inprofilewsname, 
                        Column          =   "Min",
                        ParameterNames  =   ["Width"],
                        NewFloatValue   =   0.50) #cwl*0.25)
                        
                UpdatePeakParameterTableValue( 
                        InputWorkspace  =   self.inprofilewsname, 
                        Column          =   "Max",
                        ParameterNames  =   ["Width"],
                        NewFloatValue   =   1.25) #cwl*4.0)
               
            # Generate Monte carlo table
            wsname = "MCSetupParameterTable"
            tablews = generateMCSetupTable(wsname)

            LeBailFit(
                    InputWorkspace                  = self.datawsname,
                    OutputWorkspace                 = self.outwsname,
                    InputParameterWorkspace         = self.inprofilewsname,
                    OutputParameterWorkspace        = self.outprofilewsname,
                    InputHKLWorkspace               = self.inreflectionwsname, 
                    OutputPeaksWorkspace            = self.outreflectionwsname, 
                    FitRegion                       = '%f, %f' % (self.startx, self.endx),
                    Function                        = 'MonteCarlo', 
                    NumberMinimizeSteps             = self.numsteps, 
		    BackgroundType                  = self.bkgdtype,
                    BackgroundParametersWorkspace   = self.bkgdtablewsname,
                    UseInputPeakHeights             = False, 
                    PeakRadius                      ='8',
                    Minimizer                       = 'Levenberg-Marquardt',
                    MCSetupWorkspace                = str(wsname),
                    Damping                         = '5.0',
                    RandomSeed                      = 0,
                    AnnealingTemperature            = 100.0,
            	    DrunkenWalk                     = True)	    
        # ENDIF (step)

    
        return

# Register algorithm with Mantid
AlgorithmFactory.subscribe(RefinePowderDiffProfileSeq)
