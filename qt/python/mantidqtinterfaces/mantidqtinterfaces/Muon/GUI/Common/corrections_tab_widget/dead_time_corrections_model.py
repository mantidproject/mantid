# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantid.api import ITableWorkspace
from mantidqtinterfaces.Muon.GUI.Common.ADSHandler.ADS_calls import check_if_workspace_exist, retrieve_ws
from mantidqtinterfaces.Muon.GUI.Common.contexts.corrections_context import CorrectionsContext, DEAD_TIME_FROM_FILE, DEAD_TIME_FROM_ADS
from mantidqtinterfaces.Muon.GUI.Common.contexts.muon_data_context import MuonDataContext
from mantidqtinterfaces.Muon.GUI.Common.corrections_tab_widget.corrections_model import CorrectionsModel

DEAD_TIME_TABLE_KEY = "dead-time"


class DeadTimeCorrectionsModel:
    """
    The DeadTimeCorrectionsModel calculates Dead Time corrections.
    """

    def __init__(self, corrections_model: CorrectionsModel, data_context: MuonDataContext, corrections_context: CorrectionsContext):
        """Initialize the DeadTimeCorrectionsModel with empty data."""
        self._corrections_model = corrections_model
        self._data_context = data_context
        self._corrections_context = corrections_context

    def dead_times_average(self) -> float:
        """Returns the average dead time for the currently selected run."""
        dead_times = self._current_dead_times()
        return sum(dead_times) / len(dead_times) if len(dead_times) > 0 else 0.0

    def dead_times_range(self) -> tuple:
        """Returns the minimum and maximum dead time for the currently selected run."""
        dead_times = self._current_dead_times()
        return min(dead_times, default=0.0), max(dead_times, default=0.0)

    def _current_dead_times(self) -> list:
        """Returns a list of dead times for the currently displayed run and dead time mode."""
        table_name = self._corrections_context.current_dead_time_table_name_for_run(
            self._data_context.instrument, self._corrections_model.current_runs()
        )
        table = retrieve_ws(table_name) if table_name else None
        return table.toDict()[DEAD_TIME_TABLE_KEY] if table is not None else []

    def set_dead_time_source_to_from_file(self) -> None:
        """Sets the dead time source to be 'FromFile'."""
        self._corrections_context.dead_time_source = DEAD_TIME_FROM_FILE
        self._corrections_context.dead_time_table_name_from_ads = None

    def set_dead_time_source_to_from_ads(self, table_name: str) -> None:
        """Sets the dead time source to be 'FromADS'."""
        if table_name == "None":
            self.set_dead_time_source_to_none()
        else:
            self._corrections_context.dead_time_source = DEAD_TIME_FROM_ADS
            self._corrections_context.dead_time_table_name_from_ads = table_name

    def set_dead_time_source_to_none(self) -> None:
        """Sets the dead time source to be 'None'."""
        self._corrections_context.dead_time_source = None
        self._corrections_context.dead_time_table_name_from_ads = None

    def is_dead_time_source_from_data_file(self) -> bool:
        """Returns true if the dead time should be retrieved from a data file."""
        return self._corrections_context.dead_time_source == DEAD_TIME_FROM_FILE

    def is_dead_time_source_from_workspace(self) -> bool:
        """Returns true if the dead time should be retrieved from a workspace."""
        return self._corrections_context.dead_time_source == DEAD_TIME_FROM_ADS

    def is_dead_time_source_from_none(self) -> bool:
        """Returns true if the dead time should not be retrieved from any."""
        return self._corrections_context.dead_time_source is None

    def validate_selected_dead_time_workspace(self, table_name: str) -> str:
        """Validates the selected dead time workspace. Returns a string containing an error message if its invalid."""
        if check_if_workspace_exist(table_name):
            table = retrieve_ws(table_name)
            return self._validate_dead_time_table(table)
        else:
            return f"Workspace '{table_name}' does not exist in the ADS."

    def _validate_dead_time_table(self, table: ITableWorkspace) -> str:
        """Validates that the dead time table provided has the expected format."""
        if not isinstance(table, ITableWorkspace):
            return "The dead time table selected is not a Table Workspace."
        column_names = table.getColumnNames()
        if len(column_names) != 2:
            return f"Expected 2 columns, found {str(max(0, len(column_names)))} columns."
        if column_names[0] != "spectrum" or column_names[1] != DEAD_TIME_TABLE_KEY:
            return f"Columns have incorrect names. Column 1 should be 'spectrum' and column 2 should be '{DEAD_TIME_TABLE_KEY}'."
        number_of_rows = table.rowCount()
        number_of_histograms = self._data_context.current_workspace.getNumberHistograms()
        if number_of_rows != number_of_histograms:
            return (
                f"The number of histograms ({number_of_histograms}) does not match the number of rows "
                f"({number_of_rows}) in dead time table."
            )
        return ""
