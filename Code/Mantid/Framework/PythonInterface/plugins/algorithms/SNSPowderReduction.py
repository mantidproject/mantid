import mantid.simpleapi as api
from mantid.api import *
from mantid.kernel import *
import os

if AlgorithmFactory.exists('GatherWorkspaces'):
    HAVE_MPI = True
    from mpi4py import MPI
    mpiRank = MPI.COMM_WORLD.Get_rank()
else:
    HAVE_MPI = False
    mpiRank = 0 # simplify if clauses

COMPRESS_TOL_TOF = .01
EVENT_WORKSPACE_ID = "EventWorkspace"

class SNSPowderReduction(DataProcessorAlgorithm):
    def category(self):
        return "Diffraction;PythonAlgorithms"

    def name(self):
        return "SNSPowderReduction"

    def summary(self):
        " "
        return "The algorithm used for reduction of powder diffraction data obtained on SNS instruments (e.g. PG3) "

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
                             "If larger and equal to 0, then process low resolution TOF and offset is the spectra number. Otherwise, ignored.")

        self.declareProperty("NormalizeByCurrent", True, "Normalize by current")

        return


    def PyExec(self):
        """ Main execution body
        """
        # get generic information
        SUFFIX = self.getProperty("Extension").value
        self._loadCharacterizations(self.getProperty("CharacterizationRunsFile").value)
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
        self._scaleFactor = self.getProperty("ScaleData").value
        self._outDir = self.getProperty("OutputDirectory").value
        self._outPrefix = self.getProperty("OutputFilePrefix").value
        self._outTypes = self.getProperty("SaveAs").value.lower()
        samRuns = self.getProperty("RunNumber").value
        preserveEvents = self.getProperty("PreserveEvents").value
        if HAVE_MPI and preserveEvents == True:
            self.log().warning("preserveEvents set to False for MPI tasks.")
            preserveEvents = False
        self._info = None
        self._infodict = {}
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

        # Process data
        workspacelist = [] # all data workspaces that will be converted to d-spacing in the end
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

            samRun = None
            info = None
            for temp in samRuns:
                runnumber = temp
                self.log().information("[Sum] Process run number %s. " %(str(runnumber)))

                temp = self._focusChunks(temp, SUFFIX, timeFilterWall, calib,
                        preserveEvents=preserveEvents)
                tempinfo = self._getinfo(temp)

                if samRun is None:
                    samRun = temp
                    info = tempinfo
                else:
                    if (tempinfo["frequency"] is not None) and (info["frequency"] is not None) \
                            and (abs(tempinfo["frequency"] - info["frequency"])/info["frequency"] > .05):
                        raise RuntimeError("Cannot add incompatible frequencies (%f!=%f)" \
                                           % (tempinfo["frequency"], info["frequency"]))
                    if (tempinfo["wavelength"] is not None) and (info["wavelength"] is not None) \
                            and abs(tempinfo["wavelength"] - info["wavelength"])/info["wavelength"] > .05:
                        raise RuntimeError("Cannot add incompatible wavelengths (%f != %f)" \
                                           % (tempinfo["wavelength"], info["wavelength"]))
                    samRun = api.Plus(LHSWorkspace=samRun, RHSWorkspace=temp, OutputWorkspace=samRun)
                    if samRun.id() == EVENT_WORKSPACE_ID:
                        samRun = api.CompressEvents(InputWorkspace=samRun, OutputWorkspace=samRun,
                                       Tolerance=COMPRESS_TOL_TOF) # 10ns
                    api.DeleteWorkspace(str(temp))
                # ENDIF
            # ENDFOR (processing each)

            factor = 1 / float(len(samRuns))
            samRun = api.Scale(samRun, Factor=factor, OutputWorkspace=samRun)

            samRuns = [samRun]
            workspacelist.append(str(samRun))
            samwksplist.append(str(samRun))
        # ENDIF (SUM)

        for samRun in samRuns:
            # first round of processing the sample
            if not self.getProperty("Sum").value and samRun > 0:
                self._info = None
                returned = self._focusChunks(samRun, SUFFIX, timeFilterWall, calib, self._splitws,
                        preserveEvents=preserveEvents)

                if returned.__class__.__name__ == "list":
                    # Returned with a list of workspaces
                    focusedwksplist = returned
                    irun = 0
                    for run in focusedwksplist:
                        if run is not None:
                            samwksplist.append(run)
                            workspacelist.append(str(run))
                        else:
                            self.log().warning("Found a None entry in returned focused workspaces.  Index = %d." % (irun))
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
                self.log().information("[F1136] Sample Run %s:  number of events = %d" % (str(samRun), samRun.getNumberEvents()))
            except Exception as e:
                self.log().information("[F1136] Unable to get number of events of sample run %s.  Error message: %s" % (str(samRun), str(e)))

            # Get run number
            runnumber = samRun.getRunNumber()
            if self._infodict.has_key(runnumber):
                self.log().debug("[F1022A] Found run number %d in info dict." % (runnumber))
                self._info = self._infodict[runnumber]
            else:
                self.log().debug("[F1022B] Unable to find _info for run number %d in info dict. "% (runnumber))
                self._info = self._getinfo(samRun)

            # process the container
            canRun = self._info["container"]
            if canRun > 0:
                if self.getProperty("FilterCharacterizations").value:
                    canFilterWall = timeFilterWall
                else:
                    canFilterWall = (0., 0.)
                if ("%s_%d" % (self._instrument, canRun)) in mtd:
                    canRun = mtd["%s_%d" % (self._instrument, canRun)]
                else:
                    canRun = self._focusChunks(canRun, SUFFIX, canFilterWall, calib,
                               preserveEvents=preserveEvents)
                canRun = api.ConvertUnits(InputWorkspace=canRun, OutputWorkspace=canRun, Target="TOF")
                workspacelist.append(str(canRun))
            else:
                canRun = None

            # process the vanadium run
            vanRun = self._info["vanadium"]
            if vanRun > 0:
                if self.getProperty("FilterCharacterizations").value:
                    vanFilterWall = timeFilterWall
                else:
                    vanFilterWall = (0., 0.)
                if ("%s_%d" % (self._instrument, vanRun)) in mtd:
                    vanRun = mtd["%s_%d" % (self._instrument, vanRun)]
                    vanRun = api.ConvertUnits(InputWorkspace=vanRun, OutputWorkspace=vanRun, Target="TOF")
                else:
                    # load the vanadium
                    vanRun = self._loadData(vanRun, SUFFIX, vanFilterWall)
                    name = "_".join(str(vanRun).split("_")[:-1])
                    vanRun = api.RenameWorkspace(InputWorkspace=vanRun, OutputWorkspace=name)
                    try:
                        if self._normalisebycurrent is True:
                            vanRun = api.NormaliseByCurrent(InputWorkspace=vanRun,
                                                            OutputWorkspace=vanRun)
                            vanRun.getRun()['gsas_monitor'] = 1
                    except Exception, e:
                        self.log().warning(str(e))


                    # load the vanadium background (if appropriate)
                    vbackRun = self._info["empty"]
                    if vbackRun > 0:
                        vbackRun = self._loadData(vbackRun, SUFFIX, vanFilterWall, outname="vbackRun")
                        try:
                            if self._normalisebycurrent is True:
                                vbackRun = api.NormaliseByCurrent(InputWorkspace=vbackRun,
                                                                  OutputWorkspace=vbackRun)
                                vbackRun.getRun()['gsas_monitor'] = 1
                        except Exception, e:
                            self.log().warning(str(e))

                        vanRun = api.Minus(LHSWorkspace=vanRun, RHSWorkspace=vbackRun, OutputWorkspace=vanRun)
                        api.DeleteWorkspace(Workspace=vbackRun)
                    else:
                        vbackRun = None

                    # compress events
                    if vanRun.id() == EVENT_WORKSPACE_ID:
                        vanRun = api.CompressEvents(InputWorkspace=vanRun, OutputWorkspace=vanRun,
                                                    Tolerance=COMPRESS_TOL_TOF) # 10ns

                    # do the absorption correction
                    vanRun = api.ConvertUnits(InputWorkspace=vanRun, OutputWorkspace=vanRun, Target="TOF")
                    api.SetSampleMaterial(InputWorkspace=vanRun, ChemicalFormula="V", SampleNumberDensity=0.0721)
                    vanRun = api.MultipleScatteringCylinderAbsorption(InputWorkspace=vanRun, OutputWorkspace=vanRun)

                    # focus the data
                    vanRun = api.AlignAndFocusPowder(InputWorkspace=vanRun, OutputWorkspace=vanRun, CalFileName=calib,
                                                     Params=self._binning, ResampleX=self._resampleX, Dspacing=self._bin_in_dspace,
                                                     DMin=self._info["d_min"], DMax=self._info["d_max"],
                                                     TMin=self._info["tof_min"], TMax=self._info["tof_max"],
                                                     RemovePromptPulseWidth=self._removePromptPulseWidth, CompressTolerance=COMPRESS_TOL_TOF,
                                                     UnwrapRef=self._LRef, LowResRef=self._DIFCref, LowResSpectrumOffset=self._lowResTOFoffset,
                                                     CropWavelengthMin=self._wavelengthMin, **(focuspos))


                    # strip peaks
                    if self.getProperty("StripVanadiumPeaks").value:
                        vanRun = api.ConvertUnits(InputWorkspace=vanRun, OutputWorkspace=vanRun, Target="dSpacing")
                        # api.CloneWorkspace(InputWorkspace=vanRun, OutputWorkspace=str(vanRun)+"_Raw")
                        vanRun = api.StripVanadiumPeaks(InputWorkspace=vanRun, OutputWorkspace=vanRun, FWHM=self._vanPeakFWHM,
                                           PeakPositionTolerance=self.getProperty("VanadiumPeakTol").value,
                                           BackgroundType="Quadratic", HighBackground=True)
                        # api.CloneWorkspace(InputWorkspace=vanRun, OutputWorkspace=str(vanRun)+"_PostStrip")
                    else:
                        self.log().information("Not strip vanadium peaks")
                    vanRun = api.ConvertUnits(InputWorkspace=vanRun, OutputWorkspace=vanRun, Target="TOF")
                    vanRun = api.FFTSmooth(InputWorkspace=vanRun, OutputWorkspace=vanRun, Filter="Butterworth",
                              Params=self._vanSmoothing,IgnoreXBins=True,AllSpectra=True)
                    vanRun = api.SetUncertainties(InputWorkspace=vanRun, OutputWorkspace=vanRun)
                    vanRun = api.ConvertUnits(InputWorkspace=vanRun, OutputWorkspace=vanRun, Target="TOF")
                workspacelist.append(str(vanRun))
            else:
                vanRun = None

            if mpiRank > 0:
                return
            if samRun == 0:
                return
            # the final bit of math
            if canRun is not None:
                samRun = api.Minus(LHSWorkspace=samRun, RHSWorkspace=canRun, OutputWorkspace=samRun)
                if samRun.id() == EVENT_WORKSPACE_ID:
                    samRun = api.CompressEvents(InputWorkspace=samRun, OutputWorkspace=samRun,
                               Tolerance=COMPRESS_TOL_TOF) # 10ns
                canRun = str(canRun)
            if vanRun is not None:
                samRun = api.Divide(LHSWorkspace=samRun, RHSWorkspace=vanRun, OutputWorkspace=samRun)
                normalized = True
                samRun.getRun()['van_number'] = vanRun.getRun()['run_number'].value
                vanRun = str(vanRun)
            else:
                normalized = False

            if samRun.id() == EVENT_WORKSPACE_ID:
                samRun = api.CompressEvents(InputWorkspace=samRun, OutputWorkspace=samRun,
                           Tolerance=COMPRESS_TOL_TOF) # 5ns/

            # make sure there are no negative values - gsas hates them
            if self.getProperty("PushDataPositive").value != "None":
                addMin = (self.getProperty("PushDataPositive").value == "AddMinimum")
                samRun = api.ResetNegatives(InputWorkspace=samRun, OutputWorkspace=samRun, AddMinimum=addMin, ResetValue=0.)

            # write out the files
            if mpiRank == 0:
                if self._scaleFactor != 1.:
                    samRun = api.Scale(samRun, Factor=self._scaleFactor, OutputWorkspace=samRun)
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

    def _loadCharacterizations(self, filename):
        self._focusPos = {}
        if filename is None or len(filename) <= 0:
            self.iparmFile = None
            return
        results = api.PDLoadCharacterizations(Filename=filename,
                                              OutputWorkspace="characterizations")
        self._charTable = results[0]
        self.iparmFile = results[1]
        self._focusPos['PrimaryFlightPath'] = results[2]
        self._focusPos['SpectrumIDs'] = results[3]
        self._focusPos['L2'] = results[4]
        self._focusPos['Polar'] = results[5]
        self._focusPos['Azimuthal'] = results[6]

    def _loadData(self, runnumber, extension, filterWall=None, outname=None, **chunk):
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
        if outname is not None:
            name = outname

        if extension.endswith("_event.nxs"):
            chunk["Precount"] = True
            if filterWall is not None:
                if filterWall[0] > 0.:
                    chunk["FilterByTimeStart"] = filterWall[0]
                if filterWall[1] > 0.:
                    chunk["FilterByTimeStop"] = filterWall[1]

        wksp = api.Load(Filename=filename, OutputWorkspace=name, **chunk)
        try:
            self.log().debug("Load run %s: number of events = %d" % (str(runnumber), wksp.getNumberEvents()))
        except Exception as e:
            self.log().debug("Load run %s: unable to get events of %s.  Error message: %s" % (str(runnumber), str(wksp), str(e)))

        if HAVE_MPI:
            msg = "MPI Task = %s ;" % (str(mpiRank))
            try:
                msg += "Number Events = " + str(wksp.getNumberEvents())
            except Exception as e:
                msg += "Unable to get events of %s.  Error message: %s" % (str(wksp), str(e))
            self.log().debug(msg)

        # filter bad pulses
        if self._filterBadPulses > 0.:
            wksp = api.FilterBadPulses(InputWorkspace=wksp, OutputWorkspace=wksp,
                                       LowerCutoff=self._filterBadPulses)
            if str(type(wksp)).count("IEvent") > 0:
                # Event workspace
                self.log().information("F1141D There are %d events after FilterBadPulses in workspace %s." % (
                        wksp.getNumberEvents(), str(wksp)))

        return wksp

    def _getStrategy(self, runnumber, extension):
        # generate the workspace name
        wksp = "%s_%d" % (self._instrument, runnumber)
        strategy = []
        self.log().debug("[Fx116] Run file Name : %s,\t\tMax chunk size: %s" % (str(wksp+extension), str(self._chunks)))
        chunks = api.DetermineChunking(Filename=wksp+extension,MaxChunkSize=self._chunks)
        for row in chunks:
            strategy.append(row)
        #For table with no rows
        if not strategy:
            strategy.append({})

        # delete chunks workspace
        chunks = str(chunks)
        mtd.remove(chunks)

        return strategy

    def __logChunkInfo(self, chunk):
        keys = chunk.keys()
        keys.sort()

        keys = [ str(key) + "=" + str(chunk[key]) for key in keys ]
        self.log().information("Working on chunk [" + ", ".join(keys) + "]")

    def _focusChunks(self, runnumber, extension, filterWall, calib, splitwksp=None, preserveEvents=True):
        """ Load, (optional) split and focus data in chunks

        Arguments:
         - runnumber : integer for run number
         - splitwksp:  SplittersWorkspace (if None then no split)
         - filterWall: Enabled if splitwksp is defined

        Return:
        """
        # generate the workspace name
        wksp = "%s_%d" % (self._instrument, runnumber)
        self.log().information("_focusChunks(): runnumber = %d, extension = %s" % (runnumber, extension))

        strategy = self._getStrategy(runnumber, extension)

        dosplit = False
        # Number of output workspaces from _focusChunk
        numwksp = 1
        if splitwksp is not None:
            # Check consistency in the code
            if filterWall[0] < 1.0E-20 and filterWall[1] < 1.0E-20:
                # Default definition of filterWall when there is no split workspace specified.
                raise NotImplementedError("It is impossible to have a not-NONE splitters workspace and (0,0) time filter wall.")
            # ENDIF

            # FIXME Unfiltered workspace (remainder) is not considered here
            numwksp = self.getNumberOfSplittedWorkspace(splitwksp)
            numsplitters = splitwksp.rowCount()

            # Do explicit FilterEvents if number of splitters is larger than 1.
            # If number of splitters is equal to 1, then filterWall will do the job itself.
            if numsplitters > 1:
                dosplit = True
            self.log().debug("[Fx948] Number of split workspaces = %d; Do split = %s" % (numwksp, str(dosplit)))
        # ENDIF

        firstChunkList = []
        wksplist = []
        for n in xrange(numwksp):
            # In some cases, there will be 1 more splitted workspace (unfiltered)
            firstChunkList.append(True)
            wksplist.append(None)

        self.log().debug("F1141A: Number of workspace to process = %d" %(numwksp))

        # reduce data by chunks
        ichunk = -1
        for chunk in strategy:
            self.log().debug("F1141B: Start of Chunk %s" % (str(chunk)))
            ichunk += 1

            # Log information
            self.__logChunkInfo(chunk)

            # Load chunk
            temp = self._loadData(runnumber, extension, filterWall, **chunk)
            if str(type(temp)).count("IEvent") > 0:
                # Event workspace
                self.log().debug("F1141C There are %d events after data is loaded in workspace %s." % (
                    temp.getNumberEvents(), str(temp)))

            if self._info is None:
                if not self._infodict.has_key(int(runnumber)):
                    self._info = self._getinfo(temp)
                    self._infodict[int(runnumber)] = self._info
                    self.log().debug("[F1012] Add info for run number %d." % (int(runnumber)))

            # Filtering...
            tempwslist = []
            if temp.id() == EVENT_WORKSPACE_ID:
                # Filter to bad
                if dosplit:
                    # Splitting workspace
                    basename = str(temp)
                    if self._splitinfotablews is None:
                        api.FilterEvents(InputWorkspace=temp, OutputWorkspaceBaseName=basename,
                                SplitterWorkspace=splitwksp, GroupWorkspaces=True)
                    else:
                        self.log().information("SplitterWorkspace = %s, Information Workspace = %s. " % (
                            str(splitwksp), str(self._splitinfotablews)))
                        api.FilterEvents(InputWorkspace=temp, OutputWorkspaceBaseName=basename,
                                SplitterWorkspace=splitwksp, InformationWorkspace = str(self._splitinfotablews),
                                GroupWorkspaces=True)
                    # ENDIF
                    wsgroup = mtd[basename]
                    tempwsnamelist = wsgroup.getNames()

                    dbstr = "[Fx951] Splitted workspace names: "
                    for wsname in tempwsnamelist:
                        dbstr += "%s, " % (wsname)
                    self.log().debug(dbstr)

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

            msg = "[Fx1142] Workspace of chunk %d is %d/%d. \n" % (ichunk, len(tempwslist), numwksp)
            for iws in xrange(len(tempwslist)):
                ws = tempwslist[iws]
                msg += "%s\t\t" % (str(ws))
                if iws %5 == 4:
                    msg += "\n"
            self.log().debug(msg)

            for itemp in xrange(numwksp):
                temp = tempwslist[itemp]
                # Align and focus
                self.log().information("[F1141] Align and focus workspace %s; Number of events = %d of chunk %d " % (str(temp), temp.getNumberEvents(), ichunk))

                focuspos = self._focusPos

                temp = api.AlignAndFocusPowder(InputWorkspace=temp, OutputWorkspace=temp, CalFileName=calib,
                    Params=self._binning, ResampleX=self._resampleX, Dspacing=self._bin_in_dspace,
                    DMin=self._info["d_min"], DMax=self._info["d_max"], TMin=self._info["tof_min"], TMax=self._info["tof_max"],
                    PreserveEvents=preserveEvents,
                    RemovePromptPulseWidth=self._removePromptPulseWidth, CompressTolerance=COMPRESS_TOL_TOF,
                    UnwrapRef=self._LRef, LowResRef=self._DIFCref, LowResSpectrumOffset=self._lowResTOFoffset,
                    CropWavelengthMin=self._wavelengthMin, **(focuspos))
                for iws in xrange(temp.getNumberHistograms()):
                    spec = temp.getSpectrum(iws)
                    self.log().debug("[DBx131] ws %d: spectrum ID = %d. " % (iws, spec.getSpectrumNo()))

                # Rename and/or add to workspace of same splitter but different chunk
                wkspname = wksp
                if numwksp > 1:
                    wkspname += "_%s" % ( (str(temp)).split("_")[-1] )

                if firstChunkList[itemp]:
                    self.log().debug("[F1145] Slot %d is renamed to %s" % (itemp, wkspname))
                    wksplist[itemp] = api.RenameWorkspace(InputWorkspace=temp, OutputWorkspace=wkspname)
                    firstChunkList[itemp] = False
                else:
                    wksplist[itemp] = api.Plus(LHSWorkspace=wksplist[itemp], RHSWorkspace=temp, OutputWorkspace=wksplist[itemp])
                    api.DeleteWorkspace(temp)
                # ENDIF
            # ENDFOR (spliited workspaces)
        # ENDFOR  Chunk

        self.log().information("[F1207] Number of workspace in workspace list after loading by chunks = %d. " %(len(wksplist)))

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

        if (self.iparmFile is not None) and (len(self.iparmFile) > 0):
            # When chunks are added, add iparamFile
            for itemp in xrange(numwksp):
                wksplist[itemp].getRun()['iparm_file'] = self.iparmFile

        for itemp in xrange(numwksp):
            #if wksplist[itemp].__class__.__name__.count("Event") > 0:
            #    try:
            #        print "[DB1050-X] Number of events = %d of split-workspace %d" % (wksplist[itemp].getNumberEvents(), itemp)
            #    except Exception as e:
            #        print e
            if wksplist[itemp].id() == EVENT_WORKSPACE_ID:
                wksplist[itemp] = api.CompressEvents(InputWorkspace=wksplist[itemp],
                    OutputWorkspace=wksplist[itemp], Tolerance=COMPRESS_TOL_TOF) # 100ns

            try:
                if self._normalisebycurrent is True:
                    wksplist[itemp] = api.NormaliseByCurrent(InputWorkspace=wksplist[itemp],
                                                             OutputWorkspace=wksplist[itemp])
                    wksplist[itemp].getRun()['gsas_monitor'] = 1
            except Exception, e:
                self.log().warning(str(e))

            self._save(wksplist[itemp], self._info, False, True)
            self.log().information("Done focussing data of %d." % (itemp))

            #if wksplist[itemp].__class__.__name__.count("Event") > 0:
            #    try:
            #        print "[DB1050-Z] Number of events = %d of split-workspace %d" % (wksplist[itemp].getNumberEvents(), itemp)
            #    except Exception as e:
            #        print e

        self.log().information("[E1207] Number of workspace in workspace list after clean = %d. " %(len(wksplist)))

        # About return
        if splitwksp is None:
            return wksplist[0]
        else:
            return wksplist

    def _getinfo(self, wksp):
        rowValues = {}

        if mtd.doesExist("characterizations"):
            # get the correct row of the table
            charac = api.PDDetermineCharacterizations(InputWorkspace=wksp,
                                                      Characterizations="characterizations",
                                                      ReductionProperties="__snspowderreduction",
                                                      BackRun=self.getProperty("BackgroundNumber").value,
                                                      NormRun=self.getProperty("VanadiumNumber").value,
                                                      NormBackRun=self.getProperty("VanadiumBackgroundNumber").value)
            # convert the result into a dict
            manager = PropertyManagerDataService.retrieve("__snspowderreduction")
            for name in ["frequency", "wavelength", "bank", "vanadium", "container",
                         "empty", "d_min", "d_max", "tof_min", "tof_max"]:
                rowValues[name] = manager.getProperty(name).value
        else:
            # "frequency", "wavelength"
            rowValues["bank"] = 0
            rowValues["container"] = self.getProperty("BackgroundNumber").value
            rowValues["vanadium"]  = self.getProperty("VanadiumNumber").value
            rowValues["empty"]     = self.getProperty("VanadiumBackgroundNumber").value
            rowValues["d_min"]     = 0.
            rowValues["d_max"]     = 0.
            rowValues["tof_min"]   = 0.
            rowValues["tof_max"]   = 0.

        return rowValues

    def _save(self, wksp, info, normalized, pdfgetn):
        prefix = str(wksp)
        if len(self._outPrefix) > 0: # non-empty string
            prefix = self._outPrefix
        filename = os.path.join(self._outDir, prefix)
        if pdfgetn:
            if "pdfgetn" in self._outTypes:
                pdfwksp = str(wksp)+"_norm"
                pdfwksp = api.SetUncertainties(InputWorkspace=wksp, OutputWorkspace=pdfwksp, SetError="sqrt")
                api.SaveGSS(InputWorkspace=pdfwksp, Filename=filename+".getn", SplitFiles=False, Append=False,
                        MultiplyByBinWidth=False, Bank=info["bank"], Format="SLOG", ExtendedHeader=True)
                api.DeleteWorkspace(pdfwksp)
            return # don't do the other bits of saving
        if "gsas" in self._outTypes:
            api.SaveGSS(InputWorkspace=wksp, Filename=filename+".gsa", SplitFiles=False, Append=False,
                    MultiplyByBinWidth=normalized, Bank=info["bank"], Format="SLOG", ExtendedHeader=True)
        if "fullprof" in self._outTypes:
            api.SaveFocusedXYE(InputWorkspace=wksp, StartAtBankNumber=info["bank"], Filename=filename+".dat")
        if "topas" in self._outTypes:
            api.SaveFocusedXYE(InputWorkspace=wksp, StartAtBankNumber=info["bank"], Filename=filename+".xye",
                               Format="TOPAS")
        if "nexus" in self._outTypes:
            api.ConvertUnits(InputWorkspace=wksp, OutputWorkspace=wksp, Target=self.getProperty("FinalDataUnits").value)
            #api.Rebin(InputWorkspace=wksp, OutputWorkspace=wksp, Params=self._binning) # crop edges
            api.SaveNexus(InputWorkspace=wksp, Filename=filename+".nxs")

        # always save python script
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

# Register algorthm with Mantid.
AlgorithmFactory.subscribe(SNSPowderReduction)
