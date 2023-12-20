# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2023 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantid.utils.reflectometry.orso_helper import MantidORSODataColumns, MantidORSODataset, MantidORSOSaver

from mantid.kernel import Direction, config
from mantid.api import AlgorithmFactory, MatrixWorkspaceProperty, FileProperty, FileAction, PythonAlgorithm, PropertyMode

from pathlib import Path
from typing import Optional, Tuple, Union, List
import re
from collections import OrderedDict


class Prop:
    INPUT_WS = "InputWorkspace"
    WRITE_RESOLUTION = "WriteResolution"
    FILENAME = "Filename"


class SaveISISReflectometryORSO(PythonAlgorithm):
    """
    See https://www.reflectometry.org/ for more information about the ORSO .ort format
    """

    _FACILITY = "ISIS"
    _ISIS_DOI_PREFIX = "10.5286/ISIS.E.RB"
    _RB_NUM_LOGS = ("rb_proposal", "experiment_identifier")
    _INVALID_HEADER_COMMENT = "Mantid@ISIS output may not be fully ORSO compliant"
    _REDUCTION_ALG = "ReflectometryReductionOneAuto"
    _REDUCTION_WORKFLOW_ALG = "ReflectometryISISLoadAndProcess"
    _STITCH_ALG = "Stitch1DMany"
    _REBIN_ALG = "Rebin"
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
        self.declareProperty(
            MatrixWorkspaceProperty(name=Prop.INPUT_WS, defaultValue="", direction=Direction.Input, optional=PropertyMode.Mandatory),
            doc="The workspace containing the reduced reflectivity data to save.",
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

        ws = self.getProperty(Prop.INPUT_WS).value
        if ws and not self._is_momentum_transfer_ws(ws):
            issues[Prop.INPUT_WS] = f"{Prop.INPUT_WS} must have units of {self._Q_UNIT}"
        return issues

    def _is_momentum_transfer_ws(self, ws) -> bool:
        return ws.getAxis(0).getUnit().unitID() == self._Q_UNIT

    def PyExec(self):
        ws = self.getProperty(Prop.INPUT_WS).value

        # We cannot include all the mandatory information required by the standard, so we include a comment to highlight
        # this at the top of the file. In future this comment will not be needed, or we may need to add validation to
        # determine if it should be included (although ideally validation would be implemented in the orsopy library).
        orso_saver = MantidORSOSaver(self.getProperty(Prop.FILENAME).value, self._INVALID_HEADER_COMMENT)

        # Create the file contents
        orso_saver.add_dataset(self._create_dataset(ws, ws.name()))

        # Write the file to disk in the ORSO ASCII format
        if Path(orso_saver.filename).is_file():
            self.log().warning("File already exists and will be overwritten")

        try:
            orso_saver.save_orso_ascii()
        except OSError as e:
            raise RuntimeError(
                f"Error writing ORSO file. Check that the filepath is valid and does not contain any invalid characters.\n{e}"
            )

    def _create_dataset(self, ws, dataset_name: str = None) -> MantidORSODataset:
        reduction_history = self._get_reduction_alg_history(ws)
        data_columns = self._create_data_columns(ws, reduction_history)
        dataset = self._create_dataset_with_mandatory_header(ws, dataset_name, reduction_history, data_columns)
        self._add_optional_header_info(dataset, ws)
        return dataset

    def _create_data_columns(self, ws, reduction_history) -> MantidORSODataColumns:
        """
        Set up the column headers and data values
        """
        resolution = self._get_resolution(ws, reduction_history)

        alg = self.createChildAlgorithm("ConvertToPointData", InputWorkspace=ws, OutputWorkspace="pointData")
        alg.execute()
        point_data = alg.getProperty("OutputWorkspace").value

        q_data = point_data.extractX()[0]
        reflectivity = point_data.extractY()[0]
        reflectivity_error = point_data.extractE()[0]
        q_resolution = resolution if resolution is None else q_data * resolution

        data_columns = MantidORSODataColumns(q_data, reflectivity, reflectivity_error, q_resolution)

        return data_columns

    def _create_dataset_with_mandatory_header(
        self, ws, dataset_name: str, reduction_history, data_columns: MantidORSODataColumns
    ) -> MantidORSODataset:
        """
        Create a dataset with the data columns and the mandatory information populated in the header
        """
        return MantidORSODataset(
            dataset_name,
            data_columns,
            ws,
            reduction_timestamp=self._get_reduction_timestamp(reduction_history),
            creator_name=self.name(),
            creator_affiliation=MantidORSODataset.SOFTWARE_NAME,
        )

    def _add_optional_header_info(self, dataset: MantidORSODataset, ws) -> None:
        """
        Populate the non_mandatory information in the header
        """
        run = ws.getRun()
        rb_number, doi = self._get_rb_number_and_doi(run)
        dataset.set_facility(self._FACILITY)
        dataset.set_proposal_id(rb_number)
        dataset.set_doi(doi)

        reduction_workflow_histories = self._get_reduction_workflow_alg_histories(ws)
        if not reduction_workflow_histories:
            self.log().warning(f"Unable to find history for {self._REDUCTION_WORKFLOW_ALG} - some metadata will be excluded from the file.")
            return

        instrument_name = ws.getInstrument().getName()

        for file, theta in self._get_individual_angle_files(instrument_name, reduction_workflow_histories):
            dataset.add_measurement_file(True, file, comment=f"Incident angle {theta}")

        first_trans_files, second_trans_files = self._get_transmission_files(instrument_name, reduction_workflow_histories)
        for file in first_trans_files:
            dataset.add_measurement_file(False, file, comment="First transmission run")

        for file in second_trans_files:
            dataset.add_measurement_file(False, file, comment="Second transmission run")

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

    def _get_resolution(self, ws, reduction_history) -> Optional[float]:
        if not self.getProperty(Prop.WRITE_RESOLUTION).value:
            return None

        # Attempt to get the resolution from the workspace history if it hasn't been passed in
        history = ws.getHistory()
        if not history.empty():
            for history in reversed(history.getAlgorithmHistories()):
                if history.name() == self._STITCH_ALG:
                    # The absolute value of the stitch parameter is the resolution
                    return abs(float(history.getPropertyValue("Params")))

            if reduction_history:
                rebin_alg = reduction_history.getChildHistories()[-1]
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
                angle_files.extend([(file, theta) for file in self._get_file_names_from_run_numbers(input_runs, instrument_name)])
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
            for file_name in self._get_file_names_from_run_numbers(run_list, instrument_name):
                trans_files[file_name] = None

        for history in reduction_workflow_histories:
            try:
                add_run_file_names(history.getPropertyValue("FirstTransmissionRunList"), first_trans_files)
                add_run_file_names(history.getPropertyValue("SecondTransmissionRunList"), second_trans_files)
            except RuntimeError as ex:
                self.log().warning(f"{ex}. Transmission file information will be excluded from the file.")
                return [], []

        return list(first_trans_files.keys()), list(second_trans_files.keys())

    @staticmethod
    def _get_file_names_from_run_numbers(run_numbers: str, instrument_name: str) -> List[str]:
        """
        Get the fully qualified file names from a string of comma-separated run numbers.
        """
        # If the run number part is all digits then append the instrument name and Nexus file extension, with the
        # correct instrument padding, to give the full file name.
        # We do this manually because using the FileFinder is too slow for this algorithm. We can't guarantee
        # that it will be possible to retrieve this from the history of ReflectometryISISLoadAndProcess either.
        file_names = []
        ext = ".nxs"
        run_num_width = config.getInstrument(instrument_name).zeroPadding(0)

        def is_only_digits(str_to_check):
            return re.search(r"\D+", str_to_check) is None

        for run in run_numbers.split(","):
            # The ISIS Reflectometry GUI is fairly flexible about what can be entered, so this could be a run number
            # or file name, with or without padding. Less commonly, it could be a workspace name, which we will exclude
            # from the ORSO metadata.
            filename_part = run.split(".")[0]
            if not filename_part:
                raise RuntimeError(f"Cannot convert run {run} to a full file name")

            # Get the run number part of the run information
            run_number_part = None
            if is_only_digits(filename_part):
                # This must be the run number
                run_number_part = filename_part
            elif filename_part.startswith(instrument_name):
                # Get the part of the filename left once the instrument prefix is removed
                maybe_run_num = filename_part.split(instrument_name)[1]
                if is_only_digits(maybe_run_num):
                    run_number_part = maybe_run_num

            if not run_number_part:
                raise RuntimeError(f"Cannot convert run {run} to a full file name for instrument {instrument_name}")

            file_names.append(f"{instrument_name}{run_number_part.rjust(run_num_width, '0')}{ext}")

        return file_names

    def _get_reduction_alg_history(self, ws):
        """
        Find the first occurrence of the reduction algorithm in the workspace history, otherwise return None
        """
        ws_history = ws.getHistory()
        if ws_history.empty():
            return None

        for history in reversed(ws_history.getAlgorithmHistories()):
            if history.name() == self._REDUCTION_ALG:
                return history

            if history.name() == self._REDUCTION_WORKFLOW_ALG:
                for child_history in reversed(history.getChildHistories()):
                    if child_history.name() == self._REDUCTION_ALG:
                        return child_history

        return None

    def _get_reduction_workflow_alg_histories(self, ws):
        """
        Return a list containing all the occurrences of the reduction workflow algorithm in the workspace history
        """
        ws_history = ws.getHistory()
        if ws_history.empty():
            return []

        return [history for history in ws_history.getAlgorithmHistories() if history.name() == self._REDUCTION_WORKFLOW_ALG]


# Register algorithm with Mantid
AlgorithmFactory.subscribe(SaveISISReflectometryORSO)
