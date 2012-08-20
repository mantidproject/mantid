"""*WIKI* 


*WIKI*"""

import mantid.simpleapi as api
from mantid.api import *
from mantid.kernel import *
import os

all_algs = AlgorithmFactory.getRegisteredAlgorithms(True)
if 'GatherWorkspaces' in all_algs:
    HAVE_MPI = True
    import boostmpi as mpi
else:
    HAVE_MPI = False

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
            if len(self.iparmFile) <= 0:
                self.iparmFile = None
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
                if frequency is None:
                    raise RuntimeError("Unable to determine frequency from data")
                if wavelength is None:
                    raise RuntimeError("Unable to determine wavelength from data")
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
        sns = ConfigService.getFacility("SNS")
        instruments = []
        for item in sns.instruments("Neutron Diffraction"): instruments.append(item.shortName())
        self.declareProperty("Instrument", "PG3", StringListValidator(instruments), "Powder diffractometer's name")
        arrvalidator = IntArrayBoundedValidator()
        arrvalidator.setLower(0)
        self.declareProperty(IntArrayProperty("RunNumber", values=[0], validator=arrvalidator,
                             direction=Direction.Input))
        extensions = [ "_histo.nxs", "_event.nxs", "_runinfo.xml"]
        self.declareProperty("Extension", "_event.nxs",
                             StringListValidator(extensions))
        self.declareProperty("PreserveEvents", True,
                             "Argument to supply to algorithms that can change from events to histograms.")
        self.declareProperty("Sum", False,
                             "Sum the runs. Does nothing for characterization runs")
        self.declareProperty("PushDataPositive", "None",
                             StringListValidator(["None", "ResetToZero", "AddMinimum"]),
                             "Add a constant to the data that makes it positive over the whole range.")
        self.declareProperty("BackgroundNumber", defaultValue=0, validator=IntBoundedValidator(lower=-1),
                             doc="If specified overrides value in CharacterizationRunsFile If -1 turns off correction.")
        self.declareProperty("VanadiumNumber", defaultValue=0, validator=IntBoundedValidator(lower=-1),
                             doc="If specified overrides value in CharacterizationRunsFile. If -1 turns off correction.")
        self.declareProperty("VanadiumBackgroundNumber", defaultValue=0, validator=IntBoundedValidator(lower=-1),
                             doc="If specified overrides value in CharacterizationRunsFile. If -1 turns off correction.")
        self.declareProperty(FileProperty(name="CalibrationFile",defaultValue="",action=FileAction.Load, 
                                      extensions = ["cal"]))
        self.declareProperty(FileProperty(name="CharacterizationRunsFile",defaultValue="",action=FileAction.OptionalLoad, 
                                      extensions = ["txt"]),"File with characterization runs denoted")
        self.declareProperty("UnwrapRef", 0.,
                             "Reference total flight path for frame unwrapping. Zero skips the correction")
        self.declareProperty("LowResRef", 0.,
                             "Reference DIFC for resolution removal. Zero skips the correction")
        self.declareProperty("CropWavelengthMin", 0.,
                             "Crop the data at this minimum wavelength. Overrides LowResRef.")
        self.declareProperty("RemovePromptPulseWidth", 0.0,
                             "Width of events (in microseconds) near the prompt pulse to remove. 0 disables")
        self.declareProperty("FilterByTimeMin", 0.,
                             "Relative time to start filtering by in seconds. Applies only to sample.")
        self.declareProperty("FilterByTimeMax", 0.,
                             "Relative time to stop filtering by in seconds. Applies only to sample.")
        self.declareProperty("MaxChunkSize", 0.0, "Specify maximum Gbytes of file to read in one chunk.  Default is whole file.")
        self.declareProperty("FilterCharacterizations", False,
                             "Filter the characterization runs using above parameters. This only works for event files.")
        self.declareProperty(FloatArrayProperty("Binning", values=[0.,0.,0.],
                             direction=Direction.Input), "Positive is linear bins, negative is logorithmic")
        self.declareProperty("BinInDspace", True,
                             "If all three bin parameters a specified, whether they are in dspace (true) or time-of-flight (false)")
        self.declareProperty("StripVanadiumPeaks", True,
                             "Subtract fitted vanadium peaks from the known positions.")
        self.declareProperty("VanadiumFWHM", 7, "Default=7")
        self.declareProperty("VanadiumPeakTol", 0.05,
                             "How far from the ideal position a vanadium peak can be during StripVanadiumPeaks. Default=0.05, negative turns off")
        self.declareProperty("VanadiumSmoothParams", "20,2", "Default=20,2")
        self.declareProperty("FilterBadPulses", True, "Filter out events measured while proton charge is more than 5% below average")
        outfiletypes = ['gsas', 'fullprof', 'gsas and fullprof', 'gsas and fullprof and pdfgetn']
        self.declareProperty("FilterByLogValue", "", "Name of log value to filter by")
        self.declareProperty("FilterMinimumValue", 0.0, "Minimum log value for which to keep events.")
        self.declareProperty("FilterMaximumValue", 0.0, "Maximum log value for which to keep events.")
        self.declareProperty("SaveAs", "gsas", StringListValidator(outfiletypes))
        self.declareProperty(FileProperty(name="OutputDirectory",defaultValue="",action=FileAction.Directory))
        self.declareProperty("NormalizeByCurrent", True, "Normalized by Current")
        self.declareProperty("FinalDataUnits", "dSpacing", StringListValidator(["dSpacing","MomentumTransfer"]))

    def _loadData(self, runnumber, extension, filterWall=None, **chunk):
        if  runnumber is None or runnumber <= 0:
            return None
        
        name = "%s_%d" % (self._instrument, runnumber)
        filename = name + extension
        # EMPTY_INT() from C++
        if int(chunk["ChunkNumber"]) < 2147483647:
            name += "_%02d" % (int(chunk["ChunkNumber"]))        
        else:
            name += "_%02d" % 0

        if extension.endswith("_event.nxs"):
            chunk["Precount"] = True
            if filterWall is not None:
                if filterWall[0] > 0.:
                    chunk["FilterByTimeStart"] = filterWall[0]
                if filterWall[1] > 0.:
                    chunk["FilterByTimeStop"] = filterWall[1]
        elif extension.endswith("_histo.nxs"):
            chunk = {}
            
        wksp = api.Load(Filename=filename, OutputWorkspace=name, **chunk)
        return wksp

    def _getStrategy(self, runnumber, extension):
        # generate the workspace name
        wksp = "%s_%d" % (self._instrument, runnumber)
        strategy = []
        Chunks = api.DetermineChunking(Filename=wksp+extension,MaxChunkSize=self._chunks,OutputWorkspace='Chunks')
        for row in Chunks: strategy.append(row)

        return strategy

    def _focusChunks(self, runnumber, extension, filterWall, calib, filterLogs=None, preserveEvents=True,
               normByCurrent=True, filterBadPulsesOverride=True):
        # generate the workspace name
        wksp = "%s_%d" % (self._instrument, runnumber)
        strategy = self._getStrategy(runnumber, extension)
        firstChunk = True
        for chunk in strategy:
            if "ChunkNumber" in chunk:
                self.log().information("Working on chunk %d of %d" % (chunk["ChunkNumber"], chunk["TotalChunks"]))
            temp = self._loadData(runnumber, extension, filterWall, **chunk)
            if self._info is None:
                self._info = self._getinfo(temp)
            temp = self._focus(temp, calib, self._info, filterLogs, preserveEvents, normByCurrent, filterBadPulsesOverride)
            if firstChunk:
                wksp = api.RenameWorkspace(InputWorkspace=temp, OutputWorkspace=wksp)
                firstChunk = False
            else:
                wksp += temp
                api.DeleteWorkspace(temp)
        # Sum workspaces for all mpi tasks
        if HAVE_MPI:
            wksp = api.GatherWorkspaces(InputWorkspace=wksp, PreserveEvents=preserveEvents, AccumulationMethod="Add", OutputWorkspace=wksp)
        if self._chunks > 0:
            # When chunks are added, proton charge is summed for all chunks
            wksp.getRun().integrateProtonCharge()
        api.DeleteWorkspace('Chunks')
        if preserveEvents and not "histo" in self.getProperty("Extension").value:
            wksp = api.CompressEvents(InputWorkspace=wksp, OutputWorkspace=wksp, Tolerance=COMPRESS_TOL_TOF) # 100ns
        if normByCurrent:
            wksp = api.NormaliseByCurrent(InputWorkspace=wksp, OutputWorkspace=wksp)
            wksp.getRun()['gsas_monitor'] = 1

        self._save(wksp, self._info, False, True)
        self.log().information("Done focussing data")

        return wksp

    def _focus(self, wksp, calib, info, filterLogs=None, preserveEvents=True,
               normByCurrent=True, filterBadPulsesOverride=True):
        if wksp is None:
            return None

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
        XMin = 0
        XMax = 0
        if info.tmin > 0.:
            XMin = info.tmin
        if info.tmax > 0.:
            XMax = info.tmax
        if not info.has_dspace:
            XMin = binning[0]
            XMax = binning[-1]
            
        focusPos = self._config.getFocusPos()
        if focusPos is None:
	    focusPos = {}
        wksp = api.AlignAndFocusPowder(InputWorkspace=wksp,OutputWorkspace=wksp,CalFileName=calib,Params=binning,Dspacing=info.has_dspace,
            CropMin=XMin,CropMax=XMax,PreserveEvents=preserveEvents,
            FilterBadPulses=self._filterBadPulses and filterBadPulsesOverride,
            RemovePromptPulseWidth=self._removePromptPulseWidth,CompressTolerance=COMPRESS_TOL_TOF,
            FilterLogName=self.getProperty("FilterByLogValue").value,FilterLogMinimumValue=self.getProperty("FilterMinimumValue").value,
            FilterLogMaximumValue=self.getProperty("FilterMaximumValue").value,
            UnwrapRef=self.getProperty("UnwrapRef").value,LowResRef=self.getProperty("LowResRef").value,
            CropWavelengthMin=self.getProperty("CropWavelengthMin").value,TMin=info.tmin,TMax=info.tmax, **focusPos)

        return wksp

    def _getinfo(self, wksp):
        logs = wksp.getRun()
        # get the frequency
        frequency = None
        if "SpeedRequest1" in logs.keys():
            frequency = logs['SpeedRequest1']
        else:
            self.log().information("'SpeedRequest1' is not specified in logs")
            if "frequency" in logs.keys():
                frequency = logs['frequency']
            else:
                self.log().information("'frequency' is not specified in logs")
                return self._config.getInfo(None, None)
        if frequency.units != "Hz":
            raise RuntimeError("Only know how to deal with frequency in Hz, not %s" % frequency.units)
        frequency = frequency.getStatistics().mean

        if not "LambdaRequest" in logs.keys():
            self.log().information("'LambdaRequest' is not in the datafile")
            return self._config.getInfo(None, None)
        wavelength = logs['LambdaRequest']
        if wavelength.units != "Angstrom":
            raise RuntimeError("Only know how to deal with LambdaRequest in Angstrom, not $s" % wavelength)
        wavelength = wavelength.getStatistics().mean

        self.log().information("frequency: " + str(frequency) + "Hz center wavelength:" + str(wavelength) + "Angstrom")
        return self._config.getInfo(frequency, wavelength)

    def _save(self, wksp, info, normalized, pdfgetn):
        filename = os.path.join(self._outDir, str(wksp))
        if pdfgetn:
            if "pdfgetn" in self._outTypes:
                pdfwksp = str(wksp)+"_norm"
                pdfwksp = api.SetUncertainties(InputWorkspace=wksp, OutputWorkspace=pdfwksp, SetError="sqrt")
                pdfwksp = api.SaveGSS(InputWorkspace=pdfwksp, Filename=filename+".getn", SplitFiles="False", Append=False,
                        MultiplyByBinWidth=False, Bank=info.bank, Format="SLOG", ExtendedHeader=True)
                api.DeleteWorkspace(pdfwksp)
            return # don't do the other bits of saving
        if "gsas" in self._outTypes:
            api.SaveGSS(InputWorkspace=wksp, Filename=filename+".gsa", SplitFiles="False", Append=False, 
                    MultiplyByBinWidth=normalized, Bank=info.bank, Format="SLOG", ExtendedHeader=True)
        if "fullprof" in self._outTypes:
            api.SaveFocusedXYE(InputWorkspace=wksp, StartAtBankNumber=info.bank, Filename=filename+".dat")

        # always save python script
        api.GeneratePythonScript(InputWorkspace=wksp, Filename=filename+".py")

    def PyExec(self):
        # get generic information
        SUFFIX = self.getProperty("Extension").value
        self._config = self.PDConfigFile(self.getProperty("CharacterizationRunsFile").value)
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
#        self._timeMin = self.getProperty("FilterByTimeMin")
#        self._timeMax = self.getProperty("FilterByTimeMax")
        self._filterBadPulses = self.getProperty("FilterBadPulses").value
        self._removePromptPulseWidth = self.getProperty("RemovePromptPulseWidth").value
        filterLogs = self.getProperty("FilterByLogValue").value
        if len(filterLogs.strip()) <= 0:
            filterLogs = None
        else:
            filterLogs = [filterLogs, 
                          self.getProperty("FilterMinimumValue").value, self.getProperty("FilterMaximumValue").value]
        self._vanPeakFWHM = self.getProperty("VanadiumFWHM").value
        self._vanSmoothing = self.getProperty("VanadiumSmoothParams").value
        calib = self.getProperty("CalibrationFile").value
        self._outDir = self.getProperty("OutputDirectory").value
        self._outTypes = self.getProperty("SaveAs").value
        samRuns = self.getProperty("RunNumber").value
        filterWall = (self.getProperty("FilterByTimeMin").value, self.getProperty("FilterByTimeMax").value)
        preserveEvents = self.getProperty("PreserveEvents").value
        normbycurrent = self.getProperty("NormalizeByCurrent").value
        self._info = None
        self._chunks = self.getProperty("MaxChunkSize").value

        workspacelist = [] # all data workspaces that will be converted to d-spacing in the end

        if self.getProperty("Sum").value:
            samRun = None
            info = None
            for temp in samRuns:
                temp = self._focusChunks(temp, SUFFIX, filterWall, calib, filterLogs,
                               preserveEvents=preserveEvents, normByCurrent=normbycurrent)
                tempinfo = self._getinfo(temp)
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
                    samRun = api.Plus(LHSWorkspace=samRun, RHSWorkspace=temp, OutputWorkspace=samRun)
                    if not "histo" in self.getProperty("Extension").value:
                        samRun = api.CompressEvents(InputWorkspace=samRun, OutputWorkspace=samRun,
                                       Tolerance=COMPRESS_TOL_TOF) # 10ns
                    api.DeleteWorkspace(str(temp))
            samRun /= float(len(samRuns))
            samRuns = [samRun]
            workspacelist.append(str(samRun))

        for samRun in samRuns:
            # first round of processing the sample
            if not self.getProperty("Sum").value:
                self._info = None
                samRun = self._focusChunks(samRun, SUFFIX, filterWall, calib, filterLogs,
                               preserveEvents=preserveEvents, normByCurrent=normbycurrent)
                workspacelist.append(str(samRun))

            # process the container
            canRun = self.getProperty("BackgroundNumber").value
            if canRun == 0: # use the version in the info
                canRun = self._info.can
            elif canRun < 0: # turn off the correction
                canRun = 0
            if canRun > 0:
                if ("%s_%d" % (self._instrument, canRun)) in mtd:
                    canRun = mtd["%s_%d" % (self._instrument, canRun)]
                else:
                    if self.getProperty("FilterCharacterizations").value:
                        canRun = self._focusChunks(canRun, SUFFIX, filterWall, calib,
                               preserveEvents=preserveEvents)
                    else:
                        canRun = self._focusChunks(canRun, SUFFIX, (0., 0.), calib,
                               preserveEvents=preserveEvents)
                if HAVE_MPI:
                    if mpi.world.rank == 0:
                        canRun = api.ConvertUnits(InputWorkspace=canRun, OutputWorkspace=canRun, Target="TOF")
                        workspacelist.append(str(canRun))
                else:
                    canRun = api.ConvertUnits(InputWorkspace=canRun, OutputWorkspace=canRun, Target="TOF")
                    workspacelist.append(str(canRun))
            else:
                canRun = None

            # process the vanadium run
            vanRun = self.getProperty("VanadiumNumber").value
            if vanRun == 0: # use the version in the info
                vanRun = self._info.van
            elif vanRun < 0: # turn off the correction
                vanRun = 0
            if vanRun > 0:
                if ("%s_%d" % (self._instrument, vanRun)) in mtd:
                    vanRun = mtd["%s_%d" % (self._instrument, vanRun)]
                else:
                    vnoiseRun = self._info.vnoise # noise run for the vanadium
                    if self.getProperty("FilterCharacterizations").value:
                        vanRun = self._focusChunks(vanRun, SUFFIX, filterWall, calib,
                               preserveEvents=False, normByCurrent = (vnoiseRun <= 0))
                    else:
                        vanRun = self._focusChunks(vanRun, SUFFIX, (0., 0.), calib,
                               preserveEvents=False, normByCurrent = (vnoiseRun <= 0))

                    if (vnoiseRun > 0):
                        if self.getProperty("FilterCharacterizations").value:
                            vnoiseRun = self._focusChunks(vnoiseRun, SUFFIX, filterWall, calib,
                               preserveEvents=False, normByCurrent = False, filterBadPulsesOverride=False)
                        else:
                            vnoiseRun = self._focusChunks(vnoiseRun, SUFFIX, (0., 0.), calib,
                               preserveEvents=False, normByCurrent = False, filterBadPulsesOverride=False)
                        if HAVE_MPI:
                            if mpi.world.rank == 0:
                                vnoiseRun = api.ConvertUnits(InputWorkspace=vnoiseRun, OutputWorkspace=vnoiseRun, Target="TOF")
                                vnoiseRun = api.FFTSmooth(InputWorkspace=vnoiseRun, OutputWorkspace=vnoiseRun, Filter="Butterworth",
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
                                vanRun = api.NormaliseByCurrent(InputWorkspace=vanRun, OutputWorkspace=vanRun)
                                workspacelist.append(str(vnoiseRun))
                        else:
                            vnoiseRun = api.ConvertUnits(InputWorkspace=vnoiseRun, OutputWorkspace=vnoiseRun, Target="TOF")
                            vnoiseRun = api.FFTSmooth(InputWorkspace=vnoiseRun, OutputWorkspace=vnoiseRun, Filter="Butterworth",
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
                            vanRun = api.NormaliseByCurrent(InputWorkspace=vanRun, OutputWorkspace=vanRun)
                            workspacelist.append(str(vnoiseRun))
                    else:
                        vnoiseRun = None

                    vbackRun = self.getProperty("VanadiumBackgroundNumber").value
                    if vbackRun > 0:
                        if ("%s_%d" % (self._instrument, vbackRun)) in mtd:
                            vbackRun = mtd["%s_%d" % (self._instrument, vbackRun)]
                        else:
                            if self.getProperty("FilterCharacterizations").value:
                                vbackRun = self._focusChunks(vbackRun, SUFFIX, filterWall, calib,
                                   preserveEvents=False)
                            else:
                                vbackRun = self._focusChunks(vbackRun, SUFFIX, (0., 0.), calib,
                                   preserveEvents=False)
                        vanRun -= vbackRun
                        workspacelist.append(str(vbackRun))

                    if HAVE_MPI:
                        if mpi.world.rank > 0:
                            return
                    if self.getProperty("StripVanadiumPeaks").value:
                        vanRun = api.ConvertUnits(InputWorkspace=vanRun, OutputWorkspace=vanRun, Target="dSpacing")
                        vanRun = api.StripVanadiumPeaks(InputWorkspace=vanRun, OutputWorkspace=vanRun, FWHM=self._vanPeakFWHM,
                                           PeakPositionTolerance=self.getProperty("VanadiumPeakTol").value,
                                           BackgroundType="Quadratic", HighBackground=True)
                    vanRun = api.ConvertUnits(InputWorkspace=vanRun, OutputWorkspace=vanRun, Target="TOF")
                    vanRun = api.FFTSmooth(InputWorkspace=vanRun, OutputWorkspace=vanRun, Filter="Butterworth",
                              Params=self._vanSmoothing,IgnoreXBins=True,AllSpectra=True)
                    vanRun = api.MultipleScatteringCylinderAbsorption(InputWorkspace=vanRun, OutputWorkspace=vanRun, # numbers for vanadium
                                                         AttenuationXSection=2.8, ScatteringXSection=5.1,
                                                         SampleNumberDensity=0.0721, CylinderSampleRadius=.3175)
                    vanRun = api.SetUncertainties(InputWorkspace=vanRun, OutputWorkspace=vanRun)
                if HAVE_MPI:
                    if mpi.world.rank > 0:
                        return
                vanRun = api.ConvertUnits(InputWorkspace=vanRun, OutputWorkspace=vanRun, Target="TOF")
                workspacelist.append(str(vanRun))
            else:
                vanRun = None

            # the final bit of math
            if canRun is not None:
                samRun -= canRun
                if not "histo" in self.getProperty("Extension").value and preserveEvents:
                    samRun = api.CompressEvents(InputWorkspace=samRun, OutputWorkspace=samRun,
                               Tolerance=COMPRESS_TOL_TOF) # 10ns
                canRun = str(canRun)
            if vanRun is not None:
                samRun /= vanRun
                normalized = True
                samRun.getRun()['van_number'] = vanRun.getRun()['run_number'].value
                vanRun = str(vanRun)
            else:
                normalized = False

            if not "histo" in self.getProperty("Extension").value and preserveEvents and HAVE_MPI is False:
                samRun = api.CompressEvents(InputWorkspace=samRun, OutputWorkspace=samRun,
                           Tolerance=COMPRESS_TOL_TOF) # 5ns/

            # make sure there are no negative values - gsas hates them
            if self.getProperty("PushDataPositive").value != "None":
                addMin = (self.getProperty("PushDataPositive").value == "AddMinimum")
                samRun = api.ResetNegatives(InputWorkspace=samRun, OutputWorkspace=samRun, AddMinimum=addMin, ResetValue=0.)

            # write out the files
            if HAVE_MPI:
                if mpi.world.rank == 0:
                    self._save(samRun, self._info, normalized, False)
                    samRun = str(samRun)
            else:
                self._save(samRun, self._info, normalized, False)
                samRun = str(samRun)
            #mtd.releaseFreeMemory()

        # convert everything into d-spacing
        workspacelist = set(workspacelist) # only do each workspace once
        if HAVE_MPI is False:
            for wksp in workspacelist:
                wksp = api.ConvertUnits(InputWorkspace=wksp, OutputWorkspace=wksp, Target=self.getProperty("FinalDataUnits").value)

# Register algorthm with Mantid.
registerAlgorithm(SNSPowderReduction)
