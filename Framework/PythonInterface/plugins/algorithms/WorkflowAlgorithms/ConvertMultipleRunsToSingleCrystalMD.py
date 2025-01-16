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
    LoadInstrument,
    SetGoniometer,
    ConvertToMD,
    Load,
    LoadIsawDetCal,
    LoadMask,
    DeleteWorkspace,
    MaskDetectors,
    ConvertToMDMinMaxGlobal,
    ApplyCalibration,
    CopyInstrumentParameters,
    ConvertUnits,
    CropWorkspaceForMDNorm,
)
from mantid.kernel import VisibleWhenProperty, PropertyCriterion, Direction, StringListValidator, Property
from mantid import logger


class ConvertMultipleRunsToSingleCrystalMD(DataProcessorAlgorithm):
    def category(self):
        return "MDAlgorithms\\Creation"

    def seeAlso(self):
        return ["ConvertToDiffractionMDWorkspace", "ConvertToMD"]

    def name(self):
        return "ConvertMultipleRunsToSingleCrystalMD"

    def summary(self):
        return "Convert multiple runs to one Single Crystal MDEventWorkspace"

    def PyInit(self):
        # files to reduce
        self.declareProperty(
            MultipleFileProperty(name="Filename", extensions=["_event.nxs", ".nxs.h5", ".nxs"]), "Files to combine in reduction"
        )

        # Filter by time
        self.copyProperties("LoadEventNexus", ["FilterByTofMin", "FilterByTofMax", "FilterByTimeStop"])

        # Filter momentum
        self.declareProperty(
            "MomentumMin",
            Property.EMPTY_DBL,
            doc="Minimum value in momentum. This should match the Flux momentum if "
            "the output is to be used with :ref:`MDNorm <algm-MDNorm>`",
        )
        self.declareProperty(
            "MomentumMax",
            Property.EMPTY_DBL,
            doc="Maximum value in momentum. This should match the Fluxmomentum if "
            "the output is to be used with :ref:`MDNorm <algm-MDNorm>`",
        )
        # UBMatrix
        self.declareProperty(
            FileProperty(name="UBMatrix", defaultValue="", action=FileAction.OptionalLoad, extensions=[".mat", ".ub", ".txt"]),
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

        self.declareProperty(
            WorkspaceProperty("OutputWorkspace", "", optional=PropertyMode.Mandatory, direction=Direction.Output), "Output Workspace"
        )

        # Convert Settings
        self.declareProperty(
            "QFrame",
            "Q_sample",
            validator=StringListValidator(["Q_sample", "HKL"]),
            doc="Selects Q-dimensions of the output workspace. Q (sample frame): "
            "Wave-vector converted into the frame of the sample (taking out the "
            "goniometer rotation). HKL: Use the sample's UB matrix to convert "
            "Wave-vector to crystal's HKL indices.",
        )

        self.copyProperties(
            "ConvertToMD",
            ["Uproj", "Vproj", "Wproj", "MinValues", "MaxValues", "SplitInto", "SplitThreshold", "MaxRecursionDepth", "OverwriteExisting"],
        )

        self.setPropertyGroup("FilterByTofMin", "Loading")
        self.setPropertyGroup("FilterByTofMax", "Loading")
        self.setPropertyGroup("FilterByTimeStop", "Loading")
        self.setPropertyGroup("MomentumMin", "Loading")
        self.setPropertyGroup("MomentumMax", "Loading")

        # Goniometer
        self.setPropertyGroup("SetGoniometer", "Goniometer")
        self.setPropertyGroup("Goniometers", "Goniometer")
        self.setPropertyGroup("Axis0", "Goniometer")
        self.setPropertyGroup("Axis1", "Goniometer")
        self.setPropertyGroup("Axis2", "Goniometer")

        # Corrections
        self.setPropertyGroup("LoadInstrument", "Corrections")
        self.setPropertyGroup("DetCal", "Corrections")
        self.setPropertyGroup("MaskFile", "Corrections")

        # ConvertToMD
        self.setPropertyGroup("QFrame", "ConvertToMD")
        self.setPropertyGroup("Uproj", "ConvertToMD")
        self.setPropertyGroup("Vproj", "ConvertToMD")
        self.setPropertyGroup("Wproj", "ConvertToMD")
        self.setPropertyGroup("MinValues", "ConvertToMD")
        self.setPropertyGroup("MaxValues", "ConvertToMD")
        self.setPropertyGroup("SplitInto", "ConvertToMD")
        self.setPropertyGroup("SplitThreshold", "ConvertToMD")
        self.setPropertyGroup("MaxRecursionDepth", "ConvertToMD")

    def PyExec(self):
        self._load_inst = bool(self.getProperty("LoadInstrument").value)
        self._apply_cal = bool(self.getProperty("ApplyCalibration").value)
        self._detcal = bool(self.getProperty("DetCal").value)
        self._copy_params = bool(self.getProperty("CopyInstrumentParameters").value)
        self._masking = bool(self.getProperty("MaskFile").value)
        _outWS_name = self.getPropertyValue("OutputWorkspace")
        _UB = bool(self.getProperty("UBMatrix").value)

        self.XMin = self.getProperty("MomentumMin").value
        self.XMax = self.getProperty("MomentumMax").value

        MinValues = self.getProperty("MinValues").value
        MaxValues = self.getProperty("MaxValues").value

        if self.getProperty("OverwriteExisting").value:
            if mtd.doesExist(_outWS_name):
                DeleteWorkspace(_outWS_name)

        progress = Progress(self, 0.0, 1.0, len(self.getProperty("Filename").value))

        for run in self.getProperty("Filename").value:
            logger.notice("Working on " + run)

            self.load_file_and_apply(run, "__run")

            if self.getProperty("SetGoniometer").value:
                SetGoniometer(
                    Workspace="__run",
                    Goniometers=self.getProperty("Goniometers").value,
                    Axis0=self.getProperty("Axis0").value,
                    Axis1=self.getProperty("Axis1").value,
                    Axis2=self.getProperty("Axis2").value,
                )

            if _UB:
                LoadIsawUB(InputWorkspace="__run", Filename=self.getProperty("UBMatrix").value)

            if len(MinValues) == 0 or len(MaxValues) == 0:
                MinValues, MaxValues = ConvertToMDMinMaxGlobal(
                    "__run",
                    dEAnalysisMode="Elastic",
                    Q3DFrames="Q" if self.getProperty("QFrame").value == "Q_sample" else "HKL",
                    QDimensions="Q3D",
                )

            ConvertToMD(
                InputWorkspace="__run",
                OutputWorkspace=_outWS_name,
                QDimensions="Q3D",
                dEAnalysisMode="Elastic",
                Q3DFrames=self.getProperty("QFrame").value,
                QConversionScales="Q in A^-1" if self.getProperty("QFrame").value == "Q_sample" else "HKL",
                Uproj=self.getProperty("Uproj").value,
                Vproj=self.getProperty("Vproj").value,
                Wproj=self.getProperty("Wproj").value,
                MinValues=MinValues,
                MaxValues=MaxValues,
                SplitInto=self.getProperty("SplitInto").value,
                SplitThreshold=self.getProperty("SplitThreshold").value,
                MaxRecursionDepth=self.getProperty("MaxRecursionDepth").value,
                OverwriteExisting=False,
            )
            DeleteWorkspace("__run")
            progress.report()

        if mtd.doesExist("__mask"):
            DeleteWorkspace("__mask")

        self.setProperty("OutputWorkspace", mtd[_outWS_name])

    def load_file_and_apply(self, filename, ws_name):
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
        if self._masking:
            if not mtd.doesExist("__mask"):
                LoadMask(
                    Instrument=mtd[ws_name].getInstrument().getName(),
                    InputFile=self.getProperty("MaskFile").value,
                    OutputWorkspace="__mask",
                )
            MaskDetectors(Workspace=ws_name, MaskedWorkspace="__mask")
        if self.XMin != Property.EMPTY_DBL and self.XMax != Property.EMPTY_DBL:
            ConvertUnits(InputWorkspace=ws_name, OutputWorkspace=ws_name, Target="Momentum")
            CropWorkspaceForMDNorm(InputWorkspace=ws_name, OutputWorkspace=ws_name, XMin=self.XMin, XMax=self.XMax)


AlgorithmFactory.subscribe(ConvertMultipleRunsToSingleCrystalMD)
