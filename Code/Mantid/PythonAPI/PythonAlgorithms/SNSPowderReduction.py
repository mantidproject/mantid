from MantidFramework import *
from mantidsimple import *
import os

class PDInfo:
    def __init__(self, data):
        self.bank = int(data[0])
        self.van = int(data[1])
        self.can = int(data[2])
        self.tmin = data[3] * 1000. # convert to microseconds
        self.tmax = data[4] * 1000.

class PDConfigFile(object):
    def __init__(self, filename):
        self.filename = filename
        handle = file(filename, 'r')
        self._data = {}
        for line in handle.readlines():
            self._addData(line)
    def _addData(self, line):
        if line.startswith('#') or len(line.strip()) <= 0:
            return
        data = line.strip().split()
        data = [float(i) for i in data]
        if data[0] not in self._data.keys():
            self._data[data[0]]={}
        self._data[data[0]][data[1]]=PDInfo(data[2:])
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
        frequency = self.__getFrequency(float(frequency))
        wavelength = self.__getWavelength(frequency, float(wavelength))

        return self._data[frequency][wavelength]

class SNSPowderReduction(PythonAlgorithm):
    def category(self):
        return "Diffraction"

    def name(self):
        return "SNSPowderReduction"

    def PyInit(self):
        instruments = ["PG3"] #, "VULCAN", "SNAP", "NOMAD"]
        self.declareProperty("Instrument", "PG3",
                             Validator=ListValidator(instruments))
        #types = ["Event preNeXus", "Event NeXus"]
        #self.declareProperty("FileType", "Event NeXus",
        #                     Validator=ListValidator(types))
        self.declareListProperty("RunNumber", [0], Validator=BoundedValidator(Lower=0))
        self.declareFileProperty("CalibrationFile", "", FileAction.Load,
                                 [".cal"])
        self.declareFileProperty("CharacterizationRunsFile", "", FileAction.Load,
                                 ['.txt'],
                                 Description="File with characterization runs denoted")
#        self.declareProperty("FilterByTimeMin", 0.,
#                             Description="Relative time to start filtering by")
#        self.declareProperty("FilterByTimeMax", 0.,
#                             Description="Relative time to stop filtering by")
        self.declareProperty("BinWidth", 0.,
                             Description="Positive is linear bins, negative is logorithmic")
        self.declareProperty("VanadiumPeakWidthPercentage", 5.)
        self.declareProperty("VanadiumSmoothNumPoints", 11)
        self.declareProperty("FilterBadPulses", True, Description="Filter out events measured while proton charge is more than 5% below average")
        outfiletypes = ['gsas', 'fullprof', 'gsas and fullprof']
        self.declareProperty("FilterByLogValue", "", Description="Name of log value to filter by")
        self.declareProperty("FilterMinimumValue", 0.0, Description="Minimum log value for which to keep events.")
        self.declareProperty("FilterMaximumValue", 0.0, Description="Maximum log value for which to keep events.")
        self.declareProperty("SaveAs", "gsas", ListValidator(outfiletypes))
        self.declareFileProperty("OutputDirectory", "", FileAction.Directory)

    def _findData(self, runnumber, extension):
        #self.log().information(str(dir()))
        #self.log().information(str(dir(mantidsimple)))
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

    def _loadNeXusData(self, runnumber, extension):
        # find the file to load
        filename = self._findData(runnumber, extension)

        # generate the workspace name
        (path, name) = os.path.split(filename)
        name = name.split('.')[0] # remove the extension
        if "_event" in name:
            name = name[0:-1*len("_event")]

        # TODO use timemin and timemax to filter what events are being read
        alg = LoadSNSEventNexus(Filename=filename, OutputWorkspace=name)

        return alg.workspace()

    def _loadData(self, runnumber, extension):
        if  runnumber is None or runnumber <= 0:
            return None

        if extension.endswith(".nxs"):
            return self._loadNeXusData(runnumber, extension)
        else:
            return self._loadPreNeXusData(runnumber, extension)

    def _focus(self, wksp, calib, info, filterLogs=None):
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
        
        AlignDetectors(InputWorkspace=wksp, OutputWorkspace=wksp, CalibrationFile=calib)
        DiffractionFocussing(InputWorkspace=wksp, OutputWorkspace=wksp,
                             GroupingFileName=calib)
        ConvertUnits(InputWorkspace=wksp, OutputWorkspace=wksp, Target="TOF")
        Sort(InputWorkspace=wksp, SortBy="Time of Flight")
        Rebin(InputWorkspace=wksp, OutputWorkspace="temp", Params=[info.tmin, self._delta, info.tmax]) # TODO should be done in place
        NormaliseByCurrent(InputWorkspace="temp", OutputWorkspace="temp") # TODO should be in place
        RenameWorkspace(InputWorkspace="temp", OutputWorkspace=str(wksp))
        return wksp

    def _getinfo(self, wksp):
        logs = wksp.getRun()
        # get the frequency
        frequency = logs['SpeedRequest1']
        if frequency.units != "Hz":
            raise RuntimeError("Only know how to deal with frequency in Hz, not %s" % frequency.units)
        frequency = frequency.getStatistics().mean

        wavelength = logs['LambdaRequest']
        if wavelength.units != "Angstrom":
            raise RuntimeError("Only know how to deal with LambdaRequest in Angstrom, not $s" % wavelength)
        wavelength = wavelength.getStatistics().mean

        self.log().information("frequency: " + str(frequency) + "Hz center wavelength:" + str(wavelength) + "Angstrom")
        return self._config.getInfo(frequency, wavelength)        

    def _save(self, wksp, info, normalized):
        filename = os.path.join(self._outDir, str(wksp))
        if "gsas" in self._outTypes:
            SaveGSS(InputWorkspace=wksp, Filename=filename+".gsa", SplitFiles="False", Append=False, MultiplyByBinWidth=normalized, Bank=info.bank)
        if "fullprof" in self._outTypes:
            SaveFocusedXYE(InputWorkspace=wksp, Filename=filename+".dat")

    def PyExec(self):
        # temporary hack for getting python algorithms working
        import mantidsimple
        reload(mantidsimple)
        globals()["FindSNSNeXus"] = mantidsimple.FindSNSNeXus

        # get generic information
        SUFFIX = "_event.nxs"
        self._config = PDConfigFile(self.getProperty("CharacterizationRunsFile"))
        self._delta = self.getProperty("BinWidth")
        self._instrument = self.getProperty("Instrument")
