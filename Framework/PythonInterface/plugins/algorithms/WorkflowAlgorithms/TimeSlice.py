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
    CalculateFlatBackground,
    CropWorkspace,
    DeleteWorkspace,
    Direction,
    Divide,
    GroupWorkspaces,
    Integration,
    FloatArrayProperty,
    IntArrayProperty,
    StringArrayProperty,
    Transpose,
)
from mantid.simpleapi import Load

from IndirectCommon import check_hist_zero

import os


def _count_monitors(raw_file):
    """
    Returns the number of monitors and if they're at the start or end of the file
    """

    raw_file = mtd[raw_file]
    num_hist = raw_file.getNumberHistograms()
    mon_count = 1

    spectrumInfo = raw_file.spectrumInfo()
    if spectrumInfo.isMonitor(0):
        # Monitors are at the start
        for i in range(1, num_hist):
            if spectrumInfo.isMonitor(i):
                mon_count += 1
            else:
                break

        return mon_count, True
    else:
        # Monitors are at the end
        if not spectrumInfo.isMonitor(num_hist):
            # if it's not, we don't have any monitors!
            return 0, True

        for i in range(num_hist, 0, -1):
            if spectrumInfo.isMonitor(i):
                mon_count += 1
            else:
                break

        return mon_count, False


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

            # Only need to process the calib file once
            if index == 0 and self._calib_ws is not None:
                self._process_calib(raw_file)

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

    def _process_calib(self, raw_file):
        """
        Run the calibration file with the raw file workspace.

        @param raw_file Name of calibration file
        """

        calib_spec_min = int(self._spectra_range[0])
        calib_spec_max = int(self._spectra_range[1])

        if calib_spec_max - calib_spec_min > mtd[self._calib_ws].getNumberHistograms():
            raise IndexError("Number of spectra used is greater than the number of spectra in the calibration file.")

        # Offset cropping range to account for monitors
        (mon_count, at_start) = _count_monitors(raw_file)

        if at_start:
            calib_spec_min -= mon_count + 1
            calib_spec_max -= mon_count + 1

        # Crop the calibration workspace, excluding the monitors
        CropWorkspace(
            InputWorkspace=self._calib_ws,
            OutputWorkspace=self._calib_ws,
            StartWorkspaceIndex=calib_spec_min,
            EndWorkspaceIndex=calib_spec_max,
        )

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
            Divide(LHSWorkspace=raw_file, RHSWorkspace=self._calib_ws, OutputWorkspace=raw_file)

        # Construct output workspace name
        run = mtd[raw_file].getRun().getLogData("run_number").value
        inst = mtd[raw_file].getInstrument().getName()
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
