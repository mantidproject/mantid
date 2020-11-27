# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import numpy as np
import os, math

from scipy.special import lambertw
from mantid.kernel import Direction, IntBoundedValidator, FloatBoundedValidator
from mantid.api import (AlgorithmFactory, DistributedDataProcessorAlgorithm,
                        FileProperty, FileAction, Progress)
from mantid.simpleapi import Load, FindDetectorsPar, FilterBadPulses, RemovePromptPulse, LoadDiffCal, MaskDetectors, AlignDetectors, ConvertUnits, CylinderAbsorption, Divide, Bin2DPowderDiffraction, StripVanadiumPeaks, FFTSmooth, Minus, SaveP2D, ResetNegatives2D


class PowderReduceP2D(DistributedDataProcessorAlgorithm):
    def category(self):
        return 'Diffraction\\Reduction'

    def summary(self):
        """
        summary of the algorithm
        :return:
        """
        return "The algorithm used to process the results of powder diffraction experiments and create a '.p2d' file for multidimensional Rietveld refinement."

    def name(self):
        return "PowderReduceP2D"

    def seeAlso(self):
        return ["SaveP2D"]

    def PyInit(self):
        # Input files
        self.declareProperty(FileProperty('SampleData',
                                          '',
                                          action=FileAction.Load,
                                          direction=Direction.Input),
                             doc='Datafile that should be used.')
        self.declareProperty(
            'DoIntensityCorrection',
            False,
            direction=Direction.Input,
            doc=
            'If set to True you have to declare a vanadium measurement for intensity correction.'
        )
        self.declareProperty(
            FileProperty('VanaData',
                         '',
                         action=FileAction.OptionalLoad,
                         direction=Direction.Input),
            doc='Vanadium measurement for intensity correction.')
        self.declareProperty(
            'DoBackgroundCorrection',
            False,
            direction=Direction.Input,
            doc=
            'If set to True you have to declare an empty can measurement for background correction.'
        )
        self.declareProperty(
            FileProperty('EmptyData',
                         '',
                         action=FileAction.OptionalLoad,
                         direction=Direction.Input),
            doc='Empty measurement of the can for background correction.')
        self.declareProperty(FileProperty('CalFile',
                                          '',
                                          action=FileAction.OptionalLoad,
                                          direction=Direction.Input),
                             doc='Calibration file.')
        self.declareProperty(
            'DoEdgebinning',
            False,
            direction=Direction.Input,
            doc='If set to True you have to declare a BinEdges file.')
        self.declareProperty(FileProperty('BinEdgesFile',
                                          '',
                                          action=FileAction.OptionalLoad,
                                          direction=Direction.Input),
                             doc='BinEdges file used for edgebinning.')
        grp1 = 'Input and Output Files'
        self.setPropertyGroup('SampleData', grp1)
        self.setPropertyGroup('DoIntensityCorrection', grp1)
        self.setPropertyGroup('VanaData', grp1)
        self.setPropertyGroup('DoBackgroundCorrection', grp1)
        self.setPropertyGroup('EmptyData', grp1)
        self.setPropertyGroup('CalFile', grp1)
        self.setPropertyGroup('DoEdgebinning', grp1)
        self.setPropertyGroup('BinEdgesFile', grp1)
        # OutputFile
        self.declareProperty(FileProperty('OutputFile',
                                          '',
                                          action=FileAction.Save,
                                          direction=Direction.Input),
                             doc='Output File for p2d Data.')
        self.setPropertyGroup('OutputFile', grp1)
        # Data range
        self.declareProperty(
            'TwoThetaMin',
            50,
            validator=IntBoundedValidator(lower=0),
            direction=Direction.Input,
            doc='Minimum value for 2 Theta. Everything smaller gets removed.')
        self.declareProperty(
            'TwoThetaMax',
            132,
            validator=IntBoundedValidator(lower=0),
            direction=Direction.Input,
            doc='Maximum value for 2 Theta. Everything bigger gets removed.')
        self.declareProperty(
            'WavelengthCenter',
            0.7,
            validator=FloatBoundedValidator(lower=0.0),
            direction=Direction.Input,
            doc=
            'Center Wavelength is used to calculate automatic values for lambdaMin and lambdaMax if they are not specified.'
        )
        self.declareProperty(
            'LambdaMin',
            0.2,
            validator=FloatBoundedValidator(lower=0.0),
            direction=Direction.Input,
            doc=
            'Minimum value for lambda. Everything smaller gets removed. If zero it is not used and values get calculated from center wavelength.'
        )
        self.declareProperty(
            'LambdaMax',
            2.0,
            validator=FloatBoundedValidator(lower=0.0),
            direction=Direction.Input,
            doc=
            'Maximum value for lambda. Everything bigger gets removed. If zero it is not used and values get calculated from center wavelength.'
        )
        self.declareProperty(
            'DMin',
            0.5,
            validator=FloatBoundedValidator(lower=0.0),
            direction=Direction.Input,
            doc=
            'Minimum value for d. Everything smaller gets removed. If zero it is not used and values get calculated from 2 theta and lambda.'
        )
        self.declareProperty(
            'DMax',
            6.0,
            validator=FloatBoundedValidator(lower=0.0),
            direction=Direction.Input,
            doc=
            'Maximum value for d. Everything bigger gets removed. If zero it is not used and values get calculated from 2 theta and lambda.'
        )
        self.declareProperty(
            'DpMin',
            0.2,
            validator=FloatBoundedValidator(lower=0.0),
            direction=Direction.Input,
            doc=
            'Minimum value for dp. Everything smaller gets removed. If zero it is not used and values get calculated from 2 theta and lambda.'
        )
        self.declareProperty(
            'DpMax',
            2.0,
            validator=FloatBoundedValidator(lower=0.0),
            direction=Direction.Input,
            doc=
            'Maximum value for dp. Everything bigger gets removed. If zero it is not used and values get calculated from 2 theta and lambda.'
        )
        grp2 = 'Data Ranges'
        self.setPropertyGroup('TwoThetaMin', grp2)
        self.setPropertyGroup('TwoThetaMax', grp2)
        self.setPropertyGroup('WavelengthCenter', grp2)
        self.setPropertyGroup('LambdaMin', grp2)
        self.setPropertyGroup('LambdaMax', grp2)
        self.setPropertyGroup('DMin', grp2)
        self.setPropertyGroup('DMax', grp2)
        self.setPropertyGroup('DpMin', grp2)
        self.setPropertyGroup('DpMax', grp2)
        # Input for FindDetectorsPar
        self.copyProperties('FindDetectorsPar',
                            ['ReturnLinearRanges', 'ParFile'])
        self.declareProperty(
            'OutputParTable',
            'Detec',
            direction=Direction.Input,
            doc=
            'If not empty, a name of a table workspace which will contain the calculated par or phx values for the detectors.'
        )
        grp3 = 'FindDetectorsPar'
        self.setPropertyGroup('ReturnLinearRanges', grp3)
        self.setPropertyGroup('ParFile', grp3)
        self.setPropertyGroup('OutputParTable', grp3)
        # Input for FilterBadPulses
        self.declareProperty(
            'LowerCutoff',
            99.998,
            direction=Direction.Input,
            doc='The percentage of the average to use as the lower bound.')
        grp4 = 'FilterBadPulses'
        self.setPropertyGroup('LowerCutoff', grp4)
        # Input for RemovePromptPulse
        self.declareProperty(
            'Width',
            150,
            direction=Direction.Input,
            doc=
            'The width of the time of flight (in microseconds) to remove from the data.'
        )
        self.copyProperties('RemovePromptPulse', ['Frequency'])
        grp5 = 'RemovePromptPulse'
        self.setPropertyGroup('Width', grp5)
        self.setPropertyGroup('Frequency', grp5)
        # Input for LoadDiffCal
        self.declareProperty(
            'WorkspaceName',
            'POWTEX',
            direction=Direction.Input,
            doc=
            'The base of the output workspace names. Names will have _group, _cal, _mask appended to them.'
        )
        self.copyProperties('LoadDiffCal', [
            'InstrumentName', 'InstrumentFilename', 'MakeGroupingWorkspace',
            'MakeCalWorkspace', 'MakeMaskWorkspace', 'TofMin', 'TofMax',
            'FixConversionIssues'
        ])
        grp6 = 'LoadDiffCal'
        self.setPropertyGroup('WorkspaceName', grp6)
        self.setPropertyGroup('InstrumentName', grp6)
        self.setPropertyGroup('InstrumentFilename', grp6)
        self.setPropertyGroup('MakeGroupingWorkspace', grp6)
        self.setPropertyGroup('MakeCalWorkspace', grp6)
        self.setPropertyGroup('MakeMaskWorkspace', grp6)
        self.setPropertyGroup('TofMin', grp6)
        self.setPropertyGroup('TofMax', grp6)
        self.setPropertyGroup('FixConversionIssues', grp6)
        # Input for MaskDetectors
        self.declareProperty(
            'MaskedWorkspace',
            self.getProperty('WorkspaceName').value + '_mask',
            direction=Direction.Input,
            doc=
            'If given but not as a SpecialWorkspace2D, the masking from this workspace will be copied. If given as a SpecialWorkspace2D, the masking is read from its Y values.'
        )
        self.copyProperties('MaskDetectors', [
            'SpectraList', 'DetectorList', 'WorkspaceIndexList',
            'ForceInstrumentMasking', 'StartWorkspaceIndex',
            'EndWorkspaceIndex', 'ComponentList'
        ])
        grp7 = 'MaskDetectors'
        self.setPropertyGroup('MaskedWorkspace', grp7)
        self.setPropertyGroup('SpectraList', grp7)
        self.setPropertyGroup('DetectorList', grp7)
        self.setPropertyGroup('WorkspaceIndexList', grp7)
        self.setPropertyGroup('ForceInstrumentMasking', grp7)
        self.setPropertyGroup('StartWorkspaceIndex', grp7)
        self.setPropertyGroup('EndWorkspaceIndex', grp7)
        self.setPropertyGroup('ComponentList', grp7)
        # Input for AlignDetectors
        #self.copyProperties('AlignDetectors', ['CalibrationWorkspace', 'OffsetsWorkspace'])
        #grp8 = 'AlignDetectors'
        #self.setPropertyGroup('CalibrationWorkspace', grp8)
        #self.setPropertyGroup('OffsetsWorkspace', grp8)
        # Input for CylinderAbsorption
        self.declareProperty(
            'AttenuationXSection',
            5.08,
            direction=Direction.Input,
            doc=
            'The ABSORPTION cross-section, at 1.8 Angstroms, for the sample material in barns. Column 8 of a table generated from http://www.ncnr.nist.gov/resources/n-lengths/.'
        )
        self.declareProperty(
            'ScatteringXSection',
            5.1,
            direction=Direction.Input,
            doc=
            'The (coherent + incoherent) scattering cross-section for the sample material in barns. Column 7 of a table generated from http://www.ncnr.nist.gov/resources/n-lengths/.'
        )
        self.declareProperty(
            'SampleNumberDensity',
            0.07192,
            direction=Direction.Input,
            doc=
            'The number density of the sample in number of atoms per cubic angstrom if not set with SetSampleMaterial.'
        )
        self.declareProperty(
            'CylinderSampleHeight',
            4,
            direction=Direction.Input,
            doc='The height of the cylindrical sample in centimetres.')
        self.declareProperty(
            'CylinderSampleRadius',
            0.4,
            direction=Direction.Input,
            doc='The radius of the cylindrical sample in centimetres.')
        self.declareProperty(
            'NumberOfSlices',
            10,
            direction=Direction.Input,
            doc=
            'The number of slices into which the cylinder is divided for the calculation.'
        )
        self.declareProperty(
            'NumberOfAnnuli',
            10,
            direction=Direction.Input,
            doc=
            'The number of annuli into which each slice is divided for the calculation.'
        )
        self.copyProperties('CylinderAbsorption', [
            'ScatterFrom', 'NumberOfWavelengthPoints', 'ExpMethod', 'EMode',
            'EFixed', 'CylinderAxis'
        ])
        grp9 = 'CylinderAbsorption'
        self.setPropertyGroup('ScatterFrom', grp9)
        self.setPropertyGroup('AttenuationXSection', grp9)
        self.setPropertyGroup('ScatteringXSection', grp9)
        self.setPropertyGroup('SampleNumberDensity', grp9)
        self.setPropertyGroup('CylinderSampleHeight', grp9)
        self.setPropertyGroup('CylinderSampleRadius', grp9)
        self.setPropertyGroup('NumberOfSlices', grp9)
        self.setPropertyGroup('NumberOfAnnuli', grp9)
        self.setPropertyGroup('NumberOfWavelengthPoints', grp9)
        self.setPropertyGroup('ExpMethod', grp9)
        self.setPropertyGroup('EMode', grp9)
        self.setPropertyGroup('EFixed', grp9)
        self.setPropertyGroup('CylinderAxis', grp9)
        # Input for Bin2DPowderDiffraction
        self.copyProperties('Bin2DPowderDiffraction',
                            ['dSpaceBinning', 'dPerpendicularBinning'])
        self.declareProperty(
            'NormalizeByBinArea',
            False,
            direction=Direction.Input,
            doc='Normalize the binned workspace by the bin area.')
        grp10 = 'Bin2DPowderDiffraction'
        self.setPropertyGroup('dSpaceBinning', grp10)
        self.setPropertyGroup('dPerpendicularBinning', grp10)
        self.setPropertyGroup('NormalizeByBinArea', grp10)
        # Input for StripVanadiumPeaks
        self.declareProperty(
            'FWHM',
            2,
            direction=Direction.Input,
            doc=
            'The number of points covered, on average, by the fwhm of a peak. Passed through to FindPeaks. Default 7.'
        )
        self.declareProperty(
            'Tolerance',
            2,
            direction=Direction.Input,
            doc=
            'A measure of the strictness desired in meeting the condition on peak candidates. Passed through to FindPeaks. Default 4.'
        )
        self.declareProperty(
            'PeakPositionTolerance',
            0.05,
            direction=Direction.Input,
            doc=
            'Tolerance on the found peaks positions against the input peak positions. A non-positive value turns this option off.'
        )
        self.declareProperty(
            'BackgroundType',
            'Quadratic',
            direction=Direction.Input,
            doc=
            'The type of background of the histogram. Present choices include Linear and Quadratic. Allowed values: [Linear, Quadratic]'
        )
        self.copyProperties('StripVanadiumPeaks',
                            ['HighBackground', 'WorkspaceIndex'])
        grp11 = 'StripVanadiumPeaks'
        self.setPropertyGroup('FWHM', grp11)
        self.setPropertyGroup('Tolerance', grp11)
        self.setPropertyGroup('BackgroundType', grp11)
        self.setPropertyGroup('HighBackground', grp11)
        self.setPropertyGroup('PeakPositionTolerance', grp11)
        self.setPropertyGroup('WorkspaceIndex', grp11)
        # Input for FFTSmooth
        self.declareProperty(
            'Filter',
            'Butterworth',
            direction=Direction.Input,
            doc=
            'The type of the applied filter. Allowed values: [Zeroing, Butterworth]'
        )
        self.declareProperty(
            'Params',
            '20,2',
            direction=Direction.Input,
            doc=
            'The filter parameters: For Zeroing, 1 parameter: n - an integer greater than 1 meaning that the Fourier coefficients with frequencies outside the 1/n of the original range will be set to zero. For Butterworth, 2 parameters: n and order, giving the 1/n truncation and the smoothing order.'
        )
        self.declareProperty(
            'IgnoreXBins',
            True,
            direction=Direction.Input,
            doc=
            'Ignores the requirement that X bins be linear and of the same size. Set this to true if you are using log binning. The output X axis will be the same as the input either way.'
        )
        self.declareProperty('AllSpectra',
                             True,
                             direction=Direction.Input,
                             doc='Smooth all spectra.')
        self.declareProperty('WorkspaceIndexSmooth',
                             0,
                             direction=Direction.Input,
                             doc='Workspace index for smoothing')
        grp12 = 'FFTSmooth'
        self.setPropertyGroup('Filter', grp12)
        self.setPropertyGroup('Params', grp12)
        self.setPropertyGroup('IgnoreXBins', grp12)
        self.setPropertyGroup('AllSpectra', grp12)
        self.setPropertyGroup('WorkspaceIndexSmooth', grp12)

    def getInputs(self):
        # Output File
        self._outputFile = self.getProperty('OutputFile').value
        # Names for Workspaces
        self._sampleWS = 'Sample'
        self._vanaWS = 'Vana'
        self._emptyWS = 'Empty'
        # 2 theta and lambda limits
        self._tthMin = self.getProperty('TwoThetaMin').value
        self._tthMax = self.getProperty('TwoThetaMax').value
        self._wlCenter = self.getProperty('WavelengthCenter').value
        self._lambdaMin = self.getProperty('LambdaMin').value
        self._lambdaMax = self.getProperty('LambdaMax').value
        self._calcLambdaMin = self._wlCenter - 1.07 / 2.
        self._calcLambdaMax = self._wlCenter + 1.07 / 2.
        if self._calcLambdaMin > self._lambdaMin:
            self._lambdaMin = self._calcLambdaMin
        if self._calcLambdaMax < self._lambdaMax:
            self._lambdaMax = self._calcLambdaMax
        # d and dperp limits
        self._dMin = self.getProperty('DMin').value
        self._dMax = self.getProperty('DMax').value
        self._dpMin = self.getProperty('DpMin').value
        self._dpMax = self.getProperty('DpMax').value
        self._calcDMin = self._lambdaMin / (
            2. * math.sin(self._tthMax / 2. / 180. * math.pi))
        self._calcDMax = self._lambdaMax / (
            2. * math.sin(self._tthMin / 2. / 180. * math.pi))
        self._calcDpMin = math.sqrt(
            self._lambdaMin**2 -
            2. * math.log(math.cos(self._tthMin / 2. / 180. * math.pi)))
        self._calcDpMax = math.sqrt(
            self._lambdaMax**2 -
            2. * math.log(math.cos(self._tthMax / 2. / 180. * math.pi)))
        if self._calcDMin > self._dMin:
            self._dMin = self._calcDMin
        if self._calcDMax < self._dMax:
            self._dMax = self._calcDMax
        if self._calcDpMin > self._dpMin:
            self._dpMin = self._calcDpMin
        if self._calcDpMax < self._dpMax:
            self._dpMax = self._calcDpMax
        # True False questions for Vanadium, Empty and edgebinning
        self._doEdge = self.getProperty('DoEdgebinning').value
        self._doVana = self.getProperty('DoIntensityCorrection').value
        self._doEmpty = self.getProperty('DoBackgroundCorrection').value
        # Load
        self._sample = self.getPropertyValue('SampleData')
        self._vana = self.getPropertyValue('VanaData')
        self._empty = self.getPropertyValue('EmptyData')
        # FindDetectorsPar
        self._outputParTable = self.getProperty('OutputParTable').value
        self._returnLinearRanges = self.getProperty('ReturnLinearRanges').value
        self._parFile = self.getProperty('ParFile').value
        # FilterBadPulses
        self._lowerCutoff = self.getProperty('LowerCutoff').value
        # RemovePromptPulse
        self._width = self.getProperty('Width').value
        self._frequency = self.getProperty('Frequency').value
        # LoadDiffCal
        self._filename = self.getPropertyValue('CalFile')
        self._instrumentName = self.getProperty('InstrumentName').value
        self._instrumentFilename = self.getProperty('InstrumentFilename').value
        self._makeGroupingWorkspace = self.getProperty(
            'MakeGroupingWorkspace').value
        self._makeCalWorkspace = self.getProperty('MakeCalWorkspace').value
        self._makeMaskWorkspace = self.getProperty('MakeMaskWorkspace').value
        self._workspaceName = self.getProperty('WorkspaceName').value
        self._tofMin = self.getProperty('TofMin').value
        self._tofMax = self.getProperty('TofMax').value
        self._fixConversionIssues = self.getProperty(
            'FixConversionIssues').value
        # MaskDetectors
        self._spectraList = self.getProperty('SpectraList').value
        self._detectorList = self.getProperty('DetectorList').value
        self._workspaceIndexList = self.getProperty('WorkspaceIndexList').value
        self._maskedWorkspace = self.getProperty('MaskedWorkspace').value
        self._forceInstrumentMasking = self.getProperty(
            'ForceInstrumentMasking').value
        self._startWorkspaceIndex = self.getProperty(
            'StartWorkspaceIndex').value
        self._endWorkspaceIndex = self.getProperty('EndWorkspaceIndex').value
        self._componentList = self.getProperty('ComponentList').value
        # AlignDetectors
        self._calibrationFile = self._filename
        #self._calibrationWorkspace = self.getProperty('CalibrationWorkspace')
        #self._offsetsWorkspace = self.getProperty('OffsetsWorkspace')
        # CylinderAbsorption
        self._scatterFrom = self.getProperty('ScatterFrom').value
        self._attenuationXSection = self.getProperty(
            'AttenuationXSection').value
        self._scatteringXSection = self.getProperty('ScatteringXSection').value
        self._sampleNumberDensity = self.getProperty(
            'SampleNumberDensity').value
        self._numberOfWavelengthPoints = self.getProperty(
            'NumberOfWavelengthPoints').value
        self._expMethod = self.getProperty('ExpMethod').value
        self._eMode = self.getProperty('EMode').value
        self._eFixed = self.getProperty('EFixed').value
        self._cylinderSampleHeight = self.getProperty(
            'CylinderSampleHeight').value
        self._cylinderSampleRadius = self.getProperty(
            'CylinderSampleRadius').value
        self._cylinderAxis = self.getProperty('CylinderAxis').value
        self._numberOfSlices = self.getProperty('NumberOfSlices').value
        self._numberOfAnnuli = self.getProperty('NumberOfAnnuli').value
        # Bin2DPowderDiffraction
        self._dSpaceBinning = self.getProperty('dSpaceBinning').value
        self._dPerpendicularBinning = self.getProperty(
            'dPerpendicularBinning').value
        self._binEdgesFile = self.getProperty('BinEdgesFile').value
        self._normalizeByBinArea = self.getProperty('NormalizeByBinArea').value
        # StripVanadiumPeaks
        self._FWHM = self.getProperty('FWHM').value
        self._tolerance = self.getProperty('Tolerance').value
        self._backgroundType = self.getProperty('BackgroundType').value
        self._highBackground = self.getProperty('HighBackground').value
        self._peakPositionTolerance = self.getProperty(
            'PeakPositionTolerance').value
        self._workspaceIndex = self.getProperty('WorkspaceIndex').value
        # FFTSmooth
        self._workspaceIndexSmooth = self.getProperty(
            'WorkspaceIndexSmooth').value
        self._filter = self.getProperty('Filter').value
        self._params = self.getProperty('Params').value
        self._ignoreXBins = self.getProperty('IgnoreXBins').value
        self._allSpectra = self.getProperty('AllSpectra').value

    def processData(self, filename, wsName):
        Load(Filename=filename, OutputWorkspace=wsName)
        FindDetectorsPar(InputWorkspace=wsName,
                         ReturnLinearRanges=self._returnLinearRanges,
                         ParFile=self._parFile,
                         OutputParTable=self._outputParTable)
        FilterBadPulses(InputWorkspace=wsName,
                        Outputworkspace=wsName,
                        LowerCutoff=self._lowerCutoff)
        RemovePromptPulse(InputWorkspace=wsName,
                          OutputWorkspace=wsName,
                          Width=self._width,
                          Frequency=self._frequency)
        LoadDiffCal(InputWorkspace=wsName,
                    InstrumentName=self._instrumentName,
                    InstrumentFilename=self._instrumentFilename,
                    Filename=self._filename,
                    MakeGroupingWorkspace=self._makeGroupingWorkspace,
                    MakeCalWorkspace=self._makeCalWorkspace,
                    MakeMaskWorkspace=self._makeMaskWorkspace,
                    WorkspaceName=self._workspaceName,
                    TofMin=self._tofMin,
                    TofMax=self._tofMax,
                    FixConversionIssues=self._fixConversionIssues)
        MaskDetectors(Workspace=wsName,
                      SpectraList=self._spectraList,
                      DetectorList=self._detectorList,
                      WorkspaceIndexList=self._workspaceIndexList,
                      MaskedWorkspace=self._maskedWorkspace,
                      ForceInstrumentMasking=self._forceInstrumentMasking,
                      StartWorkspaceIndex=self._startWorkspaceIndex,
                      EndWorkspaceIndex=self._endWorkspaceIndex,
                      ComponentList=self._componentList)
        AlignDetectors(InputWorkspace=wsName,
                       OutputWorkspace=wsName,
                       CalibrationFile=self._calibrationFile)
        ConvertUnits(InputWorkspace=wsName,
                     OutputWorkspace=wsName,
                     Target='Wavelength')

    def processVana(self, wsName):
        CylinderAbsorption(
            InputWorkspace=wsName,
            OutputWorkspace='Atten',
            AttenuationXSection=self._attenuationXSection,
            ScatteringXSection=self._scatteringXSection,
            SampleNumberDensity=self._sampleNumberDensity,
            NumberOfWavelengthPoints=self._numberOfWavelengthPoints,
            ExpMethod=self._expMethod,
            EMode=self._eMode,
            EFixed=self._eFixed,
            CylinderSampleHeight=self._cylinderSampleHeight,
            CylinderSampleRadius=self._cylinderSampleRadius,
            NumberOfSlices=self._numberOfSlices,
            NumberOfAnnuli=self._numberOfAnnuli)
        Divide(LHSWorkspace=wsName,
               RHSWorkspace='Atten',
               OutputWorkspace=wsName)

    def binDataEdge(self, wsName):
        Bin2DPowderDiffraction(InputWorkspace=wsName,
                               OutputWorkspace=wsName,
                               BinEdgesFile=self._binEdgesFile,
                               NormalizeByBinArea=self._normalizeByBinArea)

    def binDataLog(self, wsName, dSpaceBinning, dPerpBinning):
        Bin2DPowderDiffraction(InputWorkspace=wsName,
                               OutputWorkspace=wsName,
                               dSpaceBinning=[
                                   self._dMin - dSpaceBinning, dSpaceBinning,
                                   self._dMax + dSpaceBinning
                               ],
                               dPerpendicularBinning=[
                                   self._dPMin - dPerpBinning, dPerpBinning,
                                   self.dPMax + dPerpBinning
                               ])

    def postProcessVana(self, wsName):
        StripVanadiumPeaks(InputWorkspace=wsName,
                           OutputWorkspace=wsName,
                           FWHM=self._FWHM,
                           Tolerance=self._tolerance,
                           BackgroundType=self._backgroundType,
                           HighBackground=self._highBackground,
                           PeakPositionTolerance=self._peakPositionTolerance,
                           WorkspaceIndex=self._workspaceIndex)
        FFTSmooth(InputWorkspace=wsName,
                  OutputWorkspace=wsName,
                  WorkspaceIndex=self._workspaceIndexSmooth,
                  Filter=self._filter,
                  Params=self._params,
                  IgnoreXBins=self._ignoreXBins,
                  AllSpectra=self._allSpectra)

    def correctSampleData(self, sampleWsName, useVana, vanaWsName, useEmpty,
                          emptyWsName):
        if useEmpty:
            Minus(LHSWorkspace=sampleWsName,
                  RHSWorkspace=emptyWsName,
                  OutputWorkspace=sampleWsName)
        if useVana:
            Divide(LHSWorkspace=sampleWsName,
                   RHSWorkspace=vanaWsName,
                   OutputWorkspace=sampleWsName)

    def checkForNegatives(self, wsName, useVana, vanaWsName, useEmpty,
                          emptyWsName):
        ResetNegatives2D(wsName)
        if useVana:
            ResetNegatives2D(vanaWsName)
        if useEmpty:
            ResetNegatives2D(emptyWsName)

    def PyExec(self):
        # Input laden
        self.getInputs()

        # Process Sample data
        self.processData(self._sample, self._sampleWS)
        if self._doEdge:
            self.binDataEdge(self._sampleWS)
        else:
            self.binDataLog(self._sampleWS, self._dSpaceBinning,
                            self._dPerpendicularBinning)

        # Process empty data if given
        if self._doEmpty:
            self.processData(self._empty, self._emptyWS)
            if self._doEdge:
                self.binDataEdge(self._emptyWS)
            else:
                self.binDataLog(self._emptyWS, self._dSpaceBinning,
                                self._dPerpendicularBinning)

        # Process vana data if given
        if self._doVana:
            self.processData(self._vana, self._vanaWS)
            self.processVana(self._vanaWS)
            if self._doEdge:
                self.binDataEdge(self._vanaWS)
            else:
                self.binDataLog(self._vanaWS, self._dSpaceBinning,
                                self._dPerpendicularBinning)
            self.postProcessVana(self._vanaWS)

        # Check all datafiles for negative Values and correct those
        self.checkForNegatives(self._sampleWS, self._doVana, self._vanaWS,
                               self._doEmpty, self._emptyWS)

        # Correct sample data with empty and vana data if they are there
        print(self._doVana, self._doEmpty)
        if self._doVana or self._doEmpty:
            self.correctSampleData(self._sampleWS, self._doVana, self._vanaWS,
                                   self._doEmpty, self._emptyWS)

        # Check final results again for negative Values and correct those
        self.checkForNegatives(self._sampleWS, self._doVana, self._vanaWS,
                               self._doEmpty, self._emptyWS)

        # Print sample data to p2d file
        SaveP2D(Workspace=self._sampleWS,
                OutputFile=self._outputFile,
                RemoveNaN=True,
                RemoveNegatives=True,
                CutData=True,
                TthMin=self._tthMin,
                TthMax=self._tthMax,
                lambdaMin=self._lambdaMin,
                LambdaMax=self._lambdaMax,
                DMin=self._dMin,
                DMax=self._dMax,
                DpMin=self._dpMin,
                DpMax=self._dpMax)


AlgorithmFactory.subscribe(PowderReduceP2D)
