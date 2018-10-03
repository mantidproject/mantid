# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#pylint: disable=invalid-name,no-init,too-many-lines
from __future__ import (absolute_import, division, print_function)

import os

import mantid.simpleapi as api
from mantid.api import mtd, AlgorithmFactory, AnalysisDataService, DistributedDataProcessorAlgorithm, \
    FileAction, FileProperty, ITableWorkspaceProperty, MultipleFileProperty, PropertyMode, WorkspaceProperty, \
    ITableWorkspace, MatrixWorkspace
from mantid.kernel import ConfigService, Direction, FloatArrayProperty, \
    FloatBoundedValidator, IntArrayBoundedValidator, IntArrayProperty, \
    PropertyManagerDataService, StringListValidator
from mantid.dataobjects import SplittersWorkspace  # SplittersWorkspace
from six.moves import range #pylint: disable=redefined-builtin

if AlgorithmFactory.exists('GatherWorkspaces'):
    HAVE_MPI = True
    from mpi4py import MPI
    mpiRank = MPI.COMM_WORLD.Get_rank()
else:
    HAVE_MPI = False
    mpiRank = 0 # simplify if clauses


EVENT_WORKSPACE_ID = "EventWorkspace"
EXTENSIONS_NXS = ["_event.nxs", ".nxs.h5"]


def noRunSpecified(runs):
    """ Check whether any run is specified
    Purpose:
        Check whether any run, which is larger than 0, is specified in the input numpy array
    Requirements:
        Input is a numpy array
    Guarantees:
        A boolean is returned
    :param runs:
    :return:
    """
    # return True if runs is of size zero
    if len(runs) <= 0:
        return True

    # return True if there is one and only one run in runs and it is not greater than 0.
    if len(runs) == 1:
        # break off the instrument part
        value = int(runs[0].split('_')[-1])
        # -1 turns off the runnumber
        return value <= 0

    return False


def get_workspace(workspace_name):
    """
    Purpose: Get the reference of a workspace
    Requirements:
    1. workspace_name is a string
    Guarantees:
    The reference of the workspace with same is returned.
    :exception RuntimeError: a RuntimeError from Mantid will be thrown
    :param workspace_name:
    :return:
    """
    assert isinstance(workspace_name, str), 'Input workspace name {0} must be a string but not a {1}.' \
                                            ''.format(workspace_name, type(workspace_name))

    return AnalysisDataService.retrieve(workspace_name)


def is_event_workspace(workspace):
    if isinstance(workspace, str):
        return get_workspace(workspace).id() == EVENT_WORKSPACE_ID
    else:
        return workspace.id() == EVENT_WORKSPACE_ID


def allEventWorkspaces(*args):
    """
    Purpose:
        Check whether all the inputs are event workspace
    Requirements:
        Each element in the args is a string as name of workspace
    Guarantees:
        return boolean
    @param args:
    @return:
    """
    result = True

    args = [is_event_workspace(arg) for arg in args]
    for arg in args:
        result = result and arg

    return result


def getBasename(filename):
    if type(filename) == list:
        filename = filename[0]
    name = os.path.split(filename)[-1]
    for extension in EXTENSIONS_NXS:
        name = name.replace(extension, '')
    return name

#pylint: disable=too-many-instance-attributes


