# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantid.api import ITableWorkspace
from Muon.GUI.Common.ADSHandler.ADS_calls import check_if_workspace_exist, retrieve_ws
from Muon.GUI.Common.contexts.corrections_context import CorrectionsContext
from Muon.GUI.Common.contexts.muon_context import MuonContext


class CorrectionsModel:
    """
    The CorrectionsModel calculates Dead Time corrections and Background corrections.
    """

    def __init__(self, context: MuonContext, corrections_context: CorrectionsContext):
        """Initialize the CorrectionsModel with emtpy data."""
        self.context = context
        self.corrections_context = corrections_context

    def dead_times_average(self) -> float:
        """Returns the average dead time."""
        dead_times = self._get_dead_times_from_table_workspace()
        return sum(dead_times) / len(dead_times) if len(dead_times) > 0 else 0.0

    def dead_times_range(self) -> tuple:
        """Returns the minimum and maximum dead time."""
        dead_times = self._get_dead_times_from_table_workspace()
        return min(dead_times, default=0.0), max(dead_times, default=0.0)

    def _get_dead_times_from_table_workspace(self) -> list:
        """Gets the dead times from the selected dead time table workspace."""
        dead_time_table = self._get_dead_time_table_workspace()
        return dead_time_table.toDict()['dead-time'] if dead_time_table is not None else []

    def _get_dead_time_table_workspace(self) -> ITableWorkspace:
        """Returns the selected dead time table containing the dead times to use for corrections."""
        dead_time_table_name = self.context.data_context.current_data["DataDeadTimeTable"]
        return retrieve_ws(dead_time_table_name) if dead_time_table_name else None

    def set_dead_time_source_to_from_file(self) -> None:
        """Sets the dead time source to be 'FromFile'."""
        self.context.gui_context.update_and_send_signal(DeadTimeSource="FromFile")

    def set_dead_time_source_to_from_ADS(self, table_name: str) -> None:
        """Sets the dead time source to be 'FromADS'."""
        if table_name == "None":
            self.set_dead_time_source_to_none()
        else:
            self.context.gui_context.update_and_send_non_calculation_signal(DeadTimeSource="FromADS")
            self.context.gui_context.update_and_send_signal(DeadTimeTable=retrieve_ws(table_name))

    def set_dead_time_source_to_none(self) -> None:
        """Sets the dead time source to be 'None'."""
        self.context.gui_context.update_and_send_signal(DeadTimeSource="None", DeadTimeTable=None)

    def validate_selected_dead_time_workspace(self, table_name: str) -> str:
        """Validates the selected dead time workspace. Returns a string containing an error message if its invalid."""
        if check_if_workspace_exist(table_name):
            table = retrieve_ws(table_name)
            return self._validate_dead_time_table(table)
        else:
            return f"Workspace {table_name} does not exist in the ADS."

    def _validate_dead_time_table(self, table: ITableWorkspace) -> str:
        """Validates that the dead time table provided has the expected format."""
        if not isinstance(table, ITableWorkspace):
            return "The dead time table selected is not a Table Workspace."
        column_names = table.getColumnNames()
        if len(column_names) != 2:
            return f"Expected 2 columns, found {str(max(0, len(column_names)))} columns."
        if column_names[0] != "spectrum" or column_names[1] != "dead-time":
            return f"Columns have incorrect names."
        if table.rowCount() != self.context.data_context.current_workspace.getNumberHistograms():
            return "The number of histograms does not match the number of rows in dead time table."
        return ""
