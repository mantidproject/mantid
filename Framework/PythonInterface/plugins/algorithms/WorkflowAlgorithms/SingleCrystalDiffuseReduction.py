# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantid.api import (
    DataProcessorAlgorithm,
    mtd,
    AlgorithmFactory,
    FileProperty,
    FileAction,
    MultipleFileProperty,
    WorkspaceProperty,
    PropertyMode,
    Progress,
    MatrixWorkspaceProperty,
    ITableWorkspaceProperty,
)
from mantid.simpleapi import (
    LoadIsawUB,
    MaskDetectors,
    ConvertUnits,
    CropWorkspace,
    LoadInstrument,
    SetGoniometer,
    ConvertToMD,
    MDNorm,
    MinusMD,
    Load,
    DeleteWorkspace,
    RenameWorkspaces,
    CreateSingleValuedWorkspace,
    LoadNexus,
    MultiplyMD,
    LoadIsawDetCal,
    LoadMask,
    CopyInstrumentParameters,
    ApplyCalibration,
    CopySample,
    RecalculateTrajectoriesExtents,
    CropWorkspaceForMDNorm,
)
from mantid.kernel import VisibleWhenProperty, PropertyCriterion, FloatArrayLengthValidator, FloatArrayProperty, Direction, Property
from mantid import logger
import numpy as np


