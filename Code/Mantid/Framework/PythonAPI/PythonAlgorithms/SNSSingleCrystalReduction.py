from MantidFramework import *
from mantidsimple import *
import os

class SNSSingleCrystalReduction(PythonAlgorithm):
    def category(self):
        return "Diffraction"

    def name(self):
        return "SNSSingleCrystalReduction"

    def PyInit(self):
        instruments = ["TOPAZ"]
        self.declareProperty("Instrument", "TOPAZ",
                             Validator=ListValidator(instruments))
        #types = ["Event preNeXus", "Event NeXus"]
        #self.declareProperty("FileType", "Event NeXus",
        #                     Validator=ListValidator(types))
        self.declareListProperty("RunNumber", [0], Validator=BoundedValidator(Lower=0))
        self.declareProperty("BackgroundNumber", 0, Validator=BoundedValidator(Lower=0))
        self.declareProperty("EmptyInstrumentNumber", 0, Validator=BoundedValidator(Lower=0))
        self.declareProperty("VanadiumNumber", 0, Validator=BoundedValidator(Lower=0))
        self.declareProperty("TOFMin", 400.0,
                             Description="Relative time to start filtering by")
        self.declareProperty("TOFMax", 45000.0,
                             Description="Relative time to stop filtering by")
        self.declareProperty("TOFBinWidth", -0.04,
                             Description="Positive is linear bins, negative is logorithmic")
        self.declareProperty("VanadiumPeakWidthPercentage", 5.)
        self.declareProperty("VanadiumSmoothNumPoints", 11)
        self.declareProperty("FilterBadPulses", True, Description="Filter out events measured while proton charge is more than 5% below average")
        outfiletypes = ['nxs'] #, 'gsas', 'fullprof', 'gsas and fullprof']
        self.declareProperty("FilterByLogValue", "", Description="Name of log value to filter by")
        self.declareProperty("FilterMinimumValue", 0.0, Description="Minimum log value for which to keep events.")
        self.declareProperty("FilterMaximumValue", 0.0, Description="Maximum log value for which to keep events.")
        self.declareProperty("SaveAs", "nxs", ListValidator(outfiletypes))
        self.declareFileProperty("OutputDirectory", "", FileAction.Directory)

    def _findData(self, runnumber, extension):
        #self.log().information(str(dir()))
        #self.log().information(str(dir(mantidsimple)))
        if extension.endswith(".nxs"):
            result = "/home/vel/" + str(runnumber) + "/NeXus/" + str(self._instrument) + "_" + str(runnumber) + extension
        else:
            result = "/home/vel/" + str(runnumber) + "/preNeXus/" + str(self._instrument) + "_" + str(runnumber) + extension
        #result = FindSNSNeXus(Instrument=self._instrument, RunNumber=runnumber,
                              #Extension=extension)
        return result
