# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2023 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantid.utils.reflectometry.orso_helper import MantidORSODataColumns, MantidORSODataset, MantidORSOSaver
from mantid.utils.reflectometry import SpinStatesORSO

from mantid.kernel import (
    Direction,
    config,
    StringArrayLengthValidator,
    StringArrayMandatoryValidator,
    StringArrayProperty,
    CompositeValidator,
)
from mantid.api import AlgorithmFactory, FileProperty, FileAction, PythonAlgorithm, AnalysisDataService, WorkspaceGroup

from pathlib import Path
from typing import Optional, Tuple, Union, List
import re
from collections import OrderedDict
import numpy as np


class Prop:
    WORKSPACE_LIST = "WorkspaceList"
    WRITE_RESOLUTION = "WriteResolution"
    INCLUDE_EXTRA_COLS = "IncludeAdditionalColumns"
    FILENAME = "Filename"


class ReflectometryDataset:
    REDUCTION_WORKFLOW_ALG = "ReflectometryISISLoadAndProcess"
    CONVERT_ALG = "ConvertUnits"
    REF_ROI_ALG = "RefRoi"

    _REDUCTION_ALG = "ReflectometryReductionOneAuto"
    _RRO_ALG = "ReflectometryReductionOne"
    _STITCH_ALG = "Stitch1DMany"

    def __init__(self, ws, is_ws_grp_member: bool):
        self._name: str = ""
        self._ws = ws
        self._is_ws_grp_member: bool = is_ws_grp_member
        self._reduction_history = None
        self._reduction_workflow_histories = []
        self._stitch_history = None
        self._q_conversion_history = None
        self._q_conversion_theta: Optional[float] = None
        self._spin_state: str = ""

        self._populate_histories()
        self._populate_q_conversion_info()
        self._set_spin_state_from_logs()
        self._set_name()

    @property
    def name(self) -> str:
        return self._name

    @property
    def ws(self):
        return self._ws

    @property
    def is_ws_grp_member(self) -> bool:
        return self._is_ws_grp_member

    @property
    def instrument_name(self) -> str:
        return self._ws.getInstrument().getName()

    @property
    def spin_state(self) -> str:
        return self._spin_state

    @property
    def reduction_history(self):
        return self._reduction_history

    @property
    def reduction_workflow_histories(self):
        return self._reduction_workflow_histories

    @property
    def stitch_history(self):
        return self._stitch_history

    @property
    def is_stitched(self) -> bool:
        return self._stitch_history is not None

    @property
    def is_polarized(self) -> bool:
        return len(self._spin_state) > 0

    @property
    def q_conversion_history(self):
        return self._q_conversion_history

    @property
    def q_conversion_theta(self) -> Optional[float]:
        return self._q_conversion_theta

    def _populate_histories(self):
        ws_history = self._ws.getHistory()
        if ws_history.empty():
            return

        for history in ws_history.getAlgorithmHistories():
            if history.name() == self.REDUCTION_WORKFLOW_ALG:
                self._reduction_workflow_histories.append(history)
            elif history.name() == self._STITCH_ALG:
                # We want the last call to the stitch algorithm in the history
                # (we would normally expect there to be only one)
                self._stitch_history = history

        # Get the last occurrence of the reduction algorithm in the workspace history
        for history in reversed(ws_history.getAlgorithmHistories()):
            if history.name() == self._REDUCTION_ALG:
                self._reduction_history = history
                return
            elif history.name() == self.REDUCTION_WORKFLOW_ALG:
                for child_history in reversed(history.getChildHistories()):
                    if child_history.name() == self._REDUCTION_ALG:
                        self._reduction_history = child_history
                        return

    def _populate_q_conversion_info(self):
        if self.is_stitched or self._reduction_history is None:
            # Q conversion information isn't relevant for a stitched dataset
            return

        for child in self._reduction_history.getChildHistories():
            if child.name() == self._RRO_ALG:
                rro_child_algs = child.getChildHistories()
                if rro_child_algs:
                    self._q_conversion_history = rro_child_algs[-1]
                    break

        if self._q_conversion_history is None:
            return

        if self._q_conversion_history.name() == self.REF_ROI_ALG:
            self._q_conversion_theta = float(self._q_conversion_history.getPropertyValue("ScatteringAngle"))
        elif self._q_conversion_history.name() == self.CONVERT_ALG:
            self._q_conversion_theta = float(np.rad2deg(self._ws.spectrumInfo().signedTwoTheta(0))) / 2.0

    def _set_name(self):
        if self.is_stitched:
            self._name = "Stitched"
        elif self._q_conversion_theta is not None:
            self._name = f"{self._q_conversion_theta:.3f}"
        else:
            self._name = self._ws.name()

        if self.is_polarized:
            self._name = f"{self._name} {self._spin_state}"
            return

        # Ensure unique dataset names for workspace group members.
        if self._is_ws_grp_member and self._ws.name() != self._name:
            self._name = f"{self._ws.name()} {self._name}"

    def _set_spin_state_from_logs(self) -> None:
        if self._ws.getRun().hasProperty(SpinStatesORSO.LOG_NAME):
            self._spin_state = self._ws.getRun().getLogData(SpinStatesORSO.LOG_NAME).value


