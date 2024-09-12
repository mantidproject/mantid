# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=no-init,too-many-instance-attributes
from mantid.kernel import *
from mantid.simpleapi import *
from mantid.api import *


import os.path


class IndirectCalibration(DataProcessorAlgorithm):
    _input_files = None
    _out_ws = None
    _peak_range = None
    _back_range = None
    _spec_range = None
    _intensity_scale = None
    _run_numbers = None

    def category(self):
        return "Workflow\\Inelastic;Inelastic\\Calibration"

    def summary(self):
        return "Creates a calibration workspace from a White-Beam Vanadium run."

    def PyInit(self):
        self.declareProperty(StringArrayProperty(name="InputFiles"), doc="Comma separated list of input files")

        self.declareProperty(
            IntArrayProperty(name="DetectorRange", values=[0, 1], validator=IntArrayMandatoryValidator()), doc="Range of detectors."
        )

        self.declareProperty(
            FloatArrayProperty(name="PeakRange", values=[0.0, 100.0], validator=FloatArrayMandatoryValidator()),
            doc="Time of flight range over the peak.",
        )

        self.declareProperty(
            FloatArrayProperty(name="BackgroundRange", values=[0.0, 1000.0], validator=FloatArrayMandatoryValidator()),
            doc="Time of flight range over the background.",
        )

        self.declareProperty(
            name="ScaleByFactor", defaultValue=False, doc="Set to True to enable Scaling, if false output will be normalised to 1."
        )

        self.declareProperty(name="ScaleFactor", defaultValue=1.0, doc="Factor by which to scale the result.")

        self.declareProperty(name="LoadLogFiles", defaultValue=False, doc="Option to load log files.")

        self.declareProperty(
            WorkspaceProperty("OutputWorkspace", "", direction=Direction.Output), doc="Output workspace for calibration data."
        )

    def validateInputs(self):
        """
        Validates input ranges.
        """
        issues = dict()

        issues["DetectorRange"] = self._validate_range("DetectorRange")
        issues["PeakRange"] = self._validate_range("PeakRange")
        issues["BackgroundRange"] = self._validate_range("BackgroundRange")

        return issues

    def _validate_range(self, property_name):
        """
        Validates a range property.

        @param property_name Name of the property to validate
        @returns String detailing error, None if no error
        """

        prop_range = self.getProperty(property_name).value
        if len(prop_range) == 2:
            if prop_range[0] > prop_range[1]:
                return "Invalid range"
        else:
            return "Incorrect number of values (should be 2)"

        return None

    def PyExec(self):
        from IndirectCommon import get_run_number

        self._setup()
        runs = []
        self._run_numbers = []
        load_prog = Progress(self, start=0.0, end=0.30, nreports=len(self._input_files))
        i = 0
        for in_file in self._input_files:
            (_, filename) = os.path.split(in_file)
            (root, _) = os.path.splitext(filename)
            try:
                Load(
                    Filename=in_file,
                    OutputWorkspace=root,
                    SpectrumMin=int(self._spec_range[0]),
                    SpectrumMax=int(self._spec_range[1]),
                    LoadLogFiles=self.getProperty("LoadLogFiles").value,
                )

                runs.append(root)
                self._run_numbers.append(get_run_number(root))
            except (RuntimeError, ValueError) as exc:
                logger.error('Could not load raw file "%s": %s' % (in_file, str(exc)))
            load_prog.report("Loading file: " + str(i))
            i += 1

        calib_ws_name = "calibration"
        merge_prog = Progress(self, start=0.30, end=0.40, nreports=2)
        if len(runs) > 1:
            MergeRuns(InputWorkspaces=",".join(runs), OutputWorkspace=calib_ws_name)
            merge_prog.report("Workspaces merged")
            factor = 1.0 / len(runs)
            Scale(InputWorkspace=calib_ws_name, OutputWorkspace=calib_ws_name, Factor=factor)
            merge_prog.report("Scaling complete")
        else:
            calib_ws_name = runs[0]
            merge_prog.report()

        workflow_prog = Progress(self, start=0.40, end=0.90, nreports=10)
        workflow_prog.report("Calculating flat background")
        CalculateFlatBackground(
            InputWorkspace=calib_ws_name, OutputWorkspace=calib_ws_name, StartX=self._back_range[0], EndX=self._back_range[1], Mode="Mean"
        )

        workflow_prog.report("Masking detectors")
        number_historgrams = mtd[calib_ws_name].getNumberHistograms()
        ws_mask, num_zero_spectra = FindDetectorsOutsideLimits(InputWorkspace=calib_ws_name, OutputWorkspace="__temp_ws_mask")
        DeleteWorkspace(ws_mask)

        workflow_prog.report("Integrating calibration file")
        Integration(
            InputWorkspace=calib_ws_name, OutputWorkspace=calib_ws_name, RangeLower=self._peak_range[0], RangeUpper=self._peak_range[1]
        )

        workflow_prog.report("Summing Spectra")
        temp_sum = SumSpectra(InputWorkspace=calib_ws_name, OutputWorkspace="__temp_sum")
        total = temp_sum.readY(0)[0]
        DeleteWorkspace(temp_sum)

        if self._intensity_scale is None:
            self._intensity_scale = 1 / (total / (number_historgrams - num_zero_spectra))

        workflow_prog.report("Scaling calibration")
        Scale(InputWorkspace=calib_ws_name, OutputWorkspace=self._out_ws, Factor=self._intensity_scale, Operation="Multiply")

        # Remove old workspaces
        if len(runs) > 1:
            for run in runs:
                DeleteWorkspace(Workspace=run)
                workflow_prog.report("Deleting workspaces")

        self._add_logs()
        self.setProperty("OutputWorkspace", self._out_ws)

    def _setup(self):
        """
        Gets properties.
        """

        self._input_files = self.getProperty("InputFiles").value
        self._out_ws = self.getPropertyValue("OutputWorkspace")

        self._peak_range = self.getProperty("PeakRange").value
        self._back_range = self.getProperty("BackgroundRange").value
        self._spec_range = self.getProperty("DetectorRange").value
        self._scale_by_factor = self.getProperty("ScaleByFactor").value
        self._intensity_scale = self.getProperty("ScaleFactor").value
        if not self._scale_by_factor:
            self._intensity_scale = None

    def _add_logs(self):
        """
        Handles adding sample logs.
        """

        # Add sample logs to output workspace
        sample_logs = [
            ("calib_peak_min", self._peak_range[0]),
            ("calib_peak_max", self._peak_range[1]),
            ("calib_back_min", self._back_range[0]),
            ("calib_back_max", self._back_range[1]),
            ("calib_run_numbers", ",".join(self._run_numbers)),
        ]

        if self._intensity_scale is not None:
            sample_logs.append(("calib_scale_factor", self._intensity_scale))

        log_alg = self.createChildAlgorithm(name="AddSampleLogMultiple", startProgress=0.9, endProgress=1.0, enableLogging=True)
        log_alg.setProperty("Workspace", self._out_ws)
        log_alg.setProperty("LogNames", [log[0] for log in sample_logs])
        log_alg.setProperty("LogValues", [log[1] for log in sample_logs])


# Register algorithm with Mantid
AlgorithmFactory.subscribe(IndirectCalibration)
