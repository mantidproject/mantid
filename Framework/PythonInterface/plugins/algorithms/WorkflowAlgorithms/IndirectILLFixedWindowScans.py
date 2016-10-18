#pylint: disable=no-init
from __future__ import (absolute_import, division, print_function)

<<<<<<< HEAD

from mantid.simpleapi import *
from mantid.api import DataProcessorAlgorithm, AlgorithmFactory, \
    PropertyMode, MatrixWorkspaceProperty, WorkspaceGroupProperty, \
    FileProperty, MultipleFileProperty, FileAction, Progress
from mantid.kernel import Direction, logger, StringListValidator
import numpy as np
=======
>>>>>>> bfb96d2fc647cadcabbf8b7a89e91e9f35a80ec0
import re
import numpy as np
from mantid.simpleapi import *
from mantid.api import *
from mantid.kernel import *

from IndirectILLReduction import IndirectILLReduction

# This function already exists in IndirectILLReduction
def monitor_range(workspace):
    """
    Get sensible x-range where monitor count is not zero
    Used to mask out the first and last few channels
    @param workspace :: name of workspace
    @return   :: tuple of xmin and xmax
    """
    x_values = mtd[workspace].readX(0)
    y_values = mtd[workspace].readY(0)
    # mid x value in order to search for left and right monitor range delimiter
    size = len(x_values)
    # Maximum search in left and right half of the workspace
    mid = int(size / 2)
    # Maximum position left (differs from IndirectILLReduction)
    imin = np.nanargmax(np.array(y_values[0:mid]))
    # Maximum position right
    imax = np.nanargmax(np.array(y_values[mid:size])) + 1 + mid
    return x_values[imin], x_values[imax]


