"""*WIKI* 

The purpose of this algorithm is to subtract collected background data from the sample data pixel-by-pixel and divide by the Vanadium sample data.  The Vanadium data is focussed on each detector, peaks removed, and smoothed.  Time of flight limits, and binning can be changed as well as bad pulses removed.  Must have both Event NeXus and NeXus file for RunNumber in OutputDirectory or available using output from FindNeXus.  NeXus file is used for saving the output in NeXus format.

*WIKI*"""

from MantidFramework import *
from mantidsimple import *
import os

COMPRESS_TOL_TOF = .01

class SNSSingleCrystalReduction(PythonAlgorithm):

    def category(self):
        return "Crystal;Diffraction;PythonAlgorithms"

    def name(self):
        return "SNSSingleCrystalReduction"

    def PyInit(self):
        instruments = ["TOPAZ"]
        self.declareProperty("Instrument", "TOPAZ",
                             Validator=ListValidator(instruments))
        self.declareListProperty("SampleNumbers", [0], Validator=ArrayBoundedValidator(Lower=0))
        self.declareProperty("BackgroundNumber", 0, Validator=BoundedValidator(Lower=0),
                             Description="If specified overrides value in CharacterizationRunsFile")
        self.declareProperty("VanadiumNumber", 0, Validator=BoundedValidator(Lower=0),
                             Description="If specified overrides value in CharacterizationRunsFile")
        self.declareListProperty("Binning", [0.,0.,0.],
                             Description="Positive is linear bins, negative is logorithmic")
        self.declareProperty("SubtractBackgroundFromSample", False, Description="Subtract background from sample.  Background always subtracted from vanadium if specified.")
        self.declareProperty("DivideSamplebyIntegratedVanadium", False, Description="Divide sample by integrated vanadium.")
        self.declareProperty("LinearScatteringCoef", 0.0, Description="Linear Scattering Coefficient for Anvred correction of sample.")
        self.declareProperty("LinearAbsorptionCoef", 0.0, Description="Linear Absorption Coefficient for Anvred correction of sample.")
        self.declareProperty("Radius", 0.0, Description="Radius of sphere for Anvred correction of sample. Set to 0 for no Anvred corrections")
        self.declareProperty("PowerLambda", 4.0, Description="Power of wavelength for Anvred correction of sample.")
        self.declareProperty("VanadiumRadius", 0.0, Description="Radius of sphere for Anvred correction of vanadium. Set to 0 for no Anvred corrections")
        self.declareProperty("MinimumdSpacing", 0.5, Description="Minimum d-spacing.  Default is 0.5")
        self.declareProperty("MinimumWavelength", 0.6, Description="Minimum Wavelength.  Default is 0.6")
        self.declareProperty("MaximumWavelength", 3.5, Description="Maximum Wavelength.  Default is 3.5")
        self.declareProperty("ScaleFactor", 0.01, Description="Multiply FSQ and sig(FSQ) by ScaleFactor.  Default is 0.01")
        self.declareProperty("EdgePixels", 24, Description="Number of edge pixels to ignore.  Default is 24")
        self.declareListProperty("LatticeParameters", [4.7582,4.7582,12.9972,90.0,90.0,120.0],
                             Description="a,b,c,alpha,beta,gamma (Default is Sapphire Lattice Parameters)")
        self.declareFileProperty("IsawDetCalFile", "", FileAction.OptionalLoad, ['.DetCal'], Description="Isaw style file of location of detectors.")
        outfiletypes = ['', 'hkl']
        self.declareProperty("SaveAs", "hkl", ListValidator(outfiletypes))
        self.declareFileProperty("OutputFile", "", FileAction.OptionalLoad, outfiletypes,  Description="Name of output file to write/append.")
        self.declareProperty("AppendHKLFile", False, Description="Append existing hkl file")

    def _loadNeXusData(self, filename, name, bank, extension, **kwargs):
        alg = LoadEventNexus(Filename=filename, OutputWorkspace=name, BankName=bank, SingleBankPixelsOnly=1, FilterByTofMin=self._binning[0], FilterByTofMax=self._binning[2], LoadMonitors=True, MonitorsAsEvents=True, **kwargs)
        wksp = alg['OutputWorkspace']
        if self._DetCalfile is not None:
            LoadIsawDetCal(InputWorkspace=wksp,Filename=self._DetCalfile)
        #Normalise by sum of counts in upstream monitor
        NormaliseToMonitor(InputWorkspace=wksp,OutputWorkspace=wksp,MonitorWorkspace=mtd[str(name)+'_monitors'],IntegrationRangeMin=self._binning[0],IntegrationRangeMax=self._binning[2])
        wksp *= 1e8
        mtd.deleteWorkspace(str(name)+'_monitors')
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
        SortEvents(InputWorkspace=wksp, SortBy="X Value")
        ConvertUnits(InputWorkspace=wksp, OutputWorkspace=wksp, Target="TOF")
        CompressEvents(InputWorkspace=wksp, OutputWorkspace=wksp, Tolerance=COMPRESS_TOL_TOF) # 100ns
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
            ConvertToDiffractionMDWorkspace(InputWorkspace=wksp,OutputWorkspace='MD2',LorentzCorrection=0,
                    SplitInto=2,SplitThreshold=10)
            wkspMD = mtd['MD2']
            # Find the UB matrix using the peaks and known lattice parameters
            FindPeaksMD(InputWorkspace=wkspMD,MaxPeaks=10,OutputWorkspace='Peaks',AppendPeaks=True)
            peaksWS = mtd['Peaks']
            mtd.releaseFreeMemory()
            CentroidPeaks(InputWorkspace=wksp,InPeaksWorkspace=peaksWS,EdgePixels=self._edge,OutPeaksWorkspace=peaksWS)
            IntegratePeaksMD(InputWorkspace=wkspMD,PeakRadius='0.12',ReplaceIntensity=False,
                    BackgroundOuterRadius='0.18',BackgroundInnerRadius='0.15',
                    PeaksWorkspace=peaksWS,OutputWorkspace=peaksWS)
            mtd.deleteWorkspace('MD2')

    def PyExec(self):
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
        self._filterBadPulses = False
        self._SubtractBkg = self.getProperty("SubtractBackgroundFromSample")
        self._DivideVan = self.getProperty("DivideSamplebyIntegratedVanadium")
        self._vanPeakWidthPercent = 5. #self.getProperty("VanadiumPeakWidthPercentage")
        self._vanSmoothing = 11
        self._outFile = self.getProperty("OutputFile")
        self._outTypes = self.getProperty("SaveAs")
        samRuns = self.getProperty("SampleNumbers")
        filterWall = None #(self.getProperty("FilterByTimeMin"), self.getProperty("FilterByTimeMax"))
        self._amu = self.getProperty("LinearScatteringCoef")
        self._smu = self.getProperty("LinearAbsorptionCoef")
        self._radius = self.getProperty("Radius")
        self._scale = self.getProperty("ScaleFactor")
        self._minD = self.getProperty("MinimumdSpacing")
        self._minWL = self.getProperty("MinimumWavelength")
        self._maxWL = self.getProperty("MaximumWavelength")
        self._vanradius = self.getProperty("VanadiumRadius")
        self._powlam = self.getProperty("PowerLambda")
        self._edge = self.getProperty("EdgePixels")
        self._lattice = self.getProperty("LatticeParameters")
        self._DetCalfile = self.getProperty("IsawDetCalFile")
        self._append = self.getProperty("AppendHKLFile")
    
        # process the vanadium run
        vanRun = self.getProperty("VanadiumNumber")
        if vanRun > 0:
            temp = mtd["%s_%d" % (self._instrument, vanRun)]
            if temp is None:
                vanRun = self._loadData(vanRun, "", SUFFIX, (0., 0.))
                if self._DivideVan:
                    Integration(InputWorkspace=vanRun,OutputWorkspace='VanSumTOF',IncludePartialBins=1)
                    vanI = mtd["VanSumTOF"]
                    yavg = 0
                    for s in range(0,vanI.getNumberHistograms()):
                        y_s = vanI.readY(s)
                        yavg += y_s[0]
                    yavg /= vanI.getNumberHistograms()
                    print "Average pixel=",yavg
                    vanI /= yavg
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
                    Minus(LHSWorkspace=vanRun, RHSWorkspace=bkgRun, OutputWorkspace=vanRun, ClearRHSWorkspace=True)
                    mtd.deleteWorkspace(str(bkgRun))
                    mtd.releaseFreeMemory()
                    CompressEvents(InputWorkspace=vanRun, OutputWorkspace=vanRun, Tolerance=COMPRESS_TOL_TOF) # 10ns
                
                #Anvred corrections converts from TOF to Wavelength now.
                if self._vanradius > 0:
                    AnvredCorrection(InputWorkspace=vanRun,OutputWorkspace=vanRun,PreserveEvents=1,
                        LinearScatteringCoef=0.367,LinearAbsorptionCoef=0.366,Radius=self._vanradius,OnlySphericalAbsorption=True)

                vanRun = self._focus(vanRun, "bank17,bank18,bank26,bank27,bank36,bank37,bank38,bank39,bank46,bank47,bank48,bank49,bank57,bank58")
                ConvertToMatrixWorkspace(InputWorkspace=vanRun, OutputWorkspace=vanRun)
                ConvertToDistribution(Workspace=vanRun)
                SmoothData(InputWorkspace=vanRun, OutputWorkspace=vanRun, NPoints=self._vanSmoothing)
                NormaliseVanadium(InputWorkspace=vanRun, OutputWorkspace=vanRun, Wavelength=1.0)
            else:
                vanRun = temp
        else:
            vanRun = None
    
        savemat = None
        Banks = ["bank17","bank18","bank26","bank27","bank36","bank37","bank38","bank39","bank46","bank47","bank48","bank49","bank57","bank58"]

        for samRun in samRuns:
            samRunnum = samRun
            for bank in Banks:
                # first round of processing the sample
                samRun = self._loadData(samRunnum, bank, SUFFIX, filterWall)

                # process the background
                bkgRun = self.getProperty("BackgroundNumber")
                if bkgRun > 0 and self._SubtractBkg:
                    temp = mtd["%s_%d" % (self._instrument, bkgRun)]
                    if temp is None:
                                bkgRun = self._loadData(bkgRun, bank, SUFFIX, (0., 0.))
                    else:
                                bkgRun = temp
                else:
                    bkgRun = None
    
                if bkgRun is not None:
                    Minus(LHSWorkspace=samRun, RHSWorkspace=bkgRun, OutputWorkspace=samRun, ClearRHSWorkspace=True)
                    mtd.deleteWorkspace(str(bkgRun))
                    mtd.releaseFreeMemory()
                    CompressEvents(InputWorkspace=samRun, OutputWorkspace=samRun, Tolerance=COMPRESS_TOL_TOF) # 10ns
    
                # the final bit of math
                if vanRun is not None:
                    if self._DivideVan:
                        samRun /= vanI
                    Divide(LHSWorkspace=samRun, RHSWorkspace=vanRun, OutputWorkspace=samRun, AllowDifferentNumberSpectra=1)
                    normalized = True
                else:
                    normalized = False

                #Anvred corrections converts from TOF to Wavelength now.
                if self._radius > 0:
                    AnvredCorrection(InputWorkspace=samRun,OutputWorkspace=samRun,PreserveEvents=1,
                        LinearScatteringCoef=self._amu,LinearAbsorptionCoef=self._smu,Radius=self._radius,PowerLambda=self._powlam)

                # write out the files
                # scale data so fitting routines do not run out of memory
                samRun = self._bin(samRun)
                mtd.releaseFreeMemory()
                self._save(samRun, normalized)
                if self._outTypes is not '':
                    mtd.deleteWorkspace(str(samRun))
                mtd.releaseFreeMemory()
            peaksWS = mtd['Peaks']
            try:
                if savemat is not None:
                    LoadIsawUB(InputWorkspace=peaksWS, Filename=savemat)
                else:
                    FindUBUsingLatticeParameters(PeaksWorkspace=peaksWS,a=self._lattice[0],b=self._lattice[1],c=self._lattice[2],
                                alpha=self._lattice[3],beta=self._lattice[4],gamma=self._lattice[5], NumInitial=4, Tolerance=0.01)
                # Add index to HKL             
                IndexPeaks(PeaksWorkspace=peaksWS, Tolerance=0.01)
                CalculateUMatrix(PeaksWorkspace=peaksWS,a=self._lattice[0],b=self._lattice[1],c=self._lattice[2],
                    alpha=self._lattice[3],beta=self._lattice[4],gamma=self._lattice[5])
            except:
                if savemat is not None:
                    LoadIsawUB(InputWorkspace=peaksWS, Filename=savemat)
                else:
                    FindUBUsingLatticeParameters(PeaksWorkspace=peaksWS,a=self._lattice[0],b=self._lattice[1],c=self._lattice[2],
                                alpha=self._lattice[3],beta=self._lattice[4],gamma=self._lattice[5], NumInitial=4, Tolerance=0.05)
                # Add index to HKL             
                IndexPeaks(PeaksWorkspace=peaksWS, Tolerance=0.05)
                SaveIsawUB(InputWorkspace=peaksWS, Filename="lsint"+str(samRunnum)+".mat")
                CalculateUMatrix(PeaksWorkspace=peaksWS,a=self._lattice[0],b=self._lattice[1],c=self._lattice[2],
                    alpha=self._lattice[3],beta=self._lattice[4],gamma=self._lattice[5])
            IndexPeaks(PeaksWorkspace=peaksWS,Tolerance=0.5)
            SaveIsawUB(InputWorkspace=peaksWS, Filename="lsintUB"+str(samRunnum)+".mat")
            if savemat is None:
		savemat = "lsintUB"+str(samRunnum)+".mat"
            SaveHKL(ScalePeaks=self._scale,
                MinDSpacing=self._minD,MinWavelength=self._minWL,MaxWavelength=self._maxWL,
                Filename=self._outFile, AppendFile=self._append,InputWorkspace=peaksWS)
            self._append = True
            mtd.deleteWorkspace(str(peaksWS))

mtd.registerPyAlgorithm(SNSSingleCrystalReduction())