class SaveISISReflectometryORSO(PythonAlgorithm):
    """
    See https://www.reflectometry.org/ for more information about the ORSO .ort format
    """

    _FACILITY = "ISIS"
    _ISIS_DOI_PREFIX = "10.5286/ISIS.E.RB"
    _RB_NUM_LOGS = ("rb_proposal", "experiment_identifier")
    _RUN_NUM_LOG = "run_number"
    _INVALID_HEADER_COMMENT = "Mantid@ISIS output may not be fully ORSO compliant"
    _Q_UNIT = "MomentumTransfer"

    # Algorithms
    _REBIN_ALG = "Rebin"
    _CREATE_FLOOD_ALG = "CreateFloodWorkspace"

    def category(self):
        return "Reflectometry\\ISIS"

    def name(self):
        """Return the name of the algorithm."""
        return "SaveISISReflectometryORSO"

    def summary(self):
        """Return a summary of the algorithm."""
        return "Saves ISIS processed reflectometry workspaces into either the ASCII or Nexus implementation of the ORSO data standard."

    def PyInit(self):
        mandatory_ws_list = CompositeValidator()
        mandatory_ws_list.add(StringArrayMandatoryValidator())
        len_validator = StringArrayLengthValidator()
        len_validator.setLengthMin(1)
        mandatory_ws_list.add(len_validator)
        self.declareProperty(
            StringArrayProperty(Prop.WORKSPACE_LIST, values=[], validator=mandatory_ws_list),
            doc="A list of workspace names containing the reduced reflectivity data to be saved.",
        )

        self.declareProperty(
            name=Prop.WRITE_RESOLUTION,
            defaultValue=True,
            direction=Direction.Input,
            doc="Whether to compute resolution values and write them as the fourth data column.",
        )

        self.declareProperty(
            name=Prop.INCLUDE_EXTRA_COLS,
            defaultValue=False,
            direction=Direction.Input,
            doc="Whether to include the four additional columns lambda, dlambda, theta and dtheta for unstitched datasets. "
            "If set to True then a resolution column will be included for all datasets, regardless of the value of "
            f"the {Prop.WRITE_RESOLUTION} parameter.",
        )

        self.declareProperty(
            FileProperty(Prop.FILENAME, "", FileAction.Save, extensions=[MantidORSOSaver.ASCII_FILE_EXT, MantidORSOSaver.NEXUS_FILE_EXT]),
            doc="File path to save the ORSO file to. Must end with a supported ORSO file extension. "
            f"Use {MantidORSOSaver.ASCII_FILE_EXT} to save into the ASCII format or {MantidORSOSaver.NEXUS_FILE_EXT} to "
            "save into the Nexus format",
        )

    def validateInputs(self):
        """Return a dictionary containing issues found in properties."""
        issues = dict()

        ws_list = self.getProperty(Prop.WORKSPACE_LIST).value
        for ws_name in ws_list:
            ws_issue = self._validate_ws(ws_name)
            if ws_issue:
                issues[Prop.WORKSPACE_LIST] = ws_issue
                break

        if not MantidORSOSaver.is_supported_extension(self.getProperty(Prop.FILENAME).value):
            issues[Prop.FILENAME] = (
                f"File path to save to must end with a supported ORSO extension. Use {MantidORSOSaver.ASCII_FILE_EXT} "
                f"to save as ORSO ASCII or {MantidORSOSaver.NEXUS_FILE_EXT} to save as ORSO Nexus."
            )

        return issues

    def _validate_ws(self, ws_name: str) -> str:
        if not AnalysisDataService.doesExist(ws_name):
            return f"Cannot find workspace with name {ws_name} in the ADS."

        workspace = AnalysisDataService.retrieve(ws_name)
        for ws in workspace if isinstance(workspace, WorkspaceGroup) else [workspace]:
            if not ws.getAxis(0).getUnit().unitID() == self._Q_UNIT:
                return f"Workspace {ws_name} must have units of {self._Q_UNIT}"

            if ws.spectrumInfo().size() != 1:
                return f"Workspace {ws_name} must contain only one spectrum"

        return ""

    def PyExec(self):
        # We cannot include all the mandatory information required by the standard, so we include a comment to highlight
        # this at the top of the file. In future this comment will not be needed, or we may need to add validation to
        # determine if it should be included (although ideally validation would be implemented in the orsopy library).
        orso_saver = MantidORSOSaver(self.getProperty(Prop.FILENAME).value, self._INVALID_HEADER_COMMENT)

        # Create the file contents
        for refl_dataset in self._create_and_sort_refl_datasets():
            orso_saver.add_dataset(self._create_orso_dataset(refl_dataset))

        # Write the file to disk in the relevant ORSO format
        save_filepath = Path(orso_saver.filename)
        if save_filepath.is_file():
            self.log().warning("File already exists and will be overwritten")

        file_ext = save_filepath.suffix
        try:
            if file_ext == MantidORSOSaver.ASCII_FILE_EXT:
                orso_saver.save_orso_ascii()
            else:
                orso_saver.save_orso_nexus()
        except OSError as e:
            raise RuntimeError(
                f"Error writing ORSO {file_ext} file. Check that the filepath is valid and does not contain any invalid characters.\n{e}"
            )

    @staticmethod
    def _get_all_workspace_groups():
        ws_groups = []
        for item_name in AnalysisDataService.getObjectNames():
            ws = AnalysisDataService.retrieve(item_name)
            if ws.isGroup():
                ws_groups.append(ws)
        return ws_groups

    def _create_and_sort_refl_datasets(self) -> List[ReflectometryDataset]:
        """Retrieve the workspaces from the input list, transform them into ReflectometryDataset objects and sort them
        into the order that the datasets should appear in the ORSO file"""
        ws_groups_in_ADS = self._get_all_workspace_groups()

        def is_workspace_group_member(workspace_name):
            for ws_group in ws_groups_in_ADS:
                if ws_group.contains(workspace_name):
                    return True
            return False

        dataset_list = []

        for ws_name in self.getProperty(Prop.WORKSPACE_LIST).value:
            ws = AnalysisDataService.retrieve(ws_name)
            if isinstance(ws, WorkspaceGroup):
                dataset_list.extend([ReflectometryDataset(child_ws, True) for child_ws in ws])
            else:
                dataset_list.append(ReflectometryDataset(ws, is_workspace_group_member(ws_name)))

        # Stitched datasets should be sorted to the end of the list
        dataset_list.sort(key=lambda refl_dataset: refl_dataset.is_stitched)
        return dataset_list

    def _create_orso_dataset(self, refl_dataset: ReflectometryDataset) -> MantidORSODataset:
        data_columns = self._create_data_columns(refl_dataset)
        dataset = self._create_dataset_with_mandatory_header(data_columns, refl_dataset)
        self._add_optional_header_info(dataset, refl_dataset)
        return dataset

    def _create_data_columns(self, refl_dataset: ReflectometryDataset) -> MantidORSODataColumns:
        """
        Set up the column headers and data values
        """
        resolution = self._get_resolution(refl_dataset)
        point_data = self._convert_to_point_data(refl_dataset.ws, "pointData")

        q_data = point_data.extractX()[0]
        reflectivity = point_data.extractY()[0]
        reflectivity_error = point_data.extractE()[0]
        q_resolution = resolution if resolution is None else q_data * resolution

        data_columns = MantidORSODataColumns(q_data, reflectivity, reflectivity_error, q_resolution, q_error_value_is=None)

        if self.getProperty(Prop.INCLUDE_EXTRA_COLS).value and not refl_dataset.is_stitched:
            # Add additional data columns
            try:
                l_data = self._convert_from_q_to_wavelength(refl_dataset, q_data)
            except RuntimeError as ex:
                self.log().warning(f"{ex} Additional data columns will be excluded.")
                return data_columns

            size = q_data.size

            data_columns.add_column("lambda", MantidORSODataColumns.Unit.Angstrom, "wavelength", l_data)
            data_columns.add_error_column("lambda", MantidORSODataColumns.ErrorType.Resolution, None, np.full(size, 0))
            data_columns.add_column(
                "incident theta", MantidORSODataColumns.Unit.Degrees, "incident theta", np.full(size, refl_dataset.q_conversion_theta)
            )
            if resolution is not None:
                # d incident theta = dQ/Q * incident theta
                d_theta = resolution * refl_dataset.q_conversion_theta
                data_columns.add_error_column("incident theta", MantidORSODataColumns.ErrorType.Uncertainty, None, np.full(size, d_theta))

        return data_columns

    def _convert_from_q_to_wavelength(self, refl_dataset: ReflectometryDataset, q_data: np.ndarray) -> np.ndarray:
        q_convert_history = refl_dataset.q_conversion_history
        if q_convert_history is None:
            raise RuntimeError(
                "Unable to calculate lambda values as cannot find algorithm used for original Q conversion in the workspace history."
            )

        # The method to convert back to wavelength depends on which algorithm was used to perform the conversion to Q
        if q_convert_history.name() == ReflectometryDataset.REF_ROI_ALG:
            return 4 * np.pi * np.sin(np.radians(refl_dataset.q_conversion_theta)) / q_data

        if q_convert_history.name() == ReflectometryDataset.CONVERT_ALG:
            alg = self.createChildAlgorithm(
                ReflectometryDataset.CONVERT_ALG,
                InputWorkspace=refl_dataset.ws,
                Target="Wavelength",
                AlignBins=False,
                OutputWorkspace="lambdaWs",
            )
            alg.execute()
            lambda_ws = alg.getProperty("OutputWorkspace").value
            lambda_point_data = self._convert_to_point_data(lambda_ws, "lambdaPointData")
            # There is an inverse relationship between lambda and Q, and workspace X values are in ascending order.
            # This means the first wavelength bin in the lambda workspace is the conversion for the last Q bin in the
            # reduced workspace.
            return np.flip(lambda_point_data.extractX()[0])

        raise RuntimeError("Unable to calculate lambda values as cannot find a supported Q conversion algorithm in the workspace history.")

    def _convert_to_point_data(self, ws, out_ws_name):
        alg = self.createChildAlgorithm("ConvertToPointData", InputWorkspace=ws, OutputWorkspace=out_ws_name)
        alg.execute()
        return alg.getProperty("OutputWorkspace").value

    def _create_dataset_with_mandatory_header(
        self,
        data_columns: MantidORSODataColumns,
        refl_dataset: ReflectometryDataset,
    ) -> MantidORSODataset:
        """
        Create a dataset with the data columns and the mandatory information populated in the header
        """
        return MantidORSODataset(
            refl_dataset.name,
            data_columns,
            refl_dataset.ws,
            reduction_timestamp=self._get_reduction_timestamp(refl_dataset.reduction_history),
            creator_name=self.name(),
            creator_affiliation=MantidORSODataset.SOFTWARE_NAME,
            is_polarized_dataset=refl_dataset.is_polarized,
        )

    def _add_optional_header_info(self, dataset: MantidORSODataset, refl_dataset: ReflectometryDataset) -> None:
        """
        Populate the non_mandatory information in the header
        """
        run = refl_dataset.ws.getRun()
        rb_number, doi = self._get_rb_number_and_doi(run)
        dataset.set_facility(self._FACILITY)
        dataset.set_proposal_id(rb_number)
        dataset.set_doi(doi)
        dataset.set_reduction_call(self._get_reduction_script(refl_dataset))
        if refl_dataset.is_polarized:
            dataset.set_polarization(refl_dataset.spin_state)

        reduction_workflow_histories = refl_dataset.reduction_workflow_histories
        if not refl_dataset.reduction_workflow_histories:
            self.log().debug(
                f"Unable to find history for {ReflectometryDataset.REDUCTION_WORKFLOW_ALG} - some metadata will be excluded from the file."
            )
            return

        instrument_name = refl_dataset.instrument_name

        for file, theta in self._get_individual_angle_files(instrument_name, reduction_workflow_histories):
            dataset.add_measurement_data_file(file, comment=f"Reduction input angle {theta}")

        first_trans_files, second_trans_files = self._get_transmission_files(instrument_name, reduction_workflow_histories)
        for file in first_trans_files:
            dataset.add_measurement_additional_file(file, comment="First transmission run")

        for file in second_trans_files:
            dataset.add_measurement_additional_file(file, comment="Second transmission run")

        flood_entry = self._get_flood_correction_entry(reduction_workflow_histories, refl_dataset.reduction_history)
        if flood_entry:
            dataset.add_measurement_additional_file(flood_entry[0], comment=flood_entry[1])

        calib_file_entry = self._get_calibration_file_entry(reduction_workflow_histories)
        if calib_file_entry:
            dataset.add_measurement_additional_file(calib_file_entry, comment="Calibration file")

    def _get_rb_number_and_doi(self, run) -> Union[Tuple[str, str], Tuple[None, None]]:
        """
        Check if the experiment RB number can be found in the workspace logs.
        This can be stored under different log names depending on whether time slicing was performed.
        If found, the RB number is used to provide the ISIS experiment DOI.
        """
        for log_name in self._RB_NUM_LOGS:
            if run.hasProperty(log_name):
                rb_num = str(run.getProperty(log_name).value)
                return rb_num, f"{self._ISIS_DOI_PREFIX}{rb_num}"
        return None, None

    def _get_resolution(self, refl_dataset: ReflectometryDataset) -> Optional[float]:
        if not self.getProperty(Prop.WRITE_RESOLUTION).value and not self.getProperty(Prop.INCLUDE_EXTRA_COLS).value:
            return None

        # Attempt to get the resolution from the workspace history
        if refl_dataset.is_stitched:
            # The absolute value of the stitch parameter of the stitch algorithm is the resolution
            return abs(float(refl_dataset.stitch_history.getPropertyValue("Params")))

        if refl_dataset.reduction_history:
            rebin_alg = refl_dataset.reduction_history.getChildHistories()[-1]
            if rebin_alg.name() == self._REBIN_ALG:
                rebin_params = rebin_alg.getPropertyValue("Params").split(",")
                if len(rebin_params) == 3:
                    # The absolute value of the middle rebin parameter is the resolution
                    return abs(float(rebin_params[1]))

        self.log().debug("Unable to find resolution from workspace history.")
        return None

    def _get_reduction_timestamp(self, reduction_history):
        """
        Get the reduction algorithm execution date, which is in UTC, and convert it to a
        datetime object expressed in local time
        """
        if not reduction_history:
            return None

        try:
            return MantidORSODataset.create_local_datetime_from_utc_string(reduction_history.executionDate().toISO8601String())
        except ValueError:
            self.log().debug("Could not parse reduction timestamp into required format - this information will be excluded from the file.")
            return None

    def _get_individual_angle_files(self, instrument_name, reduction_workflow_histories) -> List[Tuple[str, str]]:
        """
        Find the names of the individual angle files that were used in the reduction
        """
        angle_files = []

        for history in reduction_workflow_histories:
            input_runs = history.getPropertyValue("InputRunList")
            theta = history.getPropertyValue("ThetaIn")
            try:
                angle_files.extend([(file, theta) for file in self._get_file_names_from_history_run_list(input_runs, instrument_name)])
            except RuntimeError as ex:
                self.log().debug(f"{ex}. Angle file information will be excluded from the file.")
                return []
        return angle_files

    def _get_transmission_files(self, instrument_name, reduction_workflow_histories) -> Tuple[List[str], List[str]]:
        """
        Find the names of the transmission files that were used in the reduction
        """
        # We use an ordered dictionary to ensure that duplicates are excluded and that the files always appear in the
        # same order in the .ort file. Fixing the order makes it easier for us to write automated tests.
        first_trans_files = OrderedDict()
        second_trans_files = OrderedDict()

        def add_run_file_names(run_list, trans_files):
            if not run_list:
                return
            for file_name in self._get_file_names_from_history_run_list(run_list, instrument_name):
                trans_files[file_name] = None

        for history in reduction_workflow_histories:
            try:
                add_run_file_names(history.getPropertyValue("FirstTransmissionRunList"), first_trans_files)
                add_run_file_names(history.getPropertyValue("SecondTransmissionRunList"), second_trans_files)
            except RuntimeError as ex:
                self.log().debug(f"{ex}. Transmission file information will be excluded from the file.")
                return [], []

        return list(first_trans_files.keys()), list(second_trans_files.keys())

    def _get_file_names_from_history_run_list(self, run_list: str, instrument_name: str) -> List[str]:
        """
        Construct the run file names from the comma-separated run list values from the history
        of ReflectometryISISLoadAndProcess.
        """
        # We do this manually because using the FileFinder to look up the file is too slow for this algorithm, and we
        # can't guarantee that the file name will be available from the history of ReflectometryISISLoadAndProcess.
        file_names = []

        for entry in run_list.split(","):
            # The ISIS Reflectometry GUI is fairly flexible about what can be entered, so this could be a run number
            # or file name, with or without padding, or a workspace name.
            if AnalysisDataService.doesExist(entry):
                # If we can find a matching workspace in the ADS then get the run number from that
                run = AnalysisDataService.retrieve(entry).getRun()
                if not run.hasProperty(self._RUN_NUM_LOG):
                    raise RuntimeError(f"Cannot convert {entry} to a full run file name for instrument {instrument_name}")
                run_string = str(run.getProperty(self._RUN_NUM_LOG).value)
            else:
                run_string = entry
            file_names.append(self._construct_run_file_name_for_string(instrument_name, run_string))

        return file_names

    @staticmethod
    def _construct_run_file_name_for_string(instrument_name: str, run_string: str) -> str:
        def raise_error():
            raise RuntimeError(f"Cannot convert {run_string} to a file name for instrument {instrument_name}")

        # Check that the string is in a format that can potentially be converted to a run file name.
        # It should optionally start with any number of letters and end with any number of digits.
        # We ignore any file extensions.
        match = re.fullmatch(r"(?P<prefix>[a-zA-Z]*)(?P<padding>0*)(?P<run>[0-9]+)", run_string.split(".")[0])
        if not match:
            raise_error()
        match_prefix = match.group("prefix")
        run_num = match.group("run")

        instrument = config.getInstrument(instrument_name)
        try:
            inst_prefix = instrument.filePrefix(int(run_num))
            # Check if there's a prefix that isn't valid for the instrument
            if match_prefix and not match_prefix.lower() == inst_prefix.lower():
                raise_error()

            run_num_width = instrument.zeroPadding(int(run_num))
            if len(run_num) > run_num_width:
                raise_error()

            return f"{inst_prefix}{run_num.rjust(run_num_width, '0')}"
        except OverflowError:
            raise_error()

    def _get_flood_correction_entry(self, reduction_workflow_histories, reduction_history) -> Optional[tuple[str, str]]:
        """
        Get the flood correction file or workspace name from the reduction history.
        """
        # The flood correction may have been passed either as a full filepath or workspace name
        # The FloodCorrection parameter says Workspace in both cases, so we don't know which we have
        flood_name = reduction_workflow_histories[0].getPropertyValue("FloodWorkspace")
        if flood_name:
            return Path(flood_name).name, "Flood correction workspace or file"

        # It's possible that a flood workspace was created as the first step in the reduction
        if reduction_history:
            flood_history = reduction_history.getChildHistories()[0]
            if flood_history.name() == self._CREATE_FLOOD_ALG:
                return Path(flood_history.getPropertyValue("Filename")).name, "Flood correction run file"

        return None

    @staticmethod
    def _get_calibration_file_entry(reduction_workflow_histories) -> Optional[str]:
        """
        Get the calibration file name from the reduction history.
        """
        calibration_file = reduction_workflow_histories[0].getPropertyValue("CalibrationFile")
        return Path(calibration_file).name if calibration_file else None

    def _get_reduction_script(self, refl_dataset: ReflectometryDataset) -> Optional[str]:
        """
        Get the workspace reduction history as a script.
        """
        if refl_dataset.is_ws_grp_member:
            # We don't get an accurate history from ReflectometryISISLoadAndProcess for workspace groups, so only
            # include this if the workspace is not part of a group
            return None

        if refl_dataset.ws.getHistory().empty():
            return None

        alg = self.createChildAlgorithm("GeneratePythonScript", InputWorkspace=refl_dataset.ws, ExcludeHeader=True)
        alg.execute()
        script = alg.getPropertyValue("ScriptText")
        return "\n".join(script.split("\n")[2:])  # trim the import statement


# Register algorithm with Mantid
AlgorithmFactory.subscribe(SaveISISReflectometryORSO)