class IndirectILLFixedWindowScans(DataProcessorAlgorithm):

    _run_file = None
    _vanadium_file = None
    _background_file = None
    _debug_mode = None
    _out_ws = None
    _analyser = None
    _reflection = None
    selected_runs = None

    def category(self):
        return 'Workflow\\MIDAS;Inelastic\\Reduction'

    def summary(self):
        return 'Reduction for IN16B elastic and inelastic fixed-window scans.'

    def name(self):
        return "IndirectILLFixedWindowScans"

    def PyInit(self):
        self.declareProperty(MultipleFileProperty(name='Run', extensions=['nxs']),
                             doc='List of input file (s)')

        self.declareProperty(FileProperty('EvaluationEntity', ''),
                             doc='Post-processing entity (Nexus-file entry).')

        self.declareProperty(FileProperty('MapFile', '',
                                          action=FileAction.OptionalLoad,
                                          extensions=['xml']),
                             doc='Filename of the detector grouping map file to use. \n'
                                 'If left blank the default will be used.')

        self.declareProperty(name='Analyser',
                             defaultValue='silicon',
                             validator=StringListValidator(['silicon']),
                             doc='Analyser crystal.')

        self.declareProperty(name='Reflection',
                             defaultValue='111',
                             validator=StringListValidator(['111', '311']),
                             doc='Analyser reflection.')

        self.declareProperty(name='DebugMode',
                             defaultValue=False,
                             doc='Whether to output the workspaces in intermediate steps.')

        self.declareProperty(FileProperty('BackgroundRun', '',
                                          action=FileAction.OptionalLoad,
                                          extensions=['nxs']),
                             doc='File path of background run.')

        self.declareProperty(MatrixWorkspaceProperty('CalibrationWorkspace', '',
                                                     direction=Direction.Input,
                                                     optional=PropertyMode.Optional),
                             doc='Workspace containing calibration intensities for each detector')

        self.declareProperty(WorkspaceGroupProperty('OutputWorkspace', 'output',
                                                    direction=Direction.Output),
                             doc='Output workspace group')

    def validateInputs(self):

        issues = dict()

        return issues

    def setUp(self):

        self._run_file = self.getPropertyValue('Run')
        self._map_file = self.getPropertyValue('MapFile')
        self._analyser = self.getPropertyValue('Analyser')
        self._reflection = self.getPropertyValue('Reflection')
        self._debug_mode = self.getProperty('DebugMode').value
        self._out_ws = self.getPropertyValue('OutputWorkspace')

    def PyExec(self):
        self.setUp()

        self.log().information('Call IndirectILLReduction for .nxs file(s) : {0}'.format(self._run_file))
        IndirectILLReduction(Run=self._run_file, MapFile=self._map_file, Analyser=self._analyser,
                             Reflection=self._reflection, ReductionType='FWS', DebugMode=self._debug_mode,
                             UnmirrorOption=1, OutputWorkspace=self._out_ws)

        self.selected_runs = []

        # Figure out number of progress reports, i.e. one for each input workspace/file
        progress = Progress(self, start=0.0, end=1.0, nreports=mtd[self._out_ws].size())

        # Traverse over items in workspace group and reduce individually
        for i in range(mtd[self._out_ws].size()):
            # Get name of the i-th workspace
            input_ws = format(mtd[self._out_ws].getItem(i).getName())

            progress.report("Reducing run #" + input_ws)

            self._reduce_run(input_ws)

            self.selected_runs.append(input_ws)

        self._set_workspace_properties()

    def _reduce_run(self, input_ws):
        """
        Performs the reduction for a given single run according to Doppler settings
        @param input_ws :: string of input workspace to reduce, will be overridden
        """
        self.log().information('Reducing run #' + input_ws)
        x_values = mtd[input_ws].readX(0)

        energy = mtd[input_ws].getRun().getLogData('Doppler.maximum_delta_energy').value

        if energy == 0.:
            # Elastic
            # Attention: after the reduction, all x-values are zero!
            x_min = - 0.5
            x_max = + 0.5
            logger.information('EFWS scan from {0} to {1}'.format(x_min, x_max))
            Integration(InputWorkspace=input_ws, OutputWorkspace=input_ws,
                        RangeLower=x_min, RangeUpper=x_max)
        else:
            # Inelastic
            # Get the two maximum peak positions of the inelastic fixed-window scan
            x_min, x_max = monitor_range(input_ws)
            # Enlarge integration interval on left and right side, take into account the overall blocksize (in general
            # variable for each experiment)
            int_interval = int(mtd[input_ws].blocksize() / 50)
            delta_x = x_values[1] - x_values[0]
            x_min += int_interval * delta_x
            x_max -= int_interval * delta_x
            self.log().information('IFWS scan ranges [{0} {1}] and [{2} {3}]'.format
                                   (x_values[0], x_min, x_max, x_values[len(x_values) - 1]))
            Integration(InputWorkspace=input_ws, OutputWorkspace='__left',
                        RangeLower=x_values[0], RangeUpper=x_min)
            Integration(InputWorkspace=input_ws, OutputWorkspace='__right',
                        RangeLower=x_max, RangeUpper=x_values[len(x_values) - 1])
            Plus(LHSWorkspace='__left', RHSWorkspace='__right', OutputWorkspace=input_ws)

    def _set_workspace_properties(self):

        number_of_workspaces = mtd[self._out_ws].getNumberOfEntries()

        # Throw error if there are no workspaces in the OutputWorkspace
        try:
            if number_of_workspaces > 0:
                self.setProperty('OutputWorkspace', self._out_ws)
        except KeyError:
            raise ValueError('No valid output workspace(s), check data type.')

        energies = []

        # Count appearances of energy, this will be the number of GroupWorkspaces needed
        for i in range(number_of_workspaces):
            the_workspace = mtd[self._out_ws].getItem(i).getRun()
            if the_workspace.hasProperty('Doppler.maximum_delta_energy'):
                energy = the_workspace.getLogData('Doppler.maximum_delta_energy').value
            else:
                energy = float('nan')
            energies.append(energy)

        # return_counts would be nice to use here but requires numpy version 1.9.0
        energy_values, indices = np.unique(energies, return_inverse=True)
        self.log().information('FWS energies: {0}'.format(energy_values))
        self.log().debug('Corresponding workspaces (index): {0}'.format(indices))
        number_groups = len(energy_values)
        self.log().debug('Number of GroupWorkspaces {0}'.format(number_groups))

        _selected_runs = []
        for i in range(number_of_workspaces):
            _selected_runs.append(re.findall(r'\d+', self.selected_runs[i]))
        self.log().debug('List of runs: {0}'.format(_selected_runs))

        UnGroupWorkspace(self._out_ws)
        # Reset the name
        self._out_ws = self.getPropertyValue('OutputWorkspace')

        for i in range(number_groups):
            # Create a new list for each new group of workspaces
            group = []
            # Create GroupWorkspace name of the energy group
            group_name = self._out_ws + '_' + str(energy_values[i])

            # Add the workspaces to the group, which belong to the same energy
            for j in range(number_of_workspaces):
                if i == indices[j]:
                    # Pick workspace with same energy value
                    self.log().debug('Workspace {0} for GroupWorkspace of energy {1}'.format
                                     (_selected_runs[j][0] + '_' + self._out_ws, j))
                    group.append(_selected_runs[j][0] + '_' + self._out_ws)

            # Create the GroupWorkspace and set workspace property
            GroupWorkspaces(InputWorkspaces=group, OutputWorkspace=group_name)
            # Set output workspace properties accordingly
            self.setProperty('OutputWorkspace', group_name)

AlgorithmFactory.subscribe(IndirectILLFixedWindowScans)