#["ResultPath"].value

    def _loadPreNeXusData(self, runnumber, extension):
        # find the file to load
        filename = self._findData(runnumber, extension)

        # generate the workspace name
        (path, name) = os.path.split(filename)
        name = name.split('.')[0]
        (name, num) = name.split('_neutron')
        num = num.replace('_event', '') # TODO should do something with this

        # load the prenexus file
        alg = LoadEventPreNeXus(EventFilename=filename, OutputWorkspace=name,PadEmptyPixels="1")
        wksp = alg['OutputWorkspace']

        # add the logs to it
        nxsfile = self._findData(runnumber, ".nxs")
        LoadLogsFromSNSNexus(Workspace=wksp, Filename=nxsfile)
        # TODO filter out events using timemin and timemax

        return wksp

    def _loadNeXusData(self, runnumber, extension):
        # find the file to load
        filename = self._findData(runnumber, extension)

        # generate the workspace name
        (path, name) = os.path.split(filename)
        name = name.split('.')[0] # remove the extension
        if "_event" in name:
            name = name[0:-1*len("_event")]

        # TODO use timemin and timemax to filter what events are being read
        alg = LoadSNSEventNexus(filename, name, self._TOFMin, self._TOFMax)
        #alg = LoadSNSEventNexus(Filename=filename, OutputWorkspace=name, FilterByTof_Min=self._TOFMin, FilterByTof_Max=self._TOFMin)

        return alg.workspace()

    def _loadData(self, runnumber, extension):
        if  runnumber is None or runnumber <= 0:
            return None

        if extension.endswith(".nxs"):
            return self._loadNeXusData(runnumber, extension)
        else:
            return self._loadPreNeXusData(runnumber, extension)

    def _bin(self, wksp, filterLogs=None):
        if wksp is None:
            return None
        # take care of filtering events
        if self._filterBadPulses:
            pcharge = wksp.getRun()['proton_charge']
            pcharge = pcharge.getStatistics().mean
            FilterByLogValue(InputWorkspace=wksp, OutputWorkspace=wksp, LogName="proton_charge",
                             MinimumValue=.95*pcharge, MaximumValue=2.*pcharge)
        if filterLogs is not None:
            try:
                logparam = wksp.getRun()[filterLogs[0]]
                if logparam is not None:
                    FilterByLogValue(InputWorkspace=wksp, OutputWorkspace=wksp, LogName=filterLogs[0],
                                     MinimumValue=filterLogs[1], MaximumValue=filterLogs[2])
            except KeyError, e:
                raise RuntimeError("Failed to find log '%s' in workspace '%s'" \
                                   % (filterLogs[0], str(wksp)))
        
        Sort(InputWorkspace=wksp, SortBy="Time of Flight")
        #ConvertUnits(InputWorkspace=wksp, OutputWorkspace=wksp, Target="TOF")
        Rebin(InputWorkspace=wksp, OutputWorkspace="temp", Params=[self._TOFMin, self._delta, self._TOFMax])
        RenameWorkspace(InputWorkspace="temp", OutputWorkspace=wksp)
        NormaliseByCurrent(InputWorkspace=wksp, OutputWorkspace=wksp)

        return wksp

    def _save(self, wksp, normalized):
        filename = os.path.join(self._outDir, str(wksp))
        nxsfile = self._findData(self.getProperty("RunNumber")[0], ".nxs")
        self.log().information(nxsfile)
        if "nxs" in self._outTypes:
            SaveSNSNexus(InputFilename=nxsfile,InputWorkspace=wksp, OutputFilename=filename+"_mantid.nxs")
        if "gsas" in self._outTypes:
            SaveGSS(InputWorkspace=wksp, Filename=filename+".gsa", SplitFiles="False", Append=False, MultiplyByTOFBinWidth=normalized, Bank=self._bank, Format="SLOG")
        if "fullprof" in self._outTypes:
            SaveFocusedXYE(InputWorkspace=wksp, Filename=filename+".dat")

    def PyExec(self):
        # temporary hack for getting python algorithms working
        import mantidsimple
        reload(mantidsimple)
        globals()["FindSNSNeXus"] = mantidsimple.FindSNSNeXus

        # get generic information
        SUFFIX = "_event.nxs"
        self._delta = self.getProperty("TOFBinWidth")
        self._instrument = self.getProperty("Instrument")
        self._bank = 0 #self.getProperty("BankNumber")
        self._TOFMin = self.getProperty("TOFMin")
        self._TOFMax = self.getProperty("TOFMax")
        self._filterBadPulses = self.getProperty("FilterBadPulses")
        filterLogs = self.getProperty("FilterByLogValue")
        if len(filterLogs.strip()) <= 0:
            filterLogs = None
        else:
            filterLogs = [filterLogs, 
                          self.getProperty("FilterMinimumValue"), self.getProperty("FilterMaximumValue")]
        self._vanPeakWidthPercent = self.getProperty("VanadiumPeakWidthPercentage")
        self._vanSmoothPoints = self.getProperty("VanadiumSmoothNumPoints")
        self._outDir = self.getProperty("OutputDirectory")
        self._outTypes = self.getProperty("SaveAs")
        samRuns = self.getProperty("RunNumber")
        bkg = self.getProperty("BackgroundNumber")
        van = self.getProperty("VanadiumNumber")
        empty = self.getProperty("EmptyInstrumentNumber")

        for samRun in samRuns:
            # first round of processing the sample 
            samRun = self._loadData(samRun, SUFFIX)
            samRun = self._bin(samRun, filterLogs)

            # process the background
            if bkg > 0:
                bkgRun = mtd["%s_%d" % (self._instrument, bkg)]
                if bkgRun is None:
                    bkgRun = self._loadData(bkg, SUFFIX)
                    bkgRun = self._bin(bkgRun)
                    samRun -= bkgRun
                    mtd.deleteWorkspace("%s_%d" % (self._instrument, bkg))
            else:
                bkgRun = None 

            # process the empty instrument
            if empty > 0:
                emptyRun = mtd["%s_%d" % (self._instrument, empty)]
                if emptyRun is None:
                    emptyRun = self._loadData(empty, SUFFIX)
                    emptyRun = self._bin(emptyRun)
                    samRun -= emptyRun
                    mtd.deleteWorkspace("%s_%d" % (self._instrument, empty))
            else:
                emptyRun = None 

            # process the vanadium run
            if van > 0:
                vanRun = mtd["%s_%d" % (self._instrument, van)]
                if vanRun is None:
                    vanRun = self._loadData(van, SUFFIX)
                    vanRun = self._bin(vanRun)
                    ConvertUnits(InputWorkspace=vanRun, OutputWorkspace=vanRun, Target="dSpacing")
                    StripVanadiumPeaks(InputWorkspace=vanRun, OutputWorkspace="temp", PeakWidthPercent=self._vanPeakWidthPercent)
                    RenameWorkspace(InputWorkspace="temp", OutputWorkspace=str(vanRun))
                    ConvertUnits(InputWorkspace=vanRun, OutputWorkspace=vanRun, Target="TOF")
                    SmoothData(InputWorkspace=vanRun, OutputWorkspace=vanRun, NPoints=self._vanSmoothPoints)
            else:
                vanRun = None

            # the final bit of math
            if vanRun is not None:
                samRun /= vanRun
                mtd.deleteWorkspace("%s_%d" % (self._instrument, van))
                normalized = True
            else:
                normalized = False

            # write out the files
            ReplaceSpecialValues(InputWorkspace=samRun, OutputWorkspace=samRun, NaNValue="0.0", InfinityValue="0.0")
            self._save(samRun, normalized)

mtd.registerPyAlgorithm(SNSSingleCrystalReduction())
