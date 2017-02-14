from __future__ import (absolute_import, division, print_function)
import io
import numpy as np
from mantid.api import AlgorithmFactory, FileAction, FileProperty, PythonAlgorithm, WorkspaceProperty, mtd, Progress
from mantid.kernel import Direction, logger
from mantid.simpleapi import CreateWorkspace, GroupWorkspaces


# noinspection PyPep8Naming,PyMethodMayBeStatic
class LoadAclimax(PythonAlgorithm):

    _aclimax_file = None
    _out_ws_name = None

    def category(self):
        return "Simulation"

    def summary(self):
        return "Loads aCLIMAX csv file into workspaces."

    def PyInit(self):

        self.declareProperty(FileProperty("aClimaxCVSFile", "",
                                          action=FileAction.Load,
                                          direction=Direction.Input,
                                          extensions=["csv"]),
                             doc="File with INS data calculated by aCLIMAX.")

        self.declareProperty(WorkspaceProperty("OutputWorkspace", '', Direction.Output),
                             doc="Name to give for the output workspace.")

    def validateInputs(self):

        issues = dict()

        aclimax_file = self.getProperty("aClimaxCVSFile").value
        if aclimax_file == "":
            issues["aClimaxCVSFile"] = "Please specify name of aCLIMAX csv file to load."

        workspace_name = self.getPropertyValue("OutputWorkspace")
        if workspace_name in mtd:
            issues["OutputWorkspace"] = "Workspace with name " + workspace_name + " already in use; please give " \
                                                                                  "a different name for workspace."
        elif workspace_name == "":
            issues["OutputWorkspace"] = "Please specify name of workspace."

        return issues

    def PyExec(self):

        steps = 3
        begin = 0
        end = 1.0
        prog_reporter = Progress(self, begin, end, steps)

        self._get_properties()
        prog_reporter.report("Input data from the user has been collected.")

        self._create_workspaces()
        prog_reporter.report("Partial workspaces have been created.")

        self.setProperty('OutputWorkspace', self._out_ws_name)
        prog_reporter.report("Final workspace has been created.")

    def _create_workspaces(self):
        """
        Given the following format of .csv file:

            comment1
            comment2
            comment3
            x1 y1 y2 y3 y4 y5 y6 y7 y8 y9

        Where x1, yn are columns
        Creates five workspaces:

            wrk_1 -- created from x1 and y1
            wrk_2 -- created from x1 and y2
            wrk_3 -- created from x1 and y3
            wrk_4 -- created from x1 and y4
            wrk_5 -- created from x1 and y1 + y2 + y3 + y4

        All workspaces are grouped in group workspace out_ws_name.
        """
        # read data
        with io.open(self._aclimax_file, 'rt', newline='') as f:
            lines = f.readlines()

        x_indx = 0
        x = []

        y_indices = [1, 2, 3, 4]
        y_indx_shift = 1
        y = [[], [], [], []]

        # check if number of columns in .csv file is correct
        allowed_size = 10
        begin_data = 3

        size = len(lines[begin_data].rstrip().split(','))
        if size < allowed_size:
            raise ValueError("Invalid number of columns in " + self._aclimax_file + ".")

        # convert data to numpy arrays
        for line in lines[begin_data:]:
            tmp = line.rstrip().split(',')
            x.append(float(tmp[x_indx]))
            for y_i in y_indices:
                y[y_i - y_indx_shift].append(float(tmp[y_i]))

        num_partial_wrk = 4
        wrk_names = []

        # partial workspaces
        x = np.asarray(x)
        for i in range(num_partial_wrk):
            wrk_names.append(self._out_ws_name + "_quantum_event_%s" % (i + 1))
            y[i] = np.asarray(y[i])
            CreateWorkspace(DataX=x, DataY=y[i], OutputWorkspace=wrk_names[i])
            self._set_workspace_units(wrk=wrk_names[i])

        # total workspace
        total = self._out_ws_name + "_total"
        CreateWorkspace(DataX=x, DataY=y[0] + y[1] + y[2] + y[3], OutputWorkspace=total)
        self._set_workspace_units(wrk=total)

        # pack everything into group workspace
        wrk_names.append(total)
        group = ','.join(wrk_names)
        GroupWorkspaces(group, OutputWorkspace=self._out_ws_name)

    def _get_properties(self):

        self._aclimax_file = self.getProperty("aClimaxCVSFile").value
        self._out_ws_name = self.getPropertyValue('OutputWorkspace')

    def _set_workspace_units(self, wrk=None):
        """
        Sets x and y units for a workspace.
        @param wrk: workspace which units should be set
        """

        unitx = mtd[wrk].getAxis(0).setUnit("Label")
        unitx.setLabel("Energy Loss", 'cm<sup>-1</sup>')

        mtd[wrk].setYUnitLabel("S /Arbitrary Units")
        mtd[wrk].setYUnit("Arbitrary Units")

try:
    AlgorithmFactory.subscribe(LoadAclimax)
except ImportError:
    logger.debug('Failed to subscribe algorithm LoadAClimax; The python package may be missing.')
