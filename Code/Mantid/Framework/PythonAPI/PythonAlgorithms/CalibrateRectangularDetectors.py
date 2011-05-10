from MantidFramework import *
from mantidsimple import *
import os
import datetime
from time import localtime, strftime

COMPRESS_TOL_TOF = .01

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
                             Description="Sum detector pixels in X direction.  Must be a factor of X total pixels.  Default is 1.")
        self.declareProperty("YPixelSum", 1,
                             Description="Sum detector pixels in Y direction.  Must be a factor of Y total pixels.  Default is 1.")
        self.declareProperty("Peak1", 0.0,
                             Description="d-space position of reference peak.")
        self.declareProperty("Peak2", 0.0,
                             Description="Optional: d-space position of second reference peak.")
        self.declareProperty("DetectorsPeak1", 0,
                             Description="Number of detector banks for Peak1.  Default is all.")
        self.declareProperty("DetectorsPeak2", 0,
                             Description="Optional: Number of detector banks for Peak2.")
        self.declareProperty("PeakHalfWidth", 0.05,
                             Description="Half width of d-space around peaks. Default is 0.05")
        self.declareProperty("CrossCorrelationPoints", 100,
                             Description="Number of points to find peak from cross correlation.  Default is 100")
        self.declareListProperty("Binning", [0.,0.,0.],
                             Description="Min, Step, and Max of d-space bins.  Linear binning is better for finding offsets.")
        self.declareProperty("DiffractionFocus", False, Description="Diffraction focus by detectors.  Default is False")
        grouping = ["All", "Group(East,West)", "Column", "bank"]
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

    def _findData(self, runnumber, extension):
        #self.log().information(str(dir()))
        #self.log().information(str(dir(mantidsimple)))
        result = FindSNSNeXus(Instrument=self._instrument,
                              RunNumber=runnumber, Extension=extension)