class SNSPowderReduction(DistributedDataProcessorAlgorithm):
    COMPRESS_TOL_TOF = .01
    _resampleX = None
    _binning = None
    _bin_in_dspace = None
    _instrument = None
    _filterBadPulses = None
    _removePromptPulseWidth = None
    _LRef = None
    _DIFCref = None
    _wavelengthMin = None
    _wavelengthMax = None
    _vanPeakFWHM = None
    _vanSmoothing = None
    _vanRadius = None
    _scaleFactor = None
    _outDir = None
    _outPrefix = None
    _outTypes = None
    _chunks = None
    _splittersWS = None
    _splitinfotablews = None
    _normalisebycurrent = None
    _lowResTOFoffset = None
    _focusPos = None
    _charTable = None
    iparmFile = None
    _info = None

    def category(self):
        return "Diffraction\\Reduction"

    def seeAlso(self):
        return [ "DiffractionFocussing","AlignAndFocusPowder" ]

    def name(self):
        return "SNSPowderReduction"

    def summary(self):
        """
        summary of the algorithm
        :return:
        """
        return "The algorithm used for reduction of powder diffraction data obtained on SNS instruments (e.g. PG3) "

    def PyInit(self):
        self.copyProperties('AlignAndFocusPowderFromFiles', ['Filename', 'PreserveEvents'])

        self.declareProperty("Sum", False,
                             "Sum the runs. Does nothing for characterization runs")
        self.declareProperty("PushDataPositive", "None",
                             StringListValidator(["None", "ResetToZero", "AddMinimum"]),
                             "Add a constant to the data that makes it positive over the whole range.")
        self.declareProperty('ReloadIfWorkspaceExists', True,
                             'Load the event file again if a workspace with default name exists in ADS. '
                             'if this is specified as True.  Otherwise, skip loading to save time. '
                             'User should be cautious to use this option as False.')
        arrvalidatorBack = IntArrayBoundedValidator()
        arrvalidatorBack.setLower(-1)
        self.declareProperty(IntArrayProperty("BackgroundNumber", values=[0], validator=arrvalidatorBack),
                             doc="If specified overrides value in CharacterizationRunsFile If -1 turns off correction.")
        arrvalidatorVan = IntArrayBoundedValidator()
        arrvalidatorVan.setLower(-1)
        self.declareProperty(IntArrayProperty("VanadiumNumber", values=[0], validator=arrvalidatorVan),
                             doc="If specified overrides value in CharacterizationRunsFile. If -1 turns off correction."
                                 "")
        arrvalidatorVanBack = IntArrayBoundedValidator()
        arrvalidatorVanBack.setLower(-1)
        self.declareProperty(IntArrayProperty("VanadiumBackgroundNumber", values=[0], validator=arrvalidatorVanBack),
                             doc="If specified overrides value in CharacterizationRunsFile. If -1 turns off correction."
                                 "")
        self.declareProperty(FileProperty(name="CalibrationFile",defaultValue="",action=FileAction.OptionalLoad,
                                          extensions=[".h5", ".hd5", ".hdf", ".cal"]))  # CalFileName
        self.declareProperty(FileProperty(name="GroupingFile",defaultValue="",action=FileAction.OptionalLoad,
                                          extensions=[".xml"]), "Overrides grouping from CalibrationFile")
        self.declareProperty(MultipleFileProperty(name="CharacterizationRunsFile",
                                                  action=FileAction.OptionalLoad,
                                                  extensions=["txt"]), "File with characterization runs denoted")
        self.declareProperty(FileProperty(name="ExpIniFilename", defaultValue="", action=FileAction.OptionalLoad,
                                          extensions=[".ini"]))
        self.copyProperties('AlignAndFocusPowderFromFiles',
                            ['UnwrapRef', 'LowResRef', 'CropWavelengthMin', 'CropWavelengthMax', 'RemovePromptPulseWidth',
                             'MaxChunkSize'])
        self.declareProperty(FloatArrayProperty("Binning", values=[0., 0., 0.],
                                                direction=Direction.Input),
                             "Positive is linear bins, negative is logorithmic")  # Params
        self.copyProperties('AlignAndFocusPowderFromFiles', ['ResampleX'])
        self.declareProperty("BinInDspace", True,
                             "If all three bin parameters a specified, whether they are in dspace (true) or "
                             "time-of-flight (false)")  # DSpacing
        # section of vanadium run processing
        self.declareProperty("StripVanadiumPeaks", True,
                             "Subtract fitted vanadium peaks from the known positions.")
        self.declareProperty("VanadiumFWHM", 7, "Default=7")
        self.declareProperty("VanadiumPeakTol", 0.05,
                             "How far from the ideal position a vanadium peak can be during StripVanadiumPeaks. "
                             "Default=0.05, negative turns off")
        self.declareProperty("VanadiumSmoothParams", "20,2", "Default=20,2")
        self.declareProperty("VanadiumRadius", .3175, "Radius for CarpenterSampleCorrection")
        self.declareProperty("BackgroundSmoothParams", "", "Default=off, suggested 20,2")

        # filtering
        self.declareProperty("FilterBadPulses", 95.,  # different default value
                             doc="Filter out events measured while proton charge is more than 5% below average")
        self.declareProperty("ScaleData", defaultValue=1., validator=FloatBoundedValidator(lower=0., exclusive=True),
                             doc="Constant to multiply the data before writing out. This does not apply to "
                                 "PDFgetN files.")
        self.declareProperty("SaveAs", "gsas",
                             "List of all output file types. Allowed values are 'fullprof', 'gsas', 'nexus', "
                             "'pdfgetn', and 'topas'")
        self.declareProperty("OutputFilePrefix", "", "Overrides the default filename for the output file (Optional).")
        self.declareProperty(FileProperty(name="OutputDirectory",defaultValue="",action=FileAction.Directory))
        self.copyProperties('AlignAndFocusPowderFromFiles', 'CacheDir')
        self.declareProperty("FinalDataUnits", "dSpacing", StringListValidator(["dSpacing","MomentumTransfer"]))

        workspace_prop = WorkspaceProperty('SplittersWorkspace', '', Direction.Input, PropertyMode.Optional)
        self.declareProperty(workspace_prop, "Splitters workspace for split event workspace.")
        # replaced for matrix workspace, SplittersWorkspace and table workspace
        # tableprop = ITableWorkspaceProperty("SplittersWorkspace", "", Direction.Input, PropertyMode.Optional)
        # self.declareProperty(tableprop, "Splitters workspace for split event workspace.")

        infotableprop = ITableWorkspaceProperty("SplitInformationWorkspace", "", Direction.Input, PropertyMode.Optional)
        self.declareProperty(infotableprop, "Name of table workspace containing information for splitters.")

        self.declareProperty("LowResolutionSpectraOffset", -1,
                             "If larger and equal to 0, then process low resolution TOF and offset is the spectra "
                             "number. Otherwise, ignored.")  # LowResolutionSpectraOffset

        self.declareProperty("NormalizeByCurrent", True, "Normalize by current")

        self.declareProperty("CompressTOFTolerance", 0.01, "Tolerance to compress events in TOF.")

        self.copyProperties('AlignAndFocusPowderFromFiles', ['FrequencyLogNames', 'WaveLengthLogNames'])

        return

    #pylint: disable=too-many-locals,too-many-branches,too-many-statements
    def PyExec(self):  # noqa
        """ Main execution body
        """
        # get generic information
        self._loadCharacterizations()
        self._resampleX = self.getProperty("ResampleX").value
        if self._resampleX != 0.:
            self._binning = [0.]
        else:
            self._binning = self.getProperty("Binning").value
            if len(self._binning) != 1 and len(self._binning) != 3:
                raise RuntimeError("Can only specify (width) or (start,width,stop) for binning. Found %d values." % len(self._binning))
            if len(self._binning) == 3:
                if self._binning[0] == 0. and self._binning[1] == 0. and self._binning[2] == 0.:
                    raise RuntimeError("Failed to specify the binning")
        self._bin_in_dspace = self.getProperty("BinInDspace").value
        self._filterBadPulses = self.getProperty("FilterBadPulses").value
        self._removePromptPulseWidth = self.getProperty("RemovePromptPulseWidth").value
        self._LRef = self.getProperty("UnwrapRef").value
        self._DIFCref = self.getProperty("LowResRef").value
        self._wavelengthMin = self.getProperty("CropWavelengthMin").value
        self._wavelengthMax = self.getProperty("CropWavelengthMax").value
        self._vanPeakFWHM = self.getProperty("VanadiumFWHM").value
        self._vanSmoothing = self.getProperty("VanadiumSmoothParams").value
        self._vanRadius = self.getProperty("VanadiumRadius").value
        self.calib = self.getProperty("CalibrationFile").value
        self._scaleFactor = self.getProperty("ScaleData").value
        self._outDir = self.getProperty("OutputDirectory").value
        self._outPrefix = self.getProperty("OutputFilePrefix").value.strip()
        self._outTypes = self.getProperty("SaveAs").value.lower()

        samRuns = self._getLinearizedFilenames("Filename")
        self._determineInstrument(samRuns[0])

        preserveEvents = self.getProperty("PreserveEvents").value
        if HAVE_MPI and preserveEvents:
            self.log().warning("preserveEvents set to False for MPI tasks.")
            preserveEvents = False
        self._info = None
        self._chunks = self.getProperty("MaxChunkSize").value

        # define splitters workspace and filter wall time
        self._splittersWS = self.getProperty("SplittersWorkspace").value
        if self._splittersWS is not None:
            # user specifies splitters workspace
            self.log().information("SplittersWorkspace is %s" % (str(self._splittersWS)))
            if len(samRuns) != 1:
                # TODO/FUTURE - This should be supported
                raise RuntimeError("Reducing data with splitters cannot happen when there are more than 1 sample run.")
            # define range of wall-time to import data
            sample_time_filter_wall = self._get_time_filter_wall(self._splittersWS.name(), samRuns[0])
            self.log().information("The time filter wall is %s" %(str(sample_time_filter_wall)))
        else:
            sample_time_filter_wall = (0.0, 0.0)
            self.log().information("SplittersWorkspace is None, and thus there is NO time filter wall. ")

        self._splitinfotablews = self.getProperty("SplitInformationWorkspace").value

        self._normalisebycurrent = self.getProperty("NormalizeByCurrent").value

        # Tolerance for compress TOF event.  If given a negative value, then use default 0.01
        self.COMPRESS_TOL_TOF = float(self.getProperty("CompressTOFTolerance").value)
        if self.COMPRESS_TOL_TOF < -0.:
            self.COMPRESS_TOL_TOF = 0.01

        # Process data
        # List stores the workspacename of all data workspaces that will be converted to d-spacing in the end.
        workspacelist = []
        samwksplist = []

        self._lowResTOFoffset = self.getProperty("LowResolutionSpectraOffset").value
        focuspos = self._focusPos
        if self._lowResTOFoffset >= 0:
            # Dealing with the parameters for editing instrument parameters
            if "PrimaryFlightPath" in focuspos:
                l1 = focuspos["PrimaryFlightPath"]
                if l1 > 0:
                    specids = focuspos['SpectrumIDs'][:]
                    l2s = focuspos['L2'][:]
                    polars = focuspos['Polar'][:]
                    phis = focuspos['Azimuthal'][:]

                    specids.extend(specids)
                    l2s.extend(l2s)
                    polars.extend(polars)
                    phis.extend(phis)

                    focuspos['SpectrumIDs'] = specids
                    focuspos['L2'] = l2s
                    focuspos['Polar'] = polars
                    focuspos['Azimuthal'] = phis
        # ENDIF

        # get the user-option whether an existing event workspace will be reloaded or not
        reload_event_file = self.getProperty('ReloadIfWorkspaceExists').value

        if self.getProperty("Sum").value and len(samRuns) > 1:
            self.log().information('Ignoring value of "Sum" property')
            # Sum input sample runs and then do reduction
            if self._splittersWS is not None:
                raise NotImplementedError("Summing spectra and filtering events are not supported simultaneously.")

            sam_ws_name = self._focusAndSum(samRuns, reload_if_loaded=reload_event_file,
                                            preserveEvents=preserveEvents)
            assert isinstance(sam_ws_name, str), 'Returned from _focusAndSum() must be a string but not' \
                                                 '%s. ' % str(type(sam_ws_name))

            workspacelist.append(sam_ws_name)
            samwksplist.append(sam_ws_name)
        # ENDIF (SUM)
        else:
            # Process each sample run
            for sam_run_number in samRuns:
                # first round of processing the sample
                self._info = None
                if sample_time_filter_wall[0] == 0. and sample_time_filter_wall[-1] == 0. \
                        and self._splittersWS is None:
                    returned = self._focusAndSum([sam_run_number], reload_if_loaded=reload_event_file,
                                                 preserveEvents=preserveEvents)
                else:
                    returned = self._focusChunks(sam_run_number, sample_time_filter_wall,
                                                 splitwksp=self._splittersWS,
                                                 reload_if_loaded=reload_event_file,
                                                 preserveEvents=preserveEvents)

                if isinstance(returned, list):
                    # Returned with a list of workspaces
                    focusedwksplist = returned
                    for sam_ws_name in focusedwksplist:
                        assert isinstance(sam_ws_name, str), 'Impossible to have a non-string value in ' \
                                                             'returned focused workspaces\' names.'
                        samwksplist.append(sam_ws_name)
                        workspacelist.append(sam_ws_name)
                    # END-FOR
                else:
                    # returned as a single workspace
                    sam_ws_name = returned
                    assert isinstance(sam_ws_name, str)
                    samwksplist.append(sam_ws_name)
                    workspacelist.append(sam_ws_name)
                # ENDIF
            # END-FOR
        # ENDIF (Sum data or not)

        for (samRunIndex, sam_ws_name) in enumerate(samwksplist):
            assert isinstance(sam_ws_name, str), 'Assuming that samRun is a string. But it is %s' % str(type(sam_ws_name))
            if is_event_workspace(sam_ws_name):
                self.log().information('Sample Run %s:  starting number of events = %d.' % (
                    sam_ws_name, get_workspace(sam_ws_name).getNumberEvents()))

            # Get run information
            self._info = self._getinfo(sam_ws_name)

            # process the container
            can_run_numbers = self._info["container"].value
            can_run_numbers = ['%s_%d' % (self._instrument, value) for value in can_run_numbers]
            can_run_ws_name = self._process_container_runs(can_run_numbers, samRunIndex, preserveEvents)
            if can_run_ws_name is not None:
                workspacelist.append(can_run_ws_name)

            # process the vanadium run
            van_run_number_list = self._info["vanadium"].value
            van_run_number_list = ['%s_%d' % (self._instrument, value) for value in van_run_number_list]
            van_specified = not noRunSpecified(van_run_number_list)
            if van_specified:
                van_run_ws_name = self._process_vanadium_runs(van_run_number_list, samRunIndex)
                workspacelist.append(van_run_ws_name)
            else:
                van_run_ws_name = None

            # return if MPI is used and there is more than 1 processor
            if mpiRank > 0:
                return

            # return if there is no sample run
            # Note: sample run must exist in logic
            # VZ: Discuss with Pete
            # if sam_ws_name == 0:
            #   return

            # the final bit of math to remove container run and vanadium run
            if can_run_ws_name is not None:
                # must convert the sample to a matrix workspace if the can run isn't one
                if not allEventWorkspaces(can_run_ws_name, sam_ws_name):
                    api.ConvertToMatrixWorkspace(InputWorkspace=sam_ws_name,
                                                 OutputWorkspace=sam_ws_name)

                # remove container run
                api.RebinToWorkspace(WorkspaceToRebin=can_run_ws_name,
                                     WorkspaceToMatch=sam_ws_name,
                                     OutputWorkspace=can_run_ws_name)
                api.Minus(LHSWorkspace=sam_ws_name,
                          RHSWorkspace=can_run_ws_name,
                          OutputWorkspace=sam_ws_name)
                # compress event if the sample run workspace is EventWorkspace
                if is_event_workspace(sam_ws_name) and self.COMPRESS_TOL_TOF > 0.:
                    api.CompressEvents(InputWorkspace=sam_ws_name,
                                       OutputWorkspace=sam_ws_name,
                                       Tolerance=self.COMPRESS_TOL_TOF)  # 10ns
                # canRun = str(canRun)

            if van_run_ws_name is not None:
                # subtract vanadium run from sample run by division
                num_hist_sam = get_workspace(sam_ws_name).getNumberHistograms()
                num_hist_van = get_workspace(van_run_ws_name).getNumberHistograms()
                assert num_hist_sam == num_hist_van, \
                    'Number of histograms of sample %d is not equal to van %d.' % (num_hist_sam, num_hist_van)
                api.Divide(LHSWorkspace=sam_ws_name,
                           RHSWorkspace=van_run_ws_name,
                           OutputWorkspace=sam_ws_name)
                normalized = True
                van_run_ws = get_workspace(van_run_ws_name)
                sam_ws = get_workspace(sam_ws_name)
                sam_ws.getRun()['van_number'] = van_run_ws.getRun()['run_number'].value
                # van_run_number = str(van_run_number)
            else:
                normalized = False

            # Compress the event again
            if is_event_workspace(sam_ws_name) and self.COMPRESS_TOL_TOF > 0.:
                api.CompressEvents(InputWorkspace=sam_ws_name,
                                   OutputWorkspace=sam_ws_name,
                                   Tolerance=self.COMPRESS_TOL_TOF)  # 5ns/

            # make sure there are no negative values - gsas hates them
            if self.getProperty("PushDataPositive").value != "None":
                addMin = self.getProperty("PushDataPositive").value == "AddMinimum"
                api.ResetNegatives(InputWorkspace=sam_ws_name,
                                   OutputWorkspace=sam_ws_name,
                                   AddMinimum=addMin,
                                   ResetValue=0.)

            # write out the files
            # FIXME - need documentation for mpiRank
            if mpiRank == 0:
                if self._scaleFactor != 1.:
                    api.Scale(sam_ws_name, Factor=self._scaleFactor, OutputWorkspace=sam_ws_name)
                self._save(sam_ws_name, self._info, normalized, False)

        # ENDFOR

        # convert everything into d-spacing as the final units
        if mpiRank == 0:
            workspacelist = set(workspacelist) # only do each workspace once
            for wksp in workspacelist:
                api.ConvertUnits(InputWorkspace=wksp, OutputWorkspace=wksp,
                                 Target=self.getProperty("FinalDataUnits").value)

                propertyName = "OutputWorkspace%s" % wksp
                if not self.existsProperty(propertyName):
                    self.declareProperty(WorkspaceProperty(propertyName, wksp, Direction.Output))
                self.setProperty(propertyName, wksp)

        return

    def _getLinearizedFilenames(self, propertyName):
        runnumbers = self.getProperty(propertyName).value
        linearizedRuns = []
        for item in runnumbers:
            if type(item) == list:
                linearizedRuns.extend(item)
            else:
                linearizedRuns.append(item)
        return linearizedRuns

    def _determineInstrument(self, filename):
        name = getBasename(filename)
        parts = name.split('_')
        self._instrument = ConfigService.getInstrument(parts[0]).shortName()  # only works for instruments without '_'

    def _loadCharacterizations(self):
        self._focusPos = {}
        charFilename = self.getProperty("CharacterizationRunsFile").value
        charFilename = ','.join(charFilename)
        expIniFilename = self.getProperty("ExpIniFilename").value

        self._charTable = ''
        if charFilename is None or len(charFilename) <= 0:
            self.iparmFile = None
            return

        self._charTable = 'characterizations'
        results = api.PDLoadCharacterizations(Filename=charFilename,
                                              ExpIniFilename=expIniFilename,
                                              OutputWorkspace=self._charTable)
        # export the characterizations table
        charTable = results[0]
        self.declareProperty(ITableWorkspaceProperty("CharacterizationsTable", self._charTable, Direction.Output))
        self.setProperty("CharacterizationsTable", charTable)

        # get the focus positions from the properties
        self.iparmFile = results[1]
        self._focusPos['PrimaryFlightPath'] = results[2]
        self._focusPos['SpectrumIDs'] = results[3]
        self._focusPos['L2'] = results[4]
        self._focusPos['Polar'] = results[5]
        self._focusPos['Azimuthal'] = results[6]

    #pylint: disable=too-many-branches
    def _load_event_data(self, filename, filter_wall=None, out_ws_name=None, reload_if_loaded=True, **chunk):
        """ Load data optionally by chunk strategy
        Purpose:
            Load a complete or partial run, filter bad pulses.
            Output workspace name is formed as
            - user specified (highest priority)
            - instrument-name_run-number_0 (no chunking)
            - instrument-name_run-number_X: X is either ChunkNumber of SpectrumMin
        Requirements:
            1. run number is integer and larger than 0
        Guarantees:
            A workspace is created with name described above
        :param filename:
        :param filter_wall:
        :param out_ws_name: name of output workspace specified by user. it will override the automatic name
        :param reload_if_loaded:
        :param chunk:
        :return:
        """
        # Check requirements
        assert len(filename) > 0, "Input file '%s' does not exist" % filename
        assert (chunk is None) or isinstance(chunk, dict), 'Input chunk must be either a dictionary or None.'

        # get event file's base name
        base_name = getBasename(filename)

        # give out the default output workspace name
        if out_ws_name is None:
            if chunk:
                if "ChunkNumber" in chunk:
                    out_ws_name = base_name + "__chk%d" % (int(chunk["ChunkNumber"]))
                elif "SpectrumMin" in chunk:
                    seq_number = 1 + int(chunk["SpectrumMin"])/(int(chunk["SpectrumMax"])-int(chunk["SpectrumMin"]))
                    out_ws_name = base_name + "__chk%d" % seq_number
            else:
                out_ws_name = "%s_%d" % (base_name, 0)
        # END-IF

        # Specify the other chunk information including Precount, FilterByTimeStart and FilterByTimeStop.
        if filename.endswith("_event.nxs") or filename.endswith(".nxs.h5"):
            chunk["Precount"] = True
            if filter_wall is not None:
                if filter_wall[0] > 0.:
                    chunk["FilterByTimeStart"] = filter_wall[0]
                if filter_wall[1] > 0.:
                    chunk["FilterByTimeStop"] = filter_wall[1]

        # Call Mantid's Load algorithm to load complete or partial data
        api.Load(Filename=filename, OutputWorkspace=out_ws_name, **chunk)

        # Log output
        if is_event_workspace(out_ws_name):
            self.log().debug("Load run %s: number of events = %d. " % (os.path.split(filename)[-1],
                                                                       get_workspace(out_ws_name).getNumberEvents()))
        if HAVE_MPI:
            msg = "MPI Task = %s ;" % (str(mpiRank))
            if is_event_workspace(out_ws_name):
                msg += "Number Events = " + str(get_workspace(out_ws_name).getNumberEvents())
            self.log().debug(msg)

        # filter bad pulses
        if self._filterBadPulses > 0.:
            # record number of events of the original workspace
            if is_event_workspace(out_ws_name):
                # Event workspace: record original number of events
                num_original_events = get_workspace(out_ws_name).getNumberEvents()
            else:
                num_original_events = -1

            # filter bad pulse
            api.FilterBadPulses(InputWorkspace=out_ws_name, OutputWorkspace=out_ws_name,
                                LowerCutoff=self._filterBadPulses)

            if is_event_workspace(out_ws_name):
                # Event workspace
                message = "FilterBadPulses reduces number of events from %d to %d (under %s percent) " \
                          "of workspace %s." % (num_original_events, get_workspace(out_ws_name).getNumberEvents(),
                                                str(self._filterBadPulses), out_ws_name)
                self.log().information(message)
        # END-IF (filter bad pulse)

        return out_ws_name

    def _getStrategy(self, filename):
        """
        Get chunking strategy by calling mantid algorithm 'DetermineChunking'
        :param filename:
        :return: a list of dictionary.  Each dictionary is a row in table workspace
        """
        # Determine chunk strategy can search in archive.
        # Therefore this will fail: assert os.path.exists(file_name), 'NeXus file %s does not exist.' % file_name
        self.log().debug("[Fx116] Run file Name : %s,\t\tMax chunk size: %s" %
                         (filename, str(self._chunks)))
        chunks = api.DetermineChunking(Filename=filename, MaxChunkSize=self._chunks)

        strategy = []
        for row in chunks:
            strategy.append(row)

        # For table with no rows
        if len(strategy) == 0:
            strategy.append({})

        # delete chunks workspace
        chunks = str(chunks)
        mtd.remove(chunks)

        return strategy

    def __logChunkInfo(self, chunk):
        keys = sorted(chunk.keys())

        keys = [str(key) + "=" + str(chunk[key]) for key in keys]
        self.log().information("Working on chunk [" + ", ".join(keys) + "]")

    def checkInfoMatch(self, left, right):
        if (left["frequency"].value is not None) and (right["frequency"].value is not None) \
           and (abs(left["frequency"].value - right["frequency"].value)/left["frequency"].value > .05):
            raise RuntimeError("Cannot add incompatible frequencies (%f!=%f)"
                               % (left["frequency"].value, right["frequency"].value))
        if (left["wavelength"].value is not None) and (right["wavelength"].value is not None) \
                and abs(left["wavelength"].value - right["wavelength"].value)/left["wavelength"].value > .05:
            raise RuntimeError("Cannot add incompatible wavelengths (%f != %f)"
                               % (left["wavelength"].value, right["wavelength"].value))

    def _loadAndSum(self, filename_list, outName):
        """
        Load and sum
        Purpose:

        Requirements:
        1. run number are integers
        Guarantees:

        :param filename_list: list of filenames
        :param outName:
        :return:
        """
        # Check requirements
        assert isinstance(filename_list, list), \
            'Run number list is not a list but of type %s' % str(type(filename_list))

        # Form output workspaces' names
        out_ws_name_list = ['%s_loadsum' % getBasename(filename) for filename in filename_list]

        sample_ws_name = None
        info = None

        # for ws_name in out_ws_name_list:
        for (filename, ws_name) in zip(filename_list, out_ws_name_list):
            self.log().debug("[Sum] processing %s" % filename)
            api.LoadEventAndCompress(Filename=filename,
                                     OutputWorkspace=ws_name,
                                     MaxChunkSize=self._chunks,
                                     FilterBadPulses=self._filterBadPulses,
                                     CompressTOFTolerance=self.COMPRESS_TOL_TOF)
            if is_event_workspace(ws_name):
                self.log().notice('Load event file %s, compress it and get %d events.' %
                                  (filename, get_workspace(ws_name).getNumberEvents()))

            tempinfo = self._getinfo(ws_name)
            if sample_ws_name is None:
                # sample run workspace is not set up.
                sample_ws_name = ws_name
                self._info = tempinfo
            else:
                # there is sample run workspace set up previously, then add current one to previous for summation
                self.checkInfoMatch(info, tempinfo)

                api.Plus(LHSWorkspace=sample_ws_name,
                         RHSWorkspace=ws_name,
                         OutputWorkspace=sample_ws_name,
                         ClearRHSWorkspace=allEventWorkspaces(sample_ws_name, ws_name))
                api.DeleteWorkspace(ws_name)

                # compress events
                if is_event_workspace(sample_ws_name) and self.COMPRESS_TOL_TOF > 0.:
                    api.CompressEvents(InputWorkspace=sample_ws_name, OutputWorkspace=sample_ws_name,
                                       Tolerance=self.COMPRESS_TOL_TOF)  # 10ns
        # END-FOR

        # Normalize by current with new name
        if self._normalisebycurrent is True:
            self.log().information('Normalize current to workspace %s' % sample_ws_name)
            # temp_ws = self.get_workspace(sample_ws_name)
            if not (is_event_workspace(sample_ws_name) and get_workspace(sample_ws_name).getNumberEvents() == 0):
                api.NormaliseByCurrent(InputWorkspace=sample_ws_name,
                                       OutputWorkspace=sample_ws_name,
                                       RecalculatePCharge=True)
                get_workspace(sample_ws_name).getRun()['gsas_monitor'] = 1
            # END-IF
        # ENDI-IF

        # Rename to user specified output workspace
        if sample_ws_name != outName:
            api.RenameWorkspace(InputWorkspace=sample_ws_name,
                                OutputWorkspace=outName)

        return outName

    #pylint: disable=too-many-arguments
    def _focusAndSum(self, filenames, preserveEvents=True, reload_if_loaded=True, final_name=None):
        """Load, sum, and focus data in chunks
        Purpose:
            Load, sum and focus data in chunks;
        Requirements:
            1. input run numbers are in numpy array or list
        Guarantees:
            The experimental runs are focused and summed together
        @param run_number_list:
        @param extension:
        @param preserveEvents:
        @return: string as the summed workspace's name
        """
        if final_name is None:
            final_name = getBasename(filenames[0])
        api.AlignAndFocusPowderFromFiles(Filename=','.join(filenames),
                                         OutputWorkspace=final_name,
                                         MaxChunkSize=self._chunks,
                                         FilterBadPulses=self._filterBadPulses,
                                         Characterizations=self._charTable,
                                         CacheDir=self.getProperty("CacheDir").value,
                                         CalFileName=self.calib,
                                         GroupFilename=self.getProperty("GroupingFile").value,
                                         Params=self._binning,
                                         ResampleX=self._resampleX,
                                         Dspacing=self._bin_in_dspace,
                                         PreserveEvents=preserveEvents,
                                         RemovePromptPulseWidth=self._removePromptPulseWidth,
                                         CompressTolerance=self.COMPRESS_TOL_TOF,
                                         UnwrapRef=self._LRef,
                                         LowResRef=self._DIFCref,
                                         LowResSpectrumOffset=self._lowResTOFoffset,
                                         CropWavelengthMin=self._wavelengthMin,
                                         CropWavelengthMax=self._wavelengthMax,
                                         FrequencyLogNames=self.getProperty("FrequencyLogNames").value,
                                         WaveLengthLogNames=self.getProperty("WaveLengthLogNames").value,
                                         ReductionProperties="__snspowderreduction_inner",
                                         **self._focusPos)

        #TODO make sure that this funny function is called
        #self.checkInfoMatch(info, tempinfo)

        if self._normalisebycurrent is True:
            api.NormaliseByCurrent(InputWorkspace=final_name,
                                   OutputWorkspace=final_name,
                                   RecalculatePCharge=True)
            get_workspace(final_name).getRun()['gsas_monitor'] = 1

        return final_name

    #pylint: disable=too-many-arguments,too-many-locals,too-many-branches
    def _focusChunks(self, filename, filter_wall=(0.,0.),  # noqa
                     splitwksp=None, preserveEvents=True,
                     reload_if_loaded=True):  # noqa
        """
        Load, (optional) split and focus data in chunks
        @param filename: integer for run number
        @param filter_wall:  Enabled if splitwksp is defined
        @param splitwksp: SplittersWorkspace (if None then no split)
        @param preserveEvents:
        @return: a string as the returned workspace's name or a list of strings as the returned workspaces' names
                 in the case that split workspace is used.
        """
        # generate the workspace name
        self.log().information("_focusChunks(): run = %s" % (filename))

        # get chunk strategy for parallel processing (MPI)
        strategy = self._getStrategy(filename)

        # determine event splitting by checking filterWall and number of output workspaces from _focusChunk
        do_split_raw_wksp, num_out_wksp = self._determine_workspace_splitting(splitwksp, filter_wall)

        # Set up the data structure to hold and control output workspaces
        output_wksp_list = [None] * num_out_wksp
        is_first_chunk_list = [True] * num_out_wksp

        self.log().debug("F1141A: Number of workspace to process = %d" % num_out_wksp)

        # reduce data by chunks
        chunk_index = -1
        base_name = getBasename(filename)
        for chunk in strategy:
            # progress on chunk index
            chunk_index += 1
            # Load chunk, i.e., partial data into Mantid
            raw_ws_name_chunk = self._load_event_data(filename, filter_wall,
                                                      out_ws_name=None,
                                                      reload_if_loaded=reload_if_loaded,
                                                      **chunk)

            if self._info is None:
                self._info = self._getinfo(raw_ws_name_chunk)

            # Log information for current chunk
            self.__logChunkInfo(chunk)
            if is_event_workspace(raw_ws_name_chunk):
                # Event workspace
                self.log().debug("F1141C: There are %d events after data is loaded in workspace %s." % (
                    get_workspace(raw_ws_name_chunk).getNumberEvents(), raw_ws_name_chunk))

            # Split the workspace if it is required
            if do_split_raw_wksp is True:
                output_ws_name_list = self._split_workspace(raw_ws_name_chunk, splitwksp.name())
            else:
                # Histogram data
                output_ws_name_list = [raw_ws_name_chunk]
            # ENDIF
            # check
            if num_out_wksp != len(output_ws_name_list):
                self.log().warning('Projected number of output workspaces %d must be same as '
                                   'that of real output workspaces %d.' % (num_out_wksp, len(output_ws_name_list)))
                num_out_wksp = output_ws_name_list

            # log
            msg = "[Fx1142] Workspace of chunk %d is %d (vs. estimated %d). \n" % (
                chunk_index, len(output_ws_name_list), num_out_wksp)
            for iws in range(len(output_ws_name_list)):
                ws = output_ws_name_list[iws]
                msg += "%s\t\t" % (str(ws))
                if iws %5 == 4:
                    msg += "\n"
            self.log().debug(msg)

            # Do align and focus
            num_out_wksp = len(output_ws_name_list)
            for split_index in range(num_out_wksp):
                # Get workspace name
                out_ws_name_chunk_split = output_ws_name_list[split_index]
                # Align and focus
                # focuspos = self._focusPos
                self.log().notice('Align and focus workspace %s' % out_ws_name_chunk_split)
                api.AlignAndFocusPowder(InputWorkspace=out_ws_name_chunk_split,
                                        OutputWorkspace=out_ws_name_chunk_split,
                                        CalFileName=self.calib,
                                        GroupFilename=self.getProperty("GroupingFile").value,
                                        Params=self._binning,
                                        ResampleX=self._resampleX,
                                        Dspacing=self._bin_in_dspace,
                                        PreserveEvents=preserveEvents,
                                        RemovePromptPulseWidth=self._removePromptPulseWidth,
                                        CompressTolerance=self.COMPRESS_TOL_TOF,
                                        UnwrapRef=self._LRef,
                                        LowResRef=self._DIFCref,
                                        LowResSpectrumOffset=self._lowResTOFoffset,
                                        CropWavelengthMin=self._wavelengthMin,
                                        CropWavelengthMax=self._wavelengthMax,
                                        ReductionProperties="__snspowderreduction",
                                        **self._focusPos)
                # logging (ignorable)
                if is_event_workspace(out_ws_name_chunk_split):
                    self.log().information('After being aligned and focused, workspace %s: Number of events = %d '
                                           'of chunk %d ' % (out_ws_name_chunk_split,
                                                             get_workspace(out_ws_name_chunk_split).getNumberEvents(),
                                                             chunk_index))
            # END-FOR-Splits

            # Merge among chunks
            for split_index in range(num_out_wksp):
                # determine the final workspace name
                final_out_ws_name = base_name
                if num_out_wksp > 1:
                    final_out_ws_name += '_%d' % split_index

                if is_first_chunk_list[split_index] is True:
                    # Rename if it is the first chunk that is finished
                    self.log().debug("[F1145] Slot %d is renamed to %s" % (split_index, final_out_ws_name))
                    api.RenameWorkspace(InputWorkspace=output_ws_name_list[split_index],
                                        OutputWorkspace=final_out_ws_name)

                    output_wksp_list[split_index] = final_out_ws_name
                    is_first_chunk_list[split_index] = False
                else:
                    # Add this chunk of workspace to the final one
                    clear_rhs_ws = allEventWorkspaces(output_wksp_list[split_index], output_ws_name_list[split_index])
                    api.Plus(LHSWorkspace=output_wksp_list[split_index],
                             RHSWorkspace=output_ws_name_list[split_index],
                             OutputWorkspace=output_wksp_list[split_index],
                             ClearRHSWorkspace=clear_rhs_ws)

                    # Delete the chunk workspace
                    api.DeleteWorkspace(output_ws_name_list[split_index])
                # END-IF-ELSE
        # END-FOR-Chunks

        for item in output_wksp_list:
            assert isinstance(item, str)

        # Sum workspaces for all mpi tasks
        if HAVE_MPI:
            for split_index in range(num_out_wksp):
                api.GatherWorkspaces(InputWorkspace=output_wksp_list[split_index],
                                     PreserveEvents=preserveEvents,
                                     AccumulationMethod="Add",
                                     OutputWorkspace=output_wksp_list[split_index])
        # ENDIF MPI

        if self._chunks > 0:
            # When chunks are added, proton charge is summed for all chunks
            for split_index in range(num_out_wksp):
                get_workspace(output_wksp_list[split_index]).getRun().integrateProtonCharge()
        # ENDIF

        if (self.iparmFile is not None) and (len(self.iparmFile) > 0):
            # When chunks are added, add iparamFile
            for split_index in range(num_out_wksp):
                get_workspace(output_wksp_list[split_index]).getRun()['iparm_file'] = self.iparmFile

        # Compress events
        for split_index in range(num_out_wksp):
            if is_event_workspace(output_wksp_list[split_index]) and self.COMPRESS_TOL_TOF > 0.:
                api.CompressEvents(InputWorkspace=output_wksp_list[split_index],
                                   OutputWorkspace=output_wksp_list[split_index],
                                   Tolerance=self.COMPRESS_TOL_TOF)  # 100ns
            try:
                if self._normalisebycurrent is True:
                    api.NormaliseByCurrent(InputWorkspace=output_wksp_list[split_index],
                                           OutputWorkspace=output_wksp_list[split_index],
                                           RecalculatePCharge=True)
                    get_workspace(output_wksp_list[split_index]).getRun()['gsas_monitor'] = 1
            except RuntimeError as e:
                self.log().warning(str(e))

            propertyName = "OutputWorkspace%s" % str(output_wksp_list[split_index])
            self.declareProperty(WorkspaceProperty(propertyName, str(output_wksp_list[split_index]), Direction.Output))
            self.setProperty(propertyName, output_wksp_list[split_index])
            self._save(output_wksp_list[split_index], self._info, False, True)
            self.log().information("Done focussing data of %d." % (split_index))

        self.log().information("[E1207] Number of workspace in workspace list after clean = %d. " %(len(output_wksp_list)))

        for item in output_wksp_list:
            assert isinstance(item, str)

        # About return
        if splitwksp is None:
            return output_wksp_list[0]
        else:
            return output_wksp_list

    def _getinfo(self, wksp_name):
        """ Get characterization information of a certain workspace
        Purpose:

        Requirements:
        1. wksp_name is string and AnalysisDataService has such workspace
        Guarantees:

        :param wksp_name:
        :return:
        """
        # Check requirements
        assert isinstance(wksp_name, str)
        assert self.does_workspace_exist(wksp_name)

        # Reset characterization run numbers in the property manager
        if PropertyManagerDataService.doesExist('__snspowderreduction'):
            PropertyManagerDataService.remove('__snspowderreduction')

        # Determine characterization
        api.PDDetermineCharacterizations(InputWorkspace=wksp_name,
                                         Characterizations=self._charTable,
                                         ReductionProperties="__snspowderreduction",
                                         BackRun=self.getProperty("BackgroundNumber").value,
                                         NormRun=self.getProperty("VanadiumNumber").value,
                                         NormBackRun=self.getProperty("VanadiumBackgroundNumber").value,
                                         FrequencyLogNames=self.getProperty("FrequencyLogNames").value,
                                         WaveLengthLogNames=self.getProperty("WaveLengthLogNames").value)

        # convert the result into a dict
        return PropertyManagerDataService.retrieve("__snspowderreduction")

    def _save(self, wksp, info, normalized, pdfgetn):
        prefix = str(wksp)
        if len(self._outPrefix) > 0: # non-empty string
            prefix = self._outPrefix + str(wksp)
        filename = os.path.join(self._outDir, prefix)
        if pdfgetn:
            if "pdfgetn" in self._outTypes:
                self.log().notice("Saving 'pdfgetn' is deprecated. Use PDtoPDFgetN instead.")
                pdfwksp = str(wksp)+"_norm"
                api.SetUncertainties(InputWorkspace=wksp, OutputWorkspace=pdfwksp, SetError="sqrt")
                api.SaveGSS(InputWorkspace=pdfwksp, Filename=filename+".getn", SplitFiles=False, Append=False,
                            MultiplyByBinWidth=False, Bank=info["bank"].value, Format="SLOG", ExtendedHeader=True)
                api.DeleteWorkspace(pdfwksp)
            return # don't do the other bits of saving
        if "gsas" in self._outTypes:
            api.SaveGSS(InputWorkspace=wksp, Filename=filename+".gsa", SplitFiles=False, Append=False,
                        MultiplyByBinWidth=normalized, Bank=info["bank"].value, Format="SLOG", ExtendedHeader=True)
        if "fullprof" in self._outTypes:
            api.SaveFocusedXYE(InputWorkspace=wksp, StartAtBankNumber=info["bank"].value, Filename=filename+".dat")
        if "topas" in self._outTypes:
            api.SaveFocusedXYE(InputWorkspace=wksp, StartAtBankNumber=info["bank"].value, Filename=filename+".xye",
                               Format="TOPAS")
        if "nexus" in self._outTypes:
            api.ConvertUnits(InputWorkspace=wksp, OutputWorkspace=wksp, Target=self.getProperty("FinalDataUnits").value)
            api.SaveNexus(InputWorkspace=wksp, Filename=filename+".nxs")

        # always save python script - this is broken because the history isn't
        # attached until the algorithm is finished
        api.GeneratePythonScript(InputWorkspace=wksp, Filename=filename+".py")

        return

    @staticmethod
    def _get_time_filter_wall(split_ws_name, filename):
        """ Get filter wall times (relative time in seconds to run start time)from splitter workspace, i.e.,
        get the earliest and latest TIME stamp in input splitter workspace

        Arguments:
         - split_ws_name: splitters workspace
         - filename: file name

        Return: tuple of start-time and stop-time relative to run start time and in unit of second
                If there is no split workspace defined, filter is (0., 0.) as the default
        """
        # supported case: support both workspace and workspace name
        assert isinstance(split_ws_name, str) and len(split_ws_name) > 0,\
            'SplittersWorkspace {0} must be a non-empty string but not a {1}.' \
            ''.format(split_ws_name, type(split_ws_name))
        if AnalysisDataService.doesExist(split_ws_name):
            split_ws = get_workspace(split_ws_name)
        else:
            raise RuntimeError('Splitting workspace {0} cannot be found in ADS.'.format(split_ws_name))

        # get the filter wall time according to type of splitting workspace
        if isinstance(split_ws, MatrixWorkspace):
            # matrix workspace: nano seconds of epoch time
            filter_start_time = split_ws.readX(0)[0] * 1.E-9
            filter_stop_time = split_ws.readX(0)[-1] * 1.E-9

        elif isinstance(split_ws, SplittersWorkspace):
            # splitters workspace: filter start and stop time are in "nano-seconds" and epoch time
            numrow = split_ws.rowCount()

            # Searching for the
            filter_start_time = split_ws.cell(0, 0)
            filter_stop_time = split_ws.cell(0, 1)

            for r in range(1, numrow):
                timestart = split_ws.cell(r, 0)
                timeend = split_ws.cell(r, 1)
                if timestart < filter_start_time:
                    filter_start_time = timestart
                if timeend > filter_stop_time:
                    filter_stop_time = timeend
            # END-FOR

            # convert to seconds
            filter_start_time *= 1.0E-9
            filter_stop_time *= 1.0E-9

        elif isinstance(split_ws, ITableWorkspace):
            # general table workspace: filter start and stop times are in seconds
            if split_ws.columnCount() < 3:
                raise RuntimeError('Table splitters workspace {0} has too few ({1}) columns.'
                                   ''.format(split_ws_name, split_ws.columnCount()))

            num_rows = split_ws.rowCount()
            # Searching for the table
            filter_start_time = split_ws.cell(0, 0)
            filter_stop_time = split_ws.cell(0, 1)

            for r in range(1, num_rows):
                split_start_time = split_ws.cell(r, 0)
                split_stop_time = split_ws.cell(r, 1)
                filter_start_time = min(filter_start_time, split_start_time)
                filter_stop_time = max(filter_stop_time, split_stop_time)
            # END-FOR

        else:
            # unsupported case
            raise RuntimeError('Input splitters workspace is not a supported type.')

        # END-IF-ELSE

        # test relative time or not
        if filter_start_time > 1 * 356 * 24 * 3600:
            # start time is more than 1 years. then it must be an epoch time
            # Load meta data to determine wall time
            meta_ws_name = "temp_" + getBasename(filename)
            api.Load(Filename=str(filename), OutputWorkspace=str(meta_ws_name), MetaDataOnly=True)
            meta_ws = get_workspace(meta_ws_name)

            # Get start time
            run_start_ns = meta_ws.getRun().startTime().totalNanoseconds()

            # reset the time
            filter_start_time -= run_start_ns * 1.E-9
            filter_stop_time -= run_start_ns * 1.E-9

            api.DeleteWorkspace(Workspace=meta_ws_name)
        # END-IF

        filer_wall = filter_start_time, filter_stop_time

        return filer_wall

    def getNumberOfSplittedWorkspace(self, splitwksp):
        """ Get number of splitted workspaces due to input splitwksp

        Return : integer
        """
        # get handle on splitting workspace
        if isinstance(splitwksp, str):
            split_ws = AnalysisDataService.retrieve(splitwksp)
        else:
            split_ws = splitwksp

        # get information
        if isinstance(split_ws, SplittersWorkspace):
            # SplittersWorkspace
            numrows = split_ws.rowCount()
            wscountdict = {}
            for r in range(numrows):
                wsindex = split_ws.cell(r, 2)
                wscountdict[wsindex] = 0
            num_output_ws = len(list(wscountdict.keys()))
            num_splitters = split_ws.rowCount()

        elif isinstance(split_ws, MatrixWorkspace):
            # case as MatrixWorkspace splitter
            vec_y = split_ws.readY(0)
            set_y = set()
            for y in vec_y:
                int_y = int(y+0.1)
                set_y.add(int_y)
            num_output_ws = len(set_y)
            num_splitters = len(vec_y)

        elif isinstance(split_ws, ITableWorkspace):
            # case as a general TableWorkspace splitter
            num_rows = split_ws.rowCount()
            target_ws_set = set()
            for r in range(num_rows):
                target_ws = split_ws.cell(r, 2)
                target_ws_set.add(target_ws)
            num_output_ws = len(target_ws_set)
            num_splitters = num_rows

        else:
            # Exceptions
            raise NotImplementedError('Splitters workspace of type {0} is not supported.'.format(type(split_ws)))

        return num_output_ws, num_splitters

    @staticmethod
    def does_workspace_exist(workspace_name):
        """
        Purpose: Check whether a workspace exists in AnalysisDataService
        :param workspace_name:
        :return:
        """
        assert isinstance(workspace_name, str)

        return AnalysisDataService.doesExist(workspace_name)

    def _determine_workspace_splitting(self, split_wksp, filter_wall):
        """
        :return: do_split_raw_wksp, num_out_wksp
        """
        do_split_raw_wksp = False
        num_out_wksp = 1

        assert not isinstance(split_wksp, str), 'Input split workspace cannot be a string.'

        if split_wksp is not None:
            # Use splitting workspace

            # Check consistency with filterWall
            if filter_wall[0] < 1.0E-20 and filter_wall[1] < 1.0E-20:
                # Default definition of filterWall when there is no split workspace specified.
                raise RuntimeError('It is impossible to have a splitters workspace and a non-defined, i.e., (0,0) time '
                                   'filter wall.')
            # ENDIF

            # Note: Unfiltered workspace (remainder) is not considered here
            num_out_wksp, num_splitters = self.getNumberOfSplittedWorkspace(split_wksp)
            # num_out_wksp = self.getNumberOfSplittedWorkspace(split_wksp)
            # num_splitters = split_wksp.rowCount()

            # Do explicit FilterEvents if number of splitters is larger than 1.
            # If number of splitters is equal to 1, then filterWall will do the job itself.
            if num_splitters > 1:
                do_split_raw_wksp = True
            self.log().debug("[Fx948] Number of split workspaces = %d; Do split = %s" % (num_out_wksp, str(do_split_raw_wksp)))
        # ENDIF

        return do_split_raw_wksp, num_out_wksp

    def _process_container_runs(self, can_run_numbers, samRunIndex, preserveEvents):
        """ Process container runs
        :param can_run_numbers:
        :return:
        """
        assert isinstance(samRunIndex, int)

        if noRunSpecified(can_run_numbers):
            # no container run is specified
            can_run_ws_name = None
        else:
            # reduce container run such that it can be removed from sample run

            if len(can_run_numbers) == 1:
                # only 1 container run
                can_run_number = can_run_numbers[0]
            else:
                # in case of multiple container run, use the corresponding one to sample
                can_run_number = can_run_numbers[samRunIndex]

            # get reference to container run
            can_run_ws_name = getBasename(can_run_number)
            if self.does_workspace_exist(can_run_ws_name) is True:
                # container run exists to get reference from mantid
                api.ConvertUnits(InputWorkspace=can_run_ws_name,
                                 OutputWorkspace=can_run_ws_name,
                                 Target="TOF")
            else:
                fileArg = [can_run_number]
                if self.getProperty("Sum").value:
                    fileArg = can_run_numbers
                self._focusAndSum(fileArg, preserveEvents, final_name=can_run_ws_name)

                # smooth background
                smoothParams = self.getProperty("BackgroundSmoothParams").value
                if smoothParams is not None and len(smoothParams) > 0:
                    api.FFTSmooth(InputWorkspace=can_run_ws_name,
                                  OutputWorkspace=can_run_ws_name,
                                  Filter="Butterworth",
                                  Params=smoothParams,
                                  IgnoreXBins=True,
                                  AllSpectra=True)

            # END-IF-ELSE
        # END-IF (can run)

        return can_run_ws_name

    def _process_vanadium_runs(self, van_run_number_list, samRunIndex, **dummy_focuspos):
        """
        Purpose: process vanadium runs
        Requirements: if more than 1 run in given run number list, then samRunIndex must be given.
        Guarantees: have vanadium run reduced.
        :param van_run_number_list: list of vanadium run
        :param timeFilterWall: time filter wall
        :param samRunIndex: sample run index
        :param focuspos:
        :return:
        """
        # get the right van run number to this sample
        if len(van_run_number_list) == 1:
            van_run_number = van_run_number_list[0]
        else:
            assert isinstance(samRunIndex, int)
            van_run_number = van_run_number_list[samRunIndex]

        # get handle on workspace of this van run and make sure its unit is T.O.F
        if van_run_number in mtd:
            # use the existing vanadium
            van_run_ws_name = getBasename(van_run_number)
            api.ConvertUnits(InputWorkspace=van_run_ws_name,
                             OutputWorkspace=van_run_ws_name,
                             Target="TOF")
        else:
            # Explicitly load, reduce and correct vanadium runs

            # load the vanadium
            van_run_ws_name = getBasename(van_run_number)
            if self.getProperty("Sum").value:
                van_run_ws_name = self._loadAndSum(van_run_number_list, van_run_ws_name)
            else:
                van_run_ws_name = self._loadAndSum([van_run_number], van_run_ws_name)

            # load the vanadium background (if appropriate)
            van_bkgd_run_number_list = self._info["vanadium_background"].value
            van_bkgd_run_number_list = ['%s_%d' % (self._instrument, value)
                                        for value in van_bkgd_run_number_list]
            if not noRunSpecified(van_bkgd_run_number_list):
                # determine the van background workspace name
                if len(van_bkgd_run_number_list) == 1:
                    van_bkgd_run_number = van_bkgd_run_number_list[0]
                else:
                    van_bkgd_run_number = van_bkgd_run_number_list[samRunIndex]
                van_bkgd_ws_name = getBasename(van_bkgd_run_number) + "_vanbg"

                # load background runs and sum if necessary
                if self.getProperty("Sum").value:
                    van_bkgd_ws_name = self._loadAndSum(van_bkgd_run_number_list, van_bkgd_ws_name)
                else:
                    van_bkgd_ws_name = self._loadAndSum([van_bkgd_run_number], van_bkgd_ws_name)

                van_bkgd_ws = get_workspace(van_bkgd_ws_name)
                if van_bkgd_ws.id() == EVENT_WORKSPACE_ID and van_bkgd_ws.getNumberEvents() <= 0:
                    # skip if background run is empty
                    pass
                else:
                    clear_rhs_ws = allEventWorkspaces(van_run_ws_name, van_bkgd_ws_name)
                    api.Minus(LHSWorkspace=van_run_ws_name,
                              RHSWorkspace=van_bkgd_ws_name,
                              OutputWorkspace=van_run_ws_name,
                              ClearRHSWorkspace=clear_rhs_ws)
                # remove vanadium background workspace
                api.DeleteWorkspace(Workspace=van_bkgd_ws_name)
            # END-IF (vanadium background)

            # compress events
            if is_event_workspace(van_run_ws_name) and self.COMPRESS_TOL_TOF > 0.:
                api.CompressEvents(InputWorkspace=van_run_ws_name,
                                   OutputWorkspace=van_run_ws_name,
                                   Tolerance=self.COMPRESS_TOL_TOF)  # 10ns

            # do the absorption correction
            api.ConvertUnits(InputWorkspace=van_run_ws_name,
                             OutputWorkspace=van_run_ws_name,
                             Target="Wavelength")

            # set material as Vanadium and correct for multiple scattering
            api.SetSampleMaterial(InputWorkspace=van_run_ws_name,
                                  ChemicalFormula="V",
                                  SampleNumberDensity=0.0721)
            api.CarpenterSampleCorrection(InputWorkspace=van_run_ws_name,
                                          OutputWorkspace=van_run_ws_name,
                                          CylinderSampleRadius=self._vanRadius)

            # convert unit to T.O.F.
            api.ConvertUnits(InputWorkspace=van_run_ws_name,
                             OutputWorkspace=van_run_ws_name,
                             Target="TOF")

            # focus the data
            self.log().warning('Reducing vanadium run %s.' % van_run_ws_name)
            api.AlignAndFocusPowder(InputWorkspace=van_run_ws_name,
                                    OutputWorkspace=van_run_ws_name,
                                    CalFileName=self.calib,
                                    GroupFilename=self.getProperty("GroupingFile").value,
                                    Params=self._binning,
                                    ResampleX=self._resampleX,
                                    Dspacing=self._bin_in_dspace,
                                    RemovePromptPulseWidth=self._removePromptPulseWidth,
                                    CompressTolerance=self.COMPRESS_TOL_TOF,
                                    UnwrapRef=self._LRef, LowResRef=self._DIFCref,
                                    LowResSpectrumOffset=self._lowResTOFoffset,
                                    CropWavelengthMin=self._wavelengthMin,
                                    CropWavelengthMax=self._wavelengthMax,
                                    ReductionProperties="__snspowderreduction", **self._focusPos)

            # convert to d-spacing and do strip vanadium peaks
            if self.getProperty("StripVanadiumPeaks").value:
                api.ConvertUnits(InputWorkspace=van_run_ws_name,
                                 OutputWorkspace=van_run_ws_name,
                                 Target="dSpacing")

                api.StripVanadiumPeaks(InputWorkspace=van_run_ws_name,
                                       OutputWorkspace=van_run_ws_name,
                                       FWHM=self._vanPeakFWHM,
                                       PeakPositionTolerance=self.getProperty("VanadiumPeakTol").value,
                                       BackgroundType="Quadratic",
                                       HighBackground=True)
            # END-IF (strip peak)

            # convert unit to T.O.F., smooth vanadium, reset uncertainties and make sure the unit is TOF
            api.ConvertUnits(InputWorkspace=van_run_ws_name,
                             OutputWorkspace=van_run_ws_name,
                             Target="TOF")
            api.FFTSmooth(InputWorkspace=van_run_ws_name,
                          OutputWorkspace=van_run_ws_name,
                          Filter="Butterworth",
                          Params=self._vanSmoothing,
                          IgnoreXBins=True,
                          AllSpectra=True)
            api.SetUncertainties(InputWorkspace=van_run_ws_name, OutputWorkspace=van_run_ws_name)
            api.ConvertUnits(InputWorkspace=van_run_ws_name,
                             OutputWorkspace=van_run_ws_name,
                             Target="TOF")
        # END-IF-ELSE for get reference of vanadium workspace

        return van_run_ws_name

    def _split_workspace(self, raw_ws_name, split_ws_name):
        """ Split workspace
        Purpose:
            Split a workspace
        Requirements:
            1. raw_ws_name is a string
            2. an event workspace with this name exists
        Guarantees:
            Raw input workspace is split
        :param raw_ws_name:
        :param split_ws_name:
        :return: list of strings as output workspaces
        """
        # Check requirements
        assert isinstance(raw_ws_name, str), 'Raw workspace name must be a string.'
        assert isinstance(split_ws_name, str), 'Input split workspace name must be string,' \
                                               'but not of type %s' % str(type(split_ws_name))
        assert self.does_workspace_exist(split_ws_name)

        assert is_event_workspace(raw_ws_name), 'Input workspace for splitting must be an EventWorkspace.'

        # Splitting workspace
        self.log().information("SplitterWorkspace = %s, Information Workspace = %s. " % (
            split_ws_name, str(self._splitinfotablews)))

        base_name = raw_ws_name + '_split'

        # find out whether the splitters are relative time or epoch time
        split_ws = AnalysisDataService.retrieve(split_ws_name)
        if isinstance(split_ws, SplittersWorkspace):
            is_relative_time = False
        else:
            if isinstance(split_ws, MatrixWorkspace):
                # matrix workspace case
                time0 = split_ws.readX(0)[0]
            else:
                #  table workspace case
                time0 = split_ws.cell(0, 0)

            if time0 > 3600. * 24 * 356:
                # longer than 1 Y. Cannot be relative time.
                is_relative_time = False
            else:
                # even just shorter than 1 year, but there was no experiment in 1991.
                is_relative_time = True

        if self._splitinfotablews is None:
            # split without information table
            api.FilterEvents(InputWorkspace=raw_ws_name, OutputWorkspaceBaseName=base_name,
                             SplitterWorkspace=split_ws_name, GroupWorkspaces=True, RelativeTime=is_relative_time)
        else:
            # split with information table
            api.FilterEvents(InputWorkspace=raw_ws_name, OutputWorkspaceBaseName=base_name,
                             SplitterWorkspace=split_ws_name, InformationWorkspace=str(self._splitinfotablews),
                             GroupWorkspaces=True, RelativeTime=is_relative_time)
        # ENDIF

        # Get workspace group for names of split workspace
        wsgroup = mtd[base_name]
        tempwsnamelist = wsgroup.getNames()
        # logging
        dbstr = "[Fx951] Splitted workspace names: "
        for ws_name in tempwsnamelist:
            dbstr += "%s, " % (ws_name)
        self.log().debug(dbstr)

        # Build the list of workspaces' names for return
        out_ws_name_list = []
        # FIXME Keep in mind to use this option.
        # keepremainder = self.getProperty("KeepRemainder").value
        for ws_name in tempwsnamelist:
            this_ws = get_workspace(ws_name)
            if ws_name.endswith("_unfiltered") is False:
                # workspace to save
                out_ws_name_list.append(ws_name)
            else:
                # events that are not excluded by filters. delete the workspace
                api.DeleteWorkspace(Workspace=this_ws)
        # END-FOR

        return out_ws_name_list


# Register algorithm with Mantid.
AlgorithmFactory.subscribe(SNSPowderReduction)
