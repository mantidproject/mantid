# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2023 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantid.utils.reflectometry.orso_helper import MantidORSODataColumns, MantidORSODataset, MantidORSOSaver

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


class Prop:
    WORKSPACE_LIST = "WorkspaceList"
    WRITE_RESOLUTION = "WriteResolution"
    FILENAME = "Filename"


class ReflectometryDataset:
    REDUCTION_WORKFLOW_ALG = "ReflectometryISISLoadAndProcess"

    _REDUCTION_ALG = "ReflectometryReductionOneAuto"
    _STITCH_ALG = "Stitch1DMany"

    def __init__(self, ws):
        self._name = None
        self._ws = ws
        self._reduction_history = None
        self._reduction_workflow_histories = []
        self._stitch_history = None

        self._populate_histories()
        self._set_name()

    @property
    def name(self):
        return self._name

    @property
    def ws(self):
        return self._ws

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

    def _set_name(self):
        if self.is_stitched:
            self._name = "Stitched"
        elif self._reduction_history:
            self._name = self._reduction_history.getPropertyValue("ThetaIn")
        else:
            self._name = self._ws.name()

        # Temporary solution for POLREF to give unique dataset names until we can specify the polarization spin state
        if self._ws.getInstrument().getName() == "POLREF" and self._name is not self._ws.name():
            self._name = f"{self._ws.name()} {self._name}"


class SaveISISReflectometryORSO(PythonAlgorithm):
    """
    See https://www.reflectometry.org/ for more information about the ORSO .ort format
    """

    _FACILITY = "ISIS"
    _ISIS_DOI_PREFIX = "10.5286/ISIS.E.RB"
    _RB_NUM_LOGS = ("rb_proposal", "experiment_identifier")
    _RUN_NUM_LOG = "run_number"
    _INVALID_HEADER_COMMENT = "Mantid@ISIS output may not be fully ORSO compliant"
    _REBIN_ALG = "Rebin"
    _CREATE_FLOOD_ALG = "CreateFloodWorkspace"
    _Q_UNIT = "MomentumTransfer"

    def category(self):
        return "Reflectometry\\ISIS"

    def name(self):
        """Return the name of the algorithm."""
        return "SaveISISReflectometryORSO"

    def summary(self):
        """Return a summary of the algorithm."""
        return "Saves ISIS processed reflectometry workspaces into the ASCII implementation of the ORSO data standard."

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
            FileProperty(Prop.FILENAME, "", FileAction.Save, MantidORSOSaver.FILE_EXT), doc="File path to save the .ort file to"
        )

    def validateInputs(self):
        """Return a dictionary containing issues found in properties."""
        issues = dict()

        ws_list = self.getProperty(Prop.WORKSPACE_LIST).value
        for ws_name in ws_list:
            if not AnalysisDataService.doesExist(ws_name):
                issues[Prop.WORKSPACE_LIST] = f"Cannot find workspace with name {ws_name} in the ADS."
            else:
                workspace = AnalysisDataService.retrieve(ws_name)
                for ws in workspace if isinstance(workspace, WorkspaceGroup) else [workspace]:
                    if not self._is_momentum_transfer_ws(ws):
                        issues[Prop.WORKSPACE_LIST] = f"Workspace {ws_name} must have units of {self._Q_UNIT}"
        return issues

    def _is_momentum_transfer_ws(self, ws) -> bool:
        return ws.getAxis(0).getUnit().unitID() == self._Q_UNIT

    def PyExec(self):
        # We cannot include all the mandatory information required by the standard, so we include a comment to highlight
        # this at the top of the file. In future this comment will not be needed, or we may need to add validation to
        # determine if it should be included (although ideally validation would be implemented in the orsopy library).
        orso_saver = MantidORSOSaver(self.getProperty(Prop.FILENAME).value, self._INVALID_HEADER_COMMENT)

        # Create the file contents
        for refl_dataset in self._create_and_sort_refl_datasets():
            orso_saver.add_dataset(self._create_orso_dataset(refl_dataset))

        # Write the file to disk in the ORSO ASCII format
        if Path(orso_saver.filename).is_file():
            self.log().warning("File already exists and will be overwritten")

        try:
            orso_saver.save_orso_ascii()
        except OSError as e:
            raise RuntimeError(
                f"Error writing ORSO file. Check that the filepath is valid and does not contain any invalid characters.\n{e}"
            )

    def _create_and_sort_refl_datasets(self) -> List[ReflectometryDataset]:
        """Retrieve the workspaces from the input list, transform them into ReflectometryDataset objects and sort them
        into the order that the datasets should appear in the ORSO file"""
        final_list = []
        stitched_workspaces = []

        for ws_name in self.getProperty(Prop.WORKSPACE_LIST).value:
            workspace = AnalysisDataService.retrieve(ws_name)
            group = workspace if isinstance(workspace, WorkspaceGroup) else [workspace]
            for ws in group:
                refl_dataset = ReflectometryDataset(ws)
                if refl_dataset.is_stitched:
                    stitched_workspaces.append(refl_dataset)
                else:
                    final_list.append(refl_dataset)

        final_list.extend(stitched_workspaces)
        return final_list

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

        alg = self.createChildAlgorithm("ConvertToPointData", InputWorkspace=refl_dataset.ws, OutputWorkspace="pointData")
        alg.execute()
        point_data = alg.getProperty("OutputWorkspace").value

        q_data = point_data.extractX()[0]
        reflectivity = point_data.extractY()[0]
        reflectivity_error = point_data.extractE()[0]
        q_resolution = resolution if resolution is None else q_data * resolution

        data_columns = MantidORSODataColumns(q_data, reflectivity, reflectivity_error, q_resolution, q_error_value_is=None)

        return data_columns

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
        dataset.set_reduction_call(self._get_reduction_script(refl_dataset.ws))

        reduction_workflow_histories = refl_dataset.reduction_workflow_histories
        if not refl_dataset.reduction_workflow_histories:
            self.log().warning(
                f"Unable to find history for {ReflectometryDataset.REDUCTION_WORKFLOW_ALG} - some metadata will be excluded from the file."
            )
            return

        instrument_name = refl_dataset.ws.getInstrument().getName()

        for file, theta in self._get_individual_angle_files(instrument_name, reduction_workflow_histories):
            dataset.add_measurement_data_file(file, comment=f"Incident angle {theta}")

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
        if not self.getProperty(Prop.WRITE_RESOLUTION).value:
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

        self.log().warning("Unable to find resolution from workspace history.")
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
            self.log().warning(
                "Could not parse reduction timestamp into required format - this information will be excluded from the file."
            )
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
                self.log().warning(f"{ex}. Angle file information will be excluded from the file.")
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
                self.log().warning(f"{ex}. Transmission file information will be excluded from the file.")
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

    def _get_reduction_script(self, ws) -> Optional[str]:
        """
        Get the workspace reduction history as a script.
        """
        if ws.getHistory().empty():
            return None

        alg = self.createChildAlgorithm("GeneratePythonScript", InputWorkspace=ws, ExcludeHeader=True)
        alg.execute()
        script = alg.getPropertyValue("ScriptText")
        return "\n".join(script.split("\n")[2:])  # trim the import statement


# Register algorithm with Mantid
AlgorithmFactory.subscribe(SaveISISReflectometryORSO)
