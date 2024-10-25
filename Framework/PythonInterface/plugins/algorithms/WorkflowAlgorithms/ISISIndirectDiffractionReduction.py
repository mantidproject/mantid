# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=no-init,too-many-instance-attributes
import os

from IndirectReductionCommon import (
    calibrate,
    fold_chopped,
    get_multi_frame_rebin,
    group_spectra,
    load_files,
    load_file_ranges,
    identify_bad_detectors,
    mask_detectors,
    process_monitor_efficiency,
    scale_monitor,
    scale_detectors,
    rebin_logarithmic,
    rebin_reduction,
    rename_reduction,
    unwrap_monitor,
)

from mantid.api import (
    mtd,
    AlgorithmFactory,
    AnalysisDataService,
    DataProcessorAlgorithm,
    FileAction,
    FileProperty,
    PropertyMode,
    WorkspaceGroup,
    WorkspaceGroupProperty,
    WorkspaceProperty,
)
from mantid.kernel import logger, Direction, IntArrayProperty, IntBoundedValidator, StringArrayProperty, StringListValidator
from mantid.simpleapi import ConvertUnits, DeleteWorkspace, Divide, GroupWorkspaces, Minus, RebinToWorkspace, ReplaceSpecialValues, Scale
from mantid import config


def _str_or_none(string):
    return string if string != "" else None


def _ws_or_none(workspace_name):
    return mtd[workspace_name] if workspace_name != "" else None


def is_range_ascending(range_string, delimiter):
    range_limits = tuple(map(int, range_string.split(delimiter)))
    return range_limits[0] < range_limits[1]


def contains_non_ascending_range(strings, delimiter):
    range_strings = list(filter(lambda x: delimiter in x, strings))
    for range_string in range_strings:
        if not is_range_ascending(range_string, delimiter):
            return True
    return False


def find_minimum_non_zero_y_in_spectrum(y_minimum, spectrum):
    positive_y = list(filter(lambda x: x > 0, spectrum))
    y_spec_min = min(positive_y) if len(positive_y) > 0 else None
    if y_spec_min and (y_minimum is None or y_spec_min < y_minimum):
        return y_spec_min
    return y_minimum


def find_minimum_non_zero_y_in_workspace(workspace):
    y_minimum = None
    for idx in range(0, workspace.getNumberHistograms()):
        y_minimum = find_minimum_non_zero_y_in_spectrum(y_minimum, workspace.readY(idx))
    return y_minimum


