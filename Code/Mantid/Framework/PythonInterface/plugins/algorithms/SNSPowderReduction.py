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
                self.dmin = 0. # default value
                self.dmax = 0. # default value

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
            self._focusPos = {}
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
                return (lines, {})

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
            for i in range(len(data)):
                if ',' in data[i]:
                    temp = data[i].split(',')
                    temp = [float(item) for item in temp]
                    data[i] = temp
                else:
                    data[i] = float(data[i])
            if data[0] not in self._data.keys():
                self._data[data[0]]={}
            info = self.PDInfo(data, self.use_dspace, self._use_vnoise)
            self._data[info.freq][info.wl]=info
        def __getFrequency(self, request):
            for freq in self._data.keys():
                # commit 579b5941a6618dc8c4f2ad7838484e375a24ac37
                if abs(float(freq)-request) == 0.:                    
                    return freq
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
                             direction=Direction.Input), "Number of sample run or 0 for only Vanadium and/or Background")
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
        self.declareProperty("MaxChunkSize", 0.0, "Specify maximum Gbytes of file to read in one chunk.  Default is whole file.")
        self.declareProperty("FilterCharacterizations", False,
                             "Filter the characterization runs using above parameters. This only works for event files.")
        self.declareProperty(FloatArrayProperty("Binning", values=[0.,0.,0.],
                             direction=Direction.Input), "Positive is linear bins, negative is logorithmic")
        self.declareProperty("ResampleX", 0,
                             "Number of bins in x-axis. Non-zero value overrides \"Params\" property. Negative value means logorithmic binning.")
        self.declareProperty("BinInDspace", True,
                             "If all three bin parameters a specified, whether they are in dspace (true) or time-of-flight (false)")
        self.declareProperty("StripVanadiumPeaks", True,
                             "Subtract fitted vanadium peaks from the known positions.")
        self.declareProperty("VanadiumFWHM", 7, "Default=7")
        self.declareProperty("VanadiumPeakTol", 0.05,
                             "How far from the ideal position a vanadium peak can be during StripVanadiumPeaks. Default=0.05, negative turns off")
        self.declareProperty("VanadiumSmoothParams", "20,2", "Default=20,2")
        self.declareProperty("FilterBadPulses", True, "Filter out events measured while proton charge is more than 5% below average")
        outfiletypes = ['gsas', 'fullprof', 'gsas and fullprof', 'gsas and fullprof and pdfgetn', 'NeXus',
                            'gsas and NeXus', 'fullprof and NeXus', 'gsas and fullprof and NeXus', 'gsas and fullprof and pdfgetn and NeXus']
        self.declareProperty("SaveAs", "gsas", StringListValidator(outfiletypes))
        self.declareProperty("OutputFilePrefix", "", "Overrides the default filename for the output file (Optional).")
        self.declareProperty(FileProperty(name="OutputDirectory",defaultValue="",action=FileAction.Directory))
        self.declareProperty("NormalizeByCurrent", True, "Normalized by Current")
        self.declareProperty("FinalDataUnits", "dSpacing", StringListValidator(["dSpacing","MomentumTransfer"]))

        tableprop = ITableWorkspaceProperty("SplittersWorkspace", "", Direction.Input, PropertyMode.Optional)
        self.declareProperty(tableprop, "Splitters workspace for split event workspace.")
        # self.declareProperty("KeepRemainder", True, "Keeping the remainder events workspace if true.")

        """ Disabled Due To #
        self.declareProperty("FilterByLogValue", "", "Name of log value to filter by")
        self.declareProperty("FilterMinimumValue", 0.0, "Minimum log value for which to keep events.")
        self.declareProperty("FilterMaximumValue", 0.0, "Maximum log value for which to keep events.")
        self.declareProperty("FilterByTimeMin", 0.,
                             "Relative time to start filtering by in seconds. Applies only to sample.")
        self.declareProperty("FilterByTimeMax", 0.,
                             "Relative time to stop filtering by in seconds. Applies only to sample.")
        """

        return


    def PyExec(self):
        """ Main execution body
        """
        # get generic information
        SUFFIX = self.getProperty("Extension").value
        self._config = self.PDConfigFile(self.getProperty("CharacterizationRunsFile").value)
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
        self._vanPeakFWHM = self.getProperty("VanadiumFWHM").value
        self._vanSmoothing = self.getProperty("VanadiumSmoothParams").value
        calib = self.getProperty("CalibrationFile").value
        self._outDir = self.getProperty("OutputDirectory").value
        self._outPrefix = self.getProperty("OutputFilePrefix").value
        self._outTypes = self.getProperty("SaveAs").value
        samRuns = self.getProperty("RunNumber").value
        preserveEvents = self.getProperty("PreserveEvents").value
        normbycurrent = self.getProperty("NormalizeByCurrent").value
        self._info = None
        self._chunks = self.getProperty("MaxChunkSize").value

        self._splitws = self.getProperty("SplittersWorkspace").value
        if self._splitws is not None:
            print "SplittersWorkspace is %s" % (str(self._splitws))
            if len(samRuns) != 1:
                raise NotImplementedError("Reducing data with splitting cannot happen when there are more than 1 sample run.")
            filterWall = self.getFilterWall(self._splitws, samRuns[0], SUFFIX)
            print "So ... the filter wall is " , str(filterWall)
        else:
            filterWall = (0.0, 0.0)
            print "SplittersWorkspace is None!"

        # Process data

        workspacelist = [] # all data workspaces that will be converted to d-spacing in the end
        samwksplist = []

        if self.getProperty("Sum").value:
            # Sum input sample runs and then do reduction
            if self._splitws is not None:
                raise NotImplementedError("Summing spectra and filtering events are not supported simultaneously.")

            samRun = None
            info = None
            for temp in samRuns:
                runnumber = temp
                print "[Sum] Process run number ", str(runnumber)

                temp = self._focusChunks(temp, SUFFIX, filterWall, calib, 
                        preserveEvents=preserveEvents, normByCurrent=normbycurrent)
                tempinfo = self._getinfo(temp)

                print "Run %s Focused result of type %s" % (str(runnumber), str(temp))

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
                    if not "histo" in SUFFIX:
                        samRun = api.CompressEvents(InputWorkspace=samRun, OutputWorkspace=samRun,
                                       Tolerance=COMPRESS_TOL_TOF) # 10ns
                    api.DeleteWorkspace(str(temp))
                # ENDIF
            # ENDFOR (processing each)

            samRun /= float(len(samRuns))
            samRuns = [samRun]
            workspacelist.append(str(samRun))
            samwksplist.append(str(samRun))
        # ENDIF

        for samRun in samRuns:
            # first round of processing the sample
            if not self.getProperty("Sum").value and samRun > 0:
                self._info = None
                returned = self._focusChunks(samRun, SUFFIX, filterWall, calib, self._splitws, 
                        preserveEvents=preserveEvents, normByCurrent=normbycurrent)
                
                if returned.__class__.__name__ == "list":
                    # Returned with a list of workspaces
                    focusedwksplist = returned
                    irun = 0
                    for run in focusedwksplist:
                        if run is not None: 
                            samwksplist.append(run) 
                            workspacelist.append(str(run))
                        else:
                            print "Found a None entry in returned focused workspaces.  Index = %d." % (irun)
                        # ENDIF
                        irun += 1
                    # ENDFOR
                else:
                    run = returned 
                    samwksplist.append(run) 
                    workspacelist.append(str(run))
                # ENDIF
            # ENDIF
        # ENDFOR

        for samRun in samwksplist:
            samRun = mtd[str(samRun)] 
            try: 
                print "[DBx1136] Sample Run %s:  number of events = %d" % (str(samRun), samRun.getNumberEvents())
            except Exception as e:
                print "[DBx1136] Unable to get number of events of sample run %s.  Error message: %s" % (str(samRun), str(e))

            # process the container
            canRun = self.getProperty("BackgroundNumber").value
            if canRun == 0: # use the version in the info
                canRun = self._info.can
            elif canRun < 0: # turn off the correction
                canRun = 0
            if canRun > 0:
                canFile = "%s_%d" % (self._instrument, canRun)+".nxs"
                if HAVE_MPI and os.path.exists(canFile):
                    if mpi.world.rank == 0:                     
                        canRun = "%s_%d" % (self._instrument, canRun)
                        canRun = api.Load(Filename=canFile, OutputWorkspace=canRun)
                elif ("%s_%d" % (self._instrument, canRun)) in mtd:
                    canRun = mtd["%s_%d" % (self._instrument, canRun)]
                    canRun = api.ConvertUnits(InputWorkspace=canRun, OutputWorkspace=canRun, Target="TOF")
                else:
                    if self.getProperty("FilterCharacterizations").value:
                        canRun = self._focusChunks(canRun, SUFFIX, filterWall, calib,
                               preserveEvents=preserveEvents)
                    else:
                        canRun = self._focusChunks(canRun, SUFFIX, (0., 0.), calib,
                               preserveEvents=preserveEvents)
                    canRun = api.ConvertUnits(InputWorkspace=canRun, OutputWorkspace=canRun, Target="TOF")
                    if HAVE_MPI:
                        if mpi.world.rank == 0:
                            api.SaveNexus(InputWorkspace=canRun, Filename=canFile)
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
                vanFile = "%s_%d" % (self._instrument, vanRun)+".nxs"
                if HAVE_MPI and os.path.exists(vanFile):
                    if mpi.world.rank == 0:                     
                        vanRun = "%s_%d" % (self._instrument, vanRun)
                        vanRun = api.Load(Filename=vanFile, OutputWorkspace=vanRun)
                elif ("%s_%d" % (self._instrument, vanRun)) in mtd:
                    vanRun = mtd["%s_%d" % (self._instrument, vanRun)]
                    vanRun = api.ConvertUnits(InputWorkspace=vanRun, OutputWorkspace=vanRun, Target="TOF")
                else:
                    if samRun == 0:
                        vnoiseRun = 0
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
                    vanRun = api.ConvertUnits(InputWorkspace=vanRun, OutputWorkspace=vanRun, Target="TOF")
                    if HAVE_MPI:
                        if mpi.world.rank == 0:
                            api.SaveNexus(InputWorkspace=vanRun, Filename=vanFile)
                workspacelist.append(str(vanRun))
            else:
                vanRun = None

            if HAVE_MPI:
                if mpi.world.rank > 0:
                    return
            if samRun == 0:
                return
            # the final bit of math
            if canRun is not None:
                samRun -= canRun
                if not "histo" in SUFFIX and preserveEvents:
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

            if not "histo" in SUFFIX and preserveEvents and HAVE_MPI is False:
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
        # ENDFOR

        # convert everything into d-spacing
        workspacelist = set(workspacelist) # only do each workspace once
        if HAVE_MPI is False:
            for wksp in workspacelist:
                wksp = api.ConvertUnits(InputWorkspace=wksp, OutputWorkspace=wksp, Target=self.getProperty("FinalDataUnits").value)

        return

    def _loadData(self, runnumber, extension, filterWall=None, **chunk):
        if  runnumber is None or runnumber <= 0:
            return None
        
        name = "%s_%d" % (self._instrument, runnumber)
        filename = name + extension
        # EMPTY_INT() from C++
        if chunk:
            if "ChunkNumber" in chunk:
                name += "_%d" % (int(chunk["ChunkNumber"]))
            elif "SpectrumMin" in chunk:
                name += "_%d" % (1 + int(chunk["SpectrumMin"])/(int(chunk["SpectrumMax"])-int(chunk["SpectrumMin"])))        
        else:
            name += "_%d" % 0

        if extension.endswith("_event.nxs"):
            chunk["Precount"] = True
            if filterWall is not None:
                if filterWall[0] > 0.:
                    chunk["FilterByTimeStart"] = filterWall[0]
                if filterWall[1] > 0.:
                    chunk["FilterByTimeStop"] = filterWall[1]
            
        wksp = api.Load(Filename=filename, OutputWorkspace=name, **chunk)
        try: 
            print "[DB1050-1] Number of events = %d" % (wksp.getNumberEvents())
        except Exception as e:
            print "[DB1050-1] Unable to get events of %s.  Error message: %s" % (str(wksp), str(e))

        if HAVE_MPI:
            msg = "MPI Task = %s ;" % (str(mpi.world.rank))
            try: 
                msg += "Number Events = " + str(wksp.getNumberEvents())
            except Exception as e: 
                msg += "Unable to get events of %s.  Error message: %s" % (str(wksp), str(e))
            print msg

        return wksp

    def _getStrategy(self, runnumber, extension):
        # generate the workspace name
        wksp = "%s_%d" % (self._instrument, runnumber)
        strategy = []
        print "[DBx116] File Name : %s,\t\tMax chunk size: %s" % (str(wksp+extension), str(self._chunks))
        if True:
            Chunks = api.DetermineChunking(Filename=wksp+extension,MaxChunkSize=self._chunks)
        else:
            api.DetermineChunking(Filename=wksp+extension,MaxChunkSize=self._chunks,OutputWorkspace='Chunks')
            Chunks = AnalysisDataService.retrieve("Chunks")
        for row in Chunks: strategy.append(row)
        #For table with no rows
        if not strategy:
            strategy.append({})
        return strategy

    def _focusChunks(self, runnumber, extension, filterWall, calib, splitwksp=None, preserveEvents=True,
               normByCurrent=True, filterBadPulsesOverride=True):
        """ Load, (optional) split and focus data in chunks

        Arguments: 
         - splitwksp:  SplittersWorkspace (if None then no split)
         - filterWall: Enabled if splitwksp is defined

        Return:
        """
        # generate the workspace name
        wksp = "%s_%d" % (self._instrument, runnumber)
        print "runnumber = ", runnumber, " extension = ", extension

        strategy = self._getStrategy(runnumber, extension)

        dosplit = False
        numwksp = 1
        if splitwksp is not None: 
            # FIXME Unfiltered workspace (remainder) is not considered here
            numwksp = self.getNumberOfSplittedWorkspace(splitwksp)
            if filterWall[0] < 1.0E-20 and filterWall[1] < 1.0E-20: 
                # do splitting if and only if filterWall is not defined well (redundent check)
                dosplit = True
            # ENDIF
        # ENDIF

        firstChunkList = []
        wksplist = []
        for n in xrange(numwksp): 
            # In some cases, there will be 1 more splitted workspace (unfiltered)
            firstChunkList.append(True)
            wksplist.append(None)

        print "Number of workspace to process = %d" %(numwksp)

        # reduce data by chunks
        ichunk = -1
        for chunk in strategy:
            ichunk += 1

            print "Process chunk %d" %(ichunk)

            # Log information
            if "ChunkNumber" in chunk:
                self.log().information("Working on chunk %d of %d" % (chunk["ChunkNumber"], chunk["TotalChunks"]))
            elif "SpectrumMin" in chunk:
                self.log().information("Working on spectrums %d through %d" % (chunk["SpectrumMin"], chunk["SpectrumMax"]))

            # Load chunk
            temp = self._loadData(runnumber, extension, filterWall, **chunk)
            if self._info is None:
                self._info = self._getinfo(temp)

            # Filtering... 
            tempwslist = []
            if not "histo" in extension:
                # Filter bad pulses
                if (self._filterBadPulses and filterBadPulsesOverride):
                    temp = api.FilterBadPulses(InputWorkspace=temp, OutputWorkspace=temp)

                # Filter to bad 
                if dosplit:
                    # Splitting workspace
                    basename = str(temp) 
                    api.FilterEvents(InputWorkspace=temp, OutputWorkspaceBaseName=basename, 
                            SplitterWorkspace=splitwksp, GroupWorkspaces=True)
                    wsgroup = mtd[basename]
                    tempwsnamelist = wsgroup.getNames()
                    tempwslist = []
                    # FIXME Keep in mind to use this option. 
                    # keepremainder = self.getProperty("KeepRemainder").value
                    for wsname in tempwsnamelist:
                        tempws = mtd[wsname]
                        if tempws is not None: 
                            if wsname.endswith("_unfiltered") is False: 
                                tempwslist.append(tempws)
                            else:
                                api.DeleteWorkspace(Workspace=tempws)
                    # ENDFOR
                else:
                    # Non-splitting
                    tempwslist.append(temp)
                # ENDIF

                # Update number of workspaces
                numwksp = len(tempwslist)
            else:
                # Histogram data
                tempwslist.append(temp)
            # ENDIF

            msg = "[DBx1142] Workspace of chunk %d is %d/%d. \n" % (ichunk, len(tempwslist), numwksp)
            for iws in xrange(len(tempwslist)):
                ws = tempwslist[iws]
                msg += "%s\t\t" % (str(ws))
                if iws %5 == 4:
                    msg += "\n"
            print msg

            for itemp in xrange(numwksp):
                temp = tempwslist[itemp]
                # Align and focus
                print "[DB1141] Align and focus workspace %s; Number of events = %d of chunk %d " % (str(temp), temp.getNumberEvents(), ichunk)
                temp = api.AlignAndFocusPowder(InputWorkspace=temp, OutputWorkspace=temp, CalFileName=calib,
                    Params=self._binning, ResampleX=self._resampleX, Dspacing=self._bin_in_dspace,
                    DMin=self._info.dmin, DMax=self._info.dmax, TMin=self._info.tmin, TMax=self._info.tmax,
                    PreserveEvents=preserveEvents,
                    RemovePromptPulseWidth=self._removePromptPulseWidth, CompressTolerance=COMPRESS_TOL_TOF,
                    UnwrapRef=self._LRef, LowResRef=self._DIFCref,
                    CropWavelengthMin=self._wavelengthMin, **(self._config.getFocusPos()))
                try: 
                    if temp.__class__.__name__.count("IEvent") > 0: 
                        print "[DB1050-3] Number of events = %d of chunk %d" % (temp.getNumberEvents(), ichunk)
                except RuntimeError:
                    print "[DB1050-3] Not an event workspace."
                
                # Rename and/or add to workspace of same splitter but different chunk
                wkspname = wksp
                if numwksp > 1:
                    wkspname += "_%s" % ( (str(temp)).split("_")[-1] )

                if firstChunkList[itemp]:
                    print "[DBx1145] Slot %d is renamed to %s" % (itemp, wkspname)
                    wksplist[itemp] = api.RenameWorkspace(InputWorkspace=temp, OutputWorkspace=wkspname)
                    firstChunkList[itemp] = False
                else:
                    wksplist[itemp] += temp
                    api.DeleteWorkspace(temp)
                # ENDIF
            # ENDFOR (spliited workspaces)
        # ENDFOR  Chunk

        print "[DB1207] Number of workspace in workspace list = ", len(wksplist)

        # Sum workspaces for all mpi tasks
        if HAVE_MPI:
            for itemp in xrange(numwksp): 
                wksplist[itemp] = api.GatherWorkspaces(InputWorkspace=wksplist[itemp], 
                        PreserveEvents=preserveEvents, AccumulationMethod="Add", OutputWorkspace=wksplist[itemp])
        # ENDIF MPI

        if self._chunks > 0:
            # When chunks are added, proton charge is summed for all chunks
            for itemp in xrange(numwksp): 
                wksplist[itemp].getRun().integrateProtonCharge()
        # ENDIF

        if (self._config.iparmFile is not None) and (len(self._config.iparmFile) > 0):
            # When chunks are added, add iparamFile
            for itemp in xrange(numwksp): 
                wksplist[itemp].getRun()['iparm_file'] = self._config.iparmFile

        api.DeleteWorkspace('Chunks')

        for itemp in xrange(numwksp): 
            if wksplist[itemp].__class__.__name__.count("Event") > 0: 
                try: 
                    print "[DB1050-X] Number of events = %d of split-workspace %d" % (wksplist[itemp].getNumberEvents(), itemp)
                except Exception as e:
                    print e
            if preserveEvents and not "histo" in extension:
                wksplist[itemp] = api.CompressEvents(InputWorkspace=wksplist[itemp], 
                    OutputWorkspace=wksplist[itemp], Tolerance=COMPRESS_TOL_TOF) # 100ns

            if normByCurrent:
                wksplist[itemp] = api.NormaliseByCurrent(InputWorkspace=wksplist[itemp], 
                        OutputWorkspace=wksplist[itemp])
                wksplist[itemp].getRun()['gsas_monitor'] = 1

            self._save(wksplist[itemp], self._info, False, True)
            self.log().information("Done focussing data of %d." % (itemp))
           
            if wksplist[itemp].__class__.__name__.count("Event") > 0: 
                try: 
                    print "[DB1050-Z] Number of events = %d of split-workspace %d" % (wksplist[itemp].getNumberEvents(), itemp)
                except Exception as e:
                    print e

        print "[DB1208] Number of workspace in workspace list = ", len(wksplist)

        # About return
        if splitwksp is None:
            returnee = wksplist[0]
        else:
            returnee = wksplist

        return returnee

    def _getinfo(self, wksp):
        logs = wksp.getRun()
        # get the frequency
        frequency = None
        if "SpeedRequest1" in logs.keys():
            frequency = logs['SpeedRequest1']
            # f5aa61589450be6e43ee592a4aadc63926a83f82 
            if frequency.getStatistics().mean == 0.: 
                self.log().information("'SpeedRequest1' mean value is zero")                
                frequency = None 
        else:
            self.log().information("'SpeedRequest1' is not specified in logs")
        # f5aa61589450be6e43ee592a4aadc63926a83f82
        if frequency is None and "Speed1" in logs.keys():
            frequency = logs['Speed1']
            if frequency.getStatistics().mean == 0.:
                self.log().information("'Speed1' mean value is zero")
                frequency = None
        else:
            self.log().information("'Speed1' is not specified in logs")
        if frequency is None: 
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
        prefix = str(wksp)        
        if len(self._outPrefix) > 0: # non-empty string            
            prefix = self._outPrefix 
        filename = os.path.join(self._outDir, prefix) 
        if pdfgetn:
            if "pdfgetn" in self._outTypes:
                pdfwksp = str(wksp)+"_norm"
                pdfwksp = api.SetUncertainties(InputWorkspace=wksp, OutputWorkspace=pdfwksp, SetError="sqrt")
                api.SaveGSS(InputWorkspace=pdfwksp, Filename=filename+".getn", SplitFiles="False", Append=False,
                        MultiplyByBinWidth=False, Bank=info.bank, Format="SLOG", ExtendedHeader=True)
                api.DeleteWorkspace(pdfwksp)
            return # don't do the other bits of saving
        if "gsas" in self._outTypes:
            api.SaveGSS(InputWorkspace=wksp, Filename=filename+".gsa", SplitFiles="False", Append=False, 
                    MultiplyByBinWidth=normalized, Bank=info.bank, Format="SLOG", ExtendedHeader=True)
        if "fullprof" in self._outTypes:
            api.SaveFocusedXYE(InputWorkspace=wksp, StartAtBankNumber=info.bank, Filename=filename+".dat")          
        if "NeXus" in self._outTypes:
            api.ConvertUnits(InputWorkspace=wksp, OutputWorkspace=wksp, Target=self.getProperty("FinalDataUnits").value)
            #api.Rebin(InputWorkspace=wksp, OutputWorkspace=wksp, Params=self._binning) # crop edges
            api.SaveNexus(InputWorkspace=wksp, Filename=filename+".nxs")

        # always save python script
        api.GeneratePythonScript(InputWorkspace=wksp, Filename=filename+".py")

        return

    def getFilterWall(self, splitws, samrun, extension):
        """ Get filter wall from splitter workspace, i.e., 
        get the earlies and latest time stamp in input splitter workspace

        Arguments:
         - splitws      : splitters workspace
         - runstarttime : total nanoseconds of run start time (Mantid DateAndTime)

        Return: tuple of start-time and stop-time relative to run start time and in unit of second 
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
        
# Register algorthm with Mantid.
AlgorithmFactory.subscribe(SNSPowderReduction)
