# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=no-init,invalid-name,too-many-instance-attributes
from mantid.api import (
    mtd,
    AlgorithmFactory,
    AnalysisDataService,
    FileAction,
    FileProperty,
    ITableWorkspaceProperty,
    PythonAlgorithm,
    PropertyMode,
    MatrixWorkspaceProperty,
)
from mantid.kernel import Direction, StringArrayProperty, StringListValidator
from mantid.simpleapi import (
    CreateEmptyTableWorkspace,
    GroupWorkspaces,
    LeBailFit,
    LoadNexusProcessed,
    SaveNexusProcessed,
    UpdatePeakParameterTableValue,
)


class RefinePowderDiffProfileSeq(PythonAlgorithm):
    """Refine powder diffractometer profile by Le Bail algorithm sequentially"""

    dataws = None
    wsindex = None
    startx = None
    endx = None
    _lastStep = None
    _projectID = None
    functionoption = None
    peaktype = None
    bkgdtype = None
    bkgdparws = None
    profilews = None
    braggpeakws = None
    paramstofit = None
    numcycles = None
    outprojectfilename = None
    inprojectfilename = None
    datawsname = None

    def category(self):
        """Category"""
        return "Diffraction\\Fitting"

    def seeAlso(self):
        return ["RefinePowderInstrumentParameters"]

    def name(self):
        """Algorithm name"""
        return "RefinePowderDiffProfileSeq"

    def summary(self):
        return "Refine powder diffractomer profile parameters sequentially."

    def PyInit(self):
        """Declare properties"""
        self.declareProperty(
            MatrixWorkspaceProperty("InputWorkspace", "", Direction.Input, PropertyMode.Optional),
            "Name of data workspace containing the diffraction pattern in .prf file. ",
        )

        self.declareProperty(
            "WorkspaceIndex", 0, "Spectrum (workspace index starting from 0) of the data to refine against in input workspace."
        )

        self.declareProperty(
            ITableWorkspaceProperty("SeqControlInfoWorkspace", "", Direction.InOut, PropertyMode.Optional),
            "Name of table workspace containing sequential refinement information.",
        )

        self.declareProperty(
            ITableWorkspaceProperty("InputProfileWorkspace", "", Direction.Input, PropertyMode.Optional),
            "Name of table workspace containing starting profile parameters.",
        )

        self.declareProperty(
            ITableWorkspaceProperty("InputBraggPeaksWorkspace", "", Direction.Input, PropertyMode.Optional),
            "Name of table workspace containing a list of reflections. ",
        )

        self.declareProperty(
            ITableWorkspaceProperty("InputBackgroundParameterWorkspace", "", Direction.Input, PropertyMode.Optional),
            "Name of table workspace containing a list of reflections. ",
        )

        self.declareProperty("StartX", -0.0, "Start X (TOF) to refine diffraction pattern.")
        self.declareProperty("EndX", -0.0, "End X (TOF) to refine diffraction pattern.")

        funcoptions = ["Setup", "Refine", "Save", "Load"]
        self.declareProperty("FunctionOption", "Refine", StringListValidator(funcoptions), "Options of functionality")

        # refoptions = ["Levenberg-Marquardt", "Random Walk", "Single Peak Fit"]
        refoptions = ["Random Walk"]
        self.declareProperty("RefinementOption", "Random Walk", StringListValidator(refoptions), "Options of algorithm to refine. ")

        self.declareProperty(
            StringArrayProperty("ParametersToRefine", values=[], direction=Direction.Input), "List of parameters to refine."
        )

        self.declareProperty("NumRefineCycles", 1, "Number of refinement cycles.")

        peaktypes = [
            "",
            "Neutron Back-to-back exponential convoluted with pseudo-voigt",
            "Thermal neutron Back-to-back exponential convoluted with pseudo-voigt",
        ]
        self.declareProperty("ProfileType", "", StringListValidator(peaktypes), "Type of peak profile function.")

        bkgdtypes = ["", "Polynomial", "Chebyshev", "FullprofPolynomial"]
        self.declareProperty("BackgroundType", "", StringListValidator(bkgdtypes), "Type of background function.")

        self.declareProperty("FromStep", -1, "If non-negative, the previous code is not set from last step, but the step specified.")

        # Property for save project
        self.declareProperty(
            FileProperty("OutputProjectFilename", "", FileAction.OptionalSave, [".nxs"]), "Name of sequential project file."
        )

        # Property for save project
        self.declareProperty(
            FileProperty("InputProjectFilename", "", FileAction.OptionalLoad, [".nxs"]), "Name of sequential project file."
        )

        # Project ID
        self.declareProperty("ProjectID", "", "Project ID.")

        return

    def PyExec(self):
        """Main"""
        # Process input
        self._processInputProperties()

        # Instantiaze sequential refinement
        seqrefine = SeqRefineProfile(self._projectID, self.log())

        # Execute
        if self.functionoption == "Setup":
            # Set up
            if seqrefine.isSetup() is True:
                raise NotImplementedError("Impossible to have it set up already.")

            seqrefine.initSetup(
                self.dataws,
                self.wsindex,
                self.peaktype,
                self.profilews,
                self.braggpeakws,
                self.bkgdtype,
                self.bkgdparws,
                self.startx,
                self.endx,
            )

        elif self.functionoption == "Refine":
            # Refine
            if seqrefine.isSetup() is False:
                raise NotImplementedError("Exception because sequential refinement is not set up.")
            seqrefine.refine(self.dataws, self.wsindex, self.paramstofit, self.numcycles, self.startx, self.endx, self._lastStep)

        elif self.functionoption == "Save":
            # Save the current state to a project file
            seqrefine.saveProject(str(self.dataws), self.wsindex, self.outprojectfilename)

        elif self.functionoption == "Load":
            # Set up from an exiting project file
            if seqrefine.isSetup() is True:
                raise NotImplementedError("Impossible to have it set up already.")

            seqrefine.loadProject(self.inprojectfilename)

        else:
            # None-support
            raise NotImplementedError("Function is not supported.")

        return

    def _processInputProperties(self):
        """Process input properties"""
        # Input data workspace and related
        self.dataws = self.getProperty("InputWorkspace").value
        self.wsindex = self.getProperty("WorkspaceIndex").value

        self.startx = self.getProperty("StartX").value
        self.endx = self.getProperty("EndX").value

        self._lastStep = self.getProperty("FromStep").value

        self._projectID = self.getProperty("ProjectID").value
        if len(self._projectID) == 0:
            raise NotImplementedError("User must specify project ID.")

        self.functionoption = self.getProperty("FunctionOption").value
        if self.functionoption == "Setup":
            # Request on 'Setup'
            ptype = self.getProperty("ProfileType").value
            if ptype == "Neutron Back-to-back exponential convoluted with pseudo-voigt":
                self.peaktype = "NeutronBk2BkExpConvPVoigt"
            elif ptype == "Thermal neutron Back-to-back exponential convoluted with pseudo-voigt":
                self.peaktype = "ThermalNeutronBk2BkExpConvPVoigt"
            else:
                raise NotImplementedError("Peak profile is not supported.")

            self.bkgdtype = self.getProperty("BackgroundType").value
            self.bkgdparws = self.getProperty("InputBackgroundParameterWorkspace").value
            self.profilews = self.getProperty("InputProfileWorkspace").value
            self.braggpeakws = self.getProperty("InputBraggPeaksWorkspace").value

        elif self.functionoption == "Refine":
            self.paramstofit = self.getProperty("ParametersToRefine").value
            self.numcycles = self.getProperty("NumRefineCycles").value

        elif self.functionoption == "Save":
            self.outprojectfilename = self.getProperty("OutputProjectFilename").value

        elif self.functionoption == "Load":
            self.inprojectfilename = self.getProperty("InputProjectFilename").value

        else:
            raise NotImplementedError("Unsupported function mode  %s. " % (self.functionoption))

        if self.functionoption != "Load":
            self.datawsname = str(self.dataws)
            if self.wsindex < 0 or self.wsindex >= self.dataws.getNumberHistograms():
                raise NotImplementedError(
                    "Input workspace index %d is out of range (0, %d)." % (self.wsindex, self.dataws.getNumberHistograms())
                )

        return


