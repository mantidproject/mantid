# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

from mantid.api import mtd, AlgorithmFactory, DataProcessorAlgorithm, FileAction, FileProperty, Progress, PropertyMode, WorkspaceProperty
from mantid.kernel import logger, IntBoundedValidator, Direction
from mantid.simpleapi import (
    ConvertAxisByFormula,
    ConvertSpectrumAxis,
    DeleteWorkspace,
    GroupDetectors,
    Load,
    LoadNexusProcessed,
    SANSILLReduction,
    SortXAxis,
    Transpose,
)
from SANSILLAutoProcess import needs_loading, needs_processing


class SANSILLParameterScan(DataProcessorAlgorithm):
    """
    Performs treatment for scans along a parameter for D16.
    """

    progress = None
    reduction_type = None
    sample = None
    absorber = None
    container = None
    sensitivity = None
    default_mask = None
    output = None
    normalise = None
    output2D = None
    output_joined = None
    observable = None
    pixel_y_min = None
    pixel_y_max = None
    default_mask_ws = None
    sensitivity_ws = None

    def category(self):
        return "ILL\\SANS;ILL\\Auto"

    def summary(self):
        return "Integrate SANS scan data along a parameter"

    def seeAlso(self):
        return []

    def name(self):
        return "SANSILLParameterScan"

    def validateInputs(self):
        issues = dict()
        if not (self.getPropertyValue("OutputJoinedWorkspace") or self.getPropertyValue("OutputWorkspace")):
            issues["OutputJoinedWorkspace"] = "Please provide either OutputJoinedWorkspace, OutputWorkspace or both."
            issues["OutputWorkspace"] = "Please provide either OutputJoinedWorkspace, OutputWorkspace or both."
        if self.getProperty("PixelYmin").value > self.getProperty("PixelYmax").value:
            issues["PixelYMin"] = "YMin needs to be lesser than YMax"
            issues["PixelYMax"] = "YMax needs to be greater than YMin"
        return issues

    def setUp(self):
        self.sample = self.getPropertyValue("SampleRun")
        self.absorber = self.getPropertyValue("AbsorberRun").replace(",", "+")
        self.container = self.getPropertyValue("ContainerRun").replace(",", "+")
        self.sensitivity = self.getPropertyValue("SensitivityMap")
        self.default_mask = self.getPropertyValue("DefaultMaskFile")
        self.normalise = self.getPropertyValue("NormaliseBy")
        self.output2D = self.getPropertyValue("OutputWorkspace")
        self.output_joined = self.getPropertyValue("OutputJoinedWorkspace")
        self.observable = self.getPropertyValue("Observable")
        self.pixel_y_min = self.getProperty("PixelYMin").value
        self.pixel_y_max = self.getProperty("PixelYMax").value
        self.progress = Progress(self, start=0.0, end=1.0, nreports=10)

    def checkPixelY(self, height):
        if self.pixel_y_max > height:
            self.pixel_y_max = height
            logger.warning("PixelYMax value is too high. Reduced to {0}.".format(self.pixel_y_max))

    def PyInit(self):
        self.declareProperty(
            WorkspaceProperty("OutputWorkspace", "", direction=Direction.Output, optional=PropertyMode.Optional),
            doc="The output workspace containing the 2D reduced data.",
        )

        self.declareProperty(
            WorkspaceProperty("OutputJoinedWorkspace", "", direction=Direction.Output, optional=PropertyMode.Optional),
            doc="The output workspace containing all the reduced data, before grouping.",
        )

        self.declareProperty(FileProperty("SampleRun", "", action=FileAction.Load, extensions=["nxs"]), doc="Sample scan file.")

        self.declareProperty(FileProperty("AbsorberRun", "", action=FileAction.OptionalLoad, extensions=["nxs"]), doc="Absorber run.")

        self.declareProperty(
            FileProperty("ContainerRun", "", action=FileAction.OptionalLoad, extensions=["nxs"]), doc="Empty container run."
        )

        self.setPropertyGroup("SampleRun", "Numors")
        self.setPropertyGroup("AbsorberRun", "Numors")
        self.setPropertyGroup("ContainerRun", "Numors")

        self.declareProperty(
            FileProperty("SensitivityMap", "", action=FileAction.OptionalLoad, extensions=["nxs"]),
            doc="File containing the map of relative detector efficiencies.",
        )

        self.declareProperty(
            FileProperty("DefaultMaskFile", "", action=FileAction.OptionalLoad, extensions=["nxs"]),
            doc="File containing the default mask to be applied to all the detector configurations.",
        )

        self.copyProperties("SANSILLReduction", ["NormaliseBy"], version=2)

        self.declareProperty("Observable", "Omega.value", doc="Parameter from the sample logs along which the scan is made")

        self.declareProperty(
            "PixelYMin",
            3,
            validator=IntBoundedValidator(lower=0),
            doc="Minimal y-index taken in the integration. Default is based on D16B geometry.",
        )
        self.declareProperty(
            "PixelYMax",
            189,
            validator=IntBoundedValidator(lower=0),
            doc="Maximal y-index taken in the integration. Default is based on D16B geometry.",
        )

        self.copyProperties("CalculateEfficiency", ["MinThreshold", "MaxThreshold"])
        # override default documentation of copied parameters to make them understandable by user
        threshold_property = self.getProperty("MinThreshold")
        threshold_property.setDocumentation("Minimum threshold for calculated efficiency.")
        threshold_property = self.getProperty("MaxThreshold")
        threshold_property.setDocumentation("Maximum threshold for calculated efficiency.")

        self.setPropertyGroup("SensitivityMap", "Options")
        self.setPropertyGroup("DefaultMaskFile", "Options")
        self.setPropertyGroup("NormaliseBy", "Options")
        self.setPropertyGroup("Observable", "Options")
        self.setPropertyGroup("PixelYMin", "Options")
        self.setPropertyGroup("PixelYMax", "Options")
        self.setPropertyGroup("MinThreshold", "Options")
        self.setPropertyGroup("MaxThreshold", "Options")

    def PyExec(self):
        self.setUp()

        _, load_ws_name = needs_loading(self.sample, "Load")
        Load(Filename=self.sample, OutputWorkspace=load_ws_name, startProgress=0, endProgress=0.7)

        sorted_data = load_ws_name + "_sorted" if not self.output_joined else self.output_joined
        SortXAxis(InputWorkspace=load_ws_name, OutputWorkspace=sorted_data, startProgress=0.75, endProgress=0.8)
        DeleteWorkspace(Workspace=load_ws_name)

        self.load_input_files()
        self.reduce(sorted_data)

        if self.observable == "Omega.value":
            mtd[sorted_data].getAxis(0).setUnit("label").setLabel(self.observable, "degrees")

        self.group_detectors(sorted_data)

        if not self.output_joined:
            DeleteWorkspace(Workspace=sorted_data)
        else:
            self.setProperty("OutputJoinedWorkspace", mtd[self.output_joined])

        self.progress.report("Convert axis.")
        ConvertSpectrumAxis(
            InputWorkspace=self.output2D, OutputWorkspace=self.output2D, Target="SignedInPlaneTwoTheta", startProgress=0.95, endProgress=1
        )

        # ConvertSpectrumAxis uses a different convention from D16 when it comes to detector orientation, and thus the
        # 2theta axis is inverted from what is expected, so we flip it back
        # and since it is a widespread behavior for ILL instruments, this is now the default behaviour
        ConvertAxisByFormula(InputWorkspace=self.output2D, OutputWorkspace=self.output2D, Axis="Y", Formula="-y")

        Transpose(InputWorkspace=self.output2D, OutputWorkspace=self.output2D)

        # flipping the sign of the axis means it is now inverted, which is something matplotlib can't render (and is
        # really unpleasant to read anyway), so we sort everything again ...
        # which means cloning the workspace since that's what SortXaAxis does
        SortXAxis(InputWorkspace=self.output2D, OutputWorkspace=self.output2D, Ordering="Ascending")

        self.setProperty("OutputWorkspace", mtd[self.output2D])

    def load_input_files(self):
        """
        Load input files provided by the user if needed
        """

        load_sensitivity, self.sensitivity_ws = needs_loading(self.sensitivity, "Sensitivity")
        if load_sensitivity:
            self.progress.report(8, "Loading sensitivity")
            LoadNexusProcessed(Filename=self.sensitivity, OutputWorkspace=self.sensitivity_ws)

        load_default_mask, self.default_mask_ws = needs_loading(self.default_mask, "DefaultMask")
        if load_default_mask:
            self.progress.report(0, "Loading default mask")
            LoadNexusProcessed(Filename=self.default_mask, OutputWorkspace=self.default_mask_ws)

    def reduce(self, sorted_ws: str):
        """
        Do the standard data reduction using SANSILLReduction

        Keyword arguments:
        sorted_ws: the name of the sample workspace with X axis holding the sorted scanned parameter
        """
        process_absorber, absorber_name = needs_processing(self.absorber, "DarkCurrent")
        if process_absorber:
            self.progress.report(0, "Processing dark current")
            SANSILLReduction(
                Runs=self.absorber, ProcessAs="DarkCurrent", NormaliseBy=self.normalise, OutputWorkspace=absorber_name, Version=2
            )

        process_container, container_name = needs_processing(self.container, "Container")

        if process_container:
            self.progress.report(0, "Processing container")
            SANSILLReduction(
                Runs=self.container,
                ProcessAs="EmptyContainer",
                OutputWorkspace=container_name,
                DarkCurrentWorkspace=absorber_name,
                DefaultMaskWorkspace=self.default_mask_ws,
                NormaliseBy=self.normalise,
                Version=2,
            )

        # reduce the sample data
        self.progress.report(0, "Reducing data.")
        SANSILLReduction(
            SampleWorkspace=sorted_ws,
            DarkCurrentWorkspace=absorber_name,
            EmptyContainerWorkspace=container_name,
            SensitivityWorkspace=self.sensitivity_ws,
            DefaultMaskWorkspace=self.default_mask_ws,
            NormaliseBy=self.normalise,
            OutputWorkspace=sorted_ws,
            startProgress=0.8,
            endProgress=0.95,
            MinThreshold=self.getProperty("MinThreshold").value,
            MaxThreshold=self.getProperty("MaxThreshold").value,
            Version=2,
        )

    def group_detectors(self, ws: str):
        """
        Average each tube / wire value of the detector.

        Keyword arguments:
        ws: the name of the ws to group
        """
        instrument = mtd[ws].getInstrument()
        detector = instrument.getComponentByName("detector")
        if "detector-width" in detector.getParameterNames() and "detector-height" in detector.getParameterNames():
            width = int(detector.getNumberParameter("detector-width")[0])
            height = int(detector.getNumberParameter("detector-height")[0])
        else:
            raise RuntimeError("No width or height found for this instrument. Unable to group detectors.")

        self.checkPixelY(height)
        grouping = create_detector_grouping(self.pixel_y_min, self.pixel_y_max, width, height)

        GroupDetectors(InputWorkspace=ws, OutputWorkspace=self.output2D, GroupingPattern=grouping, Behaviour="Average")


def create_detector_grouping(y_min: int, y_max: int, detector_width: int, detector_height: int) -> str:
    """
    Create the pixel grouping for the detector. Shape is assumed to be D16's.
    The pixel grouping consists of the vertical columns of pixels of the detector.

    Keyword arguments:
    y_min: index of the first line to take on each column.
    y_max: index of the last line to take on each column.
    detector_width: the total number of column of pixel on the detector.
    detector_height: the total number of lines of pixel on the detector.
    """
    grouping = []
    for i in range(detector_width):
        grouping.append(str(i * detector_height + y_min) + "-" + str(i * detector_height + y_max - 1))
    grouping = ",".join(grouping)
    return grouping


AlgorithmFactory.subscribe(SANSILLParameterScan)
