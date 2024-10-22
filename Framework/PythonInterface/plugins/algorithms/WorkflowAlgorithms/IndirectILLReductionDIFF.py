# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantid.api import mtd, AlgorithmFactory, PythonAlgorithm, MatrixWorkspaceProperty, MultipleFileProperty, Progress
from mantid.kernel import Direction, IntBoundedValidator
from mantid.simpleapi import (
    ConjoinXRuns,
    ConvertSpectrumAxis,
    ConvertToPointData,
    DeleteWorkspace,
    DeleteWorkspaces,
    Divide,
    ExtractMonitors,
    ExtractUnmaskedSpectra,
    Integration,
    LoadAndMerge,
    MaskDetectors,
    RenameWorkspace,
    Scale,
    Transpose,
)


class IndirectILLReductionDIFF(PythonAlgorithm):
    """
    Performs reduction on IN16B's diffraction data. It can be on mode Doppler or BATS.
    """

    runs = None
    mode = None
    scan_parameter = None
    mask_start_pixels = None
    mask_end_pixels = None
    output = None
    progress = None
    transpose = None

    def category(self):
        return "ILL\\Indirect"

    def summary(self):
        return "Performs reduction on IN16B's diffraction data. Mode is either Doppler or BATS."

    def name(self):
        return "IndirectILLReductionDIFF"

    def setUp(self):
        self.runs = self.getPropertyValue("SampleRuns").split(",")
        self.scan_parameter = self.getPropertyValue("Observable")
        self.mask_start_pixels = self.getProperty("MaskPixelsFromStart").value
        self.mask_end_pixels = self.getProperty("MaskPixelsFromEnd").value
        self.transpose = self.getProperty("Transpose").value
        self.output = self.getPropertyValue("OutputWorkspace")
        self.progress = Progress(self, start=0.0, end=1.0, nreports=10)

    def PyInit(self):
        self.declareProperty(MultipleFileProperty("SampleRuns", extensions=["nxs"]), doc="File path for run(s).")
        self.declareProperty(
            MatrixWorkspaceProperty("OutputWorkspace", "", direction=Direction.Output),
            doc="The output workspace group containing reduced data.",
        )
        self.declareProperty(
            "MaskPixelsFromStart", 10, validator=IntBoundedValidator(lower=0), doc="Number of pixels to mask at the start of each tube"
        )
        self.declareProperty(
            "MaskPixelsFromEnd", 10, validator=IntBoundedValidator(lower=0), doc="Number of pixels to mask at the end of each tube"
        )
        self.declareProperty(
            "Observable", "sample.temperature", doc="If multiple files, the parameter from SampleLog to use as an index when conjoined."
        )
        self.declareProperty("Transpose", True, doc="Transpose the result.")
        self.declareProperty("Sum", False, doc="Sum along the scan")
        self.declareProperty(
            name="ComponentsToMask", defaultValue="", doc="Comma separated list of component names to mask, for instance: tube_1, tube_2"
        )

    def _normalize_by_monitor(self):
        """
        Normalizes the workspace by monitor value (ID is 0 for IN16B)
        """
        monitor_ws = self.output + "_mon"
        ExtractMonitors(InputWorkspace=self.output, DetectorWorkspace=self.output, MonitorWorkspace=monitor_ws)
        Divide(LHSWorkspace=self.output, RHSWorkspace=monitor_ws, OutputWorkspace=self.output, WarnOnZeroDivide=True)
        DeleteWorkspace(monitor_ws)

    def _mask_tube_ends(self):
        """
        Mask the ends of each tube according to values provided by the user
        """
        # the numbers here correspond to IN16B detectors
        cache = list(range(1, self.mask_start_pixels)) + list(range(257 - self.mask_end_pixels, 257))
        to_mask = [i + 256 * j for i in cache for j in range(8)]
        MaskDetectors(Workspace=self.output, DetectorList=to_mask)

    def _treat_doppler(self, ws):
        """
        Reduce Doppler diffraction data presents in workspace.
        @param ws: the input workspace
        """
        run = None
        if len(self.runs) > 1:
            run = mtd[ws][0].getRun()
        else:
            run = mtd[ws].getRun()
        if run.hasProperty("Doppler.incident_energy"):
            energy = run.getLogData("Doppler.incident_energy").value / 1000
        else:
            raise RuntimeError("Unable to find incident energy for Doppler mode")

        Integration(InputWorkspace=ws, OutputWorkspace=self.output)
        ConvertToPointData(InputWorkspace=self.output, OutputWorkspace=self.output)

        tmp_name = self.output + "_joined"
        ConjoinXRuns(InputWorkspaces=self.output, SampleLogAsXAxis=self.scan_parameter, FailBehaviour="Skip File", OutputWorkspace=tmp_name)
        DeleteWorkspaces(self.output)
        RenameWorkspace(InputWorkspace=tmp_name, OutputWorkspace=self.output)

        self._normalize_by_monitor()
        self._mask_tube_ends()

        components_to_mask = self.getPropertyValue("ComponentsToMask")
        if components_to_mask:
            MaskDetectors(Workspace=self.output, ComponentList=components_to_mask)

        ExtractUnmaskedSpectra(InputWorkspace=self.output, OutputWorkspace=self.output)
        ConvertSpectrumAxis(InputWorkspace=self.output, OutputWorkspace=self.output, Target="ElasticQ", EMode="Direct", EFixed=energy)

        if self.getProperty("Sum").value:
            blocksize = mtd[self.output].blocksize()
            Integration(InputWorkspace=self.output, OutputWorkspace=self.output)
            Scale(InputWorkspace=self.output, OutputWorkspace=self.output, Factor=1.0 / blocksize)

        if self.transpose:
            Transpose(InputWorkspace=self.output, OutputWorkspace=self.output)
            mtd[self.output].setDistribution(True)

    def _treat_BATS(self, ws):
        self.log().warning("BATS treatment not implemented yet.")
        pass

    def PyExec(self):
        self.setUp()
        LoadAndMerge(
            Filename=self.getPropertyValue("SampleRuns"),
            OutputWorkspace=self.output,
            LoaderOptions={"LoadDetectors": "Diffractometer"},
            startProgress=0,
            endProgress=0.9,
        )

        if len(self.runs) > 1:
            run = mtd[self.output][0].getRun()
        else:
            run = mtd[self.output].getRun()

        if run.hasProperty("acquisition_mode") and run.getLogData("acquisition_mode").value == 1:
            self.mode = "BATS"
            self.log().information("Mode recognized as BATS.")
        else:
            self.mode = "Doppler"
            self.log().information("Mode recognized as Doppler.")

        self.progress.report(9, "Treating data")
        if self.mode == "Doppler":
            self._treat_doppler(self.output)
        elif self.mode == "BATS":
            self._treat_BATS(self.output)

        self.setProperty("OutputWorkspace", mtd[self.output])


AlgorithmFactory.subscribe(IndirectILLReductionDIFF)
