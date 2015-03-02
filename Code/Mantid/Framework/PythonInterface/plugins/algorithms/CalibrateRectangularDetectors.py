#pylint: disable=no-init,invalid-name
from mantid.api import *
from mantid.kernel import *
from mantid.simpleapi import *
import os
from time import strftime
from mantid import config
from mantid.kernel import Direction

COMPRESS_TOL_TOF = .01

class CalibrateRectangularDetectors(PythonAlgorithm):

    _instrument = None
    _filterBadPulses = None
    _xpixelbin = None
    _ypixelbin = None
    _grouping = None
    _smoothoffsets = None
    _smoothGroups = None
    _peakpos = None
    _peakpos1 = None
    _peakmin = None
    _peakmax = None
    _peakpos2 = None
    _peakmin2 = None
    _peakmax2 = None
    _peakpos3 = None
    _peakmin3 = None
    _peakmax3 = None
    _lastpixel = None
    _lastpixel2 = None
    _lastpixel3 = None
    _ccnumber = None
    _maxoffset = None
    _diffractionfocus = None
    _outDir = None
    _outTypes = None
    _binning = None

    def category(self):
        return "Diffraction;PythonAlgorithms"

    def name(self):
        return "CalibrateRectangularDetectors"

    def summary(self):
        return "Calibrate the detector pixels and write a calibration file"

    def PyInit(self):
        sns = ConfigService.Instance().getFacility("SNS")

        instruments = []
        for instr in sns.instruments():
            for tech in instr.techniques():
                if "Neutron Diffraction" == str(tech):
                    instruments.append(instr.shortName())
                    break
        self.declareProperty("Instrument", "PG3",
                             StringListValidator(instruments))
        validator = IntArrayBoundedValidator()
        validator.setLower(0)
        self.declareProperty(IntArrayProperty("RunNumber", values=[0], direction=Direction.Input,
                                              validator=validator))
        validator = IntArrayBoundedValidator()
        validator.setLower(0)
        self.declareProperty(IntArrayProperty("Background", values=[0], direction=Direction.Input,
                                              validator=validator))
        extensions = [ "_event.nxs", "_runinfo.xml", ".nxs.h5"]
        self.declareProperty("Extension", "_event.nxs",
                             StringListValidator(extensions))
        self.declareProperty("CompressOnRead", False,
                             "Compress the event list when reading in the data")
        self.declareProperty("XPixelSum", 1,
                             "Sum detector pixels in X direction.  Must be a factor of X total pixels.  Default is 1.")
        self.declareProperty("YPixelSum", 1,
                             "Sum detector pixels in Y direction.  Must be a factor of Y total pixels.  Default is 1.")
        self.declareProperty("SmoothSummedOffsets", False,
                             "If the data was summed for calibration, smooth the resulting offsets workspace.")
        self.declareProperty("SmoothGroups", "",
                             "Comma delimited number of points for smoothing pixels in each group.  Default is no Smoothing.")
        self.declareProperty("UnwrapRef", 0.,
                             "Reference total flight path for frame unwrapping. Zero skips the correction")
        self.declareProperty("LowResRef", 0.,
                             "Reference DIFC for resolution removal. Zero skips the correction")
        self.declareProperty("MaxOffset", 1.0,
                             "Maximum absolute value of offsets; default is 1")
        self.declareProperty("CrossCorrelation", True,
                             "CrossCorrelation if True; minimize using many peaks if False.")
        self.declareProperty("PeakPositions", "",
                             "Comma delimited d-space positions of reference peaks.  Use 1-3 for Cross Correlation.  Unlimited for many peaks option.")
        self.declareProperty("PeakWindowMax", 0.,
                             "Maximum window around a peak to search for it. Optional.")
        self.declareProperty(ITableWorkspaceProperty("FitwindowTableWorkspace", "", Direction.Input, PropertyMode.Optional),\
                "Name of input table workspace containing the fit window information for each spectrum. ")
        self.declareProperty("MinimumPeakHeight", 2., "Minimum value allowed for peak height")
        self.declareProperty("MinimumPeakHeightObs", 0.,\
            "Minimum value of a peak's maximum observed Y value for this peak to be used to calculate offset.")

        self.declareProperty(MatrixWorkspaceProperty("DetectorResolutionWorkspace", "", Direction.Input, PropertyMode.Optional),\
                "Name of optional input matrix workspace for each detector's resolution (D(d)/d).")
        self.declareProperty(FloatArrayProperty("AllowedResRange", [0.25, 4.0], direction=Direction.Input),\
                "Range of allowed individual peak's resolution factor to input detector's resolution.")

        self.declareProperty("PeakFunction", "Gaussian", StringListValidator(["BackToBackExponential", "Gaussian", "Lorentzian"]),
                             "Type of peak to fit. Used only with CrossCorrelation=False")
        self.declareProperty("BackgroundType", "Flat", StringListValidator(['Flat', 'Linear', 'Quadratic']),
                             "Used only with CrossCorrelation=False")
        self.declareProperty("DetectorsPeaks", "",
                             "Comma delimited numbers of detector banks for each peak if using 2-3 peaks for Cross Correlation.  Default is all.")
        self.declareProperty("PeakHalfWidth", 0.05,
                             "Half width of d-space around peaks for Cross Correlation. Default is 0.05")
        self.declareProperty("CrossCorrelationPoints", 100,
                             "Number of points to find peak from Cross Correlation.  Default is 100")
        self.declareProperty(FloatArrayProperty("Binning", [0.,0.,0.]),
                             "Min, Step, and Max of d-space bins.  Logarithmic binning is used if Step is negative.")
        self.declareProperty("DiffractionFocusWorkspace", False, "Diffraction focus by detectors.  Default is False")
        grouping = ["All", "Group", "Column", "bank"]
        self.declareProperty("GroupDetectorsBy", "All", StringListValidator(grouping),
                             "Detector groups to use for future focussing: All detectors as one group, Groups (East,West for SNAP), Columns for SNAP, detector banks")
        self.declareProperty("FilterBadPulses", True, "Filter out events measured while proton charge is more than 5% below average")
        self.declareProperty("FilterByTimeMin", 0.,
                             "Relative time to start filtering by in seconds. Applies only to sample.")
        self.declareProperty("FilterByTimeMax", 0.,
                             "Relative time to stop filtering by in seconds. Applies only to sample.")
        outfiletypes = ['dspacemap', 'calibration', 'dspacemap and calibration']
        self.declareProperty("SaveAs", "calibration", StringListValidator(outfiletypes))
        self.declareProperty(FileProperty("OutputDirectory", "", FileAction.Directory))

        self.declareProperty("OutputFilename", "", Direction.Output)

        return

    def validateInputs(self):
        messages = {}

        detectors = self.getProperty("DetectorsPeaks").value.strip()
        if self.getProperty("CrossCorrelation").value:
            positions = self.getProperty("PeakPositions").value.strip()
            positions = positions.split(',')
            if not bool(detectors):
                if len(positions) != 1:
                    messages["PeakPositions"] = "Can only have one cross correlation peak without specifying 'DetectorsPeaks'"
            else:
                detectors = detectors.split(',')
                if len(detectors) != len(positions):
                    messages["PeakPositions"] = "Must be the same length as 'DetectorsPeaks' (%d != %d)" \
                        % (len(positions), len(detectors))
                    messages["DetectorsPeaks"] = "Must be the same length as 'PeakPositions' or empty"
                elif len(detectors) > 3:
                    messages["DetectorsPeaks"] = "Up to 3 peaks are supported"
        elif bool(detectors):
            messages["DetectorsPeaks"] = "Only allowed for CrossCorrelation=True"
            prop = self.getProperty("CrossCorrelationPoints")

        return messages

    def _loadPreNeXusData(self, runnumber, extension, **kwargs):
        """
            Load PreNexus data
            @param runnumer: run number (integer)
            @param extension: file extension
        """
        Logger("CalibrateRectangularDetector").warning("Loading PreNexus for run %s" % runnumber)
        mykwargs = {}
        if kwargs.has_key("FilterByTimeStart"):
            mykwargs["ChunkNumber"] = int(kwargs["FilterByTimeStart"])
        if kwargs.has_key("FilterByTimeStop"):
            mykwargs["TotalChunks"] = int(kwargs["FilterByTimeStop"])

        # generate the workspace name
        name = "%s_%d" % (self._instrument, runnumber)
        filename = name + extension

        wksp = LoadPreNexus(Filename=filename, OutputWorkspace=name, **mykwargs)

        # add the logs to it
        if str(self._instrument) == "SNAP":
            LoadInstrument(Workspace=wksp, InstrumentName=self._instrument, RewriteSpectraMap=False)

        return wksp

    def _loadEventNeXusData(self, runnumber, extension, **kwargs):
        """
            Load event Nexus data
            @param runnumer: run number (integer)
            @param extension: file extension
        """
        kwargs["Precount"] = False
        if self.getProperty("CompressOnRead").value:
            kwargs["CompressTolerance"] = .1
        name = "%s_%d" % (self._instrument, runnumber)
        filename = name + extension

        wksp = LoadEventNexus(Filename=filename, OutputWorkspace=name, **kwargs)
        # For NOMAD data before Aug 2012, use the updated geometry
        if str(wksp.getInstrument().getValidFromDate()) == "1900-01-31T23:59:59" and str(self._instrument) == "NOMAD":
            path=config["instrumentDefinition.directory"]
            LoadInstrument(Workspace=wksp, Filename=path+'/'+"NOMAD_Definition_20120701-20120731.xml",
                           RewriteSpectraMap=False)
        return wksp

    def _loadData(self, runnumber, extension, filterWall=None):
        """
            Load data
            @param runnumber: run number (integer)
            @param extension: file extension
        """
        filter = {}
        if filterWall is not None:
            if filterWall[0] > 0.:
                filter["FilterByTimeStart"] = filterWall[0]
            if filterWall[1] > 0.:
                filter["FilterByTimeStop"] = filterWall[1]

        if  runnumber is None or runnumber <= 0:
            return None

        if extension.endswith("_event.nxs") or extension.endswith(".nxs.h5"):
            wksp = self._loadEventNeXusData(runnumber, extension, **filter)
        else:
            wksp = self._loadPreNeXusData(runnumber, extension, **filter)

        if self._filterBadPulses and not self.getProperty("CompressOnRead").value:
            wksp = FilterBadPulses(InputWorkspace=wksp, OutputWorkspace=wksp.name())

        if not self.getProperty("CompressOnRead").value:
            wksp = CompressEvents(wksp, OutputWorkspace=wksp.name(),
                                  Tolerance=COMPRESS_TOL_TOF) # 100ns
        return wksp

    def _cccalibrate(self, wksp, calib):
        if wksp is None:
            return None
        LRef = self.getProperty("UnwrapRef").value
        DIFCref = self.getProperty("LowResRef").value
        if (LRef > 0.) or (DIFCref > 0.): # super special Jason stuff
            if LRef > 0:
                wksp = UnwrapSNS(InputWorkspace=wksp, OutputWorkspace=wksp.name(), LRef=LRef)
            if DIFCref > 0:
                wksp = RemoveLowResTOF(InputWorkspace=wksp, OutputWorkspace=wksp.name(),
                                       ReferenceDIFC=DIFCref)
        if not self.getProperty("CompressOnRead").value:
            wksp = CompressEvents(InputWorkspace=wksp, OutputWorkspace=wksp.name(),
                                  Tolerance=COMPRESS_TOL_TOF) # 100ns

        wksp = ConvertUnits(InputWorkspace=wksp, OutputWorkspace=wksp.name(), Target="dSpacing")
        SortEvents(InputWorkspace=wksp, SortBy="X Value")
        # Sum pixelbin X pixelbin blocks of pixels
        if self._xpixelbin*self._ypixelbin>1:
            wksp = SumNeighbours(InputWorkspace=wksp, OutputWorkspace=wksp.name(),
                                 SumX=self._xpixelbin, SumY=self._ypixelbin)
        # Bin events in d-Spacing
        wksp = Rebin(InputWorkspace=wksp, OutputWorkspace=wksp.name(),
                     Params=str(self._peakmin)+","+str(abs(self._binning[1]))+","+str(self._peakmax))
        #Find good peak for reference
        ymax = 0
        for s in range(0,wksp.getNumberHistograms()):
            y_s = wksp.readY(s)
            midBin = wksp.blocksize()/2
            if y_s[midBin] > ymax:
                refpixel = s
                ymax = y_s[midBin]
        self.log().information("Reference spectra=%s" % refpixel)
        # Remove old calibration files
        cmd = "rm "+calib
        os.system(cmd)
        # Cross correlate spectra using interval around peak at peakpos (d-Spacing)
        if self._lastpixel == 0:
            self._lastpixel = wksp.getNumberHistograms()-1
        else:
            self._lastpixel = wksp.getNumberHistograms()*self._lastpixel/self._lastpixel3-1
        CrossCorrelate(InputWorkspace=wksp, OutputWorkspace=str(wksp)+"cc",
                       ReferenceSpectra=refpixel, WorkspaceIndexMin=0,
                       WorkspaceIndexMax=self._lastpixel,
                       XMin=self._peakmin, XMax=self._peakmax)
        # Get offsets for pixels using interval around cross correlations center and peak at peakpos (d-Spacing)
        GetDetectorOffsets(InputWorkspace=str(wksp)+"cc", OutputWorkspace=str(wksp)+"offset",
                           Step=abs(self._binning[1]), DReference=self._peakpos1,
                           XMin=-self._ccnumber, XMax=self._ccnumber,
                           MaxOffset=self._maxoffset, MaskWorkspace=str(wksp)+"mask")
        if AnalysisDataService.doesExist(str(wksp)+"cc"):
            AnalysisDataService.remove(str(wksp)+"cc")
        if self._peakpos2 > 0.0:
            wksp = Rebin(InputWorkspace=wksp, OutputWorkspace=wksp.name(),
                         Params=str(self._peakmin2)+","+str(abs(self._binning[1]))+","+str(self._peakmax2))
            #Find good peak for reference
            ymax = 0
            for s in range(0,wksp.getNumberHistograms()):
                y_s = wksp.readY(s)
                midBin = wksp.blocksize()/2
                if y_s[midBin] > ymax:
                    refpixel = s
                    ymax = y_s[midBin]
            msg = "Reference spectra = %s, lastpixel_3 = %s" % (refpixel, self._lastpixel3)
            self.log().information(msg)
            self._lastpixel2 = wksp.getNumberHistograms()*self._lastpixel2/self._lastpixel3-1
            CrossCorrelate(InputWorkspace=wksp, OutputWorkspace=str(wksp)+"cc2",
                           ReferenceSpectra=refpixel, WorkspaceIndexMin=self._lastpixel+1,
                           WorkspaceIndexMax=self._lastpixel2,
                           XMin=self._peakmin2, XMax=self._peakmax2)
            # Get offsets for pixels using interval around cross correlations center and peak at peakpos (d-Spacing)
            GetDetectorOffsets(InputWorkspace=str(wksp)+"cc2", OutputWorkspace=str(wksp)+"offset2",
                               Step=abs(self._binning[1]), DReference=self._peakpos2,
                               XMin=-self._ccnumber, XMax=self._ccnumber,
                               MaxOffset=self._maxoffset, MaskWorkspace=str(wksp)+"mask2")
            Plus(LHSWorkspace=str(wksp)+"offset", RHSWorkspace=str(wksp)+"offset2",
                 OutputWorkspace=str(wksp)+"offset")
            Plus(LHSWorkspace=str(wksp)+"mask", RHSWorkspace=str(wksp)+"mask2",
                 OutputWorkspace=str(wksp)+"mask")
            for ws in [str(wksp)+"cc2", str(wksp)+"offset2", str(wksp)+"mask2"]:
                if AnalysisDataService.doesExist(ws):
                    AnalysisDataService.remove(ws)

        if self._peakpos3 > 0.0:
            wksp = Rebin(InputWorkspace=wksp, OutputWorkspace=wksp.name(),
                         Params=str(self._peakmin3)+","+str(abs(self._binning[1]))+","+str(self._peakmax3))
            #Find good peak for reference
            ymax = 0
            for s in range(0,wksp.getNumberHistograms()):
                y_s = wksp.readY(s)
                midBin = wksp.blocksize()/2
                if y_s[midBin] > ymax:
                    refpixel = s
                    ymax = y_s[midBin]
            self.log().information("Reference spectra=%s" % refpixel)
            CrossCorrelate(InputWorkspace=wksp, OutputWorkspace=str(wksp)+"cc3",
                           ReferenceSpectra=refpixel,
                           WorkspaceIndexMin=self._lastpixel2+1,
                           WorkspaceIndexMax=wksp.getNumberHistograms()-1,
                           XMin=self._peakmin3, XMax=self._peakmax3)
            # Get offsets for pixels using interval around cross correlations center and peak at peakpos (d-Spacing)
            GetDetectorOffsets(InputWorkspace=str(wksp)+"cc3", OutputWorkspace=str(wksp)+"offset3",
                               Step=abs(self._binning[1]), DReference=self._peakpos3,
                               XMin=-self._ccnumber, XMax=self._ccnumber,
                               MaxOffset=self._maxoffset, MaskWorkspace=str(wksp)+"mask3")
            Plus(LHSWorkspace=str(wksp)+"offset", RHSWorkspace=str(wksp)+"offset3",
                 OutputWorkspace=str(wksp)+"offset")
            Plus(LHSWorkspace=str(wksp)+"mask", RHSWorkspace=str(wksp)+"mask3",
                 OutputWorkspace=str(wksp)+"mask")
            for ws in [str(wksp)+"cc3", str(wksp)+"offset3", str(wksp)+"mask3"]:
                if AnalysisDataService.doesExist(ws):
                    AnalysisDataService.remove(ws)
        (temp, numGroupedSpectra, numGroups) = CreateGroupingWorkspace(InputWorkspace=wksp, GroupDetectorsBy=self._grouping,\
                                OutputWorkspace=str(wksp)+"group")
        if (numGroupedSpectra==0) or (numGroups==0):
            raise RuntimeError("%d spectra will be in %d groups" % (numGroupedSpectra, numGroups))
        lcinst = str(self._instrument)

        outfilename = None
        if "dspacemap" in self._outTypes:
            #write Dspacemap file
            outfilename = self._outDir+lcinst+"_dspacemap_d"+str(wksp).strip(self._instrument+"_")+strftime("_%Y_%m_%d.dat")
            SaveDspacemap(InputWorkspace=str(wksp)+"offset",
                          DspacemapFile=outfilename)
        if "calibration" in self._outTypes:
            outfilename = calib
            SaveCalFile(OffsetsWorkspace=str(wksp)+"offset",
                        GroupingWorkspace=str(wksp)+"group",
                        MaskWorkspace=str(wksp)+"mask",Filename=calib)

        if outfilename is not None:
            self.setProperty("OutputFilename", outfilename)

        return wksp

    def _multicalibrate(self, wksp, calib):
        if wksp is None:
            return None
        LRef = self.getProperty("UnwrapRef").value
        DIFCref = self.getProperty("LowResRef").value
        if (LRef > 0.) or (DIFCref > 0.): # super special Jason stuff
            if LRef > 0:
                wksp = UnwrapSNS(InputWorkspace=wksp, OutputWorkspace=wksp.name(), LRef=LRef)
            if DIFCref > 0:
                wksp = RemoveLowResTOF(InputWorkspace=wksp, OutputWorkspace=wksp.name(),
                                       ReferenceDIFC=DIFCref)
        if not self.getProperty("CompressOnRead").value and not "histo" in self.getProperty("Extension").value:
            wksp = CompressEvents(InputWorkspace=wksp, OutputWorkspace=wksp.name(),
                                  Tolerance=COMPRESS_TOL_TOF) # 100ns

        wksp = ConvertUnits(InputWorkspace=wksp, OutputWorkspace=wksp.name(), Target="dSpacing")
        if not "histo" in self.getProperty("Extension").value:
            SortEvents(InputWorkspace=wksp, SortBy="X Value")
        # Sum pixelbin X pixelbin blocks of pixels
        if self._xpixelbin*self._ypixelbin>1:
            wksp = SumNeighbours(InputWorkspace=wksp, OutputWorkspace=wksp.name(),
                                 SumX=self._xpixelbin, SumY=self._ypixelbin)
        # Bin events in d-Spacing
        if not "histo" in self.getProperty("Extension").value:
            wksp = Rebin(InputWorkspace=wksp, OutputWorkspace=wksp.name(),
                         Params=str(self._binning[0])+","+str((self._binning[1]))+","+str(self._binning[2]))
        (temp, numGroupedSpectra, numGroups) = CreateGroupingWorkspace(InputWorkspace=wksp, GroupDetectorsBy=self._grouping,\
                                OutputWorkspace=str(wksp)+"group")
        if (numGroupedSpectra==0) or (numGroups==0):
            raise RuntimeError("%d spectra will be in %d groups" % (numGroupedSpectra, numGroups))
        if len(self._smoothGroups) > 0:
            wksp = SmoothData(InputWorkspace=wksp, OutputWorkspace=wksp.name(),
                              NPoints=self._smoothGroups, GroupingWorkspace=str(wksp)+"group")
        # Remove old calibration files
        cmd = "rm "+calib
        os.system(cmd)

        # Get the fit window input workspace
        fitwinws = self.getProperty("FitwindowTableWorkspace").value

        # Set up resolution workspace
        resws = self.getProperty("DetectorResolutionWorkspace").value
        if resws is not None:
            resrange = self.getProperty("AllowedResRange").value
            if len(resrange) < 2:
                raise NotImplementedError("With input of 'DetectorResolutionWorkspace', number of allowed resolution range must be equal to 2.")
            reslowf = resrange[0]
            resupf = resrange[1]
            if reslowf >= resupf:
                raise NotImplementedError("Allowed resolution range factor, lower boundary (%f) must be smaller than upper boundary (%f)."\
                        % (reslowf, resupf))
        else:
            reslowf = 0.0
            resupf = 0.0

        # Get offsets for pixels using interval around cross correlations center and peak at peakpos (d-Spacing)
        GetDetOffsetsMultiPeaks(InputWorkspace=str(wksp), OutputWorkspace=str(wksp)+"offset",
                                DReference=self._peakpos,
                                FitWindowMaxWidth=self.getProperty("PeakWindowMax").value,
                                MinimumPeakHeight=self.getProperty("MinimumPeakHeight").value,
                                MinimumPeakHeightObs=self.getProperty("MinimumPeakHeightObs").value,
                                BackgroundType=self.getProperty("BackgroundType").value,
                                MaxOffset=self._maxoffset, NumberPeaksWorkspace=str(wksp)+"peaks",
                                MaskWorkspace=str(wksp)+"mask",
                                FitwindowTableWorkspace = fitwinws,
                                InputResolutionWorkspace=resws,
                                MinimumResolutionFactor = reslowf,
                                MaximumResolutionFactor = resupf)

        #Fixed SmoothNeighbours for non-rectangular and rectangular
        if self._smoothoffsets and self._xpixelbin*self._ypixelbin>1: # Smooth data if it was summed
            SmoothNeighbours(InputWorkspace=str(wksp)+"offset", OutputWorkspace=str(wksp)+"offset",
                             WeightedSum="Flat",
                             AdjX=self._xpixelbin, AdjY=self._ypixelbin)
        wksp = Rebin(InputWorkspace=wksp, OutputWorkspace=wksp.name(),
                     Params=str(self._binning[0])+","+str((self._binning[1]))+","+str(self._binning[2]))
        lcinst = str(self._instrument)

        outfilename = None
        if "dspacemap" in self._outTypes:
            #write Dspacemap file
            outfilename = self._outDir+lcinst+"_dspacemap_d"+str(wksp).strip(self._instrument+"_")+strftime("_%Y_%m_%d.dat")
            SaveDspacemap(InputWorkspace=str(wksp)+"offset",
                          DspacemapFile=outfilename)
        if "calibration" in self._outTypes:
            SaveCalFile(OffsetsWorkspace=str(wksp)+"offset",
                        GroupingWorkspace=str(wksp)+"group",
                        MaskWorkspace=str(wksp)+"mask", Filename=calib)
            outfilename = calib

        if outfilename is not None:
            self.setProperty("OutputFilename", outfilename)

        return wksp

    def _focus(self, wksp, calib):
        if wksp is None:
            return None
        MaskDetectors(Workspace=wksp, MaskedWorkspace=str(wksp)+"mask")
        wksp = AlignDetectors(InputWorkspace=wksp, OutputWorkspace=wksp.name(),\
                       OffsetsWorkspace=str(wksp)+"offset")
        # Diffraction focusing using new calibration file with offsets
        if self._diffractionfocus:
            wksp = DiffractionFocussing(InputWorkspace=wksp, OutputWorkspace=wksp.name(),
                                        GroupingWorkspace=str(wksp)+"group")
        if not "histo" in self.getProperty("Extension").value:
            SortEvents(InputWorkspace=wksp, SortBy="X Value")
        wksp = Rebin(InputWorkspace=wksp, OutputWorkspace=wksp.name(), Params=self._binning)
        return wksp

    def PyExec(self):
        # get generic information
        SUFFIX = self.getProperty("Extension").value
        self._binning = self.getProperty("Binning").value
        if len(self._binning) != 1 and len(self._binning) != 3:
            raise RuntimeError("Can only specify (width) or (start,width,stop) for binning. Found %d values." % len(self._binning))
        if len(self._binning) == 3:
            if self._binning[0] == 0. and self._binning[1] == 0. and self._binning[2] == 0.:
                raise RuntimeError("Failed to specify the binning")
        self._instrument = self.getProperty("Instrument").value
        config = ConfigService.Instance()
        config['default.facility'] = "SNS"
        config['default.instrument'] = self._instrument
        self._grouping = self.getProperty("GroupDetectorsBy").value
        self._xpixelbin = self.getProperty("XPixelSum").value
        self._ypixelbin = self.getProperty("YPixelSum").value
        self._smoothoffsets = self.getProperty("SmoothSummedOffsets").value
        self._smoothGroups = self.getProperty("SmoothGroups").value
        self._peakpos = self.getProperty("PeakPositions").value
        positions = self._peakpos.strip().split(',')
        if self.getProperty("CrossCorrelation").value:
            self._peakpos1 = float(positions[0])
            self._peakpos2 = 0
            self._peakpos3 = 0
            self._lastpixel = 0
            self._lastpixel2 = 0
            self._lastpixel3 = 0
            peakhalfwidth = self.getProperty("PeakHalfWidth").value
            self._peakmin = self._peakpos1-peakhalfwidth
            self._peakmax = self._peakpos1+peakhalfwidth
            if len(positions) >= 2:
                self._peakpos2 = float(positions[1])
                self._peakmin2 = self._peakpos2-peakhalfwidth
                self._peakmax2 = self._peakpos2+peakhalfwidth
            if len(positions) >= 3:
                self._peakpos3 = float(positions[1])
                self._peakmin3 = self._peakpos3-peakhalfwidth
                self._peakmax3 = self._peakpos3+peakhalfwidth
            detectors = self.getProperty("DetectorsPeaks").value.strip().split(',')
            if detectors[0]:
                self._lastpixel = int(detectors[0])
                self._lastpixel3 = self._lastpixel
            if len(detectors) >= 2:
                self._lastpixel2 = self._lastpixel+int(detectors[1])
                self._lastpixel3 = self._lastpixel2
            if len(detectors) >= 3:
                self._lastpixel3 = self._lastpixel2+int(detectors[2])
            pixelbin2 = self._xpixelbin*self._ypixelbin
            self._ccnumber = self.getProperty("CrossCorrelationPoints").value
        self._maxoffset = self.getProperty("MaxOffset").value
        self._diffractionfocus = self.getProperty("DiffractionFocusWorkspace").value
        self._filterBadPulses = self.getProperty("FilterBadPulses").value
        self._outDir = self.getProperty("OutputDirectory").value+"/"
        self._outTypes = self.getProperty("SaveAs").value
        samRuns = self.getProperty("RunNumber").value
        backRuns = self.getProperty("Background").value
        if len(samRuns) != len(backRuns):
            if (len(backRuns) == 1 and backRuns[0] == 0) or (len(backRuns) <= 0):
                backRuns = [0]*len(samRuns)
            else:
                raise RuntimeError("Number of samples and backgrounds must match (%d!=%d)" % (len(samRuns), len(backRuns)))
        lcinst = str(self._instrument)
        calib = self._outDir+lcinst+"_calibrate_d"+str(samRuns[0])+strftime("_%Y_%m_%d.cal")
        filterWall = (self.getProperty("FilterByTimeMin").value, self.getProperty("FilterByTimeMax").value)

        for (samNum, backNum) in zip(samRuns, backRuns):
            # first round of processing the sample
            samRun = self._loadData(samNum, SUFFIX, filterWall)
            if backNum > 0:
                backRun = self._loadData(backNum, SUFFIX, filterWall)
                samRun -= backRun
                DeleteWorkspace(backRun)
                samRun = CompressEvents(samRun, OutputWorkspace=samRun.name(),
                                        Tolerance=COMPRESS_TOL_TOF) # 100ns
            if self.getProperty("CrossCorrelation").value:
                samRun = self._cccalibrate(samRun, calib)
            else:
                samRun = self._multicalibrate(samRun, calib)
            if self._xpixelbin*self._ypixelbin>1 or len(self._smoothGroups) > 0:
                if AnalysisDataService.doesExist(str(samRun)):
                    AnalysisDataService.remove(str(samRun))
                samRun = self._loadData(samNum, SUFFIX, filterWall)
                LRef = self.getProperty("UnwrapRef").value
                DIFCref = self.getProperty("LowResRef").value
                if (LRef > 0.) or (DIFCref > 0.): # super special Jason stuff
                    if LRef > 0:
                        wksp = UnwrapSNS(InputWorkspace=wksp, OutputWorkspace=wksp.name(),
                                         LRef=LRef)
                    if DIFCref > 0:
                        wksp = RemoveLowResTOF(InputWorkspace=wksp, OutputWorkspace=wksp.name(),
                                               ReferenceDIFC=DIFCref)
            else:
                samRun = ConvertUnits(InputWorkspace=samRun, OutputWorkspace=samRun.name(),
                                      Target="TOF")
            samRun = self._focus(samRun, calib)
            RenameWorkspace(InputWorkspace=samRun, OutputWorkspace=str(samRun)+"_calibrated")

AlgorithmFactory.subscribe(CalibrateRectangularDetectors)
