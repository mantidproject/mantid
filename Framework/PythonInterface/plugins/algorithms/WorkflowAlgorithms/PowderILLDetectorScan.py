# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantid.api import (
    AlgorithmFactory,
    DataProcessorAlgorithm,
    MultipleFileProperty,
    Progress,
    WorkspaceGroupProperty,
    FileProperty,
    FileAction,
)
from mantid.kernel import (
    Direction,
    FloatArrayOrderedPairsValidator,
    FloatArrayProperty,
    StringListValidator,
)
from mantid.simpleapi import (
    ApplyDetectorScanEffCorr,
    CropToComponent,
    DeleteWorkspace,
    ExtractMonitors,
    GroupWorkspaces,
    LoadAndMerge,
    LoadNexusProcessed,
    MaskDetectors,
    NormaliseToMonitor,
    Scale,
    SumOverlappingTubes,
)


class PowderILLDetectorScan(DataProcessorAlgorithm):
    _progress = None
    _height_ranges = None
    _mirror = None
    _crop_negative = None
    _out_ws_name = None

    def category(self):
        return "ILL\\Diffraction;Diffraction\\Reduction"

    def summary(self):
        return "Performs powder diffraction data reduction for D2B and D20 (when doing a detector scan)."

    def seeAlso(self):
        return ["PowderILLParameterScan", "PowderILLEfficiency"]

    def name(self):
        return "PowderILLDetectorScan"

    def validateInputs(self):
        issues = dict()

        if not (self.getProperty("Output2DTubes").value or self.getProperty("Output2D").value or self.getProperty("Output1D").value):
            issues["Output2DTubes"] = "No output chosen"
            issues["Output2D"] = "No output chosen"
            issues["Output1D"] = "No output chosen"

        if self.getPropertyValue("ComponentsToReduce") and self.getProperty("CropNegativeScatteringAngles").value:
            issues["CropNegativeScatteringAngles"] = "For component-wise reduction, this has to be unchecked."

        if self.getPropertyValue("HeightRange"):
            detectorHeights = self.getPropertyValue("HeightRange").split(",")
            for height in detectorHeights:
                try:
                    if float(height) > 0.15 or float(height) < -0.15:
                        issues["HeightRange"] = "The height of the detector ranges from -0.15m to 0.15m"
                except ValueError:
                    issues["HeightRange"] = "The height of the detector must be expressed in meters"

        return issues

    def PyInit(self):
        self.declareProperty(MultipleFileProperty("Run", extensions=["nxs"]), doc="File path of run(s).")

        self.declareProperty(
            name="NormaliseTo",
            defaultValue="Monitor",
            validator=StringListValidator(["None", "Monitor"]),
            doc="Normalise to monitor, or skip normalisation.",
        )

        self.declareProperty(
            FileProperty("CalibrationFile", "", action=FileAction.OptionalLoad, extensions=["nxs"]),
            doc="File containing the detector efficiencies.",
        )

        self.declareProperty(
            name="Output2DTubes", defaultValue=False, doc="Output a 2D workspace of height along tube against tube scattering angle."
        )

        self.declareProperty(
            name="Output2D", defaultValue=False, doc="Output a 2D workspace of height along tube against the real scattering angle."
        )

        self.declareProperty(name="Output1D", defaultValue=True, doc="Output a 1D workspace with counts against scattering angle.")

        self.declareProperty(
            name="CropNegativeScatteringAngles", defaultValue=True, doc="Whether or not to crop the negative scattering angles."
        )

        self.declareProperty(
            FloatArrayProperty(
                name="HeightRange",
                values=[],
                validator=FloatArrayOrderedPairsValidator(),
            ),
            doc="A list (even length) of comma separated values, to give the minimum and maximum heights of the different ranges "
            "(in m). If not specified only the full height range will be used.",
        )

        self.declareProperty(
            WorkspaceGroupProperty("OutputWorkspace", "", direction=Direction.Output), doc="Output workspace containing the reduced data."
        )

        self.declareProperty(
            name="ComponentsToMask", defaultValue="", doc="Comma separated list of component names to mask, for instance: tube_1, tube_2"
        )

        self.declareProperty(
            name="ComponentsToReduce",
            defaultValue="",
            doc="Comma separated list of component names to output the reduced data for; for example tube_1",
        )

        self.declareProperty(name="AlignTubes", defaultValue=False, doc="Align the tubes vertically and horizontally according to IPF.")

    def _generate_mask(self, n_pix, instrument):
        """
        Generates the DetectorList input for MaskDetectors
        Masks the bottom and top n_pix pixels in each tube, for D2B only
        @param n_pix : Number of pixels to mask from top and bottom of each tube
        @param instrument : Instrument
        @return the DetectorList string
        """
        mask = ""
        det = instrument.getComponentByName("detectors")
        tube = instrument.getComponentByName("tube_1")
        n_tubes = det.nelements()
        n_pixels = tube.nelements()
        for tube in range(n_tubes):
            start_bottom = tube * n_pixels + 1
            end_bottom = start_bottom + n_pix - 1
            start_top = (tube + 1) * n_pixels - n_pix + 1
            end_top = start_top + n_pix - 1
            mask += str(start_bottom) + "-" + str(end_bottom) + ","
            mask += str(start_top) + "-" + str(end_top) + ","
        self.log().debug("Preparing to mask with DetectorList=" + mask[:-1])
        return mask[:-1]

    def _validate_instrument(self, instrument_name):
        supported_instruments = ["D2B", "D20"]
        if instrument_name not in supported_instruments:
            self.log().warning(
                "Running for unsupported instrument, use with caution. Supported instruments are: " + str(supported_instruments)
            )
        if instrument_name == "D20":
            if self.getProperty("Output2DTubes").value:
                raise RuntimeError("Output2DTubes is not supported for D20 (1D detector)")
            if self.getProperty("Output2D").value:
                raise RuntimeError("Output2D is not supported for D20 (1D detector)")

    def _reduce_1D(self, input_group):
        output1D = []
        for height_range in self._height_ranges:
            output1DName = self._out_ws_name + "_1D"
            if height_range:
                output1DName = output1DName + "_" + height_range
            output1D.append(
                SumOverlappingTubes(
                    InputWorkspaces=input_group,
                    OutputType="1D",
                    HeightAxis=height_range,
                    MirrorScatteringAngles=self._mirror,
                    CropNegativeScatteringAngles=self._crop_negative,
                    OutputWorkspace=output1DName,
                )
            )
        return output1D

    def _reduce_2DTubes(self, input_group):
        output2DTubes = []
        for height_range in self._height_ranges:
            output2DtubesName = self._out_ws_name + "_2DTubes"
            if height_range:
                output2DtubesName = output2DtubesName + "_" + height_range
            output2DTubes.append(
                SumOverlappingTubes(
                    InputWorkspaces=input_group,
                    OutputType="2DTubes",
                    HeightAxis=height_range,
                    MirrorScatteringAngles=self._mirror,
                    CropNegativeScatteringAngles=self._crop_negative,
                    OutputWorkspace=output2DtubesName,
                )
            )

        return output2DTubes

    def _reduce_2D(self, input_group):
        output2D = []
        for height_range in self._height_ranges:
            output2DName = self._out_ws_name + "_2D"
            if height_range:
                output2DName = output2DName + "_" + height_range
            output2D.append(
                SumOverlappingTubes(
                    InputWorkspaces=input_group,
                    OutputType="2D",
                    HeightAxis=height_range,
                    MirrorScatteringAngles=self._mirror,
                    CropNegativeScatteringAngles=self._crop_negative,
                    OutputWorkspace=output2DName,
                )
            )

        return output2D

    def PyExec(self):
        align_tubes = self.getProperty("AlignTubes").value
        self._progress = Progress(self, start=0.0, end=1.0, nreports=6)
        self._progress.report("Loading data")
        # Do not merge the runs yet, since it will break the calibration
        # Load and calibrate separately, then SumOverlappingTubes will merge correctly
        # Besides + here does not make sense, and it will also slow down D2B a lot
        input_workspace = LoadAndMerge(
            Filename=self.getPropertyValue("Run").replace("+", ","),
            LoaderName="LoadILLDiffraction",
            LoaderOptions={"AlignTubes": align_tubes},
        )
        # We might already have a group, but group just in case
        input_group = GroupWorkspaces(InputWorkspaces=input_workspace)

        instrument = input_group[0].getInstrument()
        instrument_name = instrument.getName()
        self._validate_instrument(instrument_name)

        self._progress.report("Normalising to monitor")
        if self.getPropertyValue("NormaliseTo") == "Monitor":
            input_group = NormaliseToMonitor(InputWorkspace=input_group, MonitorID=0)
            if instrument_name == "D2B":
                input_group = Scale(InputWorkspace=input_group, Factor=1e6)

        calib_file = self.getPropertyValue("CalibrationFile")
        if calib_file:
            self._progress.report("Applying detector efficiencies")
            LoadNexusProcessed(Filename=calib_file, OutputWorkspace="__det_eff")
            for ws in input_group:
                name = ws.getName()
                ExtractMonitors(InputWorkspace=name, DetectorWorkspace=name)
                ApplyDetectorScanEffCorr(InputWorkspace=name, DetectorEfficiencyWorkspace="__det_eff", OutputWorkspace=name)

        components_to_mask = self.getPropertyValue("ComponentsToMask")
        if components_to_mask:
            for ws in input_group:
                MaskDetectors(Workspace=ws, ComponentList=components_to_mask)

        height_range_prop = self.getProperty("HeightRange").value
        self._height_ranges = [""]
        if len(height_range_prop) % 2 == 0:
            i = 0
            while i < len(height_range_prop) - 1:
                self._height_ranges.append(str(height_range_prop[i]) + ", " + str(height_range_prop[i + 1]))
                i += 2
        output_workspaces = []
        self._out_ws_name = self.getPropertyValue("OutputWorkspace")
        self._mirror = False
        self._crop_negative = self.getProperty("CropNegativeScatteringAngles").value
        if instrument.hasParameter("mirror_scattering_angles"):
            self._mirror = instrument.getBoolParameter("mirror_scattering_angles")[0]

        components = self.getPropertyValue("ComponentsToReduce")
        if components:
            for ws in input_group:
                CropToComponent(InputWorkspace=ws.getName(), OutputWorkspace=ws.getName(), ComponentNames=components)

        self._progress.report("Doing Output2DTubes Option")
        if self.getProperty("Output2DTubes").value:
            output2DTubes = self._reduce_2DTubes(input_group)
            output_workspaces += output2DTubes

        self._progress.report("Doing Output2D Option")
        if self.getProperty("Output2D").value:
            output2D = self._reduce_2D(input_group)
            output_workspaces += output2D

        self._progress.report("Doing Output1D Option")
        if self.getProperty("Output1D").value:
            output1D = self._reduce_1D(input_group)
            output_workspaces += output1D

        self._progress.report("Finishing up...")
        DeleteWorkspace("input_group")
        GroupWorkspaces(InputWorkspaces=output_workspaces, OutputWorkspace=self._out_ws_name)
        self.setProperty("OutputWorkspace", self._out_ws_name)


# Register the algorithm with Mantid
AlgorithmFactory.subscribe(PowderILLDetectorScan)
