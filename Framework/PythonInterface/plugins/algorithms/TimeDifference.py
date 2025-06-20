# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=no-init,invalid-name

from __future__ import annotations

from mantid.api import (
    ADSValidator,
    AlgorithmFactory,
    AnalysisDataService,
    PythonAlgorithm,
    ITableWorkspaceProperty,
    MatrixWorkspace,
    WorkspaceProperty,
    WorkspaceGroup,
    WorkspaceFactory,
    PropertyMode,
)

from mantid.kernel import Direction, StringArrayProperty, logger

import numpy as np
from typing import TYPE_CHECKING

if TYPE_CHECKING:
    from mantid.dataobjects import TableWorkspace

# TIME LOGS
START_LOGS = ["start_time", "run_start"]
END_LOGS = ["end_time", "run_end"]
ONE_SECOND = np.timedelta64(1, "s")


def _calculate_midtime_and_duration(start: np.datetime64, end: np.datetime64) -> tuple[np.datetime64, np.timedelta64]:
    duration = end - start
    mid = start + duration / 2
    return mid, duration


class TimeDifference(PythonAlgorithm):
    reference_time = np.datetime64("NaT")
    reference_duration = np.timedelta64("NaT")

    def category(self):
        return "Utility"

    def name(self):
        return "TimeDifference"

    def summary(self):
        return "Calculates the time differences of a series of workspaces with respect to a reference"

    def PyInit(self):
        self.declareProperty(
            StringArrayProperty("InputWorkspaces", direction=Direction.Input, validator=ADSValidator()),
            doc="Input workspaces. Comma separated workspace names of either Matrix or Group Workspaces. Must be on the ADS",
        )

        self.declareProperty(
            WorkspaceProperty(name="ReferenceWorkspace", optional=PropertyMode.Optional, defaultValue="", direction=Direction.Input),
            doc="Workspace to be used as time reference",
        )

        self.declareProperty(
            ITableWorkspaceProperty(name="OutputWorkspace", defaultValue="", direction=Direction.Output),
            doc="Table containing difference time in relation to the reference workspace for each input workspace",
        )

    def checkGroups(self):
        return False

    def validateInputs(self):
        def group_or_matrix(workspace: MatrixWorkspace) -> bool:
            return isinstance(workspace, (MatrixWorkspace, WorkspaceGroup))

        issues = dict()
        workspace_names = self.getProperty("InputWorkspaces").value
        for name in workspace_names:
            ws = AnalysisDataService.retrieve(name)
            if not group_or_matrix(ws):
                issues["InputWorkspaces"] = f"Workspace {name} is not a Group or Matrix Workspace."
                return issues

        reference_workspace = self.getProperty("ReferenceWorkspace").value
        if reference_workspace:
            if not group_or_matrix(reference_workspace):
                issues["ReferenceWorkspace"] = "ReferenceWorkspace is not a Group or Matrix Workspace."
        return issues

    def _build_row(self, name: str, time: np.datetime64, duration: np.timedelta64) -> dict[str, str | float]:
        # We scale to transform to seconds
        seconds_diff = (time - self.reference_time) / ONE_SECOND
        seconds_diff_err = np.abs((duration + self.reference_duration) / ONE_SECOND)
        hours_diff = seconds_diff / 3600
        hours_diff_err = seconds_diff_err / 3600
        row = {
            "ws_name": name,
            "midtime_stamp": str(time),
            "seconds": seconds_diff,
            "seconds_error": seconds_diff_err,
            "hours": hours_diff,
            "hours_error": hours_diff_err,
        }
        return row

    def _build_output_table(self, names: list[str], times: list[tuple[np.datetime64, np.timedelta64]]) -> TableWorkspace:
        # Create Times Table
        table = WorkspaceFactory.Instance().createTable()
        column_types = ["str", "str", *["float"] * 4]
        column_names = ["ws_name", "midtime_stamp", "seconds", "seconds_error", "hours", "hours_error"]
        for col_name, col_type in zip(column_names, column_types):
            table.addColumn(type=col_type, name=col_name)
        for name, time in zip(names, times):
            table.addRow(self._build_row(name, time[0], time[1]))
        return table

    def _get_time_ws(self, ws: MatrixWorkspace) -> list[np.datetime64]:
        time_start = time_end = None
        try:
            run = ws.getRun()
            has_start_logs = [run.hasProperty(log) for log in START_LOGS]
            has_end_logs = [run.hasProperty(log) for log in END_LOGS]
            if not any(has_start_logs) * any(has_end_logs):
                raise RuntimeError("Missing time logs")

            time_start = run.getLogData(START_LOGS[np.argmax(has_start_logs)]).value
            time_end = run.getLogData(END_LOGS[np.argmax(has_end_logs)]).value

        except RuntimeError:
            logger.error("Unable to access time logs on workspace " + ws.name())
        return [np.datetime64(time, "ns") for time in [time_start, time_end]]

    def _get_time_ws_group(self, workspaces: WorkspaceGroup) -> tuple[np.datetime64, np.timedelta64]:
        starts = ends = []
        for ws in workspaces:
            if isinstance(ws, WorkspaceGroup):
                continue
            time_start, time_end = self._get_time_ws(ws)
            if not np.isnat(time_start):
                starts.append(time_start)
                ends.append(time_end)

        start = np.min(starts)
        end = np.max(ends)
        return _calculate_midtime_and_duration(start, end)

    def PyExec(self):
        ws_names = self.getProperty("InputWorkspaces").value
        reference_ws = self.getProperty("ReferenceWorkspace").value

        times = []
        # Put reference on list if not present
        reference_ws_name = ws_names[0]
        if reference_ws:
            reference_ws_name = reference_ws.name()
            if reference_ws_name not in ws_names:
                ws_names.insert(0, reference_ws_name)

        for name in ws_names:
            ws = AnalysisDataService.retrieve(name)
            if isinstance(ws, MatrixWorkspace):
                times.append(_calculate_midtime_and_duration(*self._get_time_ws(ws)))
            elif isinstance(ws, WorkspaceGroup):
                times.append(self._get_time_ws_group(ws))

        self.reference_time = times[ws_names.index(reference_ws_name)][0]
        self.reference_duration = times[ws_names.index(reference_ws_name)][1]
        table = self._build_output_table(ws_names, times)
        self.setProperty("OutputWorkspace", table)


AlgorithmFactory.subscribe(TimeDifference)