class ISISIndirectDiffractionReduction(DataProcessorAlgorithm):
    _workspace_names = None
    _cal_file = None
    _chopped_data = None
    _output_ws = None
    _data_files = None
    _container_workspace = None
    _container_data_files = None
    _container_scale_factor = None
    _load_logs = None
    _instrument_name = None
    _mode = None
    _spectra_range = None
    _grouping_method = None
    _rebin_string = None
    _ipf_filename = None
    _sum_files = None
    _vanadium_ws = None
    _replace_zeros_name = "_replace_zeros"

    # ------------------------------------------------------------------------------

    def category(self):
        return "Diffraction\\Reduction"

    def summary(self):
        return "Performs a diffraction reduction for a set of raw run files for an ISIS indirect spectrometer"

    def seeAlso(self):
        return ["AlignDetectors", "DiffractionFocussing", "SNSPowderReduction"]

        # ------------------------------------------------------------------------------

    def PyInit(self):
        self.declareProperty(
            StringArrayProperty(name="InputFiles"),
            doc="Comma separated list of input files formatted as a string; "
            "this can also incorporate +, -, or : in the same fashion as "
            "any string accepted by Load",
        )

        self.declareProperty(
            StringArrayProperty(name="ContainerFiles"),
            doc="Comma separated list of input files for the empty container runs "
            "formatted as a string; this can also incorporate +, -, or : in "
            "the same fashion as any string accepted by Load",
        )

        self.declareProperty("ContainerScaleFactor", 1.0, doc="Factor by which to scale the container runs.")

        self.declareProperty(
            FileProperty("CalFile", "", action=FileAction.OptionalLoad),
            doc="Filename of the .cal file to use in the [[ApplyDiffCal]] and [[ConvertUnits]] child algorithms.",
        )

        self.declareProperty(
            FileProperty("InstrumentParFile", "", action=FileAction.OptionalLoad, extensions=[".dat", ".par"]),
            doc="PAR file containing instrument definition. For VESUVIO only",
        )

        self.declareProperty(StringArrayProperty(name="VanadiumFiles"), doc="Comma separated array of vanadium runs")

        self.declareProperty(name="SumFiles", defaultValue=False, doc="Enabled to sum spectra from each input file.")

        self.declareProperty(name="LoadLogFiles", defaultValue=True, doc="Load log files when loading runs")

        self.declareProperty(
            name="Instrument",
            defaultValue="IRIS",
            validator=StringListValidator(["IRIS", "OSIRIS", "TOSCA", "VESUVIO"]),
            doc="Instrument used for run",
        )

        self.declareProperty(
            name="Mode", defaultValue="diffspec", validator=StringListValidator(["diffspec", "diffonly"]), doc="Diffraction mode used"
        )

        self.declareProperty(IntArrayProperty(name="SpectraRange"), doc="Range of spectra to use.")

        self.declareProperty(name="RebinParam", defaultValue="", doc="Rebin parameters.")

        self.declareProperty(
            name="GroupingPolicy",
            defaultValue="",
            validator=StringListValidator(["", "All", "File", "Workspace", "Custom", "Groups"]),
            doc="This property is deprecated (since v6.10), please use the 'GroupingMethod' property instead.",
        )
        self.declareProperty(
            name="GroupingMethod",
            defaultValue="All",
            validator=StringListValidator(["All", "File", "Workspace", "Custom", "Groups"]),
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

        self.declareProperty(
            WorkspaceGroupProperty("OutputWorkspace", "", direction=Direction.Output), doc="Group name for the result workspaces."
        )

    # ------------------------------------------------------------------------------

    def validateInputs(self):
        """
        Checks for issues with user input.
        """
        issues = dict()

        # Validate input files
        input_files = self.getProperty("InputFiles").value

        if len(input_files) == 0:
            issues["InputFiles"] = "InputFiles must contain at least one filename"

        if contains_non_ascending_range(input_files, "-"):
            issues["InputFiles"] = "Run number ranges must go from low to high"

        # Validate detector range
        detector_range = self.getProperty("SpectraRange").value
        if len(detector_range) != 2:
            issues["SpectraRange"] = "SpectraRange must be an array of 2 values only"
        else:
            if detector_range[0] > detector_range[1]:
                issues["SpectraRange"] = "SpectraRange must be in format [lower_index,upper_index]"

        cal_file = self.getProperty("CalFile").value
        inst = self.getProperty("Instrument").value
        mode = self.getProperty("Mode").value
        if cal_file != "":
            if inst != "OSIRIS":
                logger.warning("NOT OSIRIS, inst = " + str(inst))
                logger.warning("type = " + str(type(inst)))
                issues["CalFile"] = "Cal Files are currently only available for use in OSIRIS diffspec mode"
            if mode != "diffspec":
                logger.warning("NOT DIFFSPEC, mode = " + str(mode))
                logger.warning("type = " + str(type(mode)))
                issues["CalFile"] = "Cal Files are currently only available for use in OSIRIS diffspec mode"

        # Validate grouping method
        grouping_method = self.getPropertyValue("GroupingMethod")
        grouping_policy = self.getPropertyValue("GroupingPolicy")

        if grouping_policy != "":
            logger.warning(
                "The 'GroupingPolicy' algorithm property has been deprecated (since v6.10). Please use the 'GroupingMethod' "
                "algorithm property instead."
            )

        grouping_ws = _ws_or_none(self.getPropertyValue("GroupingWorkspace"))
        if (grouping_method == "Workspace" or grouping_policy == "Workspace") and grouping_ws is None:
            issues["GroupingWorkspace"] = "Must select a grouping workspace for current GroupingWorkspace"

        map_file = _str_or_none(self.getPropertyValue("MapFile"))
        if map_file is not None:
            logger.warning(
                "The 'MapFile' algorithm property has been deprecated (since v6.10). Please use the 'GroupingFile' "
                "algorithm property instead."
            )

        return issues

    # ------------------------------------------------------------------------------

    def PyExec(self):
        self._setup()

        load_opts = dict()
        if self._instrument_name == "VESUVIO":
            load_opts["InstrumentParFile"] = self._par_filename
            load_opts["Mode"] = "FoilOut"
            load_opts["LoadMonitors"] = True

        self._workspace_names, self._chopped_data = load_file_ranges(
            self._data_files,
            self._ipf_filename,
            self._spectra_range[0],
            self._spectra_range[1],
            sum_files=self._sum_files,
            load_logs=self._load_logs,
            load_opts=load_opts,
        )

        # Load vanadium runs if given
        if self._vanadium_runs:
            self._vanadium_ws, _, _ = load_files(
                self._vanadium_runs,
                self._ipf_filename,
                self._spectra_range[0],
                self._spectra_range[1],
                load_logs=self._load_logs,
                load_opts=load_opts,
            )

            if len(self._workspace_names) > len(self._vanadium_runs):
                raise RuntimeError("There cannot be more sample runs than vanadium runs.")

        # applies the changes in the provided calibration file
        self._apply_calibration()
        # Load container if run is given
        self._load_and_scale_container(self._container_scale_factor, load_opts)

        for index, c_ws_name in enumerate(self._workspace_names):
            is_multi_frame = isinstance(mtd[c_ws_name], WorkspaceGroup)

            # Get list of workspaces
            if is_multi_frame:
                workspaces = mtd[c_ws_name].getNames()
            else:
                workspaces = [c_ws_name]

            # Process rebinning for framed data
            rebin_string_2, num_bins = get_multi_frame_rebin(c_ws_name, self._rebin_string)

            masked_detectors = identify_bad_detectors(workspaces[0])

            # Process workspaces
            for ws_name in workspaces:
                monitor_ws_name = ws_name + "_mon"

                # Subtract empty container if there is one
                if self._container_workspace is not None:
                    Minus(LHSWorkspace=ws_name, RHSWorkspace=self._container_workspace, OutputWorkspace=ws_name)

                if self._vanadium_ws:
                    van_ws_name = self._vanadium_ws[index]
                    van_ws = mtd[van_ws_name]
                    if self._container_workspace is not None:
                        cont_ws = mtd[self._container_workspace]

                        if van_ws.blocksize() > cont_ws.blocksize():
                            RebinToWorkspace(
                                WorkspaceToRebin=van_ws_name, WorkspaceToMatch=self._container_workspace, OutputWorkspace=van_ws_name
                            )
                        elif cont_ws.blocksize() > van_ws.blocksize():
                            RebinToWorkspace(
                                WorkspaceToRebin=self._container_workspace,
                                WorkspaceToMatch=van_ws_name,
                                OutputWorkspace=self._container_workspace,
                            )

                        Minus(LHSWorkspace=van_ws_name, RHSWorkspace=self._container_workspace, OutputWorkspace=van_ws_name)

                    if mtd[ws_name].blocksize() > van_ws.blocksize():
                        RebinToWorkspace(WorkspaceToRebin=ws_name, WorkspaceToMatch=van_ws_name, OutputWorkspace=ws_name)
                    elif van_ws.blocksize() > mtd[ws_name].blocksize():
                        RebinToWorkspace(WorkspaceToRebin=van_ws_name, WorkspaceToMatch=ws_name, OutputWorkspace=van_ws_name)

                    replacement_value = 0.1 * find_minimum_non_zero_y_in_workspace(van_ws)
                    logger.information("Replacing zeros in {0} with {1}.".format(van_ws_name, replacement_value))
                    ReplaceSpecialValues(
                        InputWorkspace=van_ws_name,
                        SmallNumberThreshold=0.0000001,
                        SmallNumberValue=replacement_value,
                        OutputWorkspace=self._replace_zeros_name,
                    )

                    Divide(
                        LHSWorkspace=ws_name,
                        RHSWorkspace=self._replace_zeros_name,
                        OutputWorkspace=ws_name,
                        AllowDifferentNumberSpectra=True,
                    )

                    DeleteWorkspace(self._replace_zeros_name)

                # Process monitor
                if not unwrap_monitor(ws_name):
                    ConvertUnits(InputWorkspace=monitor_ws_name, OutputWorkspace=monitor_ws_name, Target="Wavelength", EMode="Elastic")

                process_monitor_efficiency(ws_name)
                scale_monitor(ws_name)

                # Scale detector data by monitor intensities
                scale_detectors(ws_name, "Elastic")

                # Remove the no longer needed monitor workspace
                DeleteWorkspace(monitor_ws_name)

                # Convert to dSpacing
                ConvertUnits(InputWorkspace=ws_name, OutputWorkspace=ws_name, Target="dSpacing", EMode="Elastic")

                # Mask noisy detectors
                if len(masked_detectors) > 0:
                    mask_detectors(ws_name, masked_detectors)

                # Handle rebinning
                rebin_reduction(ws_name, self._rebin_string, rebin_string_2, num_bins)

                # Group spectra
                grouped = group_spectra(
                    ws_name,
                    method=self._grouping_method,
                    group_file=self._grouping_file,
                    group_ws=self._grouping_workspace,
                    group_string=self._grouping_string,
                    number_of_groups=self._number_of_groups,
                    spectra_range=self._spectra_range,
                )
                AnalysisDataService.addOrReplace(ws_name, grouped)

            if is_multi_frame:
                fold_chopped(c_ws_name)

        # Remove the container workspaces
        if self._container_workspace is not None:
            self._delete_all([self._container_workspace])

        # Remove the vanadium workspaces
        if self._vanadium_ws:
            self._delete_all(self._vanadium_ws)

        # Rename output workspaces
        output_workspace_names = [rename_reduction(ws_name, self._sum_files) for ws_name in self._workspace_names]

        # Group result workspaces
        GroupWorkspaces(InputWorkspaces=output_workspace_names, OutputWorkspace=self._output_ws)

        self.setProperty("OutputWorkspace", self._output_ws)

    # ------------------------------------------------------------------------------

    def _setup(self):
        """
        Gets algorithm properties.
        """
        self._output_ws = self.getPropertyValue("OutputWorkspace")
        self._data_files = self.getProperty("InputFiles").value
        self._container_data_files = self.getProperty("ContainerFiles").value
        self._cal_file = self.getProperty("CalFile").value
        self._par_filename = self.getPropertyValue("InstrumentParFile")
        self._vanadium_runs = self.getProperty("VanadiumFiles").value
        self._container_scale_factor = self.getProperty("ContainerScaleFactor").value
        self._load_logs = self.getProperty("LoadLogFiles").value
        self._instrument_name = self.getPropertyValue("Instrument")
        self._mode = self.getPropertyValue("Mode")
        self._spectra_range = self.getProperty("SpectraRange").value
        self._rebin_string = self.getPropertyValue("RebinParam")

        grouping_policy = self.getPropertyValue("GroupingPolicy")
        grouping_method = self.getPropertyValue("GroupingMethod")
        # 'GroupingPolicy' is deprecated, but if it is provided instead of 'GroupingMethod' then try to use it anyway
        self._grouping_method = grouping_policy if grouping_policy != "" and grouping_method == "All" else grouping_method
        self._grouping_workspace = _ws_or_none(self.getPropertyValue("GroupingWorkspace"))
        self._grouping_string = _str_or_none(self.getPropertyValue("GroupingString"))
        map_file = _str_or_none(self.getPropertyValue("MapFile"))
        grouping_file = _str_or_none(self.getPropertyValue("GroupingFile"))
        # 'MapFile' is deprecated, but if it is provided instead of 'GroupingFile' then try to use it anyway
        self._grouping_file = map_file if map_file is not None and grouping_file is None else grouping_file
        self._number_of_groups = self.getProperty("NGroups").value

        if self._rebin_string == "":
            self._rebin_string = None

        self._container_workspace = None
        if len(self._container_data_files) == 0:
            self._container_data_files = None

        self._vanadium_ws = None
        if len(self._vanadium_runs) == 0:
            self._vanadium_runs = None

        # Get the IPF filename
        self._ipf_filename = self._instrument_name + "_diffraction_" + self._mode + "_Parameters.xml"
        if not os.path.exists(self._ipf_filename):
            self._ipf_filename = os.path.join(config["instrumentDefinition.directory"], self._ipf_filename)
        logger.information("IPF filename is: %s" % self._ipf_filename)

        if len(self._data_files) == 1:
            logger.warning("SumFiles options has no effect when only one file is provided")
        # Only enable sum files if we actually have more than one file
        self._sum_files = self.getProperty("SumFiles").value

    def _apply_calibration(self):
        """
        Checks to ensure a calibration file has been given and if so performs a calibration and
        a logarithmic rebinning for the spectra which are not excluded from the grouping.
        """
        if self._cal_file != "":
            for ws_name in self._workspace_names:
                calibrated = calibrate(ws_name, self._cal_file)
                rebinned = rebin_logarithmic(calibrated, self._cal_file)
                AnalysisDataService.addOrReplace(ws_name, rebinned)

            if self._vanadium_ws:
                for van_ws_name in self._vanadium_ws:
                    calibrated = calibrate(van_ws_name, self._cal_file)
                    rebinned = rebin_logarithmic(calibrated, self._cal_file)
                    AnalysisDataService.addOrReplace(van_ws_name, rebinned)

    def _load_and_scale_container(self, scale_factor, load_opts):
        """
        Loads the container file if given
        Applies the scale factor to the container if not 1.
        """
        if self._container_data_files is not None:
            self._container_workspace, _, _ = load_files(
                self._container_data_files,
                self._ipf_filename,
                self._spectra_range[0],
                self._spectra_range[1],
                sum_files=True,
                load_logs=self._load_logs,
                load_opts=load_opts,
            )
            self._container_workspace = self._container_workspace[0]

            # Scale container if factor is given
            if scale_factor != 1.0:
                Scale(
                    InputWorkspace=self._container_workspace,
                    OutputWorkspace=self._container_workspace,
                    Factor=scale_factor,
                    Operation="Multiply",
                )

    def _delete_all(self, workspace_names):
        """
        Deletes the workspaces with the specified names and their associated
        monitor workspaces.

        :param workspace_names: The names of the workspaces to delete.
        """

        for workspace_name in workspace_names:
            DeleteWorkspace(workspace_name)

            if mtd.doesExist(workspace_name + "_mon"):
                DeleteWorkspace(workspace_name + "_mon")


# ------------------------------------------------------------------------------
AlgorithmFactory.subscribe(ISISIndirectDiffractionReduction)
