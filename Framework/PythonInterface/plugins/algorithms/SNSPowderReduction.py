#pylint: disable=invalid-name,no-init,too-many-lines
import os
import numpy

import mantid
import mantid.api
import mantid.simpleapi as api
from mantid.api import *
from mantid.kernel import *

if AlgorithmFactory.exists('GatherWorkspaces'):
    HAVE_MPI = True
    from mpi4py import MPI
    mpiRank = MPI.COMM_WORLD.Get_rank()
else:
    HAVE_MPI = False
    mpiRank = 0 # simplify if clauses


EVENT_WORKSPACE_ID = "EventWorkspace"

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
    assert isinstance(runs, numpy.ndarray)

    # return True if runs is of size zero
    if runs.size <= 0:
        return True

    # return True if there is one and only one run in runs and it is not greater than 0.
    if runs.size == 1:
        return runs[0] <= 0

    return False

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

    for arg in args:
        if isinstance(arg, str):
            workspace = AnalysisDataService.retrieve(arg)
        else:
            workspace = arg
        result = result and (workspace.id() == EVENT_WORKSPACE_ID)

    return result

#pylint: disable=too-many-instance-attributes
class SNSPowderReduction(DataProcessorAlgorithm):
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
    _splitws = None
    _splitinfotablews = None
    _normalisebycurrent = None
    _lowResTOFoffset = None
    _focusPos = None
    _charTable = None
    iparmFile = None
    _info = None

    def category(self):
        return "Diffraction\\Reduction"

    def name(self):
        return "SNSPowderReduction"

    def summary(self):
        " "
        return "The algorithm used for reduction of powder diffraction data obtained on SNS instruments (e.g. PG3) "

    def PyInit(self):
        sns = ConfigService.getFacility("SNS")
        instruments = []
        for item in sns.instruments("Neutron Diffraction"):
            instruments.append(item.shortName())
        self.declareProperty("Instrument", "PG3", StringListValidator(instruments), "Powder diffractometer's name")
        arrvalidator = IntArrayBoundedValidator()
        arrvalidator.setLower(0)
        self.declareProperty(IntArrayProperty("RunNumber", values=[0], validator=arrvalidator,\
                             direction=Direction.Input), "Number of sample run or 0 for only Vanadium and/or Background")
        extensions = [ "_histo.nxs", "_event.nxs", "_runinfo.xml", ".nxs.h5"]
        self.declareProperty("Extension", "_event.nxs",
                             StringListValidator(extensions))
        self.declareProperty("PreserveEvents", True,
                             "Argument to supply to algorithms that can change from events to histograms.")
        self.declareProperty("Sum", False,
                             "Sum the runs. Does nothing for characterization runs")
        self.declareProperty("PushDataPositive", "None",
                             StringListValidator(["None", "ResetToZero", "AddMinimum"]),
                             "Add a constant to the data that makes it positive over the whole range.")
        arrvalidatorBack = IntArrayBoundedValidator()
        arrvalidator.setLower(-1)
        self.declareProperty(IntArrayProperty("BackgroundNumber", values=[0], validator=arrvalidatorBack),
                             doc="If specified overrides value in CharacterizationRunsFile If -1 turns off correction.")
        arrvalidatorVan = IntArrayBoundedValidator()
        arrvalidator.setLower(-1)
        self.declareProperty(IntArrayProperty("VanadiumNumber", values=[0], validator=arrvalidatorVan),
                             doc="If specified overrides value in CharacterizationRunsFile. If -1 turns off correction.")
        arrvalidatorVanBack = IntArrayBoundedValidator()
        arrvalidator.setLower(-1)
        self.declareProperty(IntArrayProperty("VanadiumBackgroundNumber", values=[0], validator=arrvalidatorVanBack),
                             doc="If specified overrides value in CharacterizationRunsFile. If -1 turns off correction.")
        self.declareProperty(FileProperty(name="CalibrationFile",defaultValue="",action=FileAction.Load,\
                                      extensions = [".h5", ".hd5", ".hdf", ".cal"]))
        self.declareProperty(FileProperty(name="CharacterizationRunsFile",defaultValue="",action=FileAction.OptionalLoad,\
                                      extensions = ["txt"]),"File with characterization runs denoted")
        self.declareProperty(FileProperty(name="ExpIniFilename", defaultValue="", action=FileAction.OptionalLoad,
                                          extensions=[".ini"]))
        self.declareProperty("UnwrapRef", 0.,
                             "Reference total flight path for frame unwrapping. Zero skips the correction")
        self.declareProperty("LowResRef", 0.,
                             "Reference DIFC for resolution removal. Zero skips the correction")
        self.declareProperty("CropWavelengthMin", 0.,
                             "Crop the data at this minimum wavelength. Overrides LowResRef.")
        self.declareProperty("CropWavelengthMax", 0.,
                             "Crop the data at this maximum wavelength. Forces use of CropWavelengthMin.")
        self.declareProperty("RemovePromptPulseWidth", 0.0,
                             "Width of events (in microseconds) near the prompt pulse to remove. 0 disables")
        self.declareProperty("MaxChunkSize", 0.0, "Specify maximum Gbytes of file to read in one chunk.  Default is whole file.")
        self.declareProperty("FilterCharacterizations", False,
                             "Filter the characterization runs using above parameters. This only works for event files.")
        self.declareProperty(FloatArrayProperty("Binning", values=[0.,0.,0.],\
                             direction=Direction.Input), "Positive is linear bins, negative is logorithmic")
        self.declareProperty("ResampleX", 0,
                             "Number of bins in x-axis. Non-zero value overrides \"Params\" property. "+\
                             "Negative value means logorithmic binning.")
        self.declareProperty("BinInDspace", True,
                             "If all three bin parameters a specified, whether they are in dspace (true) or time-of-flight (false)")
        self.declareProperty("StripVanadiumPeaks", True,
                             "Subtract fitted vanadium peaks from the known positions.")
        self.declareProperty("VanadiumFWHM", 7, "Default=7")
        self.declareProperty("VanadiumPeakTol", 0.05,
                             "How far from the ideal position a vanadium peak can be during StripVanadiumPeaks. "\
                             "Default=0.05, negative turns off")
        self.declareProperty("VanadiumSmoothParams", "20,2", "Default=20,2")
        self.declareProperty("VanadiumRadius", .3175, "Radius for MultipleScatteringCylinderAbsorption")
        self.declareProperty("BackgroundSmoothParams", "", "Default=off, suggested 20,2")
        self.declareProperty("FilterBadPulses", 95.,
                             doc="Filter out events measured while proton charge is more than 5% below average")
        self.declareProperty("ScaleData", defaultValue=1., validator=FloatBoundedValidator(lower=0., exclusive=True),
                             doc="Constant to multiply the data before writing out. This does not apply to PDFgetN files.")
        self.declareProperty("SaveAs", "gsas",
                             "List of all output file types. Allowed values are 'fullprof', 'gsas', 'nexus', 'pdfgetn', and 'topas'")
        self.declareProperty("OutputFilePrefix", "", "Overrides the default filename for the output file (Optional).")
        self.declareProperty(FileProperty(name="OutputDirectory",defaultValue="",action=FileAction.Directory))
        self.declareProperty("FinalDataUnits", "dSpacing", StringListValidator(["dSpacing","MomentumTransfer"]))

        tableprop = ITableWorkspaceProperty("SplittersWorkspace", "", Direction.Input, PropertyMode.Optional)
        self.declareProperty(tableprop, "Splitters workspace for split event workspace.")
        infotableprop = ITableWorkspaceProperty("SplitInformationWorkspace", "", Direction.Input, PropertyMode.Optional)
        self.declareProperty(infotableprop, "Name of table workspace containing information for splitters.")

        self.declareProperty("LowResolutionSpectraOffset", -1,
                             "If larger and equal to 0, then process low resolution TOF and offset is the spectra number. "+\
                             "Otherwise, ignored.")

        self.declareProperty("NormalizeByCurrent", True, "Normalize by current")

        self.declareProperty("CompressTOFTolerance", 0.01, "Tolerance to compress events in TOF.")

        self.declareProperty(StringArrayProperty("FrequencyLogNames", ["SpeedRequest1", "Speed1", "frequency"],\
            direction=Direction.Input),\
            "Possible log names for frequency.")

        self.declareProperty(StringArrayProperty("WaveLengthLogNames", ["LambdaRequest", "lambda"],\
            direction=Direction.Input),\
            "Candidate log names for wave length.")

        return

    #pylint: disable=too-many-locals,too-many-branches,too-many-statements
    def PyExec(self):
        """ Main execution body
        """
        # get generic information
        SUFFIX = self.getProperty("Extension").value
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
        self._instrument = self.getProperty("Instrument").value
        config['default.facility'] = "SNS"
        config['default.instrument'] = self._instrument
        self._filterBadPulses = self.getProperty("FilterBadPulses").value
        self._removePromptPulseWidth = self.getProperty("RemovePromptPulseWidth").value
        self._LRef = self.getProperty("UnwrapRef").value
        self._DIFCref = self.getProperty("LowResRef").value
        self._wavelengthMin = self.getProperty("CropWavelengthMin").value
        self._wavelengthMax = self.getProperty("CropWavelengthMax").value
        self._vanPeakFWHM = self.getProperty("VanadiumFWHM").value
        self._vanSmoothing = self.getProperty("VanadiumSmoothParams").value
        self._vanRadius = self.getProperty("VanadiumRadius").value
        calib = self.getProperty("CalibrationFile").value
        self._scaleFactor = self.getProperty("ScaleData").value
        self._outDir = self.getProperty("OutputDirectory").value
        self._outPrefix = self.getProperty("OutputFilePrefix").value.strip()
        self._outTypes = self.getProperty("SaveAs").value.lower()
        samRuns = self.getProperty("RunNumber").value
        preserveEvents = self.getProperty("PreserveEvents").value
        if HAVE_MPI and preserveEvents == True:
            self.log().warning("preserveEvents set to False for MPI tasks.")
            preserveEvents = False
        self._info = None
        self._chunks = self.getProperty("MaxChunkSize").value

        self._splitws = self.getProperty("SplittersWorkspace").value
        if self._splitws is not None:
            self.log().information("SplittersWorkspace is %s" % (str(self._splitws)))
            if len(samRuns) != 1:
                raise NotImplementedError("Reducing data with splitting cannot happen when there are more than 1 sample run.")
            timeFilterWall = self._getTimeFilterWall(self._splitws, samRuns[0], SUFFIX)
            self.log().information("The time filter wall is %s" %(str(timeFilterWall)))
        else:
            timeFilterWall = (0.0, 0.0)
            self.log().information("SplittersWorkspace is None, and thus there is NO time filter wall. ")

        self._splitinfotablews = self.getProperty("SplitInformationWorkspace").value

        self._normalisebycurrent = self.getProperty("NormalizeByCurrent").value

        # Tolerance for compress TOF event
        self.COMPRESS_TOL_TOF = float(self.getProperty("CompressTOFTolerance").value)
        if self.COMPRESS_TOL_TOF < 0.:
            self.COMPRESS_TOL_TOF = 0.01

        # Process data
        # List stores the workspacename of all data workspaces that will be converted to d-spacing in the end.
        workspacelist = []
        samwksplist = []

        self._lowResTOFoffset = self.getProperty("LowResolutionSpectraOffset").value
        focuspos = self._focusPos
        if self._lowResTOFoffset >= 0:
            # Dealing with the parameters for editing instrument parameters
            if focuspos.has_key("PrimaryFlightPath") is True:
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

        if self.getProperty("Sum").value:
            # Sum input sample runs and then do reduction
            if self._splitws is not None:
                raise NotImplementedError("Summing spectra and filtering events are not supported simultaneously.")

            sam_ws_name = self._focusAndSum(samRuns, SUFFIX, timeFilterWall, calib,
                                            preserveEvents=preserveEvents)
            assert isinstance(sam_ws_name, str), 'Returned from _focusAndSum() must be a string but not' \
                                                 '%s. ' % str(type(sam_ws_name))

            # samRuns = [sam_ws_name]
            workspacelist.append(sam_ws_name)
            samwksplist.append(sam_ws_name)
        # ENDIF (SUM)
        else:
            # Process each sample run
            for sam_run_number in samRuns:
                # check
                err_msg = 'sample run number %s must be either int or numpy.int32, ' \
                          'but not %s' % (str(sam_run_number), str(type(sam_run_number)))
                assert isinstance(sam_run_number, int) or isinstance(sam_run_number, numpy.int32), err_msg
                if sam_run_number <= 0:
                    self.log().warning('Sample run number %d is less and equal to zero!' % sam_run_number)
                    continue

                # first round of processing the sample
                self._info = None
                returned = self._focusChunks(sam_run_number, SUFFIX, timeFilterWall, calib, splitwksp=self._splitws,
                                             normalisebycurrent=self._normalisebycurrent,
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
            assert isinstance(sam_ws_name, str), 'Assuming that samRun is a string. But it is %s' % str(type(sam_ws_))
            sam_run_ws = self.get_workspace(sam_ws_name)
            if sam_run_ws.id() == EVENT_WORKSPACE_ID:
                self.log().information('Sample Run %s:  starting number of events = %d.' % (
                    sam_ws_name, sam_run_ws.getNumberEvents()))

            # Get run information
            self._info = self._getinfo(sam_ws_name)

            # process the container
            can_run_numbers = self._info["container"].value
            can_run_ws_name = self._process_container_runs(can_run_numbers, timeFilterWall, samRunIndex, SUFFIX, calib,
                                                           preserveEvents)
            if can_run_ws_name is not None:
                workspacelist.append(can_run_ws_name)

            # process the vanadium run
            van_run_number_list = self._info["vanadium"].value
            van_specified = not noRunSpecified(van_run_number_list)
            if van_specified:
                van_run_ws_name = self._process_vanadium_runs(van_run_number_list, timeFilterWall, samRunIndex, calib)
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
                if allEventWorkspaces(can_run_ws_name, sam_ws_name):
                    sam_ws = api.ConvertToMatrixWorkspace(InputWorkspace=sam_ws_name,
                                                          OutputWorkspace=sam_ws_name)
                    assert sam_ws is not None

                # remove container run
                api.RebinToWorkspace(WorkspaceToRebin=can_run_ws_name,
                                     WorkspaceToMatch=sam_ws_name,
                                     OutputWorkspace=can_run_ws_name)
                sam_ws = api.Minus(LHSWorkspace=sam_ws_name,
                                   RHSWorkspace=can_run_ws_name,
                                   OutputWorkspace=sam_ws_name)
                # compress event if the sample run workspace is EventWorkspace
                if sam_ws.id() == EVENT_WORKSPACE_ID:
                    sam_ws = api.CompressEvents(InputWorkspace=sam_ws_name,
                                                OutputWorkspace=sam_ws_name,
                                                Tolerance=self.COMPRESS_TOL_TOF)  # 10ns
                    assert sam_ws is not None
                # canRun = str(canRun)

            if van_run_ws_name is not None:
                # subtract vanadium run from sample run by division
                sam_ws = self.get_workspace(sam_ws_name)
                van_ws = self.get_workspace(van_run_ws_name)
                num_hist_sam = sam_ws.getNumberHistograms()
                num_hist_van = van_ws.getNumberHistograms()
                assert num_hist_sam == num_hist_van, \
                    'Number of histograms of sample %d is not equal to van %d.' % (num_hist_sam, num_hist_van)
                sam_ws = api.Divide(LHSWorkspace=sam_ws_name,
                                    RHSWorkspace=van_run_ws_name,
                                    OutputWorkspace=sam_ws_name)
                normalized = True
                van_run_ws = self.get_workspace(van_run_ws_name)
                sam_ws.getRun()['van_number'] = van_run_ws.getRun()['run_number'].value
                # van_run_number = str(van_run_number)
            else:
                normalized = False

            # Compress the event again
            sam_run_ws = self.get_workspace(sam_ws_name)
            if sam_run_ws.id() == EVENT_WORKSPACE_ID:
                sam_run_ws = api.CompressEvents(InputWorkspace=sam_ws_name,
                                                OutputWorkspace=sam_ws_name,
                                                Tolerance=self.COMPRESS_TOL_TOF)  # 5ns/
                assert sam_run_ws is not None

            # make sure there are no negative values - gsas hates them
            if self.getProperty("PushDataPositive").value != "None":
                addMin = self.getProperty("PushDataPositive").value == "AddMinimum"
                sam_ws = api.ResetNegatives(InputWorkspace=sam_ws_name,
                                            OutputWorkspace=sam_ws_name,
                                            AddMinimum=addMin,
                                            ResetValue=0.)

            # write out the files
            # FIXME - need documentation for mpiRank
            if mpiRank == 0:
                if self._scaleFactor != 1.:
                    sam_ws = api.Scale(sam_ws_name, Factor=self._scaleFactor, OutputWorkspace=sam_ws_name)
                self._save(sam_ws_name, self._info, normalized, False)
            #mtd.releaseFreeMemory()

        # ENDFOR

        # convert everything into d-spacing as the final units
        if HAVE_MPI is False:
            workspacelist = set(workspacelist) # only do each workspace once
            for wksp in workspacelist:
                wksp = api.ConvertUnits(InputWorkspace=wksp, OutputWorkspace=wksp,
                                        Target=self.getProperty("FinalDataUnits").value)

        return

    def _loadCharacterizations(self):
        self._focusPos = {}
        charFilename = self.getProperty("CharacterizationRunsFile").value
        expIniFilename = self.getProperty("ExpIniFilename").value

        if charFilename is None or len(charFilename) <= 0:
            self.iparmFile = None
            return

        results = api.PDLoadCharacterizations(Filename=charFilename,
                                              ExpIniFilename=expIniFilename,
                                              OutputWorkspace="characterizations")
        # export the characterizations table
        self._charTable = results[0]
        self.declareProperty(ITableWorkspaceProperty("CharacterizationsTable", "characterizations", Direction.Output))
        self.setProperty("CharacterizationsTable", self._charTable)

        # get the focus positions from the properties
        self.iparmFile = results[1]
        self._focusPos['PrimaryFlightPath'] = results[2]
        self._focusPos['SpectrumIDs'] = results[3]
        self._focusPos['L2'] = results[4]
        self._focusPos['Polar'] = results[5]
        self._focusPos['Azimuthal'] = results[6]

    #pylint: disable=too-many-branches
    def _loadData(self, run_number, extension, filterWall=None, out_ws_name=None, **chunk):
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
        :param run_number:
        :param extension: file extension
        :param filterWall:
        :param out_ws_name: name of output workspace specified by user. it will override the automatic name
        :param chunk:
        :return:
        """
        # Check requirements
        assert isinstance(run_number, int) or isinstance(run_number, numpy.int32), 'Input run number must be integer ' \
                                                                                   'but not %s.' % str(type(run_number))
        assert run_number > 0, 'Input run number must be larger than 0 but not %d.' % run_number
        assert (chunk is None) or isinstance(chunk, dict), 'Input chunk must be either a dictionary or None.'

        base_name = "%s_%d" % (self._instrument, int(run_number))
        filename = base_name + extension
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
        if extension.endswith("_event.nxs") or extension.endswith(".nxs.h5"):
            chunk["Precount"] = True
            if filterWall is not None:
                if filterWall[0] > 0.:
                    chunk["FilterByTimeStart"] = filterWall[0]
                if filterWall[1] > 0.:
                    chunk["FilterByTimeStop"] = filterWall[1]

        # Call Mantid's Load algorithm to load complete or partial data
        wksp = api.Load(Filename=filename, OutputWorkspace=out_ws_name, **chunk)
        assert wksp is not None, 'Mantid Load algorithm does not return a workspace.'

        # Log output
        if wksp.id() == EVENT_WORKSPACE_ID:
            self.log().debug("Load run %d: number of events = %d. " % (run_number, wksp.getNumberEvents()))
        if HAVE_MPI:
            msg = "MPI Task = %s ;" % (str(mpiRank))
            if wksp.id() == EVENT_WORKSPACE_ID:
                msg += "Number Events = " + str(wksp.getNumberEvents())
            self.log().debug(msg)

        # filter bad pulses
        if self._filterBadPulses > 0.:
            # record number of events of the original workspace
            is_event_ws = isinstance(wksp, mantid.api.IEventWorkspace)
            if is_event_ws is True:
                # Event workspace: record original number of events
                num_original_events = wksp.getNumberEvents()
            else:
                num_original_events = -1

            # filter bad pulse
            wksp = api.FilterBadPulses(InputWorkspace=out_ws_name, OutputWorkspace=out_ws_name,
                                       LowerCutoff=self._filterBadPulses)
            assert wksp is not None, 'Returned value from FilterBadPulses cannot be None'

            if is_event_ws is True:
                # Event workspace
                message = "FilterBadPulses reduces number of events from %d to %d (under %s percent) " \
                          "of workspace %s." % (num_original_events, wksp.getNumberEvents(),
                                                str(self._filterBadPulses), str(wksp))
                self.log().information(message)
        # END-IF (filter bad pulse)

        return wksp

    def _getStrategy(self, runnumber, extension):
        """
        Get chunking strategy by calling mantid algorithm 'DetermineChunking'
        :param runnumber:
        :param extension:
        :return: a list of dictionary.  Each dictionary is a row in table workspace
        """
        # generate the workspace name
        assert isinstance(runnumber, int) or isinstance(runnumber, numpy.int32), \
            'Expected run number %s to be either integer or numpy.int32, but not %s' % (
                str(runnumber), str(type(runnumber)))
        file_name = "%s_%d" % (self._instrument, int(runnumber)) + extension

        # Determine chunk strategy can search in archive.
        # Therefore this will fail: assert os.path.exists(file_name), 'NeXus file %s does not exist.' % file_name
        self.log().debug("[Fx116] Run file Name : %s,\t\tMax chunk size: %s" % (file_name, str(self._chunks)))
        chunks = api.DetermineChunking(Filename=file_name, MaxChunkSize=self._chunks)

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
        keys = chunk.keys()
        keys.sort()

        keys = [str(key) + "=" + str(chunk[key]) for key in keys]
        self.log().information("Working on chunk [" + ", ".join(keys) + "]")

    def checkInfoMatch(self, left, right):
        if (left["frequency"].value is not None) and (right["frequency"].value is not None) \
           and (abs(left["frequency"].value - right["frequency"].value)/left["frequency"].value > .05):
            raise RuntimeError("Cannot add incompatible frequencies (%f!=%f)" \
                               % (left["frequency"].value, right["frequency"].value))
        if (left["wavelength"].value is not None) and (right["wavelength"].value is not None) \
                   and abs(left["wavelength"].value - right["wavelength"].value)/left["wavelength"].value > .05:
            raise RuntimeError("Cannot add incompatible wavelengths (%f != %f)" \
                               % (left["wavelength"].value, right["wavelength"].value))

    def _loadAndSum(self, run_number_list, outName, **filterWall):
        """
        Load and sum
        Purpose:

        Requirements:
        1. run number are integers
        Guarantees:

        :param run_number_list: list of run numbers
        :param outName:
        :param filterWall:
        :return:
        """
        # Check requirements
        assert isinstance(run_number_list, list) or isinstance(run_number_list, numpy.ndarray),\
            'Run number list is not a list but of type %s' % str(type(run_number_list))
        for run_number in run_number_list:
            assert isinstance(run_number, int) or isinstance(run_number, numpy.int32), \
                'Run number %s must be an integer but not a %s.' % (str(run_number), str(type(run_number)))
            assert run_number > 0, 'Run number %s must be larger than 0.' % run_number

        # Form output workspaces' names
        file_number_list = ['%s_%d' % (self._instrument, runNum) for runNum in run_number_list]
        out_ws_name_list = ['%s_%d_loadsum' % (self._instrument, runNum) for runNum in run_number_list]

        sample_ws_name = None
        info = None
        SUFFIX = self.getProperty("Extension").value

        # for ws_name in out_ws_name_list:
        for i_run in xrange(len(run_number_list)):
            run_number = file_number_list[i_run]
            ws_name = out_ws_name_list[i_run]
            self.log().debug("[Sum] processing %s" % run_number)
            temp_ws = api.LoadEventAndCompress(Filename=run_number+SUFFIX,
                                               OutputWorkspace=ws_name,
                                               MaxChunkSize=self._chunks,
                                               FilterBadPulses=self._filterBadPulses,
                                               CompressTOFTolerance=self.COMPRESS_TOL_TOF, **filterWall)
            assert temp_ws is not None
            if temp_ws.id() == EVENT_WORKSPACE_ID:
                self.log().warning('Load event file %s, compress it and get %d events.' % (run_number+SUFFIX,
                                                                                           temp_ws.getNumberEvents()))

            tempinfo = self._getinfo(ws_name)
            if sample_ws_name is None:
                # sample run workspace is not set up.
                sample_ws_name = ws_name
                info = tempinfo
            else:
                # there is sample run workspace set up previously, then add current one to previous for summation
                self.checkInfoMatch(info, tempinfo)

                temp_ws = api.Plus(LHSWorkspace=sample_ws_name,
                                   RHSWorkspace=ws_name,
                                   OutputWorkspace=sample_ws_name,
                                   ClearRHSWorkspace=allEventWorkspaces(sample_ws_name, ws_name))
                assert temp_ws is not None
                api.DeleteWorkspace(ws_name)

                # comparess events
                sample_run_ws = self.get_workspace(sample_ws_name)
                if sample_run_ws.id() == EVENT_WORKSPACE_ID:
                    temp_ws = api.CompressEvents(InputWorkspace=sample_ws_name, OutputWorkspace=sample_ws_name,
                                                 Tolerance=self.COMPRESS_TOL_TOF)  # 10ns
                    assert temp_ws is not None
        # END-FOR

        # Normalize by current with new name
        if self._normalisebycurrent is True:
            self.log().warning('[SPECIAL DB] Normalize current to workspace %s' % sample_ws_name)
            # temp_ws = self.get_workspace(sample_ws_name)
            if not (temp_ws.id() == EVENT_WORKSPACE_ID and temp_ws.getNumberEvents() == 0):
                temp_ws = api.NormaliseByCurrent(InputWorkspace=sample_ws_name,
                                                 OutputWorkspace=sample_ws_name)
                assert temp_ws is not None
                temp_ws.getRun()['gsas_monitor'] = 1
            # END-IF
        # ENDI-IF

        # Rename to user specified output workspace
        if sample_ws_name != outName:
            api.RenameWorkspace(InputWorkspace=sample_ws_name,
                                OutputWorkspace=outName)

        return outName

    #pylint: disable=too-many-arguments
    def _focusAndSum(self, run_number_list, extension, filterWall, calib, preserveEvents=True):
        """Load, sum, and focus data in chunks
        Purpose:
            Load, sum and focus data in chunks;
        Requirements:
            1. input run numbers are in numpy array or list
        Guarantees:
            The experimental runs are focused and summed together
        @param run_number_list:
        @param extension:
        @param filterWall:
        @param calib:
        @param preserveEvents:
        @return: string as the summed workspace's name
        """
        # Check requirements
        assert isinstance(run_number_list, numpy.ndarray) or isinstance(run_number_list, list)

        sumRun = None
        info = None

        for run_number in run_number_list:
            # check
            error_message = 'Run number %s should be integer or numpy.int32. But it is %s.' % (str(run_number),
                                                                                               str(type(run_number)))
            assert isinstance(run_number, int) or isinstance(run_number, numpy.int32), error_message
            self.log().information("[Sum] Process run number %d. " % run_number)

            # focus one run
            out_ws_name = self._focusChunks(run_number, extension, filterWall, calib,
                                            normalisebycurrent=False,
                                            preserveEvents=preserveEvents)
            assert isinstance(out_ws_name, str), 'Output from _focusChunks() should be a string but' \
                                                 ' not %s.' % str(type(out_ws_name))
            assert self.does_workspace_exist(out_ws_name)

            tempinfo = self._getinfo(out_ws_name)

            # sum reduced runs
            if sumRun is None:
                # First run. No need to sumRun
                sumRun = out_ws_name
                info = tempinfo
            else:
                # Non-first run. Add this run to current summed run
                self.checkInfoMatch(info, tempinfo)
                # add current workspace to sub sum
                temp_ws = api.Plus(LHSWorkspace=sumRun, RHSWorkspace=out_ws_name, OutputWorkspace=sumRun,
                                   ClearRHSWorkspace=allEventWorkspaces(sumRun, out_ws_name))
                if temp_ws.id() == EVENT_WORKSPACE_ID:
                    temp_ws = api.CompressEvents(InputWorkspace=sumRun, OutputWorkspace=sumRun,
                                                 Tolerance=self.COMPRESS_TOL_TOF) # 10ns
                    assert temp_ws is not None
                # after adding all events, delete the current workspace.
                api.DeleteWorkspace(out_ws_name)
            # ENDIF
        # ENDFOR (processing each)

        if self._normalisebycurrent is True:
            temp_ws = api.NormaliseByCurrent(InputWorkspace=sumRun,
                                             OutputWorkspace=sumRun)
            temp_ws.getRun()['gsas_monitor'] = 1

        return sumRun

    #pylint: disable=too-many-arguments,too-many-locals,too-many-branches
    def _focusChunks(self, runnumber, extension, filterWall, calib,
                     normalisebycurrent, splitwksp=None, preserveEvents=True):
        """
        Load, (optional) split and focus data in chunks
        @param runnumber: integer for run number
        @param extension:
        @param filterWall:  Enabled if splitwksp is defined
        @param calib:
        @param normalisebycurrent: Set to False if summing runs for correct math
        @param splitwksp: SplittersWorkspace (if None then no split)
        @param preserveEvents:
        @return: a string as the returned workspace's name or a list of strings as the returned workspaces' names
                 in the case that split workspace is used.
        """
        # generate the workspace name
        self.log().information("_focusChunks(): runnumber = %d, extension = %s" % (runnumber, extension))

        # get chunk strategy for parallel processing (MPI)
        strategy = self._getStrategy(runnumber, extension)

        # determine event splitting by checking filterWall and number of output workspaces from _focusChunk
        do_split_raw_wksp, num_out_wksp = self._determine_workspace_splitting(splitwksp, filterWall)

        # Set up the data structure to hold and control output workspaces
        output_wksp_list = [None] * num_out_wksp
        is_first_chunk_list = [True] * num_out_wksp

        self.log().debug("F1141A: Number of workspace to process = %d" % num_out_wksp)

        # reduce data by chunks
        chunk_index = -1
        # FIXME/TODO/Now: there are 2 places that determine the base name
        base_name = '%s_%d' % (self._instrument, runnumber)
        for chunk in strategy:
            # progress on chunk index
            chunk_index += 1
            # Load chunk, i.e., partial data into Mantid
            raw_ws_chunk = self._loadData(runnumber, extension, filterWall, **chunk)
            raw_ws_name_chunk = raw_ws_chunk.name()

            if self._info is None:
                self._info = self._getinfo(raw_ws_name_chunk)

            # Log information for current chunk
            self.__logChunkInfo(chunk)
            if raw_ws_chunk.id() == EVENT_WORKSPACE_ID:
                # Event workspace
                self.log().debug("F1141C: There are %d events after data is loaded in workspace %s." % (
                    raw_ws_chunk.getNumberEvents(), raw_ws_name_chunk))

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
            for iws in xrange(len(output_ws_name_list)):
                ws = output_ws_name_list[iws]
                msg += "%s\t\t" % (str(ws))
                if iws %5 == 4:
                    msg += "\n"
            self.log().debug(msg)

            # Do align and focus
            num_out_wksp = len(output_ws_name_list)
            for split_index in xrange(num_out_wksp):
                # Get workspace name
                out_ws_name_chunk_split = output_ws_name_list[split_index]
                # Align and focus
                # focuspos = self._focusPos
                self.log().notice('Align and focus workspace %s' % out_ws_name_chunk_split)
                out_ws_c_s = api.AlignAndFocusPowder(InputWorkspace=out_ws_name_chunk_split,
                                                     OutputWorkspace=out_ws_name_chunk_split,
                                                     CalFileName=calib,
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
                assert out_ws_c_s is not None
                # logging (ignorable)
                for iws in xrange(out_ws_c_s.getNumberHistograms()):
                    spec = out_ws_c_s.getSpectrum(iws)
                    self.log().debug("[DBx131] ws %d: spectrum No = %d. " % (iws, spec.getSpectrumNo()))
                if out_ws_c_s.id() == EVENT_WORKSPACE_ID:
                    self.log().information('After being aligned and focused, workspace %s: Number of events = %d '
                                           'of chunk %d ' % (out_ws_c_s.name(), out_ws_c_s.getNumberEvents(),
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
                    temp_out_ws = api.Plus(LHSWorkspace=output_wksp_list[split_index],
                                           RHSWorkspace=output_ws_name_list[split_index],
                                           OutputWorkspace=output_wksp_list[split_index],
                                           ClearRHSWorkspace=clear_rhs_ws)
                    assert temp_out_ws is not None

                    # Delete the chunk workspace
                    api.DeleteWorkspace(output_ws_name_list[split_index])
                # END-IF-ELSE
        # END-FOR-Chunks

        for item in output_wksp_list:
            assert isinstance(item, str)

        # Sum workspaces for all mpi tasks
        if HAVE_MPI:
            for split_index in xrange(num_out_wksp):
                temp_ws = api.GatherWorkspaces(InputWorkspace=output_wksp_list[split_index],
                                               PreserveEvents=preserveEvents,
                                               AccumulationMethod="Add",
                                               OutputWorkspace=output_wksp_list[split_index])
                assert temp_ws is not None
        # ENDIF MPI

        if self._chunks > 0:
            # When chunks are added, proton charge is summed for all chunks
            for split_index in xrange(num_out_wksp):
                temp_out_ws = self.get_workspace(output_wksp_list[split_index])
                temp_out_ws.getRun().integrateProtonCharge()
        # ENDIF

        if (self.iparmFile is not None) and (len(self.iparmFile) > 0):
            # When chunks are added, add iparamFile
            for split_index in xrange(num_out_wksp):
                temp_ws = self.get_workspace(output_wksp_list[split_index])
                temp_ws.getRun()['iparm_file'] = self.iparmFile

        # Compress events
        for split_index in xrange(num_out_wksp):
            temp_out_ws = self.get_workspace(output_wksp_list[split_index])
            if temp_out_ws.id() == EVENT_WORKSPACE_ID:
                temp_out_ws = api.CompressEvents(InputWorkspace=output_wksp_list[split_index],
                                                 OutputWorkspace=output_wksp_list[split_index],
                                                 Tolerance=self.COMPRESS_TOL_TOF) # 100ns
                assert temp_out_ws is not None

            try:
                if normalisebycurrent is True:
                    temp_ws = api.NormaliseByCurrent(InputWorkspace=output_wksp_list[split_index],
                                                     OutputWorkspace=output_wksp_list[split_index])
                    temp_ws.getRun()['gsas_monitor'] = 1
            except RuntimeError as e:
                self.log().warning(str(e))

            propertyName = "OutputWorkspace%s" % str(output_wksp_list[split_index])
            self.log().warning(propertyName)
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

        # Determine characterization
        if mtd.doesExist("characterizations"):
            # get the correct row of the table if table workspace 'charactersizations' exists

            #pylint: disable=unused-variable
            charac = api.PDDetermineCharacterizations(InputWorkspace=wksp_name,
                                                      Characterizations="characterizations",
                                                      ReductionProperties="__snspowderreduction",
                                                      BackRun=self.getProperty("BackgroundNumber").value,
                                                      NormRun=self.getProperty("VanadiumNumber").value,
                                                      NormBackRun=self.getProperty("VanadiumBackgroundNumber").value,
                                                      FrequencyLogNames=self.getProperty("FrequencyLogNames").value,
                                                      WaveLengthLogNames=self.getProperty("WaveLengthLogNames").value)
        else:
            charac = api.PDDetermineCharacterizations(InputWorkspace=wksp_name,
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
            prefix = self._outPrefix
        filename = os.path.join(self._outDir, prefix)
        if pdfgetn:
            self.log().notice("Saving 'pdfgetn' is deprecated. Use PDtoPDFgetN instead.")
            if "pdfgetn" in self._outTypes:
                pdfwksp = str(wksp)+"_norm"
                pdfwksp = api.SetUncertainties(InputWorkspace=wksp, OutputWorkspace=pdfwksp, SetError="sqrt")
                api.SaveGSS(InputWorkspace=pdfwksp, Filename=filename+".getn", SplitFiles=False, Append=False,
                            MultiplyByBinWidth=False, Bank=info["bank"].value, Format="SLOG", ExtendedHeader=True)
                api.DeleteWorkspace(pdfwksp)
            return # don't do the other bits of saving
        if "gsas" in self._outTypes:
            api.SaveGSS(InputWorkspace=wksp, Filename=filename+".gsa", SplitFiles=False, Append=False,\
                    MultiplyByBinWidth=normalized, Bank=info["bank"].value, Format="SLOG", ExtendedHeader=True)
        if "fullprof" in self._outTypes:
            api.SaveFocusedXYE(InputWorkspace=wksp, StartAtBankNumber=info["bank"].value, Filename=filename+".dat")
        if "topas" in self._outTypes:
            api.SaveFocusedXYE(InputWorkspace=wksp, StartAtBankNumber=info["bank"].value, Filename=filename+".xye",
                               Format="TOPAS")
        if "nexus" in self._outTypes:
            api.ConvertUnits(InputWorkspace=wksp, OutputWorkspace=wksp, Target=self.getProperty("FinalDataUnits").value)
            #api.Rebin(InputWorkspace=wksp, OutputWorkspace=wksp, Params=self._binning) # crop edges
            api.SaveNexus(InputWorkspace=wksp, Filename=filename+".nxs")

        # always save python script - this is broken because the history isn't
        # attached until the algorithm is finished
        api.GeneratePythonScript(InputWorkspace=wksp, Filename=filename+".py")

        return

    def _getTimeFilterWall(self, splitws, samrun, extension):
        """ Get filter wall from splitter workspace, i.e.,
        get the earlies and latest TIME stamp in input splitter workspace

        Arguments:
         - splitws      : splitters workspace
         - runstarttime : total nanoseconds of run start time (Mantid DateAndTime)

        Return: tuple of start-time and stop-time relative to run start time and in unit of second
                If there is no split workspace defined, filter is (0., 0.) as the default
        """
        # None case
        if splitws is None:
            self.log().warning("Split workspace is None.  Unable to make a filter wall.  Return with default value. ")
            return (0.0, 0.0)

        # Load data
        name = "%s_%d" % (self._instrument, samrun)
        filename = name + extension
        metawsname = "temp_"+name

        metawksp = api.Load(Filename=str(filename), OutputWorkspace=str(metawsname), MetaDataOnly=True)
        if metawksp is None:
            self.log().warning("Unable to open file %s" % (filename))
            return (0.0, 0.0)

        # Get start time
        runstarttimens = metawksp.getRun().startTime().totalNanoseconds()

        numrow = splitws.rowCount()

        # Searching for the
        tmin_absns = splitws.cell(0,0)
        tmax_absns = splitws.cell(0,1)

        for r in xrange(1, numrow):
            timestart = splitws.cell(r, 0)
            timeend = splitws.cell(r, 1)
            if timestart < tmin_absns:
                tmin_absns = timestart
            if timeend > tmax_absns:
                tmax_absns = timeend
        # ENDFOR

        tmin = (tmin_absns - runstarttimens) * 1.0E-9
        tmax = (tmax_absns - runstarttimens) * 1.0E-9

        filterWall = (tmin, tmax)

        api.DeleteWorkspace(Workspace=metawsname)

        return filterWall

    def getNumberOfSplittedWorkspace(self, splitwksp):
        """ Get number of splitted workspaces due to input splitwksp

        Return : integer
        """
        # splitws = mtd["PG3_9829_event_splitters"]
        splitws = AnalysisDataService.retrieve(str(splitwksp))
        numrows = splitws.rowCount()
        wscountdict = {}
        for r in xrange(numrows):
            wsindex = splitws.cell(r,2)
            wscountdict[wsindex] = 0

        return len(wscountdict.keys())

    @staticmethod
    def does_workspace_exist(workspace_name):
        """
        Purpose: Check whether a workspace exists in AnalysisDataService
        :param workspace_name:
        :return:
        """
        assert isinstance(workspace_name, str)

        return AnalysisDataService.doesExist(workspace_name)

    def get_workspace(self, workspace_name):
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
        assert isinstance(workspace_name, str)

        return AnalysisDataService.retrieve(workspace_name)

    def _determine_workspace_splitting(self, split_wksp, filter_wall):
        """
        :return: do_split_raw_wksp, num_out_wksp
        """
        do_split_raw_wksp = False
        num_out_wksp = 1

        if split_wksp is not None:
            # Use splitting workspace

            # Check consistency with filterWall
            if filter_wall[0] < 1.0E-20 and filter_wall[1] < 1.0E-20:
                # Default definition of filterWall when there is no split workspace specified.
                raise RuntimeError("It is impossible to have a not-NONE splitters workspace and (0,0) time filter wall.")
            # ENDIF

            # Note: Unfiltered workspace (remainder) is not considered here
            num_out_wksp = self.getNumberOfSplittedWorkspace(split_wksp)
            num_splitters = split_wksp.rowCount()

            # Do explicit FilterEvents if number of splitters is larger than 1.
            # If number of splitters is equal to 1, then filterWall will do the job itself.
            if num_splitters > 1:
                do_split_raw_wksp = True
            self.log().debug("[Fx948] Number of split workspaces = %d; Do split = %s" % (num_out_wksp, str(do_split_raw_wksp)))
        # ENDIF

        return do_split_raw_wksp, num_out_wksp

    def _process_container_runs(self, can_run_numbers, timeFilterWall, samRunIndex, SUFFIX, calib, preserveEvents):
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

            # set up the filters
            if self.getProperty("FilterCharacterizations").value:
                # use common time filter
                canFilterWall = timeFilterWall
            else:
                # no time filter
                canFilterWall = (0., 0.)
            # END-IF

            if len(can_run_numbers) == 1:
                # only 1 container run
                can_run_number = can_run_numbers[0]
            else:
                # in case of multiple container run, use the corresponding one to sample
                can_run_number = can_run_numbers[samRunIndex]

            # get reference to container run
            can_run_ws_name = '%s_%d' % (self._instrument, can_run_number)
            if self.does_workspace_exist(can_run_ws_name) is True:
                # container run exists to get reference from mantid
                can_run_ws = api.ConvertUnits(InputWorkspace=can_run_ws_name,
                                              OutputWorkspace=can_run_ws_name,
                                              Target="TOF")
                assert can_run_ws is not None
            else:
                # load the container run
                if self.getProperty("Sum").value:
                    can_run_ws_name = self._focusAndSum(can_run_numbers, SUFFIX, canFilterWall, calib,
                                                        preserveEvents=preserveEvents)
                else:
                    can_run_ws_name = self._focusChunks(can_run_number, SUFFIX, canFilterWall, calib,
                                                        normalisebycurrent=self._normalisebycurrent,
                                                        preserveEvents=preserveEvents)
                can_run_ws = self.get_workspace(can_run_ws_name)
                assert can_run_ws.name() == can_run_ws_name
                # convert unit to TOF
                can_run_ws = api.ConvertUnits(InputWorkspace=can_run_ws_name,
                                              OutputWorkspace=can_run_ws_name,
                                              Target="TOF")
                assert can_run_ws is not None
                # smooth background
                smoothParams = self.getProperty("BackgroundSmoothParams").value
                if smoothParams is not None and len(smoothParams) > 0:
                    can_run_ws = api.FFTSmooth(InputWorkspace=can_run_ws_name,
                                               OutputWorkspace=can_run_ws_name,
                                               Filter="Butterworth",
                                               Params=smoothParams,
                                               IgnoreXBins=True,
                                               AllSpectra=True)
                    assert can_run_ws is not None
            # END-IF-ELSE
        # END-IF (can run)

        return can_run_ws_name

    def _process_vanadium_runs(self, van_run_number_list, timeFilterWall, samRunIndex, calib, **focuspos):
        """
        Purpose: process vanadium runs
        Requirements: if more than 1 run in given run number list, then samRunIndex must be given.
        Guarantees: have vanadium run reduced.
        :param van_run_number_list: list of vanadium run
        :param timeFilterWall: time filter wall
        :param samRunIndex: sample run index
        :param calib: calibration run
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
        if "%s_%d" % (self._instrument, van_run_number) in mtd:
            # use the existing vanadium
            van_run_ws_name = "%s_%d" % (self._instrument, van_run_number)
            van_run_ws = mtd[van_run_ws_name]
            assert van_run_ws is not None
            van_run_ws = api.ConvertUnits(InputWorkspace=van_run_ws_name,
                                          OutputWorkspace=van_run_ws_name,
                                          Target="TOF")
            assert van_run_ws is not None
        else:
            # Explicitly load, reduce and correct vanadium runs

            # set up filter wall for van run
            if self.getProperty("FilterCharacterizations").value:
                vanFilterWall = {'FilterByTimeStart': timeFilterWall[0], 'FilterByTimeStop': timeFilterWall[1]}
            else:
                vanFilterWall = {'FilterByTimeStart': Property.EMPTY_DBL, 'FilterByTimeStop': Property.EMPTY_DBL}

            # load the vanadium
            van_run_ws_name = "%s_%d" % (self._instrument, van_run_number)
            if self.getProperty("Sum").value:
                van_run_ws_name = self._loadAndSum(van_run_number_list, van_run_ws_name, **vanFilterWall)
            else:
                van_run_ws_name = self._loadAndSum([van_run_number], van_run_ws_name, **vanFilterWall)

            # load the vanadium background (if appropriate)
            van_bkgd_run_number_list = self._info["empty"].value
            if not noRunSpecified(van_bkgd_run_number_list):
                # determine the van background workspace name
                if len(van_bkgd_run_number_list) == 1:
                    van_bkgd_run_number = van_bkgd_run_number_list[0]
                else:
                    van_bkgd_run_number = van_bkgd_run_number_list[samRunIndex]
                van_bkgd_ws_name = "%s_%d_vb" % (self._instrument, van_bkgd_run_number)

                # load background runs and sum if necessary
                if self.getProperty("Sum").value:
                    van_bkgd_ws_name = self._loadAndSum(van_bkgd_run_number_list, van_bkgd_ws_name, **vanFilterWall)
                else:
                    van_bkgd_ws_name = self._loadAndSum([van_bkgd_run_number], van_bkgd_ws_name, **vanFilterWall)

                van_bkgd_ws = self.get_workspace(van_bkgd_ws_name)
                if van_bkgd_ws.id() == EVENT_WORKSPACE_ID and van_bkgd_ws.getNumberEvents() <= 0:
                    # skip if background run is empty
                    pass
                else:
                    clear_rhs_ws = allEventWorkspaces(van_run_ws_name, van_bkgd_ws_name)
                    temp_ws = api.Minus(LHSWorkspace=van_run_ws_name,
                                        RHSWorkspace=van_bkgd_ws_name,
                                        OutputWorkspace=van_run_ws_name,
                                        ClearRHSWorkspace=clear_rhs_ws)
                    assert temp_ws is not None
                # remove vanadium background workspace
                api.DeleteWorkspace(Workspace=van_bkgd_ws_name)
            # END-IF (vanadium background)

            # compress events
            van_run_ws = self.get_workspace(van_run_ws_name)
            if van_run_ws.id() == EVENT_WORKSPACE_ID:
                van_run_ws = api.CompressEvents(InputWorkspace=van_run_ws_name,
                                                OutputWorkspace=van_run_ws_name,
                                                Tolerance=self.COMPRESS_TOL_TOF)  # 10ns
                assert van_run_ws is not None

            # do the absorption correction
            van_run_ws = api.ConvertUnits(InputWorkspace=van_run_ws_name,
                                          OutputWorkspace=van_run_ws_name,
                                          Target="Wavelength")
            assert van_run_ws is not None

            # set material as Vanadium and correct for multiple scattering
            api.SetSampleMaterial(InputWorkspace=van_run_ws_name,
                                  ChemicalFormula="V",
                                  SampleNumberDensity=0.0721)
            van_run_ws = api.MultipleScatteringCylinderAbsorption(InputWorkspace=van_run_ws_name,
                                                                  OutputWorkspace=van_run_ws_name,
                                                                  CylinderSampleRadius=self._vanRadius)
            assert van_run_ws is not None

            # convert unit to T.O.F.
            api.ConvertUnits(InputWorkspace=van_run_ws_name,
                             OutputWorkspace=van_run_ws_name,
                             Target="TOF")

            # focus the data
            self.log().warning('Reducing vanadium run %s.' % van_run_ws_name)
            van_run_ws = api.AlignAndFocusPowder(InputWorkspace=van_run_ws_name,
                                                 OutputWorkspace=van_run_ws_name,
                                                 CalFileName=calib,
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
            assert van_run_ws is not None

            # convert to d-spacing and do strip vanadium peaks
            if self.getProperty("StripVanadiumPeaks").value:
                van_run_ws = api.ConvertUnits(InputWorkspace=van_run_ws_name,
                                              OutputWorkspace=van_run_ws_name,
                                              Target="dSpacing")
                assert van_run_ws is not None
                van_run_ws = api.StripVanadiumPeaks(InputWorkspace=van_run_ws_name,
                                                    OutputWorkspace=van_run_ws_name,
                                                    FWHM=self._vanPeakFWHM,
                                                    PeakPositionTolerance=self.getProperty("VanadiumPeakTol").value,
                                                    BackgroundType="Quadratic",
                                                    HighBackground=True)
                assert van_run_ws is not None
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
            van_run_ws = api.ConvertUnits(InputWorkspace=van_run_ws_name,
                                          OutputWorkspace=van_run_ws_name,
                                          Target="TOF")
            assert van_run_ws is not None
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

        raw_ws = self.get_workspace(workspace_name=raw_ws_name)
        assert raw_ws.id() == EVENT_WORKSPACE_ID, 'Input workspace for splitting must be an EventWorkspace.'

        # Splitting workspace
        self.log().information("SplitterWorkspace = %s, Information Workspace = %s. " % (
                split_ws_name, str(self._splitinfotablews)))

        base_name = raw_ws_name
        if self._splitinfotablews is None:
            # split without information table
            api.FilterEvents(InputWorkspace=raw_ws_name, OutputWorkspaceBaseName=base_name,
                             SplitterWorkspace=split_ws_name, GroupWorkspaces=True)
        else:
            # split with information table
            api.FilterEvents(InputWorkspace=raw_ws_name, OutputWorkspaceBaseName=base_name,
                             SplitterWorkspace=split_ws_name, InformationWorkspace = str(self._splitinfotablews),
                             GroupWorkspaces=True)
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
            this_ws = self.get_workspace(ws_name)
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
