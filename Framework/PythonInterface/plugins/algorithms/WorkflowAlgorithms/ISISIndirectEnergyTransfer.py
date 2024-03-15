# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=invalid-name,too-many-instance-attributes,too-many-branches,no-init,deprecated-module
from mantid.kernel import *
from mantid.api import *
from mantid.simpleapi import *
from mantid import config

import os


def _str_or_none(s):
    return s if s != "" else None


def _ws_or_none(s):
    return mtd[s] if s != "" else None


def _elems_or_none(l):
    return l if len(l) != 0 else None


def add_missing_elements(from_list, to_list):
    to_list.extend([element for element in from_list if element not in to_list])
    return sorted(to_list)


class ISISIndirectEnergyTransfer(DataProcessorAlgorithm):
    _chopped_data = None
    _data_files = None
    _load_logs = None
    _calibration_ws = None
    _instrument_name = None
    _analyser = None
    _reflection = None
    _efixed = None
    _spectra_range = None
    _background_range = None
    _rebin_string = None
    _detailed_balance = None
    _scale_factor = None
    _fold_multiple_frames = None
    _grouping_method = None
    _grouping_ws = None
    _grouping_string = None
    _grouping_file = None
    _output_x_units = None
    _output_ws = None
    _sum_files = None
    _ipf_filename = None
    _workspace_names = None

    def alias(self):
        """Alternative name for algorithm"""
        return "ISISIndirectEnergyTransferWrapper"

    def category(self):
        return "Workflow\\Inelastic;Inelastic\\Indirect"

    def summary(self):
        return "Runs an energy transfer reduction for an inelastic indirect geometry instrument."

    def PyInit(self):
        # Input properties
        self.declareProperty(StringArrayProperty(name="InputFiles"), doc="Comma separated list of input files")

        self.declareProperty(name="SumFiles", defaultValue=False, doc="Toggle input file summing or sequential processing")

        self.declareProperty(name="LoadLogFiles", defaultValue=True, doc="Load log files when loading runs")

        self.declareProperty(
            WorkspaceProperty("CalibrationWorkspace", "", direction=Direction.Input, optional=PropertyMode.Optional),
            doc="Workspace containing calibration data",
        )

        # Instrument configuration properties
        self.declareProperty(
            name="Instrument",
            defaultValue="",
            validator=StringListValidator(["IRIS", "OSIRIS", "TOSCA", "TFXA"]),
            doc="Instrument used during run.",
        )
        self.declareProperty(
            name="Analyser",
            defaultValue="",
            validator=StringListValidator(["graphite", "mica", "fmica", "silicon"]),
            doc="Analyser bank used during run.",
        )
        self.declareProperty(
            name="Reflection",
            defaultValue="",
            validator=StringListValidator(["002", "004", "006", "111", "333"]),
            doc="Reflection number for instrument setup during run.",
        )

        self.declareProperty(
            name="Efixed",
            defaultValue=Property.EMPTY_DBL,
            validator=FloatBoundedValidator(0.0),
            doc="Overrides the default Efixed value for the analyser/reflection selection.",
        )

        self.declareProperty(
            IntArrayProperty(name="SpectraRange", values=[0, 1], validator=IntArrayMandatoryValidator()),
            doc="Comma separated range of spectra number to use.",
        )
        self.declareProperty(
            FloatArrayProperty(name="BackgroundRange"), doc="Range of background to subtract from raw data in time of flight."
        )
        self.declareProperty(name="RebinString", defaultValue="", doc="Rebin string parameters.")
        self.declareProperty(name="DetailedBalance", defaultValue=Property.EMPTY_DBL, doc="")
        self.declareProperty(name="ScaleFactor", defaultValue=1.0, doc="Factor by which to scale result.")
        self.declareProperty(name="FoldMultipleFrames", defaultValue=True, doc="Folds multiple framed data sets into a single workspace.")

        # Spectra grouping options
        self.declareProperty(
            name="GroupingMethod",
            defaultValue="IPF",
            validator=StringListValidator(["Individual", "All", "File", "Workspace", "IPF", "Custom", "Groups"]),
            doc="The method used to group detectors.",
        )
        self.declareProperty(
            WorkspaceProperty("GroupingWorkspace", "", direction=Direction.Input, optional=PropertyMode.Optional),
            doc="A workspace containing a detector grouping.",
        )
        self.declareProperty(name="GroupingString", defaultValue="", direction=Direction.Input, doc="Detectors to group as a string")
        self.declareProperty(
            FileProperty("MapFile", "", action=FileAction.OptionalLoad, extensions=[".map"]),
            doc="This property is deprecated (since v6.10), please use the 'GroupingFile' property instead.",
        )
        self.declareProperty(
            FileProperty("GroupingFile", "", action=FileAction.OptionalLoad, extensions=[".map"]),
            doc="A file containing a detector grouping.",
        )
        self.declareProperty(
            name="NGroups",
            defaultValue=1,
            validator=IntBoundedValidator(lower=1),
            direction=Direction.Input,
            doc="The number of groups for grouping the detectors.",
        )
        # Output properties
        self.declareProperty(
            name="UnitX",
            defaultValue="DeltaE",
            validator=StringListValidator(["DeltaE", "DeltaE_inWavenumber"]),
            doc="X axis units for the result workspace.",
        )

        self.declareProperty(
            WorkspaceGroupProperty("OutputWorkspace", "", direction=Direction.Output), doc="Workspace group for the resulting workspaces."
        )

    # pylint: disable=too-many-locals
    def PyExec(self):
        from IndirectReductionCommon import (
            load_files,
            get_multi_frame_rebin,
            get_detectors_to_mask,
            unwrap_monitor,
            process_monitor_efficiency,
            scale_monitor,
            scale_detectors,
            rebin_reduction,
            group_spectra,
            fold_chopped,
            rename_reduction,
            mask_detectors,
        )

        self._setup()
        load_prog = Progress(self, start=0.0, end=0.10, nreports=2)
        load_prog.report("loading files")
        self._workspace_names, self._chopped_data, masked_detectors = load_files(
            self._data_files,
            self._ipf_filename,
            self._spectra_range[0],
            self._spectra_range[1],
            self._sum_files,
            self._load_logs,
            None,
            self._sum_files,
        )
        load_prog.report("files loaded")

        process_prog = Progress(self, start=0.1, end=0.9, nreports=len(self._workspace_names))
        for c_ws_name in self._workspace_names:
            process_prog.report("processing workspace" + c_ws_name)
            is_multi_frame = isinstance(mtd[c_ws_name], WorkspaceGroup)

            # Get list of workspaces
            if is_multi_frame:
                workspaces = mtd[c_ws_name].getNames()
            else:
                workspaces = [c_ws_name]

            # Process rebinning for framed data
            rebin_string_2, num_bins = get_multi_frame_rebin(c_ws_name, self._rebin_string)

            if not self._sum_files:
                masked_detectors = get_detectors_to_mask(workspaces)
            else:
                summed_file_masked_detectors = get_detectors_to_mask(workspaces)
                masked_detectors = add_missing_elements(summed_file_masked_detectors, masked_detectors)

            # Process workspaces
            for ws_name in workspaces:
                # Set Efixed if given to algorithm
                if self._efixed != Property.EMPTY_DBL:
                    SetInstrumentParameter(
                        Workspace=ws_name,
                        ComponentName=self._analyser,
                        ParameterName="Efixed",
                        ParameterType="Number",
                        Value=str(self._efixed),
                    )

                monitor_ws_name = ws_name + "_mon"

                # Process monitor
                if not unwrap_monitor(ws_name):
                    ConvertUnits(InputWorkspace=monitor_ws_name, OutputWorkspace=monitor_ws_name, Target="Wavelength", EMode="Elastic")

                process_monitor_efficiency(ws_name)
                scale_monitor(ws_name)

                # Do background removal if a range was provided
                if self._background_range is not None:
                    ConvertToDistribution(Workspace=ws_name)
                    CalculateFlatBackground(
                        InputWorkspace=ws_name,
                        OutputWorkspace=ws_name,
                        StartX=self._background_range[0],
                        EndX=self._background_range[1],
                        Mode="Mean",
                    )
                    ConvertFromDistribution(Workspace=ws_name)

                # Divide by the calibration workspace if one was provided
                if self._calibration_ws is not None:
                    index_min = self._calibration_ws.getIndexFromSpectrumNumber(int(self._spectra_range[0]))
                    index_max = self._calibration_ws.getIndexFromSpectrumNumber(int(self._spectra_range[1]))

                    CropWorkspace(
                        InputWorkspace=self._calibration_ws,
                        OutputWorkspace="__cropped_calib",
                        StartWorkspaceIndex=index_min,
                        EndWorkspaceIndex=index_max,
                    )

                    Divide(LHSWorkspace=ws_name, RHSWorkspace="__cropped_calib", OutputWorkspace=ws_name)

                    DeleteWorkspace("__cropped_calib")

                # Scale detector data by monitor intensities
                scale_detectors(ws_name, "Indirect")

                # Remove the no longer needed monitor workspace
                DeleteWorkspace(monitor_ws_name)

                # Convert to energy
                ConvertUnits(InputWorkspace=ws_name, OutputWorkspace=ws_name, Target="DeltaE", EMode="Indirect")
                CorrectKiKf(InputWorkspace=ws_name, OutputWorkspace=ws_name, EMode="Indirect")

                # Mask noisy detectors
                if len(masked_detectors) > 0:
                    mask_detectors(ws_name, masked_detectors)

                # Handle rebinning
                rebin_reduction(ws_name, self._rebin_string, rebin_string_2, num_bins)

                # Detailed balance
                if self._detailed_balance != Property.EMPTY_DBL:
                    corr_factor = 11.606 / (2 * self._detailed_balance)
                    ExponentialCorrection(InputWorkspace=ws_name, OutputWorkspace=ws_name, C0=1.0, C1=corr_factor, Operation="Multiply")

                # Scale
                if self._scale_factor != 1.0:
                    Scale(InputWorkspace=ws_name, OutputWorkspace=ws_name, Factor=self._scale_factor, Operation="Multiply")

                # Group spectra
                grouped = group_spectra(
                    ws_name,
                    method=self._grouping_method,
                    group_file=self._grouping_file,
                    group_ws=self._grouping_ws,
                    group_string=self._grouping_string,
                    number_of_groups=self._number_of_groups,
                    spectra_range=self._spectra_range,
                )
                AnalysisDataService.addOrReplace(ws_name, grouped)

            if self._fold_multiple_frames and is_multi_frame:
                fold_chopped(c_ws_name)

            # Convert to output units if needed
            if self._output_x_units != "DeltaE":
                ConvertUnits(InputWorkspace=c_ws_name, OutputWorkspace=c_ws_name, EMode="Indirect", Target=self._output_x_units)

        # Rename output workspaces
        output_workspace_names = [rename_reduction(ws_name, self._sum_files) for ws_name in self._workspace_names]

        summary_prog = Progress(self, start=0.9, end=1.0, nreports=4)

        # Group result workspaces
        summary_prog.report("grouping workspaces")
        self.output_ws = GroupWorkspaces(InputWorkspaces=output_workspace_names, OutputWorkspace=self._output_ws)

        # The spectrum numbers need to start at 1 not 0 if spectra are grouped
        if self.output_ws.getNumberOfEntries() == 1:
            for i in range(len(self.output_ws.getItem(0).getSpectrumNumbers())):
                self.output_ws.getItem(0).getSpectrum(i).setSpectrumNo(i + 1)

        self.setProperty("OutputWorkspace", mtd[self._output_ws])

        summary_prog.report("Algorithm complete")

    def validateInputs(self):
        """
        Validates algorithm properties.
        """
        issues = dict()

        # Validate the instrument configuration by checking if a parameter file exists
        instrument_name = self.getPropertyValue("Instrument")
        analyser = self.getPropertyValue("Analyser")
        reflection = self.getPropertyValue("Reflection")

        ipf_filename = os.path.join(
            config["instrumentDefinition.directory"], instrument_name + "_" + analyser + "_" + reflection + "_Parameters.xml"
        )

        if not os.path.exists(ipf_filename):
            error_message = "Invalid instrument configuration"
            issues["Instrument"] = error_message
            issues["Analyser"] = error_message
            issues["Reflection"] = error_message

        # Validate spectra range
        spectra_range = self.getProperty("SpectraRange").value
        if len(spectra_range) != 2:
            issues["SpectraRange"] = "Range must contain exactly two items"
        elif spectra_range[0] > spectra_range[1]:
            issues["SpectraRange"] = "Range must be in format: lower,upper"

        # Validate background range
        background_range = _elems_or_none(self.getProperty("BackgroundRange").value)
        if background_range is not None:
            if len(background_range) != 2:
                issues["BackgroundRange"] = "Range must contain exactly two items"
            elif background_range[0] > background_range[1]:
                issues["BackgroundRange"] = "Range must be in format: lower,upper"

        # Validate grouping method
        grouping_method = self.getPropertyValue("GroupingMethod")
        grouping_ws = _ws_or_none(self.getPropertyValue("GroupingWorkspace"))

        if grouping_method == "Workspace" and grouping_ws is None:
            issues["GroupingWorkspace"] = "Must select a grouping workspace for current GroupingWorkspace"

        map_file = _str_or_none(self.getPropertyValue("MapFile"))
        if map_file is not None:
            logger.warning(
                "The 'MapFile' algorithm property has been deprecated (since v6.10). Please use the 'GroupingFile' "
                "algorithm property instead."
            )

        efixed = self.getProperty("Efixed").value
        if efixed != Property.EMPTY_DBL and instrument_name not in ["IRIS", "OSIRIS"]:
            issues["Efixed"] = "Can only override Efixed on IRIS and OSIRIS"

        return issues

    def _setup(self):
        """
        Gets algorithm properties.
        """

        # Get properties
        self._data_files = self.getProperty("InputFiles").value
        self._sum_files = self.getProperty("SumFiles").value
        self._load_logs = self.getProperty("LoadLogFiles").value
        self._calibration_ws = _ws_or_none(self.getPropertyValue("CalibrationWorkspace"))

        self._instrument_name = self.getPropertyValue("Instrument")
        self._analyser = self.getPropertyValue("Analyser")
        self._reflection = self.getPropertyValue("Reflection")
        self._efixed = self.getProperty("Efixed").value

        self._spectra_range = self.getProperty("SpectraRange").value
        self._background_range = _elems_or_none(self.getProperty("BackgroundRange").value)
        self._rebin_string = _str_or_none(self.getPropertyValue("RebinString"))
        self._detailed_balance = self.getProperty("DetailedBalance").value
        self._scale_factor = self.getProperty("ScaleFactor").value
        self._fold_multiple_frames = self.getProperty("FoldMultipleFrames").value

        self._grouping_method = self.getPropertyValue("GroupingMethod")
        self._grouping_ws = _ws_or_none(self.getPropertyValue("GroupingWorkspace"))
        self._grouping_string = _str_or_none(self.getPropertyValue("GroupingString"))
        map_file = _str_or_none(self.getPropertyValue("MapFile"))
        grouping_file = _str_or_none(self.getPropertyValue("GroupingFile"))
        # 'MapFile' is deprecated, but if it is provided instead of 'GroupingFile' then try to use it anyway
        self._grouping_file = map_file if map_file is not None and grouping_file is None else grouping_file
        self._number_of_groups = self.getProperty("NGroups").value

        self._output_x_units = self.getPropertyValue("UnitX")

        self._output_ws = self.getPropertyValue("OutputWorkspace")

        # Disable sum files if there is only one file
        if len(self._data_files) == 1:
            if self._sum_files:
                logger.warning("SumFiles disabled when only one input file is provided.")
            self._sum_files = False

        # Get the IPF filename
        self._ipf_filename = os.path.join(
            config["instrumentDefinition.directory"],
            self._instrument_name + "_" + self._analyser + "_" + self._reflection + "_Parameters.xml",
        )
        logger.information("Instrument parameter file: %s" % self._ipf_filename)

        # Warn when grouping options are to be ignored
        if self._grouping_method != "Workspace" and self._grouping_ws is not None:
            logger.warning("GroupingWorkspace will be ignored by selected GroupingMethod")

        if self._grouping_method != "File" and self._grouping_file is not None:
            logger.warning("GroupingFile will be ignored by selected GroupingMethod")

        # The list of workspaces being processed
        self._workspace_names = []


# Register algorithm with Mantid
AlgorithmFactory.subscribe(ISISIndirectEnergyTransfer)
