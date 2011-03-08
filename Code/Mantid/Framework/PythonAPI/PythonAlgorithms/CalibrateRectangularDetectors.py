from MantidFramework import *
from mantidsimple import *
import os
import datetime
from time import localtime, strftime

class CalibrateRectangularDetectors(PythonAlgorithm):

    def category(self):
        return "Diffraction"

    def name(self):
        return "CalibrateRectangularDetectors"

    def PyInit(self):
        instruments = ["PG3", "VULCAN", "SNAP", "NOM"]
        self.declareProperty("Instrument", "PG3",
                             Validator=ListValidator(instruments))
        #types = ["Event preNeXus", "Event NeXus"]
        #self.declareProperty("FileType", "Event NeXus",
        #                     Validator=ListValidator(types))
        self.declareListProperty("RunNumber", [0], Validator=ArrayBoundedValidator(Lower=0))
        self.declareProperty("CompressOnRead", False,
                             Description="Compress the event list when reading in the data")
        self.declareProperty("XPixelSum", 1,
                             Description="Sum detector pixels in X direction.  Must be a factor of X total pixels.")
        self.declareProperty("YPixelSum", 1,
                             Description="Sum detector pixels in Y direction.  Must be a factor of Y total pixels.")
        self.declareProperty("Peak1", 0.7282933,
                             Description="D-space postion of reference peak.")
        self.declareProperty("Peak2", 1.261441,
                             Description="Optional: D-space postion of second reference peak.")
        self.declareProperty("DetectorsPeak1", 17,
                             Description="Number of detector banks for Peak1.")
        self.declareProperty("DetectorsPeak2", 6,
                             Description="Number of detector banks for Peak2.")
        self.declareProperty("PeakHalfWidth", 0.05,
                             Description="Half width of d-space around peaks.")
        self.declareProperty("CrossCorrelationPoints", 100,
                             Description="Number of points to find peak from cross correlation")
        self.declareListProperty("Binning", [0.,0.,0.],
                             Description="Positive is linear bins, negative is logorithmic")
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

    def _findData(self, runnumber, extension):
        # The default filename. Search in the data directories
        result = str(self._instrument) + "_" + str(runnumber) + extension
        if os.path.isfile(result)is not None:
            return result
        result = FindSNSNeXus(Instrument=self._instrument, RunNumber=runnumber,
                              Extension=extension)
        return result["ResultPath"].value

    def _loadPreNeXusData(self, runnumber, extension):
        # find the file to load
        filename = self._findData(runnumber, extension)

        # generate the workspace name
        (path, name) = os.path.split(filename)
        name = name.split('.')[0]
        (name, num) = name.split('_neutron')
        num = num.replace('_event', '') # TODO should do something with this

        # load the prenexus file
        alg = LoadEventPreNeXus(EventFilename=filename, OutputWorkspace=name)
        wksp = alg['OutputWorkspace']

        # add the logs to it
        nxsfile = self._findData(runnumber, ".nxs")
        LoadLogsFromSNSNexus(Workspace=wksp, Filename=nxsfile)
        # TODO filter out events using timemin and timemax

        return wksp

    def _loadNeXusData(self, runnumber, extension, **kwargs):
        if self.getProperty("CompressOnRead"):
            kwargs["CompressTolerance"] = .05
        else:
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

        # generate the workspace name
        (path, name) = os.path.split(filename)
        name = name.split('.')[0] # remove the extension
        if "_event" in name:
            name = name[0:-1*len("_event")]

        # TODO use timemin and timemax to filter what events are being read
        alg = LoadEventNexus(Filename=filename, OutputWorkspace=name, **kwargs)

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

        if extension.endswith(".nxs"):
            return self._loadNeXusData(runnumber, extension, **filter)
        else:
            return self._loadPreNeXusData(runnumber, extension)

    def _calibrate(self, wksp, calib, filterLogs=None):
        if wksp is None:
            return None
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
        CompressEvents(InputWorkspace=wksp, OutputWorkspace=wksp, Tolerance=.01) # 100ns
        
        ConvertUnits(InputWorkspace=wksp, OutputWorkspace=wksp, Target="dSpacing")
        Sort(InputWorkspace=wksp, SortBy="Time of Flight")
        # Sum pixelbin X pixelbin blocks of pixels
        if self._xpixelbin*self._ypixelbin>1:
                SumNeighbours(InputWorkspace=wksp, OutputWorkspace=wksp, SumX=self._xpixelbin, SumY=self._ypixelbin)
        # Bin events in d-Spacing
        Rebin(InputWorkspace=wksp, OutputWorkspace=wksp,Params=str(self._peakmin)+","+str(self._binning[1])+","+str(self._peakmax))
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
        cmd = "rm "+self._outDir+str(wksp)+".cal*"
        os.system(cmd)
        # Cross correlate groups using interval around peak at peakpos (d-Spacing)
        self._lastpixel = wksp.getNumberHistograms()*self._lastpixel/self._lastpixel2-1
        CrossCorrelate(InputWorkspace=wksp, OutputWorkspace=str(wksp)+"cc", ReferenceSpectra=refpixel,
            WorkspaceIndexMin=0, WorkspaceIndexMax=self._lastpixel, XMin=self._peakmin, XMax=self._peakmax)
        # Get offsets for pixels using interval around cross correlations center and peak at peakpos (d-Spacing)
        GetDetectorOffsets(InputWorkspace=str(wksp)+"cc", OutputWorkspace=str(wksp)+"offset", Step=self._binning[1],
            DReference=self._peakpos, XMin=-self._ccnumber, XMax=self._ccnumber, GroupingFileName=self._outDir+str(wksp)+".cal1")
        Rebin(InputWorkspace=wksp, OutputWorkspace=wksp,Params=str(self._peakmin2)+","+str(self._binning[1])+","+str(self._peakmax2))
         #Find good peak for reference
        ymax = 0
        for s in range(0,wksp.getNumberHistograms()):
            y_s = wksp.readY(s)
            midBin = wksp.blocksize()/2
            if y_s[midBin] > ymax:
                    refpixel = s
                    ymax = y_s[midBin]
        print "Reference spectra=",refpixel
        CrossCorrelate(InputWorkspace=wksp, OutputWorkspace=str(wksp)+"cc2", ReferenceSpectra=refpixel,
            WorkspaceIndexMin=self._lastpixel+1, WorkspaceIndexMax=wksp.getNumberHistograms()-1, XMin=self._peakmin2, XMax=self._peakmax2)
        # Get offsets for pixels using interval around cross correlations center and peak at peakpos (d-Spacing)
        GetDetectorOffsets(InputWorkspace=str(wksp)+"cc2", OutputWorkspace=str(wksp)+"offset2", Step=self._binning[1],
            DReference=self._peakpos2, XMin=-self._ccnumber, XMax=self._ccnumber, GroupingFileName=self._outDir+str(wksp)+".cal")
        cmd = "sed 1d "+self._outDir+str(wksp)+".cal >"+self._outDir+str(wksp)+".cal2"
        os.system(cmd)
        cmd = "cat "+self._outDir+str(wksp)+".cal1 "+self._outDir+str(wksp)+".cal2 > "+self._outDir+str(wksp)+".cal"
        os.system(cmd)
        cmd = "rm "+self._outDir+str(wksp)+".cal2"
        os.system(cmd)
        CreateCalFileByNames(InstrumentWorkspace=wksp, GroupingFileName=self._outDir+str(wksp)+".cal",
            GroupNames="bank22,bank23,bank24,bank42,bank43,bank44,bank62,bank63,bank64,bank82,bank83,bank84,bank102,bank103,bank104,bank105,bank106,bank123,bank124,bank143,bank144,bank164,bank184")
        #Move new calibration file to replace old calibration file without detector groups
        cmd = "mv "+self._outDir+str(wksp)+".cal2 "+self._outDir+str(wksp)+".cal"
        os.system(cmd)
        # Convert back to TOF
        ConvertUnits(InputWorkspace=wksp, OutputWorkspace=wksp, Target="TOF")
        
        filename = os.path.join(self._outDir, str(wksp))
        if "dspacemap" in self._outTypes:
            #write Dspacemap file
            CaltoDspacemap(InputWorkspace=wksp, CalibrationFile=self._outDir+str(wksp)+".cal", 
                DspacemapFile=self._outDir+"powgen_dspacemap_d"+str(wksp).strip(self._instrument+"_")+strftime("_%Y_%m_%d.dat"))
        if "calibration" in self._outTypes:
            cmd = "cp "+self._outDir+str(wksp)+".cal "+calib
            os.system(cmd) 
        return wksp

    def _focus(self, wksp, calib, filterLogs=None):
        if wksp is None:
            return None
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
        CompressEvents(InputWorkspace=wksp, OutputWorkspace=wksp, Tolerance=.01) # 100ns
        
        AlignDetectors(InputWorkspace=wksp, OutputWorkspace=wksp, CalibrationFile=calib)
        DiffractionFocussing(InputWorkspace=wksp, OutputWorkspace=wksp,
                             GroupingFileName=calib)
        Sort(InputWorkspace=wksp, SortBy="Time of Flight")
        Rebin(InputWorkspace=wksp, OutputWorkspace=wksp, Params=self._binning)
        return wksp

    def PyExec(self):
        # temporary hack for getting python algorithms working
        import mantidsimple
        globals()["FindSNSNeXus"] = mantidsimple.FindSNSNeXus

        # get generic information
        SUFFIX = "_event.nxs"
        self._binning = self.getProperty("Binning")
        if len(self._binning) != 1 and len(self._binning) != 3:
            raise RuntimeError("Can only specify (width) or (start,width,stop) for binning. Found %d values." % len(self._binning))
        if len(self._binning) == 3:
            if self._binning[0] == 0. and self._binning[1] == 0. and self._binning[2] == 0.:
                raise RuntimeError("Failed to specify the binning")
        self._instrument = self.getProperty("Instrument")
        self._xpixelbin = self.getProperty("XPixelSum")
        self._ypixelbin = self.getProperty("YPixelSum")
        self._peakpos = self.getProperty("Peak1")
        self._peakpos2 = self.getProperty("Peak2")
        pixelbin2 = self._xpixelbin*self._ypixelbin
        self._lastpixel = self.getProperty("DetectorsPeak1")
        self._lastpixel2 = (self.getProperty("DetectorsPeak1")+self.getProperty("DetectorsPeak2"))
        peakhalfwidth = self.getProperty("PeakHalfWidth")
        self._ccnumber = self.getProperty("CrossCorrelationPoints")
        self._peakmin = self._peakpos-peakhalfwidth
        self._peakmax = self._peakpos+peakhalfwidth
        self._peakmin2 = self._peakpos2-peakhalfwidth
        self._peakmax2 = self._peakpos2+peakhalfwidth
#        self._timeMin = self.getProperty("FilterByTimeMin")
#        self._timeMax = self.getProperty("FilterByTimeMax")
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
        calib = self._outDir+"powgen_calibrate_d"+str(samRuns[0])+strftime("_%Y_%m_%d.cal")
        filterWall = (self.getProperty("FilterByTimeMin"), self.getProperty("FilterByTimeMax"))

        for samRun in samRuns:
            # first round of processing the sample
            samRun = self._loadData(samRun, SUFFIX, filterWall)
            samRun = self._calibrate(samRun, calib, filterLogs)
            samRun = self._focus(samRun, calib, filterLogs)
            mtd.releaseFreeMemory()

mtd.registerPyAlgorithm(CalibrateRectangularDetectors())