#        self._timeMin = self.getProperty("FilterByTimeMin")
#        self._timeMax = self.getProperty("FilterByTimeMax")
        self._filterBadPulses = self.getProperty("FilterBadPulses")
        filterLogs = self.getProperty("FilterByLogValue")
        if len(filterLogs.strip()) <= 0:
            filterLogs = None
        else:
            filterLogs = [filterLogs, 
                          self.getProperty("FilterMinimumValue"), self.getProperty("FilterMaximumValue")]
        self._vanPeakWidthPercent = self.getProperty("VanadiumPeakWidthPercentage")
        self._vanSmoothPoints = self.getProperty("VanadiumSmoothNumPoints")
        calib = self.getProperty("CalibrationFile")
        self._outDir = self.getProperty("OutputDirectory")
        self._outTypes = self.getProperty("SaveAs")
        samRuns = self.getProperty("RunNumber")

        for samRun in samRuns:
            # first round of processing the sample 
            samRun = self._loadData(samRun, SUFFIX)
            info = self._getinfo(samRun)
            samRun = self._focus(samRun, calib, info, filterLogs)

            # process the container
            if info.can > 0:
                canRun = mtd["%s_%d" % (self._instrument, info.can)]
                if canRun is None:
                    canRun = self._loadData(info.can, SUFFIX)
                    canRun = self._focus(canRun, calib, info)
            else:
                canRun = None 

            # process the vanadium run
            if info.van > 0:
                vanRun = mtd["%s_%d" % (self._instrument, info.van)]
                if vanRun is None:
                    vanRun = self._loadData(info.van, SUFFIX)
                    vanRun = self._focus(vanRun, calib, info)
                    ConvertUnits(InputWorkspace=vanRun, OutputWorkspace=vanRun, Target="dSpacing")
                    StripVanadiumPeaks(InputWorkspace=vanRun, OutputWorkspace="temp", PeakWidthPercent=self._vanPeakWidthPercent)
                    RenameWorkspace(InputWorkspace="temp", OutputWorkspace=str(vanRun))
                    ConvertUnits(InputWorkspace=vanRun, OutputWorkspace=vanRun, Target="TOF")
                    SmoothData(InputWorkspace=vanRun, OutputWorkspace=vanRun, NPoints=self._vanSmoothPoints)
            else:
                vanRun = None

            # the final bit of math
            if canRun is not None:
                samRun -= canRun
            if vanRun is not None:
                samRun /= vanRun
                normalized = True
            else:
                normalized = False

            # write out the files
            self._save(samRun, info, normalized)

mtd.registerPyAlgorithm(SNSPowderReduction())
