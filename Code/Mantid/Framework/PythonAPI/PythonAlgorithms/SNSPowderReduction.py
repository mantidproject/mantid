"""*WIKI* 


*WIKI*"""

from MantidFramework import *
from mantidsimple import *
import os

COMPRESS_TOL_TOF = .01

class SNSPowderReduction(PythonAlgorithm):
    class PDConfigFile(object):
        class PDInfo:
            """Inner class for holding configuration information for a reduction."""
            def __init__(self, data, has_dspace=False, has_vnoise=False):
                if data is None:
                    data = [None, None, 1, 0, 0, 0., 0.]
                self.freq = data[0]
                self.wl = data[1]
                self.bank = int(data[2])
                self.van = int(data[3])
                self.can = int(data[4])
                self.vnoise = 0 # default value
                self.has_dspace = has_dspace
                self.tmin = 0. # default value
                self.tmax = 0. # default value

                # calculate the remaining indices
                offset = 5
                if has_vnoise:
                    self.vnoise = int(data[offset])
                    offset += 1

                if has_dspace:
                    self.dmin = data[offset]
                    self.dmax = data[offset+1]
                    offset += 2
                if len(data) > offset:
                    self.tmin = data[offset]
                    if len(data) > offset+1:
                        self.tmax = data[offset+1]
    
        def __init__(self, filename):
            if len(filename.strip()) <= 0:
                filename = None
            self.filename = filename
            self._data = {}
            self.use_dspace = False
            self._use_vnoise = False
            self._focusPos = None
            self.iparmFile = None
            if self.filename is None:
                return
            handle = file(filename, 'r')
            lines = handle.readlines()
            handle.close()

            # create the focus positions
            (lines, self._focusPos) = self._generateFocusPos(lines)
            if len(lines) == 0:
                self.filename = None
                return

            # get the rest of the characterization information
            for line in lines:
                self._addData(line)

        def _generateFocusPos(self, lines):
            if not lines[0].startswith("Instrument parameter file:"):
                return (lines, None)

            result = {}

            # get name of parameter file
            temp = lines[0]
            temp = temp.replace("Instrument parameter file:", "")
            self.iparmFile = temp.strip()
            lines = lines[1:] # delete this line

            # get the spectra into a buffer
            spectrainfo = []
            for line in lines:
                if line.startswith("L1"):
                    break
                spectrainfo.append(line)
            numSpectra = len(spectrainfo)

            result['PrimaryFlightPath'] = lines[numSpectra].split()[1]

            # delete the rest of the focus position info
            lines = lines[numSpectra+1:]

            # parse the focus positions
            specids = []
            l2 = []
            polar = []
            azimuthal = []
            for spec in spectrainfo:
                temp = spec.split()
                specids.append(int(temp[0]))
                l2.append(float(temp[1]))
                polar.append(float(temp[2]))
                azimuthal.append(0.)

            # assign to the correct place
            result['SpectrumIDs'] = specids
            result['L2'] = l2
            result['Polar'] = polar
            result['Azimuthal'] = azimuthal

            return (lines, result)

        def _addData(self, line):
            if line.startswith('#') or len(line.strip()) <= 0:
                if "d_min" in line and "d_max" in line:
                    self.use_dspace = True
                if "vanadium_back" in line:
                    self._use_vnoise = True
                return
            data = line.strip().split()
            data = [float(i) for i in data]
            if data[0] not in self._data.keys():
                self._data[data[0]]={}
            info = self.PDInfo(data, self.use_dspace, self._use_vnoise)
            self._data[info.freq][info.wl]=info
        def __getFrequency(self, request):
            for freq in self._data.keys():
                if 100. * abs(float(freq)-request)/request < 5.:
                    return freq
            raise RuntimeError("Failed to find frequency: %fHz" % request)
    
        def __getWavelength(self, frequency, request):
            for wavelength in self._data[frequency].keys():
                if 100. * abs(wavelength-request)/request < 5.:
                    return wavelength
            raise RuntimeError("Failed to find wavelength: %fAngstrom" % request)
    
        def getInfo(self, frequency, wavelength):
            #print "getInfo(%f, %f)" % (frequency, wavelength)
            if self.filename is not None:
                frequency = self.__getFrequency(float(frequency))
                wavelength = self.__getWavelength(frequency, float(wavelength))
        
                return self._data[frequency][wavelength]
            else:
                return self.PDInfo(None)
        def getFocusPos(self):
            return self._focusPos

    def category(self):
        return "Diffraction;PythonAlgorithms"

    def name(self):
        return "SNSPowderReduction"

    def PyInit(self):
        instruments = ["PG3", "VULCAN", "SNAP", "NOM"]
        self.declareProperty("Instrument", "PG3",
                             Validator=ListValidator(instruments))
        #types = ["Event preNeXus", "Event NeXus"]
        #self.declareProperty("FileType", "Event NeXus",
        #                     Validator=ListValidator(types))
        self.declareListProperty("RunNumber", [0], Validator=ArrayBoundedValidator(Lower=0))
        extensions = [ "_histo.nxs", "_event.nxs", "_neutron_event.dat",
                      "_neutron0_event.dat", "_neutron1_event.dat", "_neutron0_event.dat and _neutron1_event.dat"]
        self.declareProperty("Extension", "_event.nxs",
                             Validator=ListValidator(extensions))
        self.declareProperty("PreserveEvents", True,
                             Description="Argument to supply to algorithms that can change from events to histograms.")
        self.declareProperty("Sum", False,
                             Description="Sum the runs. Does nothing for characterization runs")
        self.declareProperty("BackgroundNumber", 0, Validator=BoundedValidator(Lower=-1),
                             Description="If specified overrides value in CharacterizationRunsFile If -1 turns off correction.")
        self.declareProperty("VanadiumNumber", 0, Validator=BoundedValidator(Lower=-1),
                             Description="If specified overrides value in CharacterizationRunsFile. If -1 turns off correction.")
        self.declareProperty("VanadiumBackgroundNumber", 0, Validator=BoundedValidator(Lower=-1))
        self.declareFileProperty("CalibrationFile", "", FileAction.Load,
                                 [".cal"])
        self.declareFileProperty("CharacterizationRunsFile", "", FileAction.OptionalLoad,
                                 ['.txt'],
                                 Description="File with characterization runs denoted")
        self.declareProperty("UnwrapRef", 0., 
                             Description="Reference total flight path for frame unwrapping. Zero skips the correction")
        self.declareProperty("LowResRef", 0., 
                             Description="Reference DIFC for resolution removal. Zero skips the correction")
        self.declareProperty("RemovePromptPulseWidth", 0.0,
                             Description="Width of events (in microseconds) near the prompt pulse to remove. 0 disables")
        self.declareProperty("FilterByTimeMin", 0.,
                             Description="Relative time to start filtering by in seconds. Applies only to sample.")
        self.declareProperty("FilterByTimeMax", 0.,
                             Description="Relative time to stop filtering by in seconds. Applies only to sample.")
        self.declareProperty("FilterCharacterizations", False,
                             Description="Filter the characterization runs using above parameters. This only works for event files.")
        self.declareListProperty("Binning", [0.,0.,0.],
                             Description="Positive is linear bins, negative is logorithmic")
        self.declareProperty("BinInDspace", True,
                             Description="If all three bin parameters a specified, whether they are in dspace (true) or time-of-flight (false)")
        self.declareProperty("VanadiumFWHM", 7, Description="Default=7")
        self.declareProperty("VanadiumPeakTol", 0.05,
                             Description="How far from the ideal position a vanadium peak can be during StripVanadiumPeaks. Default=0.05, negative turns off")
        self.declareProperty("VanadiumSmoothParams", "20,2", Description="Default=20,2")
        self.declareProperty("FilterBadPulses", True, Description="Filter out events measured while proton charge is more than 5% below average")
        outfiletypes = ['gsas', 'fullprof', 'gsas and fullprof']
        self.declareProperty("FilterByLogValue", "", Description="Name of log value to filter by")
        self.declareProperty("FilterMinimumValue", 0.0, Description="Minimum log value for which to keep events.")
        self.declareProperty("FilterMaximumValue", 0.0, Description="Maximum log value for which to keep events.")
        self.declareProperty("SaveAs", "gsas", ListValidator(outfiletypes))
        self.declareFileProperty("OutputDirectory", "", FileAction.Directory)
        self.declareProperty("NormalizeByCurrent", True, Description="Normalized by Current")
        self.declareProperty("FinalDataUnits", "dSpacing", ListValidator(["dSpacing","MomentumTransfer"]))

    def _findData(self, runnumber, extension):
        #self.log().information(str(dir()))
        #self.log().information(str(dir(mantidsimple)))
        result = FindSNSNeXus(Instrument=self._instrument,
                              RunNumber=runnumber, Extension=extension, UseWebService=True)