# --------------------------------------------------------------------
#
# --------------------------------------------------------------------
# pylint: disable=too-many-instance-attributes
class SeqRefineProfile(object):
    """A class to do sequential refinement on peak profile

    Use case:
    1. Set class object such as : ID/StartDate/Directory
    2. Run main to refine some parameters
    3. Check result
    4. If no further instruction, only need to set up parameters to refine
       the input/starting values should be from the last
    """

    _datawsname = None
    _profileWS = None
    _braggpeakws = None
    _bkgdtype = None
    _bkgdparws = None
    _wsgroup = None
    datawsname = None
    wsindex = None
    _recordws = None
    _recordwsLastRowValid = None
    _lastValidStep = None
    _lastValidRowIndex = None
    _peakType = None
    _bkgdType = None
    _currstep = None

    def __init__(self, ID, glog):
        """ """
        # Set up log
        self.glog = glog

        # Set up ID
        self._ID = str(ID)
        self.glog.information("SeqRefineProfile is initialized with ID = %s" % (str(ID)))

        # Standard record table and check its existence
        # FIXME - This workspace's name should be passed from Algorithm's property!
        self._recordwsname = "Record%sTable" % (str(ID))
        self.glog.notice("Using record table %s" % (self._recordwsname))

        if AnalysisDataService.doesExist(self._recordwsname):
            # Record workspace exists: has been set up
            self._isSetup = True
        else:
            # Record workspace does not exist: first time or need to load from file
            self._isSetup = False

        self._recordWSLastRowInvalid = False

        # Result workspace group
        self._wsgroupName = self._ID + "_Group"
        if AnalysisDataService.doesExist(self._wsgroupName):
            self._wsgroupCreated = True
        else:
            self._wsgroupCreated = False

        return

    # pylint: disable=too-many-arguments
    def initSetup(self, dataws, wsindex, peaktype, profilews, braggpeakws, bkgdtype, bkgdparws, startx, endx):
        """Set up the properties for LeBailFit as the first time including
        do a Le bail calculation based on the input parameters
        including profilews, braggpeakws, and etc
        """
        # Data and data range
        self._datawsname = str(dataws)
        if startx <= 0.0:
            startx = dataws.readX(wsindex)[0]
        if endx <= 0.0:
            endx = dataws.readX(wsindex)[-1]

        # Profile
        self._peakType = peaktype
        self._profileWS = profilews
        self._braggpeakws = braggpeakws
        self._bkgdtype = bkgdtype
        self._bkgdparws = bkgdparws

        # Generate record table
        self._genRecordTable()

        # Check input parameters, i.e., verification/examine input parameters
        runner = RefineProfileParameters(self.glog)

        outwsname = self._datawsname + "_Init"

        runner.setInputs(self._datawsname, self._peakType, self._profileWS, self._braggpeakws, self._bkgdtype, self._bkgdparws)
        # FIXME - Need to verify whether input and output background parameter ws name can be same
        runner.setOutputs(outwsname, self._profileWS, self._braggpeakws, self._bkgdparws)

        self._recordPreRefineInfo(runner, -1)
        runner.calculate(startx, endx)
        self._recordPostRefineInfo(runner)

        # Group the newly generated workspace and do some record
        GroupWorkspaces(
            InputWorkspaces="%s, %s, %s, %s" % (outwsname, self._profileWS, self._braggpeakws, self._bkgdparws),
            OutputWorkspace=self._wsgroupName,
        )
        self._wsgroupCreated = True

        # Repository

        # Replace 'Refine' of step 0 to ID (it is always empty)
        self._recordws.setCell(0, 5, self._ID)
        # Replace 'InputProfileWorkspace' by profile type (it is alwasy same as output)
        self._recordws.setCell(0, 9, self._peakType)

        self._isSetup = True

        return

    def loadProject(self, projectfilename):
        """Load the project from a saved project file"""
        # Load workspace group
        LoadNexusProcessed(Filename=projectfilename, OutputWorkspace=self._wsgroupName)
        self._wsgroup = AnalysisDataService.retrieve(self._wsgroupName)

        if self._wsgroup.__class__.__name__ != "WorkspaceGroup":
            raise NotImplementedError("Input is not a workspace group but a %s" % (self._wsgroup.__class__.__name__))
        else:
            self._wsgroupCreated = True

        # Parse README
        wsnames = self._wsgroup.getNames()
        readmewsname = None
        for wsname in wsnames:
            if wsname.startswith("READ"):
                readmewsname = wsname
                break
        if readmewsname is None:
            raise NotImplementedError("No README workspace is found in loaded workspace group.")

        readmews = AnalysisDataService.retrieve(readmewsname)
        infodict = {}
        numrows = readmews.rowCount()
        self.glog.information("Found %d rows in workspace %s" % (numrows, str(readmews)))
        for r in range(numrows):
            functioncat = str(readmews.cell(r, 0)).strip()
            functiontype = str(readmews.cell(r, 1)).strip()
            infodict[functioncat] = functiontype.strip()
        self.glog.information("README keys: %s" % (list(infodict.keys())))
        self._peakType = infodict["Peak"]
        self.datawsname = infodict["Data"]
        self.wsindex = infodict["Spectrum"]
        if self._ID != infodict["ID"]:
            raise NotImplementedError("ID mismatch!")

        self._recordwsname = infodict["Record"]

        self._isSetup = True

        return

    # pylint: disable=too-many-arguments
    def refine(self, dataws, wsindex, parametersToFit, numcycles, startx, endx, laststepindex):
        """Refine parameters"""
        # Range of fit
        if startx <= 0.0:
            startx = dataws.readX(wsindex)[0]
        if endx <= 0.0:
            endx = dataws.readX(wsindex)[-1]

        # Set up RefineProfileParameters object
        runner = RefineProfileParameters(self.glog)

        # Locate refinement record table
        profilewsname, braggpeakwsname, bkgdtype, bkgdparamwsname, laststep = self._parseRecordTable(laststepindex)

        # Set up runner and refine
        runner.setupMonteCarloRefine(numcycles, parametersToFit)

        outwsname, outprofilewsname, outbraggpeakwsname = self._genOutputWorkspace(str(dataws), profilewsname, braggpeakwsname)

        # Set up input and output
        runner.setInputs(str(dataws), self._peakType, profilewsname, braggpeakwsname, bkgdtype, bkgdparamwsname)
        # FIXME - Need to verify whether input and output background parameter ws name can be same
        runner.setOutputs(outwsname, outprofilewsname, outbraggpeakwsname, bkgdparamwsname)

        # Refine and record pre and post refinement information
        self._recordPreRefineInfo(runner, laststep)
        runner.refine(numcycles, parametersToFit, startx, endx)
        self._recordPostRefineInfo(runner)

        # Group newly generated workspaces and add name to reposiotry
        if self._wsgroupCreated is True:
            GroupWorkspaces(
                InputWorkspaces="%s, %s, %s" % (outwsname, outprofilewsname, outbraggpeakwsname), OutputWorkspace=self._wsgroupName
            )
        else:
            wsgroup = AnalysisDataService.retrieve(self._wsgroupName)
            hasbkgd = list(wsgroup.getNames()).count(bkgdparamwsname)
            if hasbkgd == 1:
                GroupWorkspaces(
                    InputWorkspaces="%s, %s, %s" % (outwsname, outprofilewsname, outbraggpeakwsname), OutputWorkspace=self._wsgroupName
                )
            elif hasbkgd == 0:
                GroupWorkspaces(
                    InputWorkspaces="%s, %s, %s, %s" % (outwsname, outprofilewsname, outbraggpeakwsname, bkgdparamwsname),
                    OutputWorkspace=self._wsgroupName,
                )
            else:
                raise NotImplementedError("Impossible to have 1 workspace appeared twice in a workspace group.")

        return

    def isSetup(self):
        """Status whether refinement is set up."""
        return self._isSetup

    def saveProject(self, datawsname, wsindex, projectfname):
        """Save current to a project file
        Note: MC setup table workspace is not generated in this class.  So it won't be saved
        """
        import os

        # FIXME - Find out a good way to remove existing files/directories
        if os.path.exists(projectfname) is True:
            import shutil

            try:
                os.remove(projectfname)
            except RuntimeError:
                shutil.rmtree(projectfname)
            except IOError:
                shutil.rmtree(projectfname)
            except OSError:
                shutil.rmtree(projectfname)

        SaveNexusProcessed(InputWorkspace=self._wsgroupName, Filename=projectfname, Append=False)

        # Add data workspace, tracking record table  to workspaces
        # api.GroupWorkspaces(InputWorkspaces="%s, %s, %s" % (datawsname, self._recordwsname, self._wsgroupName),
        #         OutputWorkspace=self._wsgroupName)
        self.glog.notice("Append record workspace %s" % (self._recordwsname))
        SaveNexusProcessed(InputWorkspace=self._recordwsname, Filename=projectfname, Append=True)

        self.glog.notice("Append data workspace %s" % (datawsname))
        SaveNexusProcessed(InputWorkspace=datawsname, Filename=projectfname, Append=True)

        # Create a new README table workspace for some other information
        readmewsname = "READ_%s" % (self._ID)
        readmews = CreateEmptyTableWorkspace(OutputWorkspace=readmewsname)
        readmews.addColumn("str", "Function")
        readmews.addColumn("str", "Type")

        readmews.addRow(["Peak", "Not Important"])
        readmews.addRow(["Background", "Not Important"])
        readmews.addRow(["ID", str(self._ID)])
        readmews.addRow(["Record", self._recordwsname])
        readmews.addRow(["Data", str(datawsname)])
        readmews.addRow(["Spectrum", str(wsindex)])

        SaveNexusProcessed(InputWorkspace=readmewsname, Filename=projectfname, Append=True)

        return

    def _genRecordTable(self):
        """Generate record table"""
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

        self._recordws = tablews

        return

    def _parseRecordTable(self, laststep):
        """Parse record table and return the last refinement result
        Notice that 'last row' in record table might not be a valid row (incomplete).
        It might be caused by an exception raised in refinement or its setup.
        Class variable _recordWSLastRowInvalid is used to indicate this
        """
        # Retrieve record workspace
        self._recordws = AnalysisDataService.retrieve(str(self._recordwsname))
        numrows = self._recordws.rowCount()
        if numrows == 0:
            raise NotImplementedError("Empty record table workspace. ")

        # Find last valid row
        lastvalidrow = -1
        lastrow = numrows - 1
        self._recordwsLastRowValid = False
        while self._recordwsLastRowValid is False and lastrow >= 0:
            profilewsname = self._recordws.cell(lastrow, 1)
            if profilewsname == "":
                self.glog.warning("Profile workspace name is emtpy in row %d!" % (lastrow))
                lastrow -= 1
            else:
                self._recordwsLastRowValid = True
                lastvalidrow = lastrow
        # ENDWHILE
        if lastvalidrow < 0:
            raise NotImplementedError("XXX")

        # Find out last step row
        lastrecordedstep = self._recordws.cell(lastvalidrow, 0)
        self.glog.notice("Last recorded valid step is %d. " % (lastrecordedstep))

        self._lastValidStep = lastrecordedstep
        self._lastValidRowIndex = lastvalidrow

        if laststep > lastrecordedstep:
            self.glog.warning("Last step %d is not recorded.  Using step %d instead. " % (laststep, lastrecordedstep))
            laststep = lastrecordedstep
        elif laststep < 0:
            self.glog.notice("Using default last valid step %d. " % (self._lastValidStep))
            laststep = self._lastValidStep

        profilewsname = ""
        while lastvalidrow >= 0:
            step = self._recordws.cell(lastvalidrow, 0)
            if step != laststep:
                lastvalidrow -= 1
            else:
                profilewsname = self._recordws.cell(lastvalidrow, 1).strip()
                reflectwsname = self._recordws.cell(lastvalidrow, 2).strip()
                bkgdtype = self._recordws.cell(lastrow, 3).strip()
                bkgdparamwsname = self._recordws.cell(lastrow, 4).strip()
                if profilewsname == "":
                    raise NotImplementedError("Profile workspace name is emtpy in row %d.  It is not supposed to happen." % (lastvalidrow))
                break
        # ENDWHILE
        if profilewsname == "":
            raise NotImplementedError("Step %d is not found in record table.  It is impossible. " % (laststep))

        # Current step
        self._currstep = self._lastValidStep + 1
        self.glog.notice("Current step is %d" % (self._currstep))

        # Set up for other informatin
        # Peak type
        self._peakType = self._recordws.cell(0, 9).strip()
        # Background type
        self._bkgdType = bkgdtype.strip()

        return (profilewsname, reflectwsname, bkgdtype, bkgdparamwsname, laststep)

    def _recordPreRefineInfo(self, refiner, laststep):
        """Record pre-refinement information"""
        rectablews = mtd[self._recordwsname]
        numrows = rectablews.rowCount()

        if self._recordWSLastRowInvalid is False:
            self._currstep = numrows
            rectablews.addRow(
                [self._currstep, "", "", "", "", "", -1.0, laststep, -1.0, "profilews", "reflectionws", "Polynomial", "BkgdParm"]
            )
        else:
            self._currstep = numrows - 1
            laststep = self._lastValidStep

        # print "*** Record workspace has %d rows. current step = %d. " % (rectablews.rowCount(), self._currstep)

        if len(refiner.paramToFit) > 0:
            rectablews.setCell(self._currstep, 5, str(refiner.paramToFit))
        rectablews.setCell(self._currstep, 9, str(refiner.inprofilewsname))
        rectablews.setCell(self._currstep, 10, str(refiner.inreflectionwsname))
        rectablews.setCell(self._currstep, 11, str(refiner.bkgdtype))
        rectablews.setCell(self._currstep, 12, str(refiner.bkgdtablewsname))

        return

    def _recordPostRefineInfo(self, refiner):
        """Record post-refinement information, i.e., refinement result"""
        # Parse profile table workspace
        # print "****** outprofilews type = ", type(refiner.outprofilewsname)
        outprofilews = AnalysisDataService.retrieve(str(refiner.outprofilewsname))
        # outprofilews = api.mtd[refiner.outprofilewsname]
        # FIXME - Use Name[0], Value[1] as default
        numpars = outprofilews.rowCount()
        rwp = None
        for i in range(numpars):
            parname = outprofilews.cell(i, 0)
            if parname.lower() == "rwp":
                rwp = outprofilews.cell(i, 1)
                break

        # Set the record table workspace
        rectablews = mtd[self._recordwsname]
        # numrows = rectablews.rowCount()
        # currstep = numrows-1

        rectablews.setCell(self._currstep, 1, str(refiner.outprofilewsname))
        rectablews.setCell(self._currstep, 2, str(refiner.outreflectionwsname))
        rectablews.setCell(self._currstep, 3, str(refiner.bkgdtype))
        rectablews.setCell(self._currstep, 4, str(refiner.bkgdtablewsname))
        if rwp is not None:
            rectablews.setCell(self._currstep, 6, rwp)

        return

    def _genOutputWorkspace(self, datawsname, profilewsname, braggpeakwsname):
        """ """
        outwsname = "%s_%s_Step%d" % (datawsname, self._ID, self._currstep)

        if profilewsname.count(self._ID) > 0:
            outprofilewsname = profilewsname.split(self._ID)[0]
        else:
            outprofilewsname = profilewsname
        outprofilewsname = "%s%s_Step%d" % (outprofilewsname, self._ID, self._currstep)

        if braggpeakwsname.count(str(self._ID)) > 0:
            outbpwsname = braggpeakwsname.split(self._ID)[0]
        else:
            outbpwsname = braggpeakwsname
        outbpwsname = "%s%s_Step%d" % (outbpwsname, self._ID, self._currstep)

        return (outwsname, outprofilewsname, outbpwsname)


