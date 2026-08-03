# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=no-init,invalid-name
from mantid.api import mtd, AlgorithmFactory, Progress, PropertyMode, PythonAlgorithm, WorkspaceGroupProperty, WorkspaceProperty
from mantid.kernel import (
    logger,
    Direction,
    FloatArrayProperty,
    IntArrayProperty,
    StringArrayProperty,
)
from mantid.simpleapi import (
    CalculateFlatBackground,
    CropWorkspace,
    DeleteWorkspace,
    Divide,
    GroupWorkspaces,
    Integration,
    Load,
    Transpose,
)

from IndirectCommon import check_hist_zero

import os


# pylint: disable=too-many-instance-attributes


class TimeSlice(PythonAlgorithm):
    _raw_files = None
    _spectra_range = None
    _peak_range = None
    _output_ws_name_suffix = None
    _background_range = None
    _calib_ws = None
    _out_ws_group = None

    def category(self):
        return "Inelastic\\Utility"

    def summary(self):
        return "Performa an integration on a raw file over a specified time of flight range"

    def seeAlso(self):
        return ["Integration"]

    def PyInit(self):
        self.declareProperty(StringArrayProperty(name="InputFiles"), doc="Comma separated list of input files")

        self.declareProperty(
            WorkspaceProperty(name="CalibrationWorkspace", defaultValue="", direction=Direction.Input, optional=PropertyMode.Optional),
            doc="Calibration workspace",
        )

        self.declareProperty(IntArrayProperty(name="SpectraRange"), doc="Range of spectra to use")

        self.declareProperty(FloatArrayProperty(name="PeakRange"), doc="Peak range in time of flight")

        self.declareProperty(FloatArrayProperty(name="BackgroundRange"), doc="Background range in time of flight")

        self.declareProperty(
            name="OutputNameSuffix", defaultValue="_slice", doc="Suffix to append to raw file name for name of output workspace"
        )

        self.declareProperty(
            WorkspaceGroupProperty(name="OutputWorkspace", defaultValue="", direction=Direction.Output),
            doc="Name of workspace group to group result workspaces into",
        )

    def _validate_range(self, name):
        """
        Validates a range property

        @param name Name of the property
        """

        range_prop = self.getProperty(name).value

        if len(range_prop) != 2:
            return "Range must have two values"

        if range_prop[0] > range_prop[1]:
            return 'Range must be in format "low,high"'

        return ""

    def validateInputs(self):
        issues = dict()

        issues["SpectraRange"] = self._validate_range("SpectraRange")
        issues["PeakRange"] = self._validate_range("PeakRange")

        if self.getPropertyValue("BackgroundRange") != "":
            issues["BackgroundRange"] = self._validate_range("BackgroundRange")

        return issues

    def PyExec(self):
        self._setup()

        out_ws_list = []
        i = 0
        workflow_prog = Progress(self, start=0.0, end=0.96, nreports=len(self._raw_files) * 3)
        for index, filename in enumerate(self._raw_files):
            workflow_prog.report("Reading file: " + str(i))
            raw_file = self._read_raw_file(filename)

            workflow_prog.report("Transposing Workspace: " + str(i))
            slice_file = self._process_raw_file(raw_file)
            Transpose(InputWorkspace=slice_file, OutputWorkspace=slice_file)
            unit = mtd[slice_file].getAxis(0).setUnit("Label")
            unit.setLabel("Spectrum Number", "")

            workflow_prog.report("Deleting Workspace")
            out_ws_list.append(slice_file)
            DeleteWorkspace(raw_file)

        all_workspaces = ",".join(out_ws_list)
        final_prog = Progress(self, start=0.96, end=1.0, nreports=2)
        final_prog.report("Grouping result")
        GroupWorkspaces(InputWorkspaces=all_workspaces, OutputWorkspace=self._out_ws_group)
        final_prog.report("setting result")
        self.setProperty("OutputWorkspace", self._out_ws_group)

    def _setup(self):
        """
        Gets properties.
        """

        self._raw_files = self.getProperty("InputFiles").value
        self._spectra_range = self.getProperty("SpectraRange").value
        self._peak_range = self.getProperty("PeakRange").value
        self._output_ws_name_suffix = self.getPropertyValue("OutputNameSuffix")

        self._background_range = self.getProperty("BackgroundRange").value
        if len(self._background_range) == 0:
            self._background_range = None

        self._calib_ws = self.getPropertyValue("CalibrationWorkspace")
        if self._calib_ws == "":
            self._calib_ws = None

        self._out_ws_group = self.getPropertyValue("OutputWorkspace")

    def _read_raw_file(self, filename):
        """
        Loads a raw run file.

        @param filename Data file name
        @returns Name of workspace loaded into
        """

        logger.information("Reading file :" + filename)

        # Load the raw file
        f_name = os.path.split(filename)[1]
        workspace_name = os.path.splitext(f_name)[0]

        Load(Filename=filename, OutputWorkspace=workspace_name, LoadLogFiles=False)

        return workspace_name

    def _divide_by_calibration(self, raw_file):
        """
        Divide raw_file by the calibration workspace, matched per detector ID.
        Raw spectra whose detector IDs are absent from the calibration workspace
        are divided by 1.0, so they survive and appear as raw integrated counts
        in the diagnostic output.
        """
        calib = mtd[self._calib_ws]

        calib_by_detid = {}
        for i in range(calib.getNumberHistograms()):
            y0 = calib.readY(i)[0]
            e0 = calib.readE(i)[0]
            for detid in calib.getSpectrum(i).getDetectorIDs():
                calib_by_detid[detid] = (y0, e0)

        raw = mtd[raw_file]
        raw_x = raw.readX(0)
        divisor_name = "__timeslice_padded_divisor"
        Integration(
            InputWorkspace=raw_file,
            OutputWorkspace=divisor_name,
            RangeLower=raw_x[0],
            RangeUpper=raw_x[-1],
        )
        divisor = mtd[divisor_name]

        unmatched = 0
        for i in range(divisor.getNumberHistograms()):
            match_y, match_e = 1.0, 0.0
            matched = False
            for detid in divisor.getSpectrum(i).getDetectorIDs():
                if detid in calib_by_detid:
                    match_y, match_e = calib_by_detid[detid]
                    matched = True
                    break
            if not matched:
                unmatched += 1
            divisor.dataY(i)[0] = match_y
            divisor.dataE(i)[0] = match_e

        if unmatched:
            logger.notice(f"TimeSlice: {unmatched} raw spectra had no calibration entry; using factor 1.0 for those")

        Divide(LHSWorkspace=raw_file, RHSWorkspace=divisor_name, OutputWorkspace=raw_file)
        DeleteWorkspace(divisor_name)

    def _process_raw_file(self, raw_file):
        """
        Process a raw sample file.

        @param raw_file Name of file to process
        """
        # Crop the raw file to use the desired number of spectra
        # less one because CropWorkspace is zero based
        CropWorkspace(
            InputWorkspace=raw_file,
            OutputWorkspace=raw_file,
            StartWorkspaceIndex=int(self._spectra_range[0]) - 1,
            EndWorkspaceIndex=int(self._spectra_range[1]) - 1,
        )

        num_hist = check_hist_zero(raw_file)[0]

        # Use calibration file if desired
        if self._calib_ws is not None:
            self._divide_by_calibration(raw_file)

        # Construct output workspace name
        run = mtd[raw_file].getRun().getLogData("run_number").value
        inst = mtd[raw_file].getInstrumentName()
        slice_file = inst.lower() + run + self._output_ws_name_suffix

        if self._background_range is None:
            Integration(
                InputWorkspace=raw_file,
                OutputWorkspace=slice_file,
                RangeLower=self._peak_range[0],
                RangeUpper=self._peak_range[1],
                StartWorkspaceIndex=0,
                EndWorkspaceIndex=num_hist - 1,
            )
        else:
            CalculateFlatBackground(
                InputWorkspace=raw_file,
                OutputWorkspace=slice_file,
                StartX=self._background_range[0],
                EndX=self._background_range[1],
                Mode="Mean",
            )
            Integration(
                InputWorkspace=slice_file,
                OutputWorkspace=slice_file,
                RangeLower=self._peak_range[0],
                RangeUpper=self._peak_range[1],
                StartWorkspaceIndex=0,
                EndWorkspaceIndex=num_hist - 1,
            )

        return slice_file


AlgorithmFactory.subscribe(TimeSlice)