#        result = self.executeSubAlg("FindSNSNeXus", Instrument=self._instrument,
#                                    RunNumber=runnumber, Extension=extension)
        return result["ResultPath"].value

    def _addNeXusLogs(self, wksp, nxsfile, reloadInstr):
        try:
            LoadNexusLogs(Workspace=wksp, Filename=nxsfile)
            if reloadInstr:
                LoadInstrument(Workspace=wksp, InstrumentName=self._instrument, RewriteSpectraMap=False)
            return True
        except:
            return False


    def _loadPreNeXusData(self, runnumber, extension, **kwargs):
        mykwargs = {}
        if kwargs.has_key("FilterByTimeStart"):
            mykwargs["ChunkNumber"] = int(kwargs["FilterByTimeStart"])
        if kwargs.has_key("FilterByTimeStop"):
            mykwargs["TotalChunks"] = int(kwargs["FilterByTimeStop"])

        # generate the workspace name
        name = "%s_%d" % (self._instrument, runnumber)
        filename = name + extension
        print filename

        try: # first just try loading the file
            alg = LoadEventPreNexus(EventFilename=filename, OutputWorkspace=name, **mykwargs)
            wksp = alg['OutputWorkspace']
        except:
            # find the file to load
            filename = self._findData(runnumber, extension)
            if (len(filename) == 0):
                raise RuntimeError("Failed to find prenexus file for run %d" % runnumber)
            # load the prenexus file
            alg = LoadEventPreNexus(EventFilename=filename, OutputWorkspace=name, **mykwargs)
            wksp = alg['OutputWorkspace']

        # add the logs to it
        reloadInstr = (str(self._instrument) == "SNAP")

        datadir = os.path.split(filename)[0]
        nxsfile_event = "%s_%d_event.nxs" % (self._instrument, runnumber)
        nxsfile_histo = "%s_%d_histo.nxs" % (self._instrument, runnumber)

        files = [nxsfile_event, nxsfile_histo,
                 os.path.join(datadir, nxsfile_event),
                 os.path.join(datadir, nxsfile_histo)]
        datadir = os.path.split(datadir)[0]
        files.extend([os.path.join(datadir, "NeXus", nxsfile_event),
                      os.path.join(datadir, "NeXus", nxsfile_histo)])
        for nxsfile in files:
            if self._addNeXusLogs(wksp, nxsfile, reloadInstr):
                return wksp

        # TODO filter out events using timemin and timemax

        return wksp

    def _loadEventNeXusData(self, runnumber, extension, **kwargs):
        kwargs["Precount"] = True
        name = "%s_%d" % (self._instrument, runnumber)
        filename = name + extension

        try: # first just try loading the file
            alg = LoadEventNexus(Filename=filename, OutputWorkspace=name, **kwargs)

            return alg.workspace()
        except:
            pass

        # find the file to load
        filename = self._findData(runnumber, extension)

        # TODO use timemin and timemax to filter what events are being read
        alg = LoadEventNexus(Filename=filename, OutputWorkspace=name, **kwargs)

        return alg.workspace()

    def _loadHistoNeXusData(self, runnumber, extension):
        name = "%s_%d" % (self._instrument, runnumber)
        filename = name + extension

        try: # first just try loading the file
            alg = LoadTOFRawNexus(Filename=filename, OutputWorkspace=name)

            return alg.workspace()
        except:
            pass

        # find the file to load
        filename = self._findData(runnumber, extension)

        alg = LoadTOFRawNexus(Filename=filename, OutputWorkspace=name)

        return alg.workspace()

    def _loadData(self, runnumber, extension, filterWall=None):
        filter = {}
        if filterWall is not None:
            if filterWall[0] > 0.:
                filter["FilterByTimeStart"] = filterWall[0]
            if filterWall[1] > 0.:
                filter["FilterByTimeStop"] = filterWall[1]

        if  runnumber is None or runnumber <= 0:
            return None

        if extension.endswith("_event.nxs"):
            return self._loadEventNeXusData(runnumber, extension, **filter)
        elif extension.endswith("_histo.nxs"):
            return self._loadHistoNeXusData(runnumber, extension)
        elif "and" in extension:
            wksp0 = self._loadPreNeXusData(runnumber, "_neutron0_event.dat", **filter)
            RenameWorkspace(InputWorkspace=wksp0,OutputWorkspace="tmp")
            wksp1 = self._loadPreNeXusData(runnumber, "_neutron1_event.dat", **filter)
            Plus(LHSWorkspace=wksp1, RHSWorkspace="tmp",OutputWorkspace=wksp1)
            mtd.deleteWorkspace("tmp")
            return wksp1;
        else:
            return self._loadPreNeXusData(runnumber, extension, **filter)

    def _focus(self, wksp, calib, info, filterLogs=None, preserveEvents=True,
               normByCurrent=True, filterBadPulsesOverride=True):
        if wksp is None:
            return None

        # load the calibration file if the workspaces aren't already in memory
        if (not mtd.workspaceExists(self._instrument + "_offsets")) \
              or (not mtd.workspaceExists(self._instrument + "_mask")) \
              or (not mtd.workspaceExists(self._instrument + "_group")):
            whichones = {}
            whichones['MakeGroupingWorkspace'] = (not mtd.workspaceExists(self._instrument + "_group"))
            whichones['MakeOffsetsWorkspace'] = (not mtd.workspaceExists(self._instrument + "_offsets"))
            whichones['MakeMaskWorkspace'] = (not mtd.workspaceExists(self._instrument + "_group"))
            LoadCalFile(InputWorkspace=wksp, CalFileName=calib, WorkspaceName=self._instrument,
                        **whichones)

        if not "histo" in self.getProperty("Extension"):
            # take care of filtering events
            if self._filterBadPulses and filterBadPulsesOverride:
                FilterBadPulses(InputWorkspace=wksp, OutputWorkspace=wksp)
            if self._removePromptPulseWidth > 0.:
                RemovePromptPulse(InputWorkspace=wksp, OutputWorkspace=wksp, Width= self._removePromptPulseWidth)
            if filterLogs is not None:
                try:
                    logparam = wksp.getRun()[filterLogs[0]]
                    if logparam is not None:
                        FilterByLogValue(InputWorkspace=wksp, OutputWorkspace=wksp, LogName=filterLogs[0],
                                         MinimumValue=filterLogs[1], MaximumValue=filterLogs[2])
                except KeyError, e:
                    raise RuntimeError("Failed to find log '%s' in workspace '%s'" \
                                       % (filterLogs[0], str(wksp)))            
            CompressEvents(InputWorkspace=wksp, OutputWorkspace=wksp, Tolerance=COMPRESS_TOL_TOF) # 100ns
            SortEvents(wksp)
        # determine some bits about d-space and binning
        if len(self._binning) == 3:
            info.has_dspace = self._bin_in_dspace
        if info.has_dspace:
            if len(self._binning) == 3:
                binning = self._binning
            else:
                binning = [info.dmin, self._binning[0], info.dmax]
            self.log().information("d-Spacing Binning: " + str(binning))
        else:
            if len(self._binning) == 3:
                binning = self._binning
            else:
                binning = [info.tmin, self._binning[0], info.tmax]

        # trim off the ends
        cropkwargs = {}
        if info.tmin > 0.:
            cropkwargs["XMin"] = info.tmin
        if info.tmax > 0.:
            cropkwargs["XMax"] = info.tmax
        if not info.has_dspace:
            cropkwargs["XMin"] = binning[0]
            cropkwargs["XMax"] = binning[-1]
        if len(cropkwargs) > 0:
            CropWorkspace(InputWorkspace=wksp, OutputWorkspace=wksp, **cropkwargs)
        MaskDetectors(Workspace=wksp, MaskedWorkspace=self._instrument + "_mask")
        if not info.has_dspace:
            Rebin(InputWorkspace=wksp, OutputWorkspace=wksp, Params=binning)
        AlignDetectors(InputWorkspace=wksp, OutputWorkspace=wksp, OffsetsWorkspace=self._instrument + "_offsets")
        LRef = self.getProperty("UnwrapRef")
        DIFCref = self.getProperty("LowResRef")
        if (LRef > 0.) or (DIFCref > 0.): # super special Jason stuff
            kwargs = {}
            try:
                if info.tmin > 0:
                    kwargs["Tmin"] = info.tmin
                    if info.tmax > info.tmin:
                        kwargs["Tmax"] = info.tmax
            except:
                pass
            ConvertUnits(InputWorkspace=wksp, OutputWorkspace=wksp, Target="TOF") # corrections only work in TOF for now
            if LRef > 0:
                UnwrapSNS(InputWorkspace=wksp, OutputWorkspace=wksp, LRef=LRef, **kwargs)
            if DIFCref > 0:
                if kwargs.has_key("Tmax"):
                    del kwargs["Tmax"]
                RemoveLowResTOF(InputWorkspace=wksp, OutputWorkspace=wksp, ReferenceDIFC=DIFCref,
                                K=3.22, **kwargs)
            ConvertUnits(InputWorkspace=wksp, OutputWorkspace=wksp, Target="dSpacing") # put it back to the original units
        if info.has_dspace:
            self.log().information("d-Spacing binning: " + str(binning))
            Rebin(InputWorkspace=wksp, OutputWorkspace=wksp, Params=binning)
        if not "histo" in self.getProperty("Extension"):
            SortEvents(InputWorkspace=wksp)
        DiffractionFocussing(InputWorkspace=wksp, OutputWorkspace=wksp, GroupingWorkspace=self._instrument + "_group",
                             PreserveEvents=preserveEvents)
        if not "histo" in self.getProperty("Extension") and preserveEvents:
            SortEvents(InputWorkspace=wksp)

        focusPos = self._config.getFocusPos()
        self.log().notice("FOCUS:" + str(focusPos))
        if not focusPos is None:
            EditInstrumentGeometry(Workspace=wksp, NewInstrument=False, **focusPos)
            wksp.getRun()['iparm_file'] = self._config.iparmFile

        ConvertUnits(InputWorkspace=wksp, OutputWorkspace=wksp, Target="TOF")
        if preserveEvents and not "histo" in self.getProperty("Extension"):
            CompressEvents(InputWorkspace=wksp, OutputWorkspace=wksp, Tolerance=COMPRESS_TOL_TOF) # 100ns
        if normByCurrent:
            NormaliseByCurrent(InputWorkspace=wksp, OutputWorkspace=wksp)
            wksp.getRun()['gsas_monitor'] = 1

        return wksp

    def _getinfo(self, wksp):
        logs = wksp.getRun()
        # get the frequency
        if not "SpeedRequest1" in logs.keys():
            self.log().information("SpeedRequest1 is not specified")
            return self._config.getInfo(None, None)
        frequency = logs['SpeedRequest1']
        if frequency.units != "Hz":
            raise RuntimeError("Only know how to deal with frequency in Hz, not %s" % frequency.units)
        frequency = frequency.getStatistics().mean

        if not "LambdaRequest" in logs.keys():
            self.log().information("LambdaRequest is not in the datafile")
            return self._config.getInfo(None, None)
        wavelength = logs['LambdaRequest']
        if wavelength.units != "Angstrom":
            raise RuntimeError("Only know how to deal with LambdaRequest in Angstrom, not $s" % wavelength)
        wavelength = wavelength.getStatistics().mean

        self.log().information("frequency: " + str(frequency) + "Hz center wavelength:" + str(wavelength) + "Angstrom")
        return self._config.getInfo(frequency, wavelength)

    def _getMin(self, wksp):
        minWksp = "__" + str(wksp)+"_min"
        Min(wksp, minWksp)
        minWksp = mtd[minWksp]
        minValue = 0.
        for i in range(minWksp.getNumberHistograms()):
          if minValue > minWksp.dataY(i)[0]:
            minValue = minWksp.dataY(i)[0]
        mtd.deleteWorkspace(str(minWksp))
        return minValue

    def _save(self, wksp, info, normalized):
        filename = os.path.join(self._outDir, str(wksp))
        if "gsas" in self._outTypes:
            SaveGSS(InputWorkspace=wksp, Filename=filename+".gsa", SplitFiles="False", Append=False, 
                    MultiplyByBinWidth=normalized, Bank=info.bank, Format="SLOG", ExtendedHeader=True)
        if "fullprof" in self._outTypes:
            SaveFocusedXYE(InputWorkspace=wksp, Filename=filename+".dat")

        # always save python script
        GeneratePythonScript(InputWorkspace=wksp, Filename=filename+".py")

    def PyExec(self):
        # temporary hack for getting python algorithms working
        import mantidsimple
        globals()["FindSNSNeXus"] = mantidsimple.FindSNSNeXus

        # get generic information
        SUFFIX = self.getProperty("Extension")
        self._config = self.PDConfigFile(self.getProperty("CharacterizationRunsFile"))
        self._binning = self.getProperty("Binning")
        if len(self._binning) != 1 and len(self._binning) != 3:
            raise RuntimeError("Can only specify (width) or (start,width,stop) for binning. Found %d values." % len(self._binning))
        if len(self._binning) == 3:
            if self._binning[0] == 0. and self._binning[1] == 0. and self._binning[2] == 0.:
                raise RuntimeError("Failed to specify the binning")
        self._bin_in_dspace = self.getProperty("BinInDspace")
        self._instrument = self.getProperty("Instrument")
        mtd.settings['default.facility'] = "SNS"
        mtd.settings['default.instrument'] = self._instrument
