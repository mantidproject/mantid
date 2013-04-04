"""*WIKI* 

Here are examples of input and output from PG3 and SNAP:

[[Image:PG3_Calibrate.png]]

[[Image:SNAP_Calibrate.png]]

-The purpose of this algorithm is to calibrate the detector pixels and write a calibration file.  The calibration file name contains the instrument, run number, and date of calibration.  A binary Dspacemap file that converts from TOF to d-space including the calculated offsets is also an output option.  For CrossCorrelation option:  If one peak is not in the spectra of all the detectors, you can specify the first n detectors to be calibrated with one peak and the next n detectors to be calibrated with the second peak.  If a color fill plot of the calibrated workspace does not look good, do a color fill plot of the workspace that ends in cc to see if the CrossCorrelationPoints and/or PeakHalfWidth should be increased or decreased.  Also plot the reference spectra from the cc workspace.


*WIKI*"""

from MantidFramework import *
from mantidsimple import *
import os
import datetime
from time import localtime, strftime
from mantid import config

COMPRESS_TOL_TOF = .01

class CalibrateRectangularDetectors(PythonAlgorithm):

    def category(self):
        return "Diffraction;PythonAlgorithms"

    def name(self):
        return "CalibrateRectangularDetectors"

    def PyInit(self):
        self.setWikiSummary("Calibrate the detector pixels and write a calibration file")
        sns = mtd.getSettings().facility("SNS")
        instruments = []
        for instr in sns.instruments():
          for tech in instr.techniques():
            if "Neutron Diffraction" == str(tech):
              instruments.append(instr.shortName())
              break
        self.declareProperty("Instrument", "PG3",
                             Validator=ListValidator(instruments))
        #types = ["Event preNeXus", "Event NeXus"]
        #self.declareProperty("FileType", "Event NeXus",
        #                     Validator=ListValidator(types))
        self.declareListProperty("RunNumber", [0], Validator=ArrayBoundedValidator(Lower=0))
        self.declareListProperty("Background", [0], Validator=ArrayBoundedValidator(Lower=0))
        extensions = [ "_histo.nxs", "_event.nxs", "_runinfo.xml"]
        self.declareProperty("Extension", "_event.nxs",
                             Validator=ListValidator(extensions))
        self.declareProperty("CompressOnRead", False,
                             Description="Compress the event list when reading in the data")
        self.declareProperty("XPixelSum", 1,
                             Description="Sum detector pixels in X direction.  Must be a factor of X total pixels.  Default is 1.")
        self.declareProperty("YPixelSum", 1,
                             Description="Sum detector pixels in Y direction.  Must be a factor of Y total pixels.  Default is 1.")
        self.declareProperty("SmoothSummedOffsets", False,
                             Description="If the data was summed for calibration, smooth the resulting offsets workspace.")
        self.declareProperty("SmoothGroups", "", 
                             Description="Comma delimited number of points for smoothing pixels in each group.  Default is no Smoothing.")
        self.declareProperty("UnwrapRef", 0.,
                             Description="Reference total flight path for frame unwrapping. Zero skips the correction")
        self.declareProperty("LowResRef", 0.,
                             Description="Reference DIFC for resolution removal. Zero skips the correction")
        self.declareProperty("MaxOffset", 1.0,
                             Description="Maximum absolute value of offsets; default is 1")
        self.declareProperty("CrossCorrelation", True,
                             Description="CrossCorrelation if True; minimize using many peaks if False.")
        self.declareProperty("PeakPositions", "", 
                             Description="Comma delimited d-space positions of reference peaks.  Use 1-3 for Cross Correlation.  Unlimited for many peaks option.")
        self.declareProperty("PeakWindowMax", 0.,
                             Description="Maximum window around a peak to search for it. Optional.")
        self.declareProperty("PeakFunction", "Gaussian", Validator=ListValidator(["BackToBackExponential", "Gaussian", "Lorentzian"]),
                             Description="Type of peak to fit. Used only with CrossCorrelation=False")
        self.declareProperty("BackgroundType", "Flat", Validator=ListValidator(['Flat', 'Linear', 'Quadratic']),
                             Description="Used only with CrossCorrelation=False")
        self.declareProperty("DetectorsPeaks", "",
                             Description="Comma delimited numbers of detector banks for each peak if using 2-3 peaks for Cross Correlation.  Default is all.")
        self.declareProperty("PeakHalfWidth", 0.05,
                             Description="Half width of d-space around peaks for Cross Correlation. Default is 0.05")
        self.declareProperty("CrossCorrelationPoints", 100,
                             Description="Number of points to find peak from Cross Correlation.  Default is 100")
        self.declareListProperty("Binning", [0.,0.,0.],
                             Description="Min, Step, and Max of d-space bins.  Logarithmic binning is used if Step is negative.")
        self.declareProperty("DiffractionFocusWorkspace", False, Description="Diffraction focus by detectors.  Default is False")
        grouping = ["All", "Group", "Column", "bank"]
        self.declareProperty("GroupDetectorsBy", "All", Validator=ListValidator(grouping),
                             Description="Detector groups to use for future focussing: All detectors as one group, Groups (East,West for SNAP), Columns for SNAP, detector banks")
        self.declareProperty("FilterBadPulses", True, Description="Filter out events measured while proton charge is more than 5% below average")
        self.declareProperty("FilterByTimeMin", 0.,
                             Description="Relative time to start filtering by in seconds. Applies only to sample.")
        self.declareProperty("FilterByTimeMax", 0.,
                             Description="Relative time to stop filtering by in seconds. Applies only to sample.")
        self.declareProperty("FilterByLogValue", "", Description="Name of log value to filter by")
        self.declareProperty("FilterMinimumValue", 0.0, Description="Minimum log value for which to keep events.")
        self.declareProperty("FilterMaximumValue", 0.0, Description="Maximum log value for which to keep events.")
        outfiletypes = ['dspacemap', 'calibration', 'dspacemap and calibration']
        self.declareProperty("SaveAs", "calibration", ListValidator(outfiletypes))
        self.declareFileProperty("OutputDirectory", "", FileAction.Directory)

    def _loadPreNeXusData(self, runnumber, extension, **kwargs):
        mykwargs = {}
        if kwargs.has_key("FilterByTimeStart"):
            mykwargs["ChunkNumber"] = int(kwargs["FilterByTimeStart"])
        if kwargs.has_key("FilterByTimeStop"):
            mykwargs["TotalChunks"] = int(kwargs["FilterByTimeStop"])

        # generate the workspace name
        name = "%s_%d" % (self._instrument, runnumber)
        filename = name + extension

        alg = LoadPreNexus(Filename=filename, OutputWorkspace=name, **mykwargs)
        wksp = alg['OutputWorkspace']

        # add the logs to it
        if (str(self._instrument) == "SNAP"):
            LoadInstrument(Workspace=wksp, InstrumentName=self._instrument, RewriteSpectraMap=False)

        return wksp

    def _loadEventNeXusData(self, runnumber, extension, **kwargs):
        kwargs["Precount"] = True
        name = "%s_%d" % (self._instrument, runnumber)
        filename = name + extension

        alg = LoadEventNexus(Filename=filename, OutputWorkspace=name, **kwargs)
        wksp = alg.workspace()
        # For NOMAD data before Aug 2012, use the updated geometry
        if str(wksp.getInstrument().getValidFromDate()) == "1900-01-31T23:59:59" and str(self._instrument) == "NOMAD":
            path=config["instrumentDefinition.directory"]
            LoadInstrument(Workspace=wksp, Filename=path+'/'+"NOMAD_Definition_20120701-20120731.xml",RewriteSpectraMap=False)
        return wksp

    def _loadHistoNeXusData(self, runnumber, extension):
        name = "%s_%d" % (self._instrument, runnumber)
        filename = name + extension

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
        else:
            return self._loadPreNeXusData(runnumber, extension, **filter)

    def _cccalibrate(self, wksp, calib, filterLogs=None):
        if wksp is None:
            return None
        LRef = self.getProperty("UnwrapRef")
        DIFCref = self.getProperty("LowResRef")
        if (LRef > 0.) or (DIFCref > 0.): # super special Jason stuff
            if LRef > 0:
                UnwrapSNS(InputWorkspace=wksp, OutputWorkspace=wksp, LRef=LRef)
            if DIFCref > 0:
                RemoveLowResTOF(InputWorkspace=wksp, OutputWorkspace=wksp, ReferenceDIFC=DIFCref)
        # take care of filtering events
        if self._filterBadPulses and not self.getProperty("CompressOnRead"):
            FilterBadPulses(InputWorkspace=wksp, OutputWorkspace=wksp)
        if filterLogs is not None:
            try:
                logparam = wksp.getRun()[filterLogs[0]]
                if logparam is not None:
                    FilterByLogValue(InputWorkspace=wksp, OutputWorkspace=wksp, LogName=filterLogs[0],
                                     MinimumValue=filterLogs[1], MaximumValue=filterLogs[2])
            except KeyError, e:
                raise RuntimeError("Failed to find log '%s' in workspace '%s'" \
                                   % (filterLogs[0], str(wksp)))
        if not self.getProperty("CompressOnRead"):
            CompressEvents(InputWorkspace=wksp, OutputWorkspace=wksp, Tolerance=COMPRESS_TOL_TOF) # 100ns

        ConvertUnits(InputWorkspace=wksp, OutputWorkspace=wksp, Target="dSpacing")
        SortEvents(InputWorkspace=wksp, SortBy="X Value")
        # Sum pixelbin X pixelbin blocks of pixels
        if self._xpixelbin*self._ypixelbin>1:
                SumNeighbours(InputWorkspace=wksp, OutputWorkspace=wksp, SumX=self._xpixelbin, SumY=self._ypixelbin)
        # Bin events in d-Spacing
        Rebin(InputWorkspace=wksp, OutputWorkspace=wksp,Params=str(self._peakmin)+","+str(abs(self._binning[1]))+","+str(self._peakmax))
        #Find good peak for reference
        ymax = 0
        for s in range(0,wksp.getNumberHistograms()):
            y_s = wksp.readY(s)
            midBin = wksp.blocksize()/2
            if y_s[midBin] > ymax:
                    refpixel = s
                    ymax = y_s[midBin]
        print "Reference spectra=",refpixel
        # Remove old calibration files
        cmd = "rm "+calib
        os.system(cmd)
        # Cross correlate spectra using interval around peak at peakpos (d-Spacing)
        if self._lastpixel == 0:
            self._lastpixel = wksp.getNumberHistograms()-1
        else:
            self._lastpixel = wksp.getNumberHistograms()*self._lastpixel/self._lastpixel3-1
        CrossCorrelate(InputWorkspace=wksp, OutputWorkspace=str(wksp)+"cc", ReferenceSpectra=refpixel,
            WorkspaceIndexMin=0, WorkspaceIndexMax=self._lastpixel, XMin=self._peakmin, XMax=self._peakmax)
        # Get offsets for pixels using interval around cross correlations center and peak at peakpos (d-Spacing)
        GetDetectorOffsets(InputWorkspace=str(wksp)+"cc", OutputWorkspace=str(wksp)+"offset", Step=abs(self._binning[1]),
            DReference=self._peakpos1, XMin=-self._ccnumber, XMax=self._ccnumber, MaxOffset=self._maxoffset, MaskWorkspace=str(wksp)+"mask")
        mtd.deleteWorkspace(str(wksp)+"cc")
        mtd.releaseFreeMemory()
        if self._peakpos2 > 0.0:
            Rebin(InputWorkspace=wksp, OutputWorkspace=wksp,Params=str(self._peakmin2)+","+str(abs(self._binning[1]))+","+str(self._peakmax2))
            #Find good peak for reference
            ymax = 0
            for s in range(0,wksp.getNumberHistograms()):
                y_s = wksp.readY(s)
                midBin = wksp.blocksize()/2
                if y_s[midBin] > ymax:
                        refpixel = s
                        ymax = y_s[midBin]
            print "Reference spectra = ",refpixel, ", lastpixel_3 = ", self._lastpixel3
            self._lastpixel2 = wksp.getNumberHistograms()*self._lastpixel2/self._lastpixel3-1
            CrossCorrelate(InputWorkspace=wksp, OutputWorkspace=str(wksp)+"cc2", ReferenceSpectra=refpixel,
                WorkspaceIndexMin=self._lastpixel+1, WorkspaceIndexMax=self._lastpixel2, XMin=self._peakmin2, XMax=self._peakmax2)
            # Get offsets for pixels using interval around cross correlations center and peak at peakpos (d-Spacing)
            GetDetectorOffsets(InputWorkspace=str(wksp)+"cc2", OutputWorkspace=str(wksp)+"offset2", Step=abs(self._binning[1]),
                DReference=self._peakpos2, XMin=-self._ccnumber, XMax=self._ccnumber, MaxOffset=self._maxoffset, MaskWorkspace=str(wksp)+"mask2")
            Plus(LHSWorkspace=str(wksp)+"offset", RHSWorkspace=str(wksp)+"offset2",OutputWorkspace=str(wksp)+"offset")
            Plus(LHSWorkspace=str(wksp)+"mask", RHSWorkspace=str(wksp)+"mask2",OutputWorkspace=str(wksp)+"mask")
            mtd.deleteWorkspace(str(wksp)+"cc2")
            mtd.deleteWorkspace(str(wksp)+"offset2")
            mtd.deleteWorkspace(str(wksp)+"mask2")
            mtd.releaseFreeMemory()
        if self._peakpos3 > 0.0:
            Rebin(InputWorkspace=wksp, OutputWorkspace=wksp,Params=str(self._peakmin3)+","+str(abs(self._binning[1]))+","+str(self._peakmax3))
            #Find good peak for reference
            ymax = 0
            for s in range(0,wksp.getNumberHistograms()):
                y_s = wksp.readY(s)
                midBin = wksp.blocksize()/2
                if y_s[midBin] > ymax:
                        refpixel = s
                        ymax = y_s[midBin]
            print "Reference spectra=",refpixel
            CrossCorrelate(InputWorkspace=wksp, OutputWorkspace=str(wksp)+"cc3", ReferenceSpectra=refpixel,
                WorkspaceIndexMin=self._lastpixel2+1, WorkspaceIndexMax=wksp.getNumberHistograms()-1, XMin=self._peakmin3, XMax=self._peakmax3)
            # Get offsets for pixels using interval around cross correlations center and peak at peakpos (d-Spacing)
            GetDetectorOffsets(InputWorkspace=str(wksp)+"cc3", OutputWorkspace=str(wksp)+"offset3", Step=abs(self._binning[1]),
                DReference=self._peakpos3, XMin=-self._ccnumber, XMax=self._ccnumber, MaxOffset=self._maxoffset, MaskWorkspace=str(wksp)+"mask3")
            Plus(LHSWorkspace=str(wksp)+"offset", RHSWorkspace=str(wksp)+"offset3",OutputWorkspace=str(wksp)+"offset")
            Plus(LHSWorkspace=str(wksp)+"mask", RHSWorkspace=str(wksp)+"mask3",OutputWorkspace=str(wksp)+"mask")
            mtd.deleteWorkspace(str(wksp)+"cc3")
            mtd.deleteWorkspace(str(wksp)+"offset3")
            mtd.deleteWorkspace(str(wksp)+"mask3")
            mtd.releaseFreeMemory()
        CreateGroupingWorkspace(InputWorkspace=wksp, GroupDetectorsBy=self._grouping, OutputWorkspace=str(wksp)+"group")
        lcinst = str(self._instrument)
        
        if "dspacemap" in self._outTypes:
            #write Dspacemap file
            SaveDspacemap(InputWorkspace=str(wksp)+"offset",
                DspacemapFile=self._outDir+lcinst+"_dspacemap_d"+str(wksp).strip(self._instrument+"_")+strftime("_%Y_%m_%d.dat"))
        if "calibration" in self._outTypes:
            SaveCalFile(OffsetsWorkspace=str(wksp)+"offset",GroupingWorkspace=str(wksp)+"group",MaskWorkspace=str(wksp)+"mask",Filename=calib)
        return wksp

    def _multicalibrate(self, wksp, calib, filterLogs=None):
        if wksp is None:
            return None
        LRef = self.getProperty("UnwrapRef")
        DIFCref = self.getProperty("LowResRef")
        if (LRef > 0.) or (DIFCref > 0.): # super special Jason stuff
            if LRef > 0:
                UnwrapSNS(InputWorkspace=wksp, OutputWorkspace=wksp, LRef=LRef)
            if DIFCref > 0:
                RemoveLowResTOF(InputWorkspace=wksp, OutputWorkspace=wksp, ReferenceDIFC=DIFCref)
        # take care of filtering events
        if self._filterBadPulses and not self.getProperty("CompressOnRead") and not "histo" in self.getProperty("Extension"):
            FilterBadPulses(InputWorkspace=wksp, OutputWorkspace=wksp)
        if filterLogs is not None:
            try:
                logparam = wksp.getRun()[filterLogs[0]]
                if logparam is not None:
                    FilterByLogValue(InputWorkspace=wksp, OutputWorkspace=wksp, LogName=filterLogs[0],
                                     MinimumValue=filterLogs[1], MaximumValue=filterLogs[2])
            except KeyError, e:
                raise RuntimeError("Failed to find log '%s' in workspace '%s'" \
                                   % (filterLogs[0], str(wksp)))            
        if not self.getProperty("CompressOnRead") and not "histo" in self.getProperty("Extension"):
            CompressEvents(InputWorkspace=wksp, OutputWorkspace=wksp, Tolerance=COMPRESS_TOL_TOF) # 100ns

        ConvertUnits(InputWorkspace=wksp, OutputWorkspace=wksp, Target="dSpacing")
        if not "histo" in self.getProperty("Extension"):
            SortEvents(InputWorkspace=wksp, SortBy="X Value")
        # Sum pixelbin X pixelbin blocks of pixels
        if self._xpixelbin*self._ypixelbin>1:
                SumNeighbours(InputWorkspace=wksp, OutputWorkspace=wksp, SumX=self._xpixelbin, SumY=self._ypixelbin)
        # Bin events in d-Spacing
        if not "histo" in self.getProperty("Extension"):
        	Rebin(InputWorkspace=wksp, OutputWorkspace=wksp,Params=str(self._binning[0])+","+str((self._binning[1]))+","+str(self._binning[2]))
        CreateGroupingWorkspace(InputWorkspace=wksp, GroupDetectorsBy=self._grouping, OutputWorkspace=str(wksp)+"group")
        if len(self._smoothGroups) > 0:
            SmoothData(InputWorkspace=wksp, OutputWorkspace=wksp, NPoints=self._smoothGroups, GroupingWorkspace=str(wksp)+"group")
        # Remove old calibration files
        cmd = "rm "+calib
        os.system(cmd)
        # Get offsets for pixels using interval around cross correlations center and peak at peakpos (d-Spacing)
        GetDetOffsetsMultiPeaks(InputWorkspace=str(wksp), OutputWorkspace=str(wksp)+"offset",
            DReference=self._peakpos, FitWindowMaxWidth=self.getProperty("PeakWindowMax"), BackgroundType=self.getProperty("BackgroundType"),
            MaxOffset=self._maxoffset, NumberPeaksWorkspace=str(wksp)+"peaks", MaskWorkspace=str(wksp)+"mask")
        #Fixed SmoothNeighbours for non-rectangular and rectangular
        if self._smoothoffsets and self._xpixelbin*self._ypixelbin>1: # Smooth data if it was summed
                SmoothNeighbours(InputWorkspace=str(wksp)+"offset", OutputWorkspace=str(wksp)+"offset", WeightedSum="Flat",
                                 AdjX=self._xpixelbin, AdjY=self._ypixelbin)
        Rebin(InputWorkspace=wksp, OutputWorkspace=wksp,Params=str(self._binning[0])+","+str((self._binning[1]))+","+str(self._binning[2]))
        lcinst = str(self._instrument)
        
        if "dspacemap" in self._outTypes:
            #write Dspacemap file
            SaveDspacemap(InputWorkspace=str(wksp)+"offset",
                DspacemapFile=self._outDir+lcinst+"_dspacemap_d"+str(wksp).strip(self._instrument+"_")+strftime("_%Y_%m_%d.dat"))
        if "calibration" in self._outTypes:
            SaveCalFile(OffsetsWorkspace=str(wksp)+"offset",GroupingWorkspace=str(wksp)+"group",MaskWorkspace=str(wksp)+"mask",Filename=calib)
        return wksp

    def _focus(self, wksp, calib, filterLogs=None):
        if wksp is None:
            return None
        MaskDetectors(Workspace=wksp, MaskedWorkspace=str(wksp)+"mask")
        AlignDetectors(InputWorkspace=wksp, OutputWorkspace=wksp, OffsetsWorkspace=str(wksp)+"offset")
        # Diffraction focusing using new calibration file with offsets
        if self._diffractionfocus:
            DiffractionFocussing(InputWorkspace=wksp, OutputWorkspace=wksp,
                GroupingWorkspace=str(wksp)+"group")
        if not "histo" in self.getProperty("Extension"):
            SortEvents(InputWorkspace=wksp, SortBy="X Value")
        Rebin(InputWorkspace=wksp, OutputWorkspace=wksp, Params=self._binning)
        return wksp

    def PyExec(self):
        # get generic information
        SUFFIX = self.getProperty("Extension")
        self._binning = self.getProperty("Binning")
        if len(self._binning) != 1 and len(self._binning) != 3:
            raise RuntimeError("Can only specify (width) or (start,width,stop) for binning. Found %d values." % len(self._binning))
        if len(self._binning) == 3:
            if self._binning[0] == 0. and self._binning[1] == 0. and self._binning[2] == 0.:
                raise RuntimeError("Failed to specify the binning")
        self._instrument = self.getProperty("Instrument")
        mtd.settings['default.facility'] = 'SNS'
        mtd.settings['default.instrument'] = self._instrument
        self._grouping = self.getProperty("GroupDetectorsBy")
        self._xpixelbin = self.getProperty("XPixelSum")
        self._ypixelbin = self.getProperty("YPixelSum")
        self._smoothoffsets = self.getProperty("SmoothSummedOffsets")
        self._smoothGroups = self.getProperty("SmoothGroups")
        self._peakpos = self.getProperty("PeakPositions")
        positions = self._peakpos.strip().split(',')
        if self.getProperty("CrossCorrelation"):
            self._peakpos1 = float(positions[0])
            self._peakpos2 = 0
            self._peakpos3 = 0
            self._lastpixel = 0
            self._lastpixel2 = 0
            self._lastpixel3 = 0
            peakhalfwidth = self.getProperty("PeakHalfWidth")
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
            detectors = self.getProperty("DetectorsPeaks").strip().split(',')
            if detectors[0]:
                self._lastpixel = int(detectors[0])
                self._lastpixel3 = self._lastpixel
            if len(detectors) >= 2:
                self._lastpixel2 = self._lastpixel+int(detectors[1])
                self._lastpixel3 = self._lastpixel2
            if len(detectors) >= 3:
                self._lastpixel3 = self._lastpixel2+int(detectors[2])
            pixelbin2 = self._xpixelbin*self._ypixelbin
            self._ccnumber = self.getProperty("CrossCorrelationPoints")
        self._maxoffset = self.getProperty("MaxOffset")
        self._diffractionfocus = self.getProperty("DiffractionFocusWorkspace")
        self._filterBadPulses = self.getProperty("FilterBadPulses")
        filterLogs = self.getProperty("FilterByLogValue")
        if len(filterLogs.strip()) <= 0:
            filterLogs = None
        else:
            filterLogs = [filterLogs, 
                          self.getProperty("FilterMinimumValue"), self.getProperty("FilterMaximumValue")]
        self._outDir = self.getProperty("OutputDirectory")+"/"
        self._outTypes = self.getProperty("SaveAs")
        samRuns = self.getProperty("RunNumber")
        backRuns = self.getProperty("Background")
        if len(samRuns) != len(backRuns):
            if (len(backRuns) == 1 and backRuns[0] == 0) or (len(backRuns) <= 0):
                backRuns = [0]*len(samRuns)
            else:
                raise RuntimeError("Number of samples and backgrounds must match (%d!=%d)" % (len(samRuns), len(backRuns)))
        lcinst = str(self._instrument)
        calib = self._outDir+lcinst+"_calibrate_d"+str(samRuns[0])+strftime("_%Y_%m_%d.cal")
        filterWall = (self.getProperty("FilterByTimeMin"), self.getProperty("FilterByTimeMax"))

        for (samNum, backNum) in zip(samRuns, backRuns):
            # first round of processing the sample
            samRun = self._loadData(samNum, SUFFIX, filterWall)
            if (backNum > 0):
                backRun = self._loadData(backNum, SUFFIX, filterWall)
                samRun -= backRun
            if str(self._instrument) == "SNAP":
            	alg = CloneWorkspace(samRun, "tmp")
        	origRun = alg['OutputWorkspace']
            if self.getProperty("CrossCorrelation"):
                samRun = self._cccalibrate(samRun, calib, filterLogs)
            else:
                samRun = self._multicalibrate(samRun, calib, filterLogs)
            if self._xpixelbin*self._ypixelbin>1 or len(self._smoothGroups) > 0:
               	mtd.deleteWorkspace(str(samRun))
            	if str(self._instrument) == "SNAP":
			alg = RenameWorkspace(origRun,"%s_%d" % (self._instrument, samNum))
        		samRun = alg['OutputWorkspace']
            	else:
            		samRun = self._loadData(samNum, SUFFIX, filterWall)
                        LRef = self.getProperty("UnwrapRef")
                        DIFCref = self.getProperty("LowResRef")
                        if (LRef > 0.) or (DIFCref > 0.): # super special Jason stuff
                            if LRef > 0:
                                UnwrapSNS(InputWorkspace=wksp, OutputWorkspace=wksp, LRef=LRef)
                            if DIFCref > 0:
                                RemoveLowResTOF(InputWorkspace=wksp, OutputWorkspace=wksp, ReferenceDIFC=DIFCref)
            else:
		ConvertUnits(InputWorkspace=samRun, OutputWorkspace=samRun, Target="TOF")
            samRun = self._focus(samRun, calib, filterLogs)
            RenameWorkspace(InputWorkspace=samRun,OutputWorkspace=str(samRun)+"_calibrated")

            mtd.releaseFreeMemory()

mtd.registerPyAlgorithm(CalibrateRectangularDetectors())
