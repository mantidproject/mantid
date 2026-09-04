# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=no-init,too-many-instance-attributes
from mantid.api import AlgorithmFactory, DataProcessorAlgorithm, Progress, PropertyMode, WorkspaceProperty
from mantid.kernel import Direction, FloatArrayProperty, IntArrayProperty, StringArrayProperty, StringListValidator
from mantid.simpleapi import CalculateFlatBackground, GroupDetectors, Rebin, RemoveSpectra, Scale


class IndirectResolution(DataProcessorAlgorithm):
    _input_files = None
    _out_ws = None
    _instrument = None
    _analyser = None
    _reflection = None
    _detector_range = None
    _background = None
    _rebin_string = None
    _scale_factor = None
    _load_logs = None
    _calibration_ws = None

    def category(self):
        return "Workflow\\Inelastic;Inelastic\\Indirect"

    def summary(self):
        return "Creates a resolution workspace for an indirect inelastic instrument."

    def PyInit(self):
        self.declareProperty(StringArrayProperty(name="InputFiles"), doc="Comma seperated list if input files")

        self.declareProperty(
            name="Instrument",
            defaultValue="",
            validator=StringListValidator(["IRIS", "OSIRIS", "TOSCA"]),
            doc="Instrument used during run.",
        )
        self.declareProperty(
            name="Analyser",
            defaultValue="",
            validator=StringListValidator(["graphite", "silicon", "mica", "fmica"]),
            doc="Analyser used during run.",
        )
        self.declareProperty(
            name="Reflection",
            defaultValue="",
            validator=StringListValidator(["002", "004", "006", "111", "333"]),
            doc="Reflection used during run.",
        )

        self.declareProperty(
            IntArrayProperty(name="DetectorRange", values=[0, 1]), doc="Range of detectors to use in resolution calculation."
        )
        self.declareProperty(FloatArrayProperty(name="BackgroundRange", values=[0.0, 0.0]), doc="Energy range to use as background.")

        self.declareProperty(name="RebinParam", defaultValue="", doc="Rebinning parameters (min,width,max)")
        self.declareProperty(name="ScaleFactor", defaultValue=1.0, doc="Factor to scale resolution curve by")

        self.declareProperty(name="LoadLogFiles", defaultValue=True, doc="Option to load log files")

        self.declareProperty(
            WorkspaceProperty("CalibrationWorkspace", "", direction=Direction.Input, optional=PropertyMode.Optional),
            doc="Optional calibration workspace; spectra absent from it (by detector ID) are excluded from the resolution sum.",
        )

        self.declareProperty(WorkspaceProperty("OutputWorkspace", "", direction=Direction.Output), doc="Output resolution workspace.")

    def PyExec(self):
        self._setup()

        iet_alg = self.createChildAlgorithm(name="ISISIndirectEnergyTransfer", startProgress=0.0, endProgress=0.7, enableLogging=True)
        iet_alg.setProperty("Instrument", self._instrument)
        iet_alg.setProperty("Analyser", self._analyser)
        iet_alg.setProperty("Reflection", self._reflection)
        iet_alg.setProperty("GroupingMethod", "Individual")
        iet_alg.setProperty("SumFiles", True)
        iet_alg.setProperty("InputFiles", self._input_files)
        iet_alg.setProperty("SpectraRange", self._detector_range)
        iet_alg.setProperty("LoadLogFiles", self._load_logs)
        iet_alg.execute()

        group_ws = iet_alg.getProperty("OutputWorkspace").value
        icon_ws = group_ws.getItem(0).name()

        workflow_prog = Progress(self, start=0.7, end=0.9, nreports=4)

        self._apply_calibration_mask_and_average(icon_ws)

        if self._scale_factor != 1.0:
            workflow_prog.report("Scaling Workspace")
            Scale(InputWorkspace=icon_ws, OutputWorkspace=icon_ws, Factor=self._scale_factor)

        workflow_prog.report("Calculating flat background")
        CalculateFlatBackground(
            InputWorkspace=icon_ws,
            OutputWorkspace=self._out_ws,
            StartX=self._background[0],
            EndX=self._background[1],
            Mode="Mean",
            OutputMode="Subtract Background",
        )

        workflow_prog.report("Rebinning Workspace")
        Rebin(InputWorkspace=self._out_ws, OutputWorkspace=self._out_ws, Params=self._rebin_string)

        workflow_prog.report("Completing Post Processing")
        self._post_process()
        self.setProperty("OutputWorkspace", self._out_ws)

    def _setup(self):
        """
        Gets algorithm properties.
        """

        self._input_files = self.getProperty("InputFiles").value
        self._out_ws = self.getPropertyValue("OutputWorkspace")

        self._instrument = self.getProperty("Instrument").value
        self._analyser = self.getProperty("Analyser").value
        self._reflection = self.getProperty("Reflection").value

        self._detector_range = self.getProperty("DetectorRange").value
        self._background = self.getProperty("BackgroundRange").value
        self._rebin_string = self.getProperty("RebinParam").value
        self._scale_factor = self.getProperty("ScaleFactor").value
        self._load_logs = self.getProperty("LoadLogFiles").value
        self._calibration_ws = self.getProperty("CalibrationWorkspace").value

    def _apply_calibration_mask_and_average(self, ws_name):
        """
        Reduce the per-pixel workspace to a single averaged spectrum, first
        dropping any spectra whose detector IDs are absent from the optional
        calibration workspace (i.e. pixels that IndirectCalibration already
        excluded as edge or low-calibration).
        """
        from mantid.api import AnalysisDataService as ADS

        ws = ADS.retrieve(ws_name)

        if self._calibration_ws is not None:
            calib_det_ids = set()
            for i in range(self._calibration_ws.getNumberHistograms()):
                calib_det_ids.update(self._calibration_ws.getSpectrum(i).getDetectorIDs())
            indices_to_remove = [i for i in range(ws.getNumberHistograms()) if calib_det_ids.isdisjoint(ws.getSpectrum(i).getDetectorIDs())]
            if indices_to_remove:
                RemoveSpectra(InputWorkspace=ws_name, OutputWorkspace=ws_name, WorkspaceIndices=indices_to_remove)
                ws = ADS.retrieve(ws_name)

        GroupDetectors(
            InputWorkspace=ws_name,
            OutputWorkspace=ws_name,
            Behaviour="Average",
            WorkspaceIndexList=list(range(ws.getNumberHistograms())),
        )

    def _post_process(self):
        """
        Handles adding logs, saving and plotting.
        """

        sample_logs = [("res_back_start", self._background[0]), ("res_back_end", self._background[1])]

        if self._scale_factor != 1.0:
            sample_logs.append(("res_scale_factor", self._scale_factor))

        rebin_params = self._rebin_string.split(",")
        if len(rebin_params) == 3:
            sample_logs.append(("rebin_low", rebin_params[0]))
            sample_logs.append(("rebin_width", rebin_params[1]))
            sample_logs.append(("rebin_high", rebin_params[2]))

        log_alg = self.createChildAlgorithm(name="AddSampleLogMultiple", startProgress=0.9, endProgress=1.0, enableLogging=True)
        log_alg.setProperty("Workspace", self._out_ws)
        log_alg.setProperty("LogNames", [log[0] for log in sample_logs])
        log_alg.setProperty("LogValues", [log[1] for log in sample_logs])
        self.setProperty("OutputWorkspace", self._out_ws)
        log_alg.execute()


AlgorithmFactory.subscribe(IndirectResolution)