# --------------------------------------------------------------------


def generateMCSetupTableProf9(wsname):
    """Generate a Le Bail fit Monte Carlo random walk setup table"""
    tablews = CreateEmptyTableWorkspace(OutputWorkspace=str(wsname))

    tablews.addColumn("str", "Name")
    tablews.addColumn("double", "A0")
    tablews.addColumn("double", "A1")
    tablews.addColumn("int", "NonNegative")
    tablews.addColumn("int", "Group")

    group = 0
    tablews.addRow(["Dtt1", 5.0, 0.0, 0, group])
    tablews.addRow(["Dtt2", 1.0, 0.0, 0, group])
    tablews.addRow(["Zero", 5.0, 0.0, 0, group])

    group = 1
    tablews.addRow(["Beta0", 0.50, 1.0, 0, group])
    tablews.addRow(["Beta1", 0.05, 1.0, 0, group])

    group = 2
    tablews.addRow(["Alph0", 0.05, 1.0, 0, group])
    tablews.addRow(["Alph1", 0.02, 1.0, 0, group])

    group = 3
    tablews.addRow(["Sig0", 2.0, 1.0, 1, group])
    tablews.addRow(["Sig1", 2.0, 1.0, 1, group])
    tablews.addRow(["Sig2", 2.0, 1.0, 1, group])

    group = 4
    tablews.addRow(["Gam0", 2.0, 1.0, 0, group])
    tablews.addRow(["Gam1", 2.0, 1.0, 0, group])
    tablews.addRow(["Gam2", 2.0, 1.0, 0, group])

    return tablews