class SingleCrystalDiffuseReduction(DataProcessorAlgorithm):
    temp_workspace_list = [
        "__run",
        "__md",
        "__data",
        "__norm",
        "__bkg",
        "__bkg_md",
        "__bkg_data",
        "__bkg_norm",
        "__normalizedData",
        "__normalizedBackground",
        "PreprocessedDetectorsWS",
    ]

    def category(self):
        return "Diffraction\\Reduction"

    def seeAlso(self):
        return ["ConvertToMD", "MDNormSCDPreprocessIncoherent", "MDNorm"]

    def name(self):
        return "SingleCrystalDiffuseReduction"

    def summary(self):
        return "Single Crystal Diffuse Scattering Reduction, normalisation, symmetry and background substraction"

    def PyInit(self):
        # files to reduce
        self.declareProperty(
            MultipleFileProperty(name="Filename", extensions=["_event.nxs", ".nxs.h5", ".nxs"]), "Files to combine in reduction"
        )

        # background
        self.declareProperty(
            FileProperty(name="Background", defaultValue="", action=FileAction.OptionalLoad, extensions=["_event.nxs", ".nxs.h5", ".nxs"]),
            "Background run",
        )
        self.declareProperty("BackgroundScale", 1.0, doc="The background will be scaled by this number before being subtracted.")

        # Filter by TOF
        self.copyProperties("LoadEventNexus", ["FilterByTofMin", "FilterByTofMax"])

        # Vanadium SA and flux
        self.declareProperty(
            "ReuseSAFlux",
            True,
            "If True then if a previous SolidAngle and Flux has been loaded it will be reused otherwise it will be loaded.",
        )
        self.declareProperty(
            FileProperty(name="SolidAngle", defaultValue="", action=FileAction.Load, extensions=[".nxs"]),
            doc="An input workspace containing momentum integrated vanadium (a measure "
            "of the solid angle). See :ref:`MDNormSCDPreprocessIncoherent <algm-MDNormSCDPreprocessIncoherent>` "
            "for details",
        )
        self.declareProperty(
            FileProperty(name="Flux", defaultValue="", action=FileAction.Load, extensions=[".nxs"]),
            "An input workspace containing momentum dependent flux. See :ref:`MDnormSCD <algm-MDnormSCD>` for details",
        )
        self.declareProperty(
            "MomentumMin",
            Property.EMPTY_DBL,
            doc="Minimum value in momentum. The max of this value and the flux momentum minimum will be used.",
        )
        self.declareProperty(
            "MomentumMax",
            Property.EMPTY_DBL,
            doc="Maximum value in momentum. The min of this value and the flux momentum maximum will be used.",
        )

        # UBMatrix
        self.declareProperty(
            MultipleFileProperty(name="UBMatrix", extensions=[".mat", ".ub", ".txt"]),
            doc="Path to an ISAW-style UB matrix text file. See :ref:`LoadIsawUB <algm-LoadIsawUB>`",
        )
        # Goniometer
        self.declareProperty("SetGoniometer", False, "Set which Goniometer to use. See :ref:`SetGoniometer <algm-SetGoniometer>`")
        condition = VisibleWhenProperty("SetGoniometer", PropertyCriterion.IsNotDefault)
        self.copyProperties("SetGoniometer", ["Goniometers", "Axis0", "Axis1", "Axis2"])
        self.setPropertySettings("Goniometers", condition)
        self.setPropertySettings("Axis0", condition)
        self.setPropertySettings("Axis1", condition)
        self.setPropertySettings("Axis2", condition)
        self.declareProperty(
            FloatArrayProperty("OmegaOffset", [], direction=Direction.Input),
            doc="Offset to apply to the omega rotation of the Goniometer. Need to provide one value for every run.",
        )

        # Corrections
        self.declareProperty(
            FileProperty(name="LoadInstrument", defaultValue="", action=FileAction.OptionalLoad, extensions=[".xml"]),
            "Load a different instrument IDF onto the data from a file. See :ref:`LoadInstrument <algm-LoadInstrument>`",
        )
        self.declareProperty(
            ITableWorkspaceProperty("ApplyCalibration", "", optional=PropertyMode.Optional, direction=Direction.Input),
            doc="Calibration will be applied using this TableWorkspace using :ref:`ApplyCalibration <algm-ApplyCalibration>`.",
        )
        self.declareProperty(
            FileProperty(name="DetCal", defaultValue="", action=FileAction.OptionalLoad, extensions=[".detcal"]),
            "Load an ISAW DetCal calibration onto the data from a file. See :ref:`LoadIsawDetCal <algm-LoadIsawDetCal>`",
        )
        self.declareProperty(
            MatrixWorkspaceProperty("CopyInstrumentParameters", "", optional=PropertyMode.Optional, direction=Direction.Input),
            doc="The input workpsace from which :ref:`CopyInstrumentParameters <algm-CopyInstrumentParameters>` "
            "will copy parameters to data",
        )
        self.declareProperty(
            FileProperty(name="MaskFile", defaultValue="", action=FileAction.OptionalLoad, extensions=[".xml", ".msk"]),
            "Masking file for masking. Supported file format is XML and ISIS ASCII. See :ref:`LoadMask <algm-LoadMask>`",
        )

        self.copyProperties("MDNorm", ["SymmetryOperations"])

        self.declareProperty(
            FloatArrayProperty("QDimension0", [1, 0, 0], FloatArrayLengthValidator(3), direction=Direction.Input),
            "The first Q projection axis",
        )
        self.declareProperty(
            FloatArrayProperty("QDimension1", [0, 1, 0], FloatArrayLengthValidator(3), direction=Direction.Input),
            "The second Q projection axis",
        )
        self.declareProperty(
            FloatArrayProperty("QDimension2", [0, 0, 1], FloatArrayLengthValidator(3), direction=Direction.Input),
            "The third Q projection axis",
        )

        self.copyProperties("MDNorm", ["Dimension0Binning", "Dimension1Binning", "Dimension2Binning"])

        self.declareProperty(
            "KeepTemporaryWorkspaces",
            False,
            "If True the normalization and data workspaces in addition to the normalized data will be outputted",
        )

        self.declareProperty(
            WorkspaceProperty("OutputWorkspace", "", optional=PropertyMode.Mandatory, direction=Direction.Output),
            "Output Workspace. If background is subtracted _data and _background workspaces will also be made.",
        )

        # Background
        self.setPropertyGroup("Background", "Background")
        self.setPropertyGroup("BackgroundScale", "Background")

        # Vanadium
        self.setPropertyGroup("ReuseSAFlux", "Vanadium")
        self.setPropertyGroup("SolidAngle", "Vanadium")
        self.setPropertyGroup("Flux", "Vanadium")
        self.setPropertyGroup("MomentumMin", "Vanadium")
        self.setPropertyGroup("MomentumMax", "Vanadium")

        # Goniometer
        self.setPropertyGroup("SetGoniometer", "Goniometer")
        self.setPropertyGroup("Goniometers", "Goniometer")
        self.setPropertyGroup("Axis0", "Goniometer")
        self.setPropertyGroup("Axis1", "Goniometer")
        self.setPropertyGroup("Axis2", "Goniometer")
        self.setPropertyGroup("OmegaOffset", "Goniometer")

        # Corrections
        self.setPropertyGroup("LoadInstrument", "Corrections")
        self.setPropertyGroup("ApplyCalibration", "Corrections")
        self.setPropertyGroup("DetCal", "Corrections")
        self.setPropertyGroup("CopyInstrumentParameters", "Corrections")
        self.setPropertyGroup("MaskFile", "Corrections")

        # Projection and binning
        self.setPropertyGroup("QDimension0", "Projection and binning")
        self.setPropertyGroup("QDimension1", "Projection and binning")
        self.setPropertyGroup("QDimension2", "Projection and binning")
        self.setPropertyGroup("Dimension0Binning", "Projection and binning")
        self.setPropertyGroup("Dimension1Binning", "Projection and binning")
        self.setPropertyGroup("Dimension2Binning", "Projection and binning")

    def validateInputs(self):
        issues = {}

        UBs = self.getProperty("UBMatrix").value
        Omega = self.getProperty("OmegaOffset").value
        runs = self.getProperty("Filename").value
        if not (len(UBs) == 1 or len(UBs) == len(runs)):
            issues["UBMatrix"] = "Must provide one matrix, or a separate UB matrix for every run"

        if not (len(Omega) == 0 or len(Omega) == len(runs)):
            issues["OmegaOffset"] = "Must be either empty or provide one value for every run"

        return issues

    def PyExec(self):
        # remove possible old temp workspaces
        [DeleteWorkspace(ws) for ws in self.temp_workspace_list if mtd.doesExist(ws)]

        _background = bool(self.getProperty("Background").value)
        self._load_inst = bool(self.getProperty("LoadInstrument").value)
        self._apply_cal = bool(self.getProperty("ApplyCalibration").value)
        self._detcal = bool(self.getProperty("DetCal").value)
        self._copy_params = bool(self.getProperty("CopyInstrumentParameters").value)
        _masking = bool(self.getProperty("MaskFile").value)
        _outWS_name = self.getPropertyValue("OutputWorkspace")
        _UB = self.getProperty("UBMatrix").value
        if len(_UB) == 1:
            _UB = np.tile(_UB, len(self.getProperty("Filename").value))
        _offsets = self.getProperty("OmegaOffset").value
        if len(_offsets) == 0:
            _offsets = np.zeros(len(self.getProperty("Filename").value))

        if self.getProperty("ReuseSAFlux").value and mtd.doesExist("__sa") and mtd.doesExist("__flux"):
            logger.notice(
                "Reusing previously loaded SolidAngle and Flux workspaces. "
                "Set ReuseSAFlux to False if new files are selected or you change the momentum range."
            )
        else:
            logger.notice("Loading SolidAngle and Flux from file")
            LoadNexus(Filename=self.getProperty("SolidAngle").value, OutputWorkspace="__sa")
            LoadNexus(Filename=self.getProperty("Flux").value, OutputWorkspace="__flux")

        if _masking:
            LoadMask(
                Instrument=mtd["__sa"].getInstrument().getName(), InputFile=self.getProperty("MaskFile").value, OutputWorkspace="__mask"
            )
            MaskDetectors(Workspace="__sa", MaskedWorkspace="__mask")
            DeleteWorkspace("__mask")

        self.XMin = mtd["__sa"].getXDimension().getMinimum()
        self.XMax = mtd["__sa"].getXDimension().getMaximum()

        newXMin = self.getProperty("MomentumMin").value
        newXMax = self.getProperty("MomentumMax").value
        if newXMin != Property.EMPTY_DBL or newXMax != Property.EMPTY_DBL:
            if newXMin != Property.EMPTY_DBL:
                self.XMin = max(self.XMin, newXMin)
            if newXMax != Property.EMPTY_DBL:
                self.XMax = min(self.XMax, newXMax)
            logger.notice("Using momentum range {} to {} A^-1".format(self.XMin, self.XMax))
            CropWorkspace(InputWorkspace="__flux", OutputWorkspace="__flux", XMin=self.XMin, XMax=self.XMax)
            for spectrumNumber in range(mtd["__flux"].getNumberHistograms()):
                Y = mtd["__flux"].readY(spectrumNumber)
                mtd["__flux"].setY(spectrumNumber, (Y - Y.min()) / (Y.max() - Y.min()))

        MinValues = [-self.XMax * 2] * 3
        MaxValues = [self.XMax * 2] * 3

        if _background:
            self.load_file_and_apply(self.getProperty("Background").value, "__bkg", 0)

        progress = Progress(self, 0.0, 1.0, len(self.getProperty("Filename").value))

        for n, run in enumerate(self.getProperty("Filename").value):
            logger.notice("Working on " + run)

            self.load_file_and_apply(run, "__run", _offsets[n])
            LoadIsawUB("__run", _UB[n])

            ConvertToMD(
                InputWorkspace="__run",
                OutputWorkspace="__md",
                QDimensions="Q3D",
                dEAnalysisMode="Elastic",
                Q3DFrames="Q_sample",
                MinValues=MinValues,
                MaxValues=MaxValues,
            )
            RecalculateTrajectoriesExtents(InputWorkspace="__md", OutputWorkspace="__md")
            MDNorm(
                InputWorkspace="__md",
                FluxWorkspace="__flux",
                SolidAngleWorkspace="__sa",
                OutputDataWorkspace="__data",
                TemporaryDataWorkspace="__data" if mtd.doesExist("__data") else None,
                OutputNormalizationWorkspace="__norm",
                TemporaryNormalizationWorkspace="__norm" if mtd.doesExist("__norm") else None,
                OutputWorkspace=_outWS_name,
                QDimension0=self.getProperty("QDimension0").value,
                QDimension1=self.getProperty("QDimension1").value,
                QDimension2=self.getProperty("QDimension2").value,
                Dimension0Binning=self.getProperty("Dimension0Binning").value,
                Dimension1Binning=self.getProperty("Dimension1Binning").value,
                Dimension2Binning=self.getProperty("Dimension2Binning").value,
                SymmetryOperations=self.getProperty("SymmetryOperations").value,
            )
            DeleteWorkspace("__md")

            if _background:
                # Set background Goniometer and UB to be the same as data
                CopySample(
                    InputWorkspace="__run",
                    OutputWorkspace="__bkg",
                    CopyName=False,
                    CopyMaterial=False,
                    CopyEnvironment=False,
                    CopyShape=False,
                    CopyLattice=True,
                )
                mtd["__bkg"].run().getGoniometer().setR(mtd["__run"].run().getGoniometer().getR())

                ConvertToMD(
                    InputWorkspace="__bkg",
                    OutputWorkspace="__bkg_md",
                    QDimensions="Q3D",
                    dEAnalysisMode="Elastic",
                    Q3DFrames="Q_sample",
                    MinValues=MinValues,
                    MaxValues=MaxValues,
                )
                RecalculateTrajectoriesExtents(InputWorkspace="__bkg_md", OutputWorkspace="__bkg_md")
                MDNorm(
                    InputWorkspace="__bkg_md",
                    FluxWorkspace="__flux",
                    SolidAngleWorkspace="__sa",
                    OutputDataWorkspace="__bkg_data",
                    TemporaryDataWorkspace="__bkg_data" if mtd.doesExist("__bkg_data") else None,
                    OutputNormalizationWorkspace="__bkg_norm",
                    TemporaryNormalizationWorkspace="__bkg_norm" if mtd.doesExist("__bkg_norm") else None,
                    OutputWorkspace="__normalizedBackground",
                    QDimension0=self.getProperty("QDimension0").value,
                    QDimension1=self.getProperty("QDimension1").value,
                    QDimension2=self.getProperty("QDimension2").value,
                    Dimension0Binning=self.getProperty("Dimension0Binning").value,
                    Dimension1Binning=self.getProperty("Dimension1Binning").value,
                    Dimension2Binning=self.getProperty("Dimension2Binning").value,
                    SymmetryOperations=self.getProperty("SymmetryOperations").value,
                )
                DeleteWorkspace("__bkg_md")
            progress.report()
            DeleteWorkspace("__run")

        if _background:
            # outWS = data / norm - bkg_data / bkg_norm * BackgroundScale
            CreateSingleValuedWorkspace(OutputWorkspace="__scale", DataValue=self.getProperty("BackgroundScale").value)
            MultiplyMD(LHSWorkspace="__normalizedBackground", RHSWorkspace="__scale", OutputWorkspace="__normalizedBackground")
            DeleteWorkspace("__scale")
            MinusMD(LHSWorkspace=_outWS_name, RHSWorkspace="__normalizedBackground", OutputWorkspace=_outWS_name)
            if self.getProperty("KeepTemporaryWorkspaces").value:
                RenameWorkspaces(
                    InputWorkspaces=["__data", "__norm", "__bkg_data", "__bkg_norm"],
                    WorkspaceNames=[
                        _outWS_name + "_data",
                        _outWS_name + "_normalization",
                        _outWS_name + "_background_data",
                        _outWS_name + "_background_normalization",
                    ],
                )
        else:
            if self.getProperty("KeepTemporaryWorkspaces").value:
                RenameWorkspaces(
                    InputWorkspaces=["__data", "__norm"], WorkspaceNames=[_outWS_name + "_data", _outWS_name + "_normalization"]
                )

        self.setProperty("OutputWorkspace", mtd[_outWS_name])

        # remove temp workspaces
        [DeleteWorkspace(ws) for ws in self.temp_workspace_list if mtd.doesExist(ws)]

    def load_file_and_apply(self, filename, ws_name, offset):
        Load(
            Filename=filename,
            OutputWorkspace=ws_name,
            FilterByTofMin=self.getProperty("FilterByTofMin").value,
            FilterByTofMax=self.getProperty("FilterByTofMax").value,
        )
        if self._load_inst:
            LoadInstrument(Workspace=ws_name, Filename=self.getProperty("LoadInstrument").value, RewriteSpectraMap=False)
        if self._apply_cal:
            ApplyCalibration(Workspace=ws_name, CalibrationTable=self.getProperty("ApplyCalibration").value)
        if self._detcal:
            LoadIsawDetCal(InputWorkspace=ws_name, Filename=self.getProperty("DetCal").value)
        if self._copy_params:
            CopyInstrumentParameters(OutputWorkspace=ws_name, InputWorkspace=self.getProperty("CopyInstrumentParameters").value)
        MaskDetectors(Workspace=ws_name, MaskedWorkspace="__sa")

        if offset != 0:
            if self.getProperty("SetGoniometer").value:
                SetGoniometer(
                    Workspace=ws_name,
                    Goniometers=self.getProperty("Goniometers").value,
                    Axis0="{},0,1,0,1".format(offset),
                    Axis1=self.getProperty("Axis0").value,
                    Axis2=self.getProperty("Axis1").value,
                    Axis3=self.getProperty("Axis2").value,
                )
            else:
                SetGoniometer(
                    Workspace=ws_name, Axis0="{},0,1,0,1".format(offset), Axis1="omega,0,1,0,1", Axis2="chi,0,0,1,1", Axis3="phi,0,1,0,1"
                )
        else:
            if self.getProperty("SetGoniometer").value:
                SetGoniometer(
                    Workspace=ws_name,
                    Goniometers=self.getProperty("Goniometers").value,
                    Axis0=self.getProperty("Axis0").value,
                    Axis1=self.getProperty("Axis1").value,
                    Axis2=self.getProperty("Axis2").value,
                )

        ConvertUnits(InputWorkspace=ws_name, OutputWorkspace=ws_name, Target="Momentum")
        CropWorkspaceForMDNorm(InputWorkspace=ws_name, OutputWorkspace=ws_name, XMin=self.XMin, XMax=self.XMax)


AlgorithmFactory.subscribe(SingleCrystalDiffuseReduction)
