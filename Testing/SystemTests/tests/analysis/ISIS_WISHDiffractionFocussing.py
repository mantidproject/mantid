# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)
from mantid.simpleapi import *
import systemtesting
import os.path
import numpy as np


class WISHDiffractionFocussingReductionTest(systemtesting.MantidSystemTest):

    def requiredFiles(self):
        return ["WISH00035991.raw", "35922_h00_RW.cal", "35991-foc-h00.nxs"]

    def requiredMemoryMB(self):
        return 16000

    def cleanup(self):
        for ws in self._focused_workspaces:
            try:
                path = config.getString("defaultsave.directory")
                os.remove(os.path.join(path, ws + ".dat"))
                os.remove(os.path.join(path, ws + ".nxs"))
            except:
                continue

    def runTest(self):
        output_path = config.getString("defaultsave.directory")
        grouping_filename = '35922_h00_RW.cal'
        focused_suffix = '-foc-h00'

        monitor_id = 4
        crop_limits = {"XMin": 6000, "XMax": 99900}

        run_numbers = [35991]

        min_run = min([x if type(x) == int else min(x) for x in run_numbers])
        max_run = max([x if type(x) == int else max(x) for x in run_numbers])
        group_name = str(min_run) + '-' + str(max_run) + focused_suffix

        self._focused_workspaces = []
        for runno in run_numbers:
            run_number = str(runno)
            raw_file = "WISH000" + run_number + '.raw'

            LoadRaw(Filename=raw_file, OutputWorkspace=run_number)
            CropWorkspace(InputWorkspace=run_number, OutputWorkspace=run_number, **crop_limits)
            NormaliseByCurrent(InputWorkspace=run_number, OutputWorkspace=run_number)
            ConvertUnits(InputWorkspace=run_number, OutputWorkspace=run_number, Target='Wavelength')
            NormaliseToMonitor(InputWorkspace=run_number, OutputWorkspace=run_number, MonitorID=monitor_id)
            ConvertUnits(InputWorkspace=run_number, OutputWorkspace=run_number, Target='dSpacing')

            focused = run_number + focused_suffix
            DiffractionFocussing(InputWorkspace=run_number, OutputWorkspace=focused,
                                 GroupingFileName=grouping_filename, PreserveEvents=False)

            focused_xye = os.path.join(output_path, focused + '.dat')
            focused_nxs = os.path.join(output_path, focused + '.nxs')
            SaveFocusedXYE(focused, focused_xye, SplitFiles=False)
            SaveNexusProcessed(InputWorkspace=focused, Filename=focused_nxs)

            self._focused_workspaces.append(focused)
            DeleteWorkspace(run_number)

        GroupWorkspaces(self._focused_workspaces, OutputWorkspace=group_name)
        self.focused = self._focused_workspaces[0]

    def validate(self):
        self.assertEqual(len(self._focused_workspaces), 1)
        ws = self.focused

        path = config.getString("defaultsave.directory")
        self.assertTrue(os.path.exists(os.path.join(path, ws + ".dat")))
        self.assertTrue(os.path.exists(os.path.join(path, ws + ".nxs")))

        return ws, "35991-foc-h00.nxs"


class WISHDiffractionFocussingAnalysisTest(systemtesting.MantidSystemTest):

    def requiredFiles(self):
        return ["35979-foc-h00.nxs",
                "35980-foc-h00.nxs",
                "35981-foc-h00.nxs",
                "35982-foc-h00.nxs",
                "35983-foc-h00.nxs",
                "35984-foc-h00.nxs",
                "35988-foc-h00.nxs",
                "35991-foc-h00.nxs",
                "35992-foc-h00.nxs",
                "35993-foc-h00.nxs",
                "WISHDiffractionFocussingResult.nxs"]

    def cleanup(self):
        pass

    def runTest(self):
        run_numbers = []
        run_numbers.append(35991)
        run_numbers.extend(range(35979, 35983))
        run_numbers.append(35988)
        run_numbers.extend(range(35983, 35985))

        suffix = "-foc-h00"
        integrate_suffix = "-int"

        min_run = min([x if type(x) == int else min(x) for x in run_numbers])
        max_run = max([x if type(x) == int else max(x) for x in run_numbers])

        group_name = str(min_run) + '-' + str(max_run)
        group_name += suffix + integrate_suffix

        table_name = "Ei=3.5meV (0.219,0,0)"
        log_names = ["MC_temp", "Tesl_setB"]
        integration_range = {"RangeLower": 16.2, "RangeUpper": 17.2}

        table = create_table(table_name, log_names, len(run_numbers))
        run_names = [convert_run_to_name(x, suffix) for x in run_numbers]

        output_names = []
        for i, run in enumerate(run_names):
            if type(run) == list:
                # multiple runs, average over all of them
                for name in run:
                    Load(name + ".nxs", OutputWorkspace=name)
                output_workspace = '&'.join(run) + integrate_suffix
                Mean(','.join(run), OutputWorkspace=output_workspace)
                integrated_workspace = output_workspace
            else:
                # single run, use name as is
                Load(run + ".nxs", OutputWorkspace=run)
                output_workspace = run
                integrated_workspace = output_workspace + integrate_suffix

            # integrate the run
            Integration(InputWorkspace=output_workspace,
                        OutputWorkspace=integrated_workspace,
                        **integration_range)
            w1 = mtd[integrated_workspace]

            # add to table
            row = [get_log(w1, name) for name in log_names]
            row.extend([w1.readY(0), w1.readE(0)])
            row = list(map(float, row))
            table.addRow(row)

            # add to workspace group
            output_names.append(integrated_workspace)

        GroupWorkspaces(output_names, OutputWorkspace=group_name)

        self.table = table
        self.output_names = output_names

    def validate(self):
        self.assertEqual(self.table.rowCount(), 8)
        self.assertEqual(len(self.output_names), 8)
        self.tolerance = 1e-5
        self.tolerance_is_rel_err = True
        return self.table.name(), "WISHDiffractionFocussingResult.nxs"


def create_table(name, columns, num_rows):
    """ Create an empty table workspace with the given columns """
    # build table with log names
    table = CreateEmptyTableWorkspace(OutputWorkspace=name)
    for i, c in enumerate(columns):
        table.addColumn('float', c)
        table.setPlotType(c, 1)

    # Add columns for data from workspace last
    table.addColumn('float', 'int')
    table.setPlotType('int', 2)
    table.addColumn('float', 'error')
    table.setPlotType('error', 5)
    return table


def convert_run_to_name(run_number, suffix):
    """ Convert a list of run numbers to workspace names

    The name will be <runno><suffix>. This can handle
    the case of sublists of run numbers.
    """
    def convert_name(r):
        return str(r) + suffix

    if type(run_number) == list:
        return [convert_name(r) for r in run_number]
    else:
        return convert_name(run_number)


def get_log(ws, name):
    """ Get a log from a workspace.

    If the log contains multiple values then the
    mean of the values will be returned.
    """
    logs = ws.getRun()
    prop = logs.getProperty(name)
    if isinstance(prop.value, np.ndarray):
        return prop.value.mean()
    else:
        return prop.value