def generateMCSetupTableProf10(wsname):
    """Generate a Le Bail fit Monte Carlo random walk setup table"""

    tablews = CreateEmptyTableWorkspace(OutputWorkspace=str(wsname))

    tablews.addColumn("str", "Name")
    tablews.addColumn("double", "A0")
    tablews.addColumn("double", "A1")
    tablews.addColumn("int", "NonNegative")
    tablews.addColumn("int", "Group")

    group = 0
    tablews.addRow(["Dtt1", 5.0, 0.0, 0, group])
    tablews.addRow(["Dtt1t", 5.0, 0.0, 0, group])
    tablews.addRow(["Dtt2t", 1.0, 0.0, 0, group])
    tablews.addRow(["Zero", 5.0, 0.0, 0, group])
    tablews.addRow(["Zerot", 5.0, 0.0, 0, group])
    tablews.addRow(["Width", 0.0, 0.1, 1, group])
    tablews.addRow(["Tcross", 0.0, 1.0, 1, group])

    group = 1
    tablews.addRow(["Beta0", 0.50, 1.0, 0, group])
    tablews.addRow(["Beta1", 0.05, 1.0, 0, group])
    tablews.addRow(["Beta0t", 0.50, 1.0, 0, group])
    tablews.addRow(["Beta1t", 0.05, 1.0, 0, group])

    group = 2
    tablews.addRow(["Alph0", 0.05, 1.0, 0, group])
    tablews.addRow(["Alph1", 0.02, 1.0, 0, group])
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
    """Break the parameter groups.  Such that each parameter/row has an individual group"""
    numrows = tablews.rowCount()
    for ir in range(numrows):
        tablews.setCell(ir, 4, ir)

    return


