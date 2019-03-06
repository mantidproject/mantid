# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#pylint: disable=no-init,invalid-name
from __future__ import (absolute_import, division, print_function)

from mantid.api import *
from mantid.kernel import *
from mantid.simpleapi import *
import os
from time import strftime
from mantid.kernel import Direction

COMPRESS_TOL_TOF = .01
EXTENSIONS_NXS = ["_event.nxs", ".nxs.h5"]


def getBasename(filename):
    name = os.path.split(filename)[-1]
    for extension in EXTENSIONS_NXS:
        name = name.replace(extension, '')
    return name

#pylint: disable=too-many-instance-attributes


class CalibrateRectangularDetectors(PythonAlgorithm):

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
        return "Diffraction\\Calibration"

    def seeAlso(self):
        return [ "GetDetectorOffsets" ]

    def name(self):
        return "CalibrateRectangularDetectors"

    def summary(self):
        return "Calibrate the detector pixels and write a calibration file"

    def PyInit(self):
        self.declareProperty(MultipleFileProperty(name="RunNumber",
                                                  extensions=EXTENSIONS_NXS),
                             "Event file")
        validator = IntArrayBoundedValidator(lower=0)
        self.declareProperty(IntArrayProperty("Background", values=[0], direction=Direction.Input,
                                              validator=validator))
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
        validator = FloatArrayBoundedValidator(lower=0.)
        self.declareProperty(FloatArrayProperty("PeakPositions", []),
                             "Comma delimited d-space positions of reference peaks.  Use 1-3 for Cross Correlation.  "+
                             "Unlimited for many peaks option.")
        self.declareProperty("PeakWindowMax", 0.,
                             "Maximum window around a peak to search for it. Optional.")
        self.declareProperty(ITableWorkspaceProperty("FitwindowTableWorkspace", "", Direction.Input, PropertyMode.Optional),
                             "Name of input table workspace containing the fit window information for each spectrum. ")
        self.declareProperty("MinimumPeakHeight", 2., "Minimum value allowed for peak height")
        self.declareProperty("MinimumPeakHeightObs", 0.,
                             "Minimum value of a peak's maximum observed Y value for this peak to be used to calculate offset.")

        self.declareProperty(MatrixWorkspaceProperty("DetectorResolutionWorkspace", "", Direction.Input, PropertyMode.Optional),
                             "Name of optional input matrix workspace for each detector's resolution (D(d)/d).")
        self.declareProperty(FloatArrayProperty("AllowedResRange", [0.25, 4.0], direction=Direction.Input),
                             "Range of allowed individual peak's resolution factor to input detector's resolution.")

        self.declareProperty("PeakFunction", "Gaussian", StringListValidator(["BackToBackExponential", "Gaussian", "Lorentzian"]),
                             "Type of peak to fit. Used only with CrossCorrelation=False")
        self.declareProperty("BackgroundType", "Flat", StringListValidator(['Flat', 'Linear', 'Quadratic']),
                             "Used only with CrossCorrelation=False")
        self.declareProperty(IntArrayProperty("DetectorsPeaks", []),
                             "Comma delimited numbers of detector banks for each peak if using 2-3 peaks for Cross Correlation. "+
                             "Default is all.")
        self.declareProperty("PeakHalfWidth", 0.05,
                             "Half width of d-space around peaks for Cross Correlation. Default is 0.05")
        self.declareProperty("CrossCorrelationPoints", 100,
                             "Number of points to find peak from Cross Correlation.  Default is 100")
        self.declareProperty(FloatArrayProperty("Binning", [0.,0.,0.]),
                             "Min, Step, and Max of d-space bins.  Logarithmic binning is used if Step is negative.")
        self.declareProperty("DiffractionFocusWorkspace", False, "Diffraction focus by detectors.  Default is False")
        grouping = ["All", "Group", "Column", "bank"]
        self.declareProperty("GroupDetectorsBy", "All", StringListValidator(grouping),
                             "Detector groups to use for future focussing: All detectors as one group, "+
                             "Groups (East,West for SNAP), Columns for SNAP, detector banks")
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
        """
        Validate inputs
        :return:
        """
        messages = {}

        detectors = self.getProperty("DetectorsPeaks").value
        if self.getProperty("CrossCorrelation").value:
            positions = self.getProperty("PeakPositions").value
            if len(detectors) <= 1:
                if len(positions) != 1:
                    messages["PeakPositions"] = "Can only have one cross correlation peak without " \
                                                "specifying 'DetectorsPeaks'"
            else:
                if len(detectors) != len(positions):
                    messages["PeakPositions"] = "Must be the same length as 'DetectorsPeaks' (%d != %d)" \
                        % (len(positions), len(detectors))
                    messages["DetectorsPeaks"] = "Must be the same length as 'PeakPositions' or empty"
                elif len(detectors) > 3:
                    messages["DetectorsPeaks"] = "Up to 3 peaks are supported"
        elif bool(detectors):
            messages["DetectorsPeaks"] = "Only allowed for CrossCorrelation=True"

        return messages

    def _loadData(self, filename, filterWall=None):
        if  filename is None or len(filename) <= 0:
            return None

        kwargs = {"Precount":False}
        if filterWall is not None:
            if filterWall[0] > 0.:
                kwargs["FilterByTimeStart"] = filterWall[0]
            if filterWall[1] > 0.:
                kwargs["FilterByTimeStop"] = filterWall[1]

        wkspName = getBasename(filename)

        LoadEventNexus(Filename=filename, OutputWorkspace=wkspName, **kwargs)

        FilterBadPulses(InputWorkspace=wkspName, OutputWorkspace=wkspName)

        CompressEvents(InputWorkspace=wkspName, OutputWorkspace=wkspName,
                       Tolerance=COMPRESS_TOL_TOF) # 100ns

        return wkspName

    def _saveCalibration(self, wkspName, calibFilePrefix):
        outfilename = None
        if "dspacemap" in self._outTypes:
            outfilename = calibFilePrefix.replace('_d', '_dspacemap_d') + '.dat'
            if os.path.exists(outfilename):
                os.unlink(outfilename)
            #write Dspacemap file
            SaveDspacemap(InputWorkspace=wkspName+"offset",
                          DspacemapFile=outfilename)
        if "calibration" in self._outTypes:
            # for the sake of legacy
            SaveCalFile(OffsetsWorkspace=wkspName+"offset",
                        GroupingWorkspace=wkspName+"group",
                        MaskWorkspace=wkspName+"mask",Filename=calibFilePrefix + '.cal')
            # the real version
            outfilename = calibFilePrefix + '.h5'
            if os.path.exists(outfilename):
                os.unlink(outfilename)
            ConvertDiffCal(OffsetsWorkspace=wkspName+"offset",
                           OutputWorkspace=wkspName+"cal")
            SaveDiffCal(CalibrationWorkspace=wkspName+"cal",
                        GroupingWorkspace=wkspName+"group",
                        MaskWorkspace=wkspName+"mask",
                        Filename=outfilename)

        if outfilename is not None:
            self.setProperty("OutputFilename", outfilename)

    def _createGrouping(self, wkspName):
        (_, numGroupedSpectra, numGroups) = CreateGroupingWorkspace(InputWorkspace=wkspName,
                                                                    GroupDetectorsBy=self._grouping,
                                                                    OutputWorkspace=wkspName+"group")

        if (numGroupedSpectra==0) or (numGroups==0):
            raise RuntimeError("%d spectra will be in %d groups" % (numGroupedSpectra, numGroups))

    #pylint: disable=too-many-branches
    def _cccalibrate(self, wksp):
        if wksp is None:
            return None

        # Bin events in d-Spacing
        Rebin(InputWorkspace=wksp, OutputWorkspace=wksp,
              Params=str(self._peakmin)+","+str(abs(self._binning[1]))+","+str(self._peakmax))

        #Find good peak for reference
        ymax = 0
        midBin = int(mtd[wksp].blocksize()/2)
        for s in range(0,mtd[wksp].getNumberHistograms()):
            y_s = mtd[wksp].readY(s)
            if y_s[midBin] > ymax:
                refpixel = s
                ymax = y_s[midBin]
        self.log().information("Reference spectra=%s" % refpixel)

        # Cross correlate spectra using interval around peak at peakpos (d-Spacing)
        if self._lastpixel == 0:
            self._lastpixel = mtd[wksp].getNumberHistograms()-1
        else:
            self._lastpixel = int(mtd[wksp].getNumberHistograms()*self._lastpixel/self._lastpixel3) - 1
        self.log().information("Last pixel=%s" % self._lastpixel)
        CrossCorrelate(InputWorkspace=wksp, OutputWorkspace=wksp+"cc",
                       ReferenceSpectra=refpixel, WorkspaceIndexMin=0,
                       WorkspaceIndexMax=self._lastpixel,
                       XMin=self._peakmin, XMax=self._peakmax)
        # Get offsets for pixels using interval around cross correlations center and peak at peakpos (d-Spacing)
        GetDetectorOffsets(InputWorkspace=wksp+"cc", OutputWorkspace=wksp+"offset",
                           Step=abs(self._binning[1]), DReference=self._peakpos1,
                           XMin=-self._ccnumber, XMax=self._ccnumber,
                           MaxOffset=self._maxoffset, MaskWorkspace=wksp+"mask")
        if AnalysisDataService.doesExist(wksp+"cc"):
            AnalysisDataService.remove(wksp+"cc")
        if self._peakpos2 > 0.0:
            Rebin(InputWorkspace=wksp, OutputWorkspace=wksp,
                  Params=str(self._peakmin2)+","+str(abs(self._binning[1]))+","+str(self._peakmax2))
            #Find good peak for reference
            ymax = 0
            midBin = int(mtd[wksp].blocksize()/2)
            for s in range(0,mtd[wksp].getNumberHistograms()):
                y_s = mtd[wksp].readY(s)
                if y_s[midBin] > ymax:
                    refpixel = s
                    ymax = y_s[midBin]
            msg = "Reference spectra = %s, lastpixel_3 = %s" % (refpixel, self._lastpixel3)
            self.log().information(msg)
            self._lastpixel2 = int(mtd[wksp].getNumberHistograms()*self._lastpixel2/self._lastpixel3) - 1
            CrossCorrelate(InputWorkspace=wksp, OutputWorkspace=wksp+"cc2",
                           ReferenceSpectra=refpixel, WorkspaceIndexMin=self._lastpixel+1,
                           WorkspaceIndexMax=self._lastpixel2,
                           XMin=self._peakmin2, XMax=self._peakmax2)
            # Get offsets for pixels using interval around cross correlations center and peak at peakpos (d-Spacing)
            GetDetectorOffsets(InputWorkspace=wksp+"cc2", OutputWorkspace=wksp+"offset2",
                               Step=abs(self._binning[1]), DReference=self._peakpos2,
                               XMin=-self._ccnumber, XMax=self._ccnumber,
                               MaxOffset=self._maxoffset, MaskWorkspace=wksp+"mask2")
            Plus(LHSWorkspace=wksp+"offset", RHSWorkspace=wksp+"offset2",
                 OutputWorkspace=wksp+"offset")
            Plus(LHSWorkspace=wksp+"mask", RHSWorkspace=wksp+"mask2",
                 OutputWorkspace=wksp+"mask")
            for ws in [wksp+"cc2", wksp+"offset2", wksp+"mask2"]:
                if AnalysisDataService.doesExist(ws):
                    AnalysisDataService.remove(ws)

        if self._peakpos3 > 0.0:
            Rebin(InputWorkspace=wksp, OutputWorkspace=wksp,
                  Params=str(self._peakmin3)+","+str(abs(self._binning[1]))+","+str(self._peakmax3))
            #Find good peak for reference
            ymax = 0
            midBin = mtd[wksp].blocksize()/2
            for s in range(0,mtd[wksp].getNumberHistograms()):
                y_s = mtd[wksp].readY(s)
                if y_s[midBin] > ymax:
                    refpixel = s
                    ymax = y_s[midBin]
            self.log().information("Reference spectra=%s" % refpixel)
            CrossCorrelate(InputWorkspace=wksp, OutputWorkspace=wksp+"cc3",
                           ReferenceSpectra=refpixel,
                           WorkspaceIndexMin=self._lastpixel2+1,
                           WorkspaceIndexMax=mtd[wksp].getNumberHistograms()-1,
                           XMin=self._peakmin3, XMax=self._peakmax3)
            # Get offsets for pixels using interval around cross correlations center and peak at peakpos (d-Spacing)
            GetDetectorOffsets(InputWorkspace=wksp+"cc3", OutputWorkspace=wksp+"offset3",
                               Step=abs(self._binning[1]), DReference=self._peakpos3,
                               XMin=-self._ccnumber, XMax=self._ccnumber,
                               MaxOffset=self._maxoffset, MaskWorkspace=wksp+"mask3")
            Plus(LHSWorkspace=wksp+"offset", RHSWorkspace=wksp+"offset3",
                 OutputWorkspace=str(wksp)+"offset")
            Plus(LHSWorkspace=wksp+"mask", RHSWorkspace=wksp+"mask3",
                 OutputWorkspace=wksp+"mask")
            for ws in [wksp+"cc3", wksp+"offset3", wksp+"mask3"]:
                if AnalysisDataService.doesExist(ws):
                    AnalysisDataService.remove(ws)

        return str(wksp)

    #pylint: disable=too-many-branches
    def _multicalibrate(self, wksp):
        if wksp is None:
            return None

        # Bin events in d-Spacing
        Rebin(InputWorkspace=wksp, OutputWorkspace=wksp,
              Params=str(self._binning[0])+","+str((self._binning[1]))+","+str(self._binning[2]))

        if len(self._smoothGroups) > 0:
            SmoothData(InputWorkspace=wksp, OutputWorkspace=wksp,
                       NPoints=self._smoothGroups, GroupingWorkspace=wksp+"group")

        # Get the fit window input workspace
        fitwinws = self.getProperty("FitwindowTableWorkspace").value

        # Set up resolution workspace
        resws = self.getProperty("DetectorResolutionWorkspace").value
        if resws is not None:
            resrange = self.getProperty("AllowedResRange").value
            if len(resrange) < 2:
                raise NotImplementedError("With input of 'DetectorResolutionWorkspace', "+
                                          "number of allowed resolution range must be equal to 2.")
            reslowf = resrange[0]
            resupf = resrange[1]
            if reslowf >= resupf:
                raise NotImplementedError("Allowed resolution range factor, lower boundary "+
                                          "(%f) must be smaller than upper boundary (%f)."
                                          % (reslowf, resupf))
        else:
            reslowf = 0.0
            resupf = 0.0

        # Get offsets for pixels using interval around cross correlations center and peak at peakpos (d-Spacing)
        GetDetOffsetsMultiPeaks(InputWorkspace=wksp, OutputWorkspace=wksp+"offset",
                                DReference=self._peakpos,
                                FitWindowMaxWidth=self.getProperty("PeakWindowMax").value,
                                MinimumPeakHeight=self.getProperty("MinimumPeakHeight").value,
                                MinimumPeakHeightObs=self.getProperty("MinimumPeakHeightObs").value,
                                BackgroundType=self.getProperty("BackgroundType").value,
                                MaxOffset=self._maxoffset, NumberPeaksWorkspace=wksp+"peaks",
                                MaskWorkspace=wksp+"mask",
                                FitwindowTableWorkspace = fitwinws,
                                InputResolutionWorkspace=resws,
                                MinimumResolutionFactor = reslowf,
                                MaximumResolutionFactor = resupf)

        #Fixed SmoothNeighbours for non-rectangular and rectangular
        if self._smoothoffsets and self._xpixelbin*self._ypixelbin>1: # Smooth data if it was summed
            SmoothNeighbours(InputWorkspace=wksp+"offset", OutputWorkspace=wksp+"offset",
                             WeightedSum="Flat",
                             AdjX=self._xpixelbin, AdjY=self._ypixelbin)
        Rebin(InputWorkspace=wksp, OutputWorkspace=wksp,
              Params=str(self._binning[0])+","+str((self._binning[1]))+","+str(self._binning[2]))

        return str(wksp)

    def _focus(self, wksp):
        if wksp is None:
            return None
        MaskDetectors(Workspace=wksp, MaskedWorkspace=str(wksp)+"mask")
        wksp = AlignDetectors(InputWorkspace=wksp, OutputWorkspace=wksp,
                              CalibrationWorkspace=str(wksp)+"cal")
        # Diffraction focusing using new calibration file with offsets
        if self._diffractionfocus:
            wksp = DiffractionFocussing(InputWorkspace=wksp, OutputWorkspace=wksp,
                                        GroupingWorkspace=str(wksp)+"group")

        wksp = Rebin(InputWorkspace=wksp, OutputWorkspace=wksp, Params=self._binning)
        return wksp

    def _initCCpars(self):
        self._peakpos1 = self._peakpos[0]
        self._peakpos2 = 0
        self._peakpos3 = 0
        self._lastpixel = 0
        self._lastpixel2 = 0
        self._lastpixel3 = 0
        peakhalfwidth = self.getProperty("PeakHalfWidth").value
        self._peakmin = self._peakpos1-peakhalfwidth
        self._peakmax = self._peakpos1+peakhalfwidth
        if len(self._peakpos) >= 2:
            self._peakpos2 = self._peakpos[1]
            self._peakmin2 = self._peakpos2-peakhalfwidth
            self._peakmax2 = self._peakpos2+peakhalfwidth
        if len(self._peakpos) >= 3:
            self._peakpos3 = self._peakpos[2]
            self._peakmin3 = self._peakpos3-peakhalfwidth
            self._peakmax3 = self._peakpos3+peakhalfwidth
        detectors = self.getProperty("DetectorsPeaks").value
        if len(detectors) == 0:
            detectors = [0]
        if detectors[0]:
            self._lastpixel = int(detectors[0])
            self._lastpixel3 = self._lastpixel
        if len(detectors) >= 2:
            self._lastpixel2 = self._lastpixel+int(detectors[1])
            self._lastpixel3 = self._lastpixel2
        if len(detectors) >= 3:
            self._lastpixel3 = self._lastpixel2+int(detectors[2])
        self._ccnumber = self.getProperty("CrossCorrelationPoints").value

    #pylint: disable=too-many-branches
    def PyExec(self):
        # get generic information
        self._binning = self.getProperty("Binning").value
        if len(self._binning) != 1 and len(self._binning) != 3:
            raise RuntimeError("Can only specify (width) or (start,width,stop) for binning. Found %d values." % len(self._binning))
        if len(self._binning) == 3:
            if self._binning[0] == 0. and self._binning[1] == 0. and self._binning[2] == 0.:
                raise RuntimeError("Failed to specify the binning")
        self._grouping = self.getProperty("GroupDetectorsBy").value
        self._xpixelbin = self.getProperty("XPixelSum").value
        self._ypixelbin = self.getProperty("YPixelSum").value
        self._smoothoffsets = self.getProperty("SmoothSummedOffsets").value
        self._smoothGroups = self.getProperty("SmoothGroups").value
        self._peakpos = self.getProperty("PeakPositions").value
        if self.getProperty("CrossCorrelation").value:
            self._initCCpars()
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
        filterWall = (self.getProperty("FilterByTimeMin").value, self.getProperty("FilterByTimeMax").value)

        stuff = getBasename(samRuns[0])
        stuff = stuff.split('_')
        (instrument, runNumber) = ('_'.join(stuff[:-1]), stuff[-1])

        calib = instrument+"_calibrate_d"+runNumber+strftime("_%Y_%m_%d")
        calib = os.path.join(self._outDir, calib)

        for (samNum, backNum) in zip(samRuns, backRuns):
            # first round of processing the sample
            samRun = self._loadData(samNum, filterWall)
            samRun = str(samRun)
            if backNum > 0:
                backRun = self._loadData(instrument+'_'+str(backNum), filterWall)
                Minus(LHSWorkspace=samRun, RHSWorkspace=backRun,
                      OutputWorkspace=samRun)
                DeleteWorkspace(backRun)
                CompressEvents(samRun, OutputWorkspace=samRun,
                               Tolerance=COMPRESS_TOL_TOF) # 100ns

            self._createGrouping(samRun)

            LRef = self.getProperty("UnwrapRef").value
            DIFCref = self.getProperty("LowResRef").value
            # super special Jason stuff
            if LRef > 0:
                UnwrapSNS(InputWorkspace=samRun, OutputWorkspace=samRun, LRef=LRef)
            if DIFCref > 0:
                RemoveLowResTOF(InputWorkspace=samRun, OutputWorkspace=samRun,
                                ReferenceDIFC=DIFCref)

            ConvertUnits(InputWorkspace=samRun, OutputWorkspace=samRun, Target="dSpacing")

            # Sum pixelbin X pixelbin blocks of pixels
            if self._xpixelbin*self._ypixelbin>1:
                SumNeighbours(InputWorkspace=samRun, OutputWorkspace=samRun,
                              SumX=self._xpixelbin, SumY=self._ypixelbin)

            if self.getProperty("CrossCorrelation").value:
                samRun = self._cccalibrate(samRun)
            else:
                samRun = self._multicalibrate(samRun)

            self._saveCalibration(samRun, calib)

            if self._xpixelbin*self._ypixelbin>1 or len(self._smoothGroups) > 0:
                if AnalysisDataService.doesExist(samRun):
                    AnalysisDataService.remove(samRun)
                samRun = self._loadData(samNum, filterWall)
                LRef = self.getProperty("UnwrapRef").value
                DIFCref = self.getProperty("LowResRef").value
                # super special Jason stuff
                if LRef > 0:
                    samRun = UnwrapSNS(InputWorkspace=samRun, OutputWorkspace=samRun,
                                       LRef=LRef)
                if DIFCref > 0:
                    samRun = RemoveLowResTOF(InputWorkspace=samRun, OutputWorkspace=samRun,
                                             ReferenceDIFC=DIFCref)
            else:
                samRun = ConvertUnits(InputWorkspace=samRun, OutputWorkspace=samRun,
                                      Target="TOF")
            samRun = self._focus(samRun)
            RenameWorkspace(InputWorkspace=samRun, OutputWorkspace=str(samRun)+"_calibrated")


AlgorithmFactory.subscribe(CalibrateRectangularDetectors)
