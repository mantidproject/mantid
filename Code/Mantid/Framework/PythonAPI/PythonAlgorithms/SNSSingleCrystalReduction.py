"""*WIKI* 


The purpose of this algorithm is to subtract collected background data from the sample data pixel-by-pixel and divide by the Vanadium sample data.  The Vanadium data is focussed on each detector, peaks removed, and smoothed.  Time of flight limits, and binning can be changed as well as bad pulses removed.  Must have both Event NeXus and NeXus file for RunNumber in OutputDirectory or available using output from FindNeXus.  NeXus file is used for saving the output in NeXus format.

*WIKI*"""
from MantidFramework import *
from mantidsimple import *
import os

COMPRESS_TOL_TOF = .01

class SNSSingleCrystalReduction(PythonAlgorithm):

    def category(self):
        return "Diffraction"

    def name(self):
        return "SNSSingleCrystalReduction"

    def PyInit(self):
        instruments = ["TOPAZ"]
        self.declareProperty("Instrument", "PG3",
                             Validator=ListValidator(instruments))
        self.declareListProperty("SampleNumbers", [0], Validator=ArrayBoundedValidator(Lower=0))
        self.declareProperty("BackgroundNumber", 0, Validator=BoundedValidator(Lower=0),
                             Description="If specified overrides value in CharacterizationRunsFile")
        self.declareProperty("VanadiumNumber", 0, Validator=BoundedValidator(Lower=0),
                             Description="If specified overrides value in CharacterizationRunsFile")
        self.declareListProperty("Binning", [0.,0.,0.],
                             Description="Positive is linear bins, negative is logorithmic")
        self.declareProperty("FilterBadPulses", True, Description="Filter out events measured while proton charge is more than 5% below average")
        outfiletypes = ['', 'hkl', 'nxs']
        self.declareProperty("SaveAs", "", ListValidator(outfiletypes))
        self.declareProperty("LinearScatteringCoef", 0.0, Description="Linear Scattering Coefficient for Anvred correction.")
        self.declareProperty("LinearAbsorptionCoef", 0.0, Description="Linear Absorption Coefficient for Anvred correction.")
        self.declareProperty("Radius", 0.0, Description="Radius of sphere for Anvred correction. Set to 0 for no Anvred corrections")
        self.declareProperty("PowerLambda", 4.0, Description="Power of wavelength for Anvred correction.")
        self.declareFileProperty("IsawUBFile", "", FileAction.OptionalLoad, ['.mat'], Description="Isaw style file of UB matrix for first sample run.  Sample run number will be changed for next runs.")
        self.declareFileProperty("IsawPeaksFile", "", FileAction.OptionalLoad, ['.peaks'],  Description="Isaw style file of peaks.")
        self.declareFileProperty("OutputDirectory", "", FileAction.Directory)

    def _findData(self, runnumber, extension):
        result = FindSNSNeXus(Instrument=self._instrument,
                              RunNumber=runnumber, Extension=extension)
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
        alg = LoadEventPreNexus(EventFilename=filename, OutputWorkspace=name)
        wksp = alg['OutputWorkspace']

        # add the logs to it
        nxsfile = self._findData(runnumber, ".nxs")
        LoadLogsFromSNSNexus(Workspace=wksp, Filename=nxsfile)
        # TODO filter out events using timemin and timemax

        return wksp

    def _loadNeXusData(self, filename, name, bank, extension, **kwargs):
        alg = LoadEventNexus(Filename=filename, OutputWorkspace=name, BankName=bank, SingleBankPixelsOnly=0, FilterByTofMin=self._binning[0], FilterByTofMax=self._binning[2], LoadMonitors=True, MonitorsAsEvents=True, **kwargs)
        wksp = alg['OutputWorkspace']
        #Normalise by sum of counts in upstream monitor
        Integration(InputWorkspace=mtd[str(name)+'_monitors'], OutputWorkspace='Mon', RangeLower=self._binning[0], RangeUpper=self._binning[2], EndWorkspaceIndex=0)
        mtd.deleteWorkspace(str(name)+'_monitors')
        mtd.releaseFreeMemory()
        temp = mtd['Mon']
        # Numerical precision of same limits for division sometimes results in Counts and somtimes Counts/Counts; no units avoids this
        temp.setYUnit("")
        Divide(LHSWorkspace=wksp, RHSWorkspace=temp, OutputWorkspace=wksp, AllowDifferentNumberSpectra=1)
        mtd.deleteWorkspace('Mon')
        mtd.releaseFreeMemory()
        # take care of filtering events
        if self._filterBadPulses:
            FilterBadPulses(InputWorkspace=wksp, OutputWorkspace=wksp)
        CompressEvents(InputWorkspace=wksp, OutputWorkspace=wksp, Tolerance=COMPRESS_TOL_TOF) # 100ns
        return wksp

    def _findNeXusData(self, runnumber, bank, extension, **kwargs):
        kwargs["Precount"] = True
        name = "%s_%d" % (self._instrument, runnumber)
        filename = name + extension

        try: # first just try loading the file
            return self._loadNeXusData(filename, name, bank, extension, **kwargs)
        except:
            pass

        # find the file to load
        filename = self._findData(runnumber, extension)

        # generate the workspace name
        (path, name) = os.path.split(filename)
        name = name.split('.')[0] # remove the extension
        if "_event" in name:
            name = name[0:-1*len("_event")]

        return self._loadNeXusData(filename, name, bank, extension, **kwargs)

    def _loadData(self, runnumber, bank, extension, filterWall=None):
        filter = {}
        if filterWall is not None:
            if filterWall[0] > 0.:
                filter["FilterByTofStart"] = filterWall[0]
            if filterWall[1] > 0.:
                filter["FilterByTofStop"] = filterWall[1]

        if  runnumber is None or runnumber <= 0:
            return None

        if extension.endswith(".nxs"):
            return self._findNeXusData(runnumber, bank, extension, **filter)
        else:
            return self._findPreNeXusData(runnumber, extension)

    def _focus(self, wksp, bank):
        if wksp is None:
            return None
        CreateGroupingWorkspace(InputWorkspace=wksp, GroupNames=bank, OutputWorkspace=str(wksp)+"group")
        ConvertUnits(InputWorkspace=wksp, OutputWorkspace=wksp, Target="dSpacing")
        SortEvents(InputWorkspace=wksp, SortBy="X Value")
        DiffractionFocussing(InputWorkspace=wksp, OutputWorkspace=wksp,
                             GroupingWorkspace=str(wksp)+"group")
        mtd.deleteWorkspace(str(wksp)+"group")
        mtd.releaseFreeMemory()
        CompressEvents(InputWorkspace=wksp, OutputWorkspace=wksp, Tolerance=COMPRESS_TOL_TOF) # 100ns
        ConvertUnits(InputWorkspace=wksp, OutputWorkspace=wksp, Target="TOF")
        if len(self._binning) == 3:
           binning = self._binning
        else:
            binning = [400., self._binning[0], 16000.]
        Rebin(InputWorkspace=wksp, OutputWorkspace=wksp, Params=binning)
        return wksp

    def _bin(self, wksp):
        if wksp is None:
            return None
        if len(self._binning) == 3:
            binning = self._binning
        else:
            binning = [400., self._binning[0], 16000.]
        Rebin(InputWorkspace=wksp, OutputWorkspace=wksp, Params=binning)

        return wksp

    def _save(self, wksp, normalized):
        if "hkl" in self._outTypes:
            name = str(wksp)
            name = name.split('_')[1] # remove the instrument name
            self._ubfile = self._ubfile.split(name)[0]
            ubname = self._ubfile + name + '.mat'
            LoadIsawUB(InputWorkspace=wksp,Filename=ubname)
            LoadIsawPeaks(Filename=self._peaksfile,OutputWorkspace='Peaks')
            peaksWS = mtd['Peaks']
            PeakIntegration(InputWorkspace=wksp,InPeaksWorkspace=peaksWS,OutPeaksWorkspace=peaksWS)
            hklfile = self._peaksfile.split('.')[0] + '.hkl'
            SaveHKL(LinearScatteringCoef=self._amu,LinearAbsorptionCoef=self._smu,Radius=self._radius,Filename=hklfile, AppendFile=self._append,InputWorkspace=peaksWS)
        if "nxs" in self._outTypes:
            filename = os.path.join(self._outDir, str(wksp))
            nxsfile = str(wksp) + ".nxs"
    
            if not os.path.isfile(nxsfile):
                nxsfile = str(wksp) + "_histo.nxs"
                if not os.path.isfile(nxsfile):
                    name = str(wksp)
                    name = name.split('_')[1] # remove the instrument name
                    nxsfile = self._findData(int(name), "_histo.nxs")
            self.log().information(nxsfile)
            SaveToSNSHistogramNexus(InputFilename=nxsfile,InputWorkspace=wksp, OutputFilename=filename+"_mantid.nxs", Compress=True)

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
        self._bin_in_dspace = False
        self._instrument = self.getProperty("Instrument")
        mtd.settings['default.facility'] = 'SNS'
        mtd.settings['default.instrument'] = self._instrument
        self._filterBadPulses = self.getProperty("FilterBadPulses")
        self._vanPeakWidthPercent = 5. #self.getProperty("VanadiumPeakWidthPercentage")
        self._vanSmoothing = "20,2"   #self.getProperty("VanadiumSmoothParams")
        self._outDir = self.getProperty("OutputDirectory")
        self._outTypes = self.getProperty("SaveAs")
        samRuns = self.getProperty("SampleNumbers")
        filterWall = None #(self.getProperty("FilterByTimeMin"), self.getProperty("FilterByTimeMax"))
        self._amu = self.getProperty("LinearScatteringCoef")
        self._smu = self.getProperty("LinearAbsorptionCoef")
        self._radius = self.getProperty("Radius")
        self._powlam = self.getProperty("PowerLambda")
        self._ubfile = self.getProperty("IsawUBFile")
        self._peaksfile = self.getProperty("IsawPeaksFile")
    
        # process the vanadium run
        vanRun = self.getProperty("VanadiumNumber")
        if vanRun > 0:
            temp = mtd["%s_%d" % (self._instrument, vanRun)]
            if temp is None:
                vanRun = self._loadData(vanRun, "", SUFFIX, (0., 0.))
                Integration(InputWorkspace=vanRun,OutputWorkspace='VanSumTOF',IncludePartialBins=1)
                vanI = mtd["VanSumTOF"]
                FindDetectorsOutsideLimits(vanI, "VanMask", LowThreshold=1.0e-300, HighThreshold=1.0e+300)
                maskWS = mtd["VanMask"]
                MaskDetectors(vanI, MaskedWorkspace=maskWS)
                mtd.deleteWorkspace('VanMask')
                mtd.releaseFreeMemory()
                # process the background
                bkgRun = self.getProperty("BackgroundNumber")
                if bkgRun > 0:
                    temp = mtd["%s_%d" % (self._instrument, bkgRun)]
                    if temp is None:
                                bkgRun = self._loadData(bkgRun, "", SUFFIX, (0., 0.))
                    else:
                                bkgRun = temp
                else:
                    bkgRun = None
                if bkgRun is not None:
                    vanRun -= bkgRun
                    mtd.deleteWorkspace(str(bkgRun))
                    mtd.releaseFreeMemory()
                    CompressEvents(InputWorkspace=vanRun, OutputWorkspace=vanRun, Tolerance=COMPRESS_TOL_TOF) # 10ns
                vanRun = self._focus(vanRun, "bank17,bank18,bank26,bank27,bank36,bank37,bank38,bank39,bank46,bank47,bank48,bank49,bank57,bank58")
                ConvertToMatrixWorkspace(InputWorkspace=vanRun, OutputWorkspace=vanRun)
                ConvertUnits(InputWorkspace=vanRun, OutputWorkspace=vanRun, Target="dSpacing")
                StripVanadiumPeaks(InputWorkspace=vanRun, OutputWorkspace=vanRun, PeakWidthPercent=self._vanPeakWidthPercent)
                ConvertUnits(InputWorkspace=vanRun, OutputWorkspace=vanRun, Target="TOF")
                FFTSmooth(InputWorkspace=vanRun, OutputWorkspace=vanRun, Filter="Butterworth",
                  Params=self._vanSmoothing,IgnoreXBins=True,AllSpectra=True)
                SetUncertaintiesToZero(InputWorkspace=vanRun, OutputWorkspace=vanRun)
            else:
                vanRun = temp
        else:
            vanRun = None
    
        self._append = False
        Banks = ["bank17","bank18","bank26","bank27","bank36","bank37","bank38","bank39","bank46","bank47","bank48","bank49","bank57","bank58"]

        for samRun in samRuns:
            samRunnum = samRun
            for bank in Banks:
                # first round of processing the sample
                samRun = self._loadData(samRunnum, bank, SUFFIX, filterWall)

                # process the background
                bkgRun = self.getProperty("BackgroundNumber")
                if bkgRun > 0:
                    temp = mtd["%s_%d" % (self._instrument, bkgRun)]
                    if temp is None:
                                bkgRun = self._loadData(bkgRun, bank, SUFFIX, (0., 0.))
                    else:
                                bkgRun = temp
                else:
                    bkgRun = None
    
                if bkgRun is not None:
                    samRun -= bkgRun
                    mtd.deleteWorkspace(str(bkgRun))
                    mtd.releaseFreeMemory()
                    CompressEvents(InputWorkspace=samRun, OutputWorkspace=samRun, Tolerance=COMPRESS_TOL_TOF) # 10ns
    
                # the final bit of math
                if vanRun is not None:
                    samRun /= vanI
                    Divide(LHSWorkspace=samRun, RHSWorkspace=vanRun, OutputWorkspace=samRun, AllowDifferentNumberSpectra=1)
                    normalized = True
                else:
                    normalized = False

                #Remove data at edges of rectangular detectors
                SmoothNeighbours(InputWorkspace=samRun, OutputWorkspace=samRun, 
                    AdjX=0, AdjY=0, ZeroEdgePixels=20)
                #Anvred corrections need units of wavelength
                if self._radius > 0:
                    SortEvents(InputWorkspace=samRun, SortBy="X Value")
                    ConvertUnits(InputWorkspace=samRun,OutputWorkspace=samRun,Target='Wavelength')
                    AnvredCorrection(InputWorkspace=samRun,OutputWorkspace=samRun,PreserveEvents=1,
                        LinearScatteringCoef=self._amu,LinearAbsorptionCoef=self._smu,Radius=self._radius,PowerLambda=self._powlam)
                    ConvertUnits(InputWorkspace=samRun,OutputWorkspace=samRun,Target='TOF')
                    SortEvents(InputWorkspace=samRun, SortBy="X Value")

                if bank is "bank17":
                    samRunstr = str(samRun)
                    RenameWorkspace(InputWorkspace=samRun,OutputWorkspace=samRunstr+"_total")
                    samRunT = mtd[samRunstr+"_total"]
                else:
                    samRunT += samRun
                    mtd.deleteWorkspace(str(samRun))
                    mtd.releaseFreeMemory()

            # write out the files
            RenameWorkspace(InputWorkspace=samRunT,OutputWorkspace=samRunstr)
            samRun = mtd[samRunstr]
            # scale data so fitting routines do not run out of memory
            samRun /= 500
            samRun = self._bin(samRun)
            ConvertToMatrixWorkspace(InputWorkspace=samRun, OutputWorkspace=samRun)
            mtd.releaseFreeMemory()
            self._save(samRun, normalized)
            #Append next run to hkl file
            self._append = True
            if self._outTypes is not '':
                mtd.deleteWorkspace(str(samRun))
            mtd.releaseFreeMemory()

mtd.registerPyAlgorithm(SNSSingleCrystalReduction())