def resetParametersGroups(tablews):
    """Set the group number to original setup"""
    numrows = tablews.rowCount()
    for ir in range(numrows):
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


# pylint: disable=too-many-instance-attributes


class RefineProfileParameters(object):
    """Class to refine profile parameters ONE step"""

    datawsname = None
    inprofilewsname = None
    inreflectionwsname = None
    bkgdtype = None
    bkgdtablewsname = None
    outprofilewsname = None
    outreflectionwsname = None
    outbkgdtablewsname = None

    def __init__(self, glog):
        """Initialization"""
        self.peaktype = "NOSETUP"

        # Output
        self.outwsname = None

        self.glog = glog

        self.numsteps = 0

        # Refine
        self.paramToFit = []

        # Flags
        self._inputIsSetup = False
        self._outputIsSetup = False

        return

    # pylint: disable=too-many-arguments
    def setInputs(self, datawsname, peaktype, profilewsname, braggpeakwsname, bkgdtype, bkgdparwsname):
        """ """
        self.datawsname = datawsname
        self.peaktype = peaktype
        self.inprofilewsname = profilewsname
        self.inreflectionwsname = braggpeakwsname
        self.bkgdtype = bkgdtype
        self.bkgdtablewsname = bkgdparwsname

        self._inputIsSetup = True

        return

    def setOutputs(self, outwsname, profilewsname, braggpeakwsname, bkgdparwsname):
        """Set up the variables for output"""
        self.outwsname = outwsname
        self.outprofilewsname = profilewsname
        self.outreflectionwsname = braggpeakwsname
        self.outbkgdtablewsname = bkgdparwsname

        self._outputIsSetup = True

        return

    def setupMonteCarloRefine(self, numcycles, parametersToFit):
        """Set up refinement parameters"""
        if numcycles <= 0:
            raise NotImplementedError("It is not allowed to set up a 0 or a negative number to MonteCarloRefine")
        else:
            self.numsteps = numcycles

        self.paramToFit = parametersToFit

        return

    def calculate(self, startx, endx):
        """Do Le bail calculation"""
        if (self._inputIsSetup and self._outputIsSetup) is False:
            raise NotImplementedError(
                "Either input or output is not setup: inputIsStepUp = %s, outputIsSetup = %s"
                % (str(self._inputIsSetup), str(self._outputIsSetup))
            )

        self.glog.information("**** Calculate: DataWorksapce = %s" % (str(self.datawsname)))
        self.glog.information("**** Fit range: %f, %f" % (startx, endx))
        self.glog.information("**** Profile workspace = %s, Reflection workspace = %s" % (self.inprofilewsname, self.inreflectionwsname))

        LeBailFit(
            Function="Calculation",
            InputWorkspace=self.datawsname,
            OutputWorkspace=self.outwsname,
            InputParameterWorkspace=self.inprofilewsname,
            OutputParameterWorkspace=self.outprofilewsname,
            InputHKLWorkspace=self.inreflectionwsname,
            OutputPeaksWorkspace=self.outreflectionwsname,
            FitRegion="%f, %f" % (startx, endx),
            PeakType=self.peaktype,
            BackgroundType=self.bkgdtype,
            UseInputPeakHeights=False,
            PeakRadius="8",
            BackgroundParametersWorkspace=self.bkgdtablewsname,
        )

        return

    def refine(self, numsteps, parameternames, startx, endx):
        """Main execution body (doStep4)"""
        # Check validity
        if (self._inputIsSetup and self._outputIsSetup) is False:
            raise NotImplementedError("Either input or output is not setup.")

        self.glog.debug("[Refine] Input profile workspace = %s" % (self.inprofilewsname))

        # Update parameters' fit table
        if numsteps > 0:
            # Set up the default parameters to refine

            # Set up the parameters to refine
            # FIXME - It is found that in the 'load' mode, a ID???_Group_2 might be generated by running
            #         UpdatePeakParameterTableValue().  It is not a real new table workspace, but a link
            #         to the 'inprofilewsname'
            #         There must be something wrong in AnalysisDataService.
            UpdatePeakParameterTableValue(InputWorkspace=self.inprofilewsname, Column="FitOrTie", NewStringValue="tie")
            UpdatePeakParameterTableValue(
                InputWorkspace=self.inprofilewsname, Column="FitOrTie", ParameterNames=parameternames, NewStringValue="fit"
            )

            # Limit the range of MC
            if parameternames.count("Width") > 0:
                # self.cwl = 1.33
                UpdatePeakParameterTableValue(
                    InputWorkspace=self.inprofilewsname, Column="Min", ParameterNames=["Width"], NewFloatValue=0.50
                )  # cwl*0.25)

                UpdatePeakParameterTableValue(
                    InputWorkspace=self.inprofilewsname, Column="Max", ParameterNames=["Width"], NewFloatValue=1.25
                )  # cwl*4.0)

            # Generate Monte carlo table
            wsname = "MCSetupParameterTable"
            if self.peaktype == "NeutronBk2BkExpConvPVoigt":
                tablews = generateMCSetupTableProf9(wsname)
            elif self.peaktype == "ThermalNeutronBk2BkExpConvPVoigt":
                tablews = generateMCSetupTableProf10(wsname)
            else:
                raise NotImplementedError("Peak type %s is not supported to set up MC table." % (self.peaktype))

            LeBailFit(
                InputWorkspace=self.datawsname,
                OutputWorkspace=self.outwsname,
                InputParameterWorkspace=self.inprofilewsname,
                OutputParameterWorkspace=self.outprofilewsname,
                InputHKLWorkspace=self.inreflectionwsname,
                OutputPeaksWorkspace=self.outreflectionwsname,
                FitRegion="%f, %f" % (startx, endx),
                Function="MonteCarlo",
                NumberMinimizeSteps=numsteps,
                PeakType=self.peaktype,
                BackgroundType=self.bkgdtype,
                BackgroundParametersWorkspace=self.bkgdtablewsname,
                UseInputPeakHeights=False,
                PeakRadius="8",
                Minimizer="Levenberg-Marquardt",
                MCSetupWorkspace=tablews,
                Damping="5.0",
                RandomSeed=0,
                AnnealingTemperature=100.0,
                DrunkenWalk=True,
            )
        # ENDIF (step)

        return


# Register algorithm with Mantid
AlgorithmFactory.subscribe(RefinePowderDiffProfileSeq)