#        result = self.executeSubAlg("FindSNSNeXus", Instrument=self._instrument,
#                                    RunNumber=runnumber, Extension=extension)
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
            kwargs["CompressTolerance"] = COMPRESS_TOL_TOF
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
                filter["FilterByTime_Start"] = filterWall[0]
            if filterWall[1] > 0.:
                filter["FilterByTime_Stop"] = filterWall[1]

        if  runnumber is None or runnumber <= 0:
            return None

        if extension.endswith(".nxs"):
            return self._loadNeXusData(runnumber, extension, **filter)
        else:
            return self._loadPreNeXusData(runnumber, extension)

    def _calibrate(self, wksp, calib, filterLogs=None):
        if wksp is None:
            return None
        if self._grouping == "All":
            if str(self._instrument) == "PG3":
                groups = "POWGEN"
            else:
                groups = str(self._instrument)
        elif str(self._instrument) == "SNAP" and self._grouping == "Group(East,West)":
                groups = "East,West"
        else:
            groups = ""
            numrange = 200
            if str(self._instrument) == "SNAP":
                numrange = 19
            for num in xrange(1,numrange):
                comp = wksp.getInstrument().getComponentByName("%s%d" % (self._grouping, num) )
                if not comp == None:
                    groups+=("%s%d," % (self._grouping, num) )
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

        alg = ConvertUnits(InputWorkspace=wksp, OutputWorkspace="temp", Target="dSpacing")
        temp = alg['OutputWorkspace']
        SortEvents(InputWorkspace=temp, SortBy="X Value")
        # Sum pixelbin X pixelbin blocks of pixels
        if self._xpixelbin*self._ypixelbin>1:
                SumNeighbours(InputWorkspace=temp, OutputWorkspace=temp, SumX=self._xpixelbin, SumY=self._ypixelbin)
        # Bin events in d-Spacing
        Rebin(InputWorkspace=temp, OutputWorkspace=temp,Params=str(self._peakmin)+","+str(self._binning[1])+","+str(self._peakmax))
        #Find good peak for reference
        ymax = 0
        for s in range(0,temp.getNumberHistograms()):
            y_s = temp.readY(s)
            midBin = temp.blocksize()/2
            if y_s[midBin] > ymax:
                    refpixel = s
                    ymax = y_s[midBin]
        print "Reference spectra=",refpixel
        # Remove old calibration files
        cmd = "rm "+self._outDir+str(wksp)+".cal*"
        os.system(cmd)
        # Cross correlate spectra using interval around peak at peakpos (d-Spacing)
        if self._lastpixel == 0:
            self._lastpixel = temp.getNumberHistograms()-1
        else:
            self._lastpixel = temp.getNumberHistograms()*self._lastpixel/self._lastpixel2-1
        CrossCorrelate(InputWorkspace=temp, OutputWorkspace=str(wksp)+"cc", ReferenceSpectra=refpixel,
            WorkspaceIndexMin=0, WorkspaceIndexMax=self._lastpixel, XMin=self._peakmin, XMax=self._peakmax)
        # Get offsets for pixels using interval around cross correlations center and peak at peakpos (d-Spacing)
        GetDetectorOffsets(InputWorkspace=str(wksp)+"cc", OutputWorkspace=str(wksp)+"offset", Step=self._binning[1],
            DReference=self._peakpos, XMin=-self._ccnumber, XMax=self._ccnumber, GroupingFileName=self._outDir+str(wksp)+".cal")
        mtd.deleteWorkspace(str(wksp)+"cc")
        mtd.deleteWorkspace(str(wksp)+"offset")
        mtd.releaseFreeMemory()
        Rebin(InputWorkspace=temp, OutputWorkspace=temp,Params=str(self._peakmin2)+","+str(self._binning[1])+","+str(self._peakmax2))
        if self._peakpos2 > 0.0:
            #Find good peak for reference
            ymax = 0
            for s in range(0,temp.getNumberHistograms()):
                y_s = temp.readY(s)
                midBin = temp.blocksize()/2
                if y_s[midBin] > ymax:
                        refpixel = s
                        ymax = y_s[midBin]
            print "Reference spectra=",refpixel
            CrossCorrelate(InputWorkspace=temp, OutputWorkspace=str(wksp)+"cc2", ReferenceSpectra=refpixel,
                WorkspaceIndexMin=self._lastpixel+1, WorkspaceIndexMax=temp.getNumberHistograms()-1, XMin=self._peakmin2, XMax=self._peakmax2)
            # Get offsets for pixels using interval around cross correlations center and peak at peakpos (d-Spacing)
            GetDetectorOffsets(InputWorkspace=str(wksp)+"cc2", OutputWorkspace=str(wksp)+"offset2", Step=self._binning[1],
                DReference=self._peakpos2, XMin=-self._ccnumber, XMax=self._ccnumber, GroupingFileName=self._outDir+str(wksp)+".cal")
            mtd.deleteWorkspace(str(wksp)+"cc2")
            mtd.deleteWorkspace(str(wksp)+"offset2")
            mtd.releaseFreeMemory()
            cmd = "mv "+self._outDir+str(wksp)+".cal "+self._outDir+str(wksp)+".cal1"
            os.system(cmd)
            cmd = "sed 1d "+self._outDir+str(wksp)+".cal >"+self._outDir+str(wksp)+".cal2"
            os.system(cmd)
            cmd = "cat "+self._outDir+str(wksp)+".cal1 "+self._outDir+str(wksp)+".cal2 > "+self._outDir+str(wksp)+".cal"
            os.system(cmd)
            cmd = "rm "+self._outDir+str(wksp)+".cal?"
            os.system(cmd)
        CreateCalFileByNames(InstrumentWorkspace=temp, GroupingFileName=self._outDir+str(wksp)+".cal",
            GroupNames=groups)
        #Move new calibration file to replace old calibration file without detector groups
        cmd = "mv "+self._outDir+str(wksp)+".cal2 "+self._outDir+str(wksp)+".cal"
        os.system(cmd)
        lcinst = str(self._instrument)
        if lcinst == "PG3":
            lcinst == "powgen"
        
        filename = os.path.join(self._outDir, str(wksp))
        if "dspacemap" in self._outTypes:
            #write Dspacemap file
            CaltoDspacemap(InputWorkspace=temp, CalibrationFile=self._outDir+str(wksp)+".cal", 
                DspacemapFile=self._outDir+lcinst+"_dspacemap_d"+str(wksp).strip(self._instrument+"_")+strftime("_%Y_%m_%d.dat"))
        if "calibration" in self._outTypes:
            cmd = "cp "+self._outDir+str(wksp)+".cal "+calib
            os.system(cmd) 
        mtd.deleteWorkspace(str(temp))
        return wksp

    def _focus(self, wksp, calib, filterLogs=None):
        if wksp is None:
            return None
        AlignDetectors(InputWorkspace=wksp, OutputWorkspace=wksp, CalibrationFile=calib)
        # Diffraction focusing using new calibration file with offsets
        if self._diffractionfocus:
            DiffractionFocussing(InputWorkspace=wksp, OutputWorkspace=wksp,
                GroupingFileName=calib)
        SortEvents(InputWorkspace=wksp, SortBy="X Value")
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
        mtd.settings['default.facility'] = 'SNS'
        mtd.settings['default.instrument'] = self._instrument
        self._grouping = self.getProperty("GroupDetectorsBy")
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
        self._diffractionfocus = self.getProperty("DiffractionFocus")
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
        lcinst = str(self._instrument)
        if lcinst == "PG3":
            lcinst == "powgen"
        calib = self._outDir+lcinst+"_calibrate_d"+str(samRuns[0])+strftime("_%Y_%m_%d.cal")
        filterWall = (self.getProperty("FilterByTimeMin"), self.getProperty("FilterByTimeMax"))

        for samRun in samRuns:
            # first round of processing the sample
            samRun = self._loadData(samRun, SUFFIX, filterWall)
            samRun = self._calibrate(samRun, calib, filterLogs)
            samRun = self._focus(samRun, calib, filterLogs)
            RenameWorkspace(InputWorkspace=samRun,OutputWorkspace=str(samRun)+"_calibrated")

            mtd.releaseFreeMemory()

mtd.registerPyAlgorithm(CalibrateRectangularDetectors())