#        self._timeMin = self.getProperty("FilterByTimeMin")
#        self._timeMax = self.getProperty("FilterByTimeMax")
        self._filterBadPulses = self.getProperty("FilterBadPulses")
        self._removePromptPulseWidth = self.getProperty("RemovePromptPulseWidth")
        filterLogs = self.getProperty("FilterByLogValue")
        if len(filterLogs.strip()) <= 0:
            filterLogs = None
        else:
            filterLogs = [filterLogs, 
                          self.getProperty("FilterMinimumValue"), self.getProperty("FilterMaximumValue")]
        self._vanPeakFWHM = self.getProperty("VanadiumFWHM")
        self._vanSmoothing = self.getProperty("VanadiumSmoothParams")
        calib = self.getProperty("CalibrationFile")
        self._outDir = self.getProperty("OutputDirectory")
        self._outTypes = self.getProperty("SaveAs")
        samRuns = self.getProperty("RunNumber")
        filterWall = (self.getProperty("FilterByTimeMin"), self.getProperty("FilterByTimeMax"))
        preserveEvents = self.getProperty("PreserveEvents")
        normbycurrent = self.getProperty("NormalizeByCurrent")

        workspacelist = [] # all data workspaces that will be converted to d-spacing in the end

        if self.getProperty("Sum"):
            samRun = None
            info = None
            for temp in samRuns:
                temp = self._loadData(temp, SUFFIX, filterWall)
                tempinfo = self._getinfo(temp)
                temp = self._focus(temp, calib, tempinfo, filterLogs, preserveEvents=preserveEvents, normByCurrent=normbycurrent)
                if samRun is None:
                    samRun = temp
                    info = tempinfo
                else:
                    if (tempinfo.freq is not None) and (info.freq is not None) \
                            and (abs(tempinfo.freq - info.freq)/info.freq > .05):
                        raise RuntimeError("Cannot add incompatible frequencies (%f!=%f)" \
                                           % (tempinfo.freq, info.freq))
                    if (tempinfo.wl is not None) and (info.wl is not None) \
                            and abs(tempinfo.wl - info.wl)/info.freq > .05:
                        raise RuntimeError("Cannot add incompatible wavelengths (%f != %f)" \
                                           % (tempinfo.wl, info.wl))
                    Plus(samRun, temp, samRun)
                    if not "histo" in self.getProperty("Extension"):
                        CompressEvents(InputWorkspace=samRun, OutputWorkspace=samRun,
                                       Tolerance=COMPRESS_TOL_TOF) # 10ns
                    mtd.deleteWorkspace(str(temp))
            samRun /= float(len(samRuns))
            samRuns = [samRun]
            workspacelist.append(str(samRun))

        for samRun in samRuns:
            # first round of processing the sample
            if not self.getProperty("Sum"):
                samRun = self._loadData(samRun, SUFFIX, filterWall)
                info = self._getinfo(samRun)
                samRun = self._focus(samRun, calib, info, filterLogs, preserveEvents=preserveEvents, normByCurrent=normbycurrent)
                workspacelist.append(str(samRun))

            # process the container
            canRun = self.getProperty("BackgroundNumber")
            if canRun == 0: # use the version in the info
                canRun = info.can
            elif canRun < 0: # turn off the correction
                canRun = 0
            if canRun > 0:
                if mtd.workspaceExists("%s_%d" % (self._instrument, canRun)):
                    canRun = mtd["%s_%d" % (self._instrument, canRun)]
                else:
                    if self.getProperty("FilterCharacterizations"):
                        canRun = self._loadData(canRun, SUFFIX, filterWall)
                    else:
                        canRun = self._loadData(canRun, SUFFIX, (0., 0.))
                    canRun = self._focus(canRun, calib, info, preserveEvents=preserveEvents)
                ConvertUnits(InputWorkspace=canRun, OutputWorkspace=canRun, Target="TOF")
                workspacelist.append(str(canRun))
            else:
                canRun = None

            # process the vanadium run
            vanRun = self.getProperty("VanadiumNumber")
            if vanRun == 0: # use the version in the info
                vanRun = info.van
            elif vanRun < 0: # turn off the correction
                vanRun = 0
            if vanRun > 0:
                if mtd.workspaceExists("%s_%d" % (self._instrument, vanRun)):
                    vanRun = mtd["%s_%d" % (self._instrument, vanRun)]
                else:
                    if self.getProperty("FilterCharacterizations"):
                        vanRun = self._loadData(vanRun, SUFFIX, filterWall)
                    else:
                        vanRun = self._loadData(vanRun, SUFFIX, (0., 0.))
                    vnoiseRun = info.vnoise # noise run for the vanadium
                    vanRun = self._focus(vanRun, calib, info, preserveEvents=False, normByCurrent = (vnoiseRun <= 0))

                    if (vnoiseRun > 0):
                        if self.getProperty("FilterCharacterizations"):
                            vnoiseRun = self._loadData(vnoiseRun, SUFFIX, filterWall)
                        else:
                            vnoiseRun = self._loadData(vnoiseRun, SUFFIX, (0., 0.))
                        vnoiseRun = self._focus(vnoiseRun, calib, info, preserveEvents=False,
                                               normByCurrent=False, filterBadPulsesOverride=False)
                        ConvertUnits(InputWorkspace=vnoiseRun, OutputWorkspace=vnoiseRun, Target="TOF")
                        FFTSmooth(InputWorkspace=vnoiseRun, OutputWorkspace=vnoiseRun, Filter="Butterworth",
                                  Params=self._vanSmoothing,IgnoreXBins=True,AllSpectra=True)
                        try:
                            vanDuration = vanRun.getRun().get('duration')
                            vanDuration = vanDuration.value
                        except:
                            vanDuration = 1.
                        try:
                            vbackDuration = vnoiseRun.getRun().get('duration')
                            vbackDuration = vbackDuration.value
                        except:
                            vbackDuration = 1.
                        vnoiseRun *= (vanDuration/vbackDuration)
                        vanRun -= vnoiseRun
                        NormaliseByCurrent(InputWorkspace=vanRun, OutputWorkspace=vanRun)
                        workspacelist.append(str(vnoiseRun))
                    else:
                        vnoiseRun = None

                    vbackRun = self.getProperty("VanadiumBackgroundNumber")
                    if vbackRun > 0:
                        if self.getProperty("FilterCharacterizations"):
                            vbackRun = self._loadData(vbackRun, SUFFIX, filterWall)
                        else:
                            vbackRun = self._loadData(vbackRun, SUFFIX, (0., 0.))
                        vbackRun = self._focus(vbackRun, calib, info, preserveEvents=False)
                        vanRun -= vbackRun

                    ConvertUnits(InputWorkspace=vanRun, OutputWorkspace=vanRun, Target="dSpacing")
                    StripVanadiumPeaks(InputWorkspace=vanRun, OutputWorkspace=vanRun, FWHM=self._vanPeakFWHM,
                                       PeakPositionTolerance=self.getProperty("VanadiumPeakTol"), HighBackground=True)
                    ConvertUnits(InputWorkspace=vanRun, OutputWorkspace=vanRun, Target="TOF")
                    FFTSmooth(InputWorkspace=vanRun, OutputWorkspace=vanRun, Filter="Butterworth",
                              Params=self._vanSmoothing,IgnoreXBins=True,AllSpectra=True)
                    MultipleScatteringCylinderAbsorption(InputWorkspace=vanRun, OutputWorkspace=vanRun, # numbers for vanadium
                                                         AttenuationXSection=2.8, ScatteringXSection=5.1,
                                                         SampleNumberDensity=0.0721, CylinderSampleRadius=.3175)
                    SetUncertaintiesToZero(InputWorkspace=vanRun, OutputWorkspace=vanRun)
                ConvertUnits(InputWorkspace=vanRun, OutputWorkspace=vanRun, Target="TOF")
                workspacelist.append(str(vanRun))
            else:
                vanRun = None

            # the final bit of math
            if canRun is not None:
                samRun -= canRun
                if not "histo" in self.getProperty("Extension") and preserveEvents:
                    CompressEvents(InputWorkspace=samRun, OutputWorkspace=samRun,
                               Tolerance=COMPRESS_TOL_TOF) # 10ns
                canRun = str(canRun)
            if vanRun is not None:
                samRun /= vanRun
                normalized = True
                samRun.getRun()['van_number'] = vanRun.getRun()['run_number'].value
                vanRun = str(vanRun)
            else:
                normalized = False

            if not "histo" in self.getProperty("Extension") and preserveEvents:
                CompressEvents(InputWorkspace=samRun, OutputWorkspace=samRun,
                           Tolerance=COMPRESS_TOL_TOF) # 5ns

            # make sure there are no negative values - gsas hates them
            minY = self._getMin(samRun)
            if minY < 0.:
                self.log().notice("Minimum y = " + str(minY) + " adding to all y-values")
                minY *= -1.
                ConvertToMatrixWorkspace(InputWorkspace=samRun, OutputWorkspace=samRun)
                samRun += minY

            # write out the files
            self._save(samRun, info, normalized)
            samRun = str(samRun)
            mtd.releaseFreeMemory()

        # convert everything into d-spacing
        workspacelist = set(workspacelist) # only do each workspace once
        for wksp in workspacelist:
            ConvertUnits(InputWorkspace=wksp, OutputWorkspace=wksp, Target=self.getProperty("FinalDataUnits"))

mtd.registerPyAlgorithm(SNSPowderReduction())
