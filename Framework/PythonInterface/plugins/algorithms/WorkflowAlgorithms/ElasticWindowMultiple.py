from __future__ import (absolute_import, division, print_function)

from mantid.simpleapi import *
from mantid.kernel import *
from mantid.api import *


def _normalize_to_lowest_temp(elt_ws_name):
    """
    Normalise a workspace to the lowest temperature run.

    @param elt_ws_name Name of the ELT workspace
    """

    num_hist = elt_ws_name.getNumberHistograms()

    # Normalize each spectrum in the workspace
    for idx in range(0, num_hist):
        y_vals = elt_ws_name.readY(idx)
        scale = 1.0 / y_vals[0]
        y_vals_scaled = scale * y_vals
        elt_ws_name.setY(idx, y_vals_scaled)


class ElasticWindowMultiple(DataProcessorAlgorithm):
    _sample_log_name = None
    _sample_log_value = None
    _input_workspaces = None
    _q_workspace = None
    _q2_workspace = None
    _elf_workspace = None
    _elt_workspace = None
    _integration_range_start = None
    _integration_range_end = None
    _background_range_start = None
    _background_range_end = None

    def category(self):
        return 'Workflow\\Inelastic;Inelastic\\Indirect'

    def summary(self):
        return 'Performs the ElasticWindow algorithm over multiple input workspaces'

    def PyInit(self):
        self.declareProperty(WorkspaceGroupProperty('InputWorkspaces', '', Direction.Input),
                             doc='Grouped input workspaces')

        self.declareProperty(name='IntegrationRangeStart', defaultValue=0.0,
                             doc='Start of integration range in time of flight')
        self.declareProperty(name='IntegrationRangeEnd', defaultValue=0.0,
                             doc='End of integration range in time of flight')

        self.declareProperty(name='BackgroundRangeStart', defaultValue=Property.EMPTY_DBL,
                             doc='Start of background range in time of flight')
        self.declareProperty(name='BackgroundRangeEnd', defaultValue=Property.EMPTY_DBL,
                             doc='End of background range in time of flight')

        self.declareProperty(name='SampleEnvironmentLogName', defaultValue='sample',
                             doc='Name of the sample environment log entry')

        sampEnvLogVal_type = ['last_value', 'average']
        self.declareProperty('SampleEnvironmentLogValue', 'last_value',
                             StringListValidator(sampEnvLogVal_type),
                             doc='Value selection of the sample environment log entry')

        self.declareProperty(WorkspaceProperty('OutputInQ', '', Direction.Output),
                             doc='Output workspace in Q')

        self.declareProperty(WorkspaceProperty('OutputInQSquared', '', Direction.Output),
                             doc='Output workspace in Q Squared')

        self.declareProperty(WorkspaceProperty('OutputELF', '', Direction.Output,
                                               PropertyMode.Optional),
                             doc='Output workspace ELF')

        self.declareProperty(WorkspaceProperty('OutputELT', '', Direction.Output,
                                               PropertyMode.Optional),
                             doc='Output workspace ELT')

    def validateInputs(self):
        issues = dict()

        background_range_start = self.getProperty('BackgroundRangeStart').value
        background_range_end = self.getProperty('BackgroundRangeEnd').value

        if background_range_start != Property.EMPTY_DBL and background_range_end == Property.EMPTY_DBL:
            issues['BackgroundRangeEnd'] = 'If background range start was given and background range end must also be provided.'

        if background_range_start == Property.EMPTY_DBL and background_range_end != Property.EMPTY_DBL:
            issues['BackgroundRangeStart'] = 'If background range end was given and background range start must also be provided.'

        return issues

    def PyExec(self):
        from IndirectCommon import getInstrRun

        # Do setup
        self._setup()

        # Get input workspaces
        input_workspace_names = self._input_workspaces.getNames()

        # Lists of input and output workspaces
        q_workspaces = list()
        q2_workspaces = list()
        run_numbers = list()
        sample_param = list()

        progress = Progress(self, 0.0, 0.05, 3)

        # Perform the ElasticWindow algorithms
        for input_ws in input_workspace_names:

            logger.information('Running ElasticWindow for workspace: %s' % input_ws)
            progress.report('ElasticWindow for workspace: %s' % input_ws)

            q_ws = '__' + input_ws + '_q'
            q2_ws = '__' + input_ws + '_q2'

            elwin_alg = self.createChildAlgorithm("ElasticWindow", enableLogging=False)
            elwin_alg.setProperty("InputWorkspace", input_ws)
            elwin_alg.setProperty("IntegrationRangeStart", self._integration_range_start)
            elwin_alg.setProperty("IntegrationRangeEnd", self._integration_range_end)
            elwin_alg.setProperty("OutputInQ", q_ws)
            elwin_alg.setProperty("OutputInQSquared", q2_ws)

            if self._background_range_start != Property.EMPTY_DBL and self._background_range_end != Property.EMPTY_DBL:
                elwin_alg.setProperty("BackgroundRangeStart", self._background_range_start)
                elwin_alg.setProperty("BackgroundRangeEnd", self._background_range_end)

            elwin_alg.execute()

            log_alg = self.createChildAlgorithm("Logarithm", enableLogging=False)
            log_alg.setProperty("InputWorkspace", elwin_alg.getProperty("OutputInQSquared").value)
            log_alg.setProperty("OutputWorkspace", q2_ws)
            log_alg.execute()

            q_workspaces.append(elwin_alg.getProperty("OutputInQ").value)
            q2_workspaces.append(log_alg.getProperty("OutputWorkspace").value)

            # Get the run number
            run_no = getInstrRun(input_ws)[1]
            run_numbers.append(run_no)

            # Get the sample environment unit
            sample, unit = self._get_sample_units(input_ws)
            if sample is not None:
                sample_param.append(sample)
            else:
                # No need to output a temperature workspace if there are no temperatures
                self._elt_workspace = ''

        logger.information('Creating Q and Q^2 workspaces')
        progress.report('Creating Q workspaces')

        if len(input_workspace_names) == 1:
            # Just rename single workspaces
            self._q_workspace = elwin_alg.getProperty("OutputInQ").value
            self._q2_workspace = log_alg.getProperty("OutputWorkspace").value
        else:
            # Append the spectra of the first two workspaces
            append_alg = self.createChildAlgorithm("AppendSpectra", enableLogging=False)
            append_alg.setProperty("InputWorkspace1", q_workspaces[0])
            append_alg.setProperty("InputWorkspace2", q_workspaces[1])
            append_alg.setProperty("OutputWorkspace", self._q_workspace)
            append_alg.execute()
            self._q_workspace = append_alg.getProperty("OutputWorkspace").value
            append_alg.setProperty("InputWorkspace1", q2_workspaces[0])
            append_alg.setProperty("InputWorkspace2", q2_workspaces[1])
            append_alg.setProperty("OutputWorkspace", self._q2_workspace)
            append_alg.execute()
            self._q2_workspace = append_alg.getProperty("OutputWorkspace").value

            # Append to the spectra of each remaining workspace
            for idx in range(2, len(input_workspace_names)):
                append_alg.setProperty("InputWorkspace1", self._q_workspace)
                append_alg.setProperty("InputWorkspace2", q_workspaces[idx])
                append_alg.setProperty("OutputWorkspace", self._q_workspace)
                append_alg.execute()
                self._q_workspace = append_alg.getProperty("OutputWorkspace").value
                append_alg.setProperty("InputWorkspace1", self._q2_workspace)
                append_alg.setProperty("InputWorkspace2", q2_workspaces[idx])
                append_alg.setProperty("OutputWorkspace", self._q2_workspace)
                append_alg.execute()
                self._q2_workspace = append_alg.getProperty("OutputWorkspace").value

        # Set the vertical axis units
        v_axis_is_sample = len(input_workspace_names) == len(sample_param)

        if v_axis_is_sample:
            logger.notice('Vertical axis is in units of %s' % unit)
            unit = (self._sample_log_name, unit)
        else:
            logger.notice('Vertical axis is in run number')
            unit = ('Run No', 'last 3 digits')

        # Create a new vertical axis for the Q and Q**2 workspaces
        q_ws_axis = NumericAxis.create(len(input_workspace_names))
        q_ws_axis.setUnit("Label").setLabel(unit[0], unit[1])

        q2_ws_axis = NumericAxis.create(len(input_workspace_names))
        q2_ws_axis.setUnit("Label").setLabel(unit[0], unit[1])

        # Set the vertical axis values
        for idx in range(0, len(input_workspace_names)):
            if v_axis_is_sample:
                q_ws_axis.setValue(idx, float(sample_param[idx]))
                q2_ws_axis.setValue(idx, float(sample_param[idx]))
            else:
                q_ws_axis.setValue(idx, float(run_numbers[idx][-3:]))
                q2_ws_axis.setValue(idx, float(run_numbers[idx][-3:]))

        # Add the new vertical axis to each workspace
        self._q_workspace.replaceAxis(1, q_ws_axis)
        self._q2_workspace.replaceAxis(1, q2_ws_axis)

        progress.report('Creating ELF workspaces')
        transpose_alg = self.createChildAlgorithm("Transpose", enableLogging=False)
        sort_alg = self.createChildAlgorithm("SortXAxis", enableLogging=False)
        # Process the ELF workspace
        if self._elf_workspace != '':
            logger.information('Creating ELF workspace')
            transpose_alg.setProperty("InputWorkspace", self._q_workspace)
            transpose_alg.setProperty("OutputWorkspace", self._elf_workspace)
            transpose_alg.execute()

            sort_alg.setProperty("InputWorkspace",transpose_alg.getProperty("OutputWorkspace").value)
            sort_alg.setProperty("OutputWorkspace", self._elf_workspace)
            sort_alg.execute()

            self.setProperty('OutputELF', sort_alg.getProperty("OutputWorkspace").value)

        # Do temperature normalisation
        if self._elt_workspace != '':
            logger.information('Creating ELT workspace')

            # If the ELT workspace was not already created then create it here,
            # otherwise just clone it
            if self._elf_workspace == '':
                transpose_alg.setProperty("InputWorkspace", self._q_workspace)
                transpose_alg.setProperty("OutputWorkspace", self._elt_workspace)
                transpose_alg.execute()
                sort_alg.setProperty("InputWorkspace", self._elt_workspace)
                sort_alg.setProperty("OutputWorkspace", self._elt_workspace)
                sort_alg.execute()
                self._elt_workspace = sort_alg.getProperty("OutputWorkspace").value
            else:
                clone_alg = self.createChildAlgorithm("CloneWorkspace", enableLogging=False)
                clone_alg.setProperty("InputWorkspace", self.getProperty("OutputELF").value)
                clone_alg.setProperty("OutputWorkspace", self._elt_workspace)
                clone_alg.execute()
                self._elt_workspace = clone_alg.getProperty("OutputWorkspace").value

            _normalize_to_lowest_temp(self._elt_workspace)

            self.setProperty('OutputELT', self._elt_workspace)

        # Set the output workspace
        self.setProperty('OutputInQ', self._q_workspace)
        self.setProperty('OutputInQSquared', self._q2_workspace)

    def _setup(self):
        """
        Gets algorithm properties.
        """

        self._sample_log_name = self.getPropertyValue('SampleEnvironmentLogName')
        self._sample_log_value = self.getPropertyValue('SampleEnvironmentLogValue')

        self._input_workspaces = self.getProperty('InputWorkspaces').value
        self._q_workspace = self.getPropertyValue('OutputInQ')
        self._q2_workspace = self.getPropertyValue('OutputInQSquared')
        self._elf_workspace = self.getPropertyValue('OutputELF')
        self._elt_workspace = self.getPropertyValue('OutputELT')

        self._integration_range_start = self.getProperty('IntegrationRangeStart').value
        self._integration_range_end = self.getProperty('IntegrationRangeEnd').value

        self._background_range_start = self.getProperty('BackgroundRangeStart').value
        self._background_range_end = self.getProperty('BackgroundRangeEnd').value

    def _get_sample_units(self, ws_name):
        """
        Gets the sample environment units for a given workspace.

        @param ws_name Name of workspace
        @returns sample in given units or None if not found
        """
        from IndirectCommon import getInstrRun

        instr, run_number = getInstrRun(ws_name)

        facility = config.getFacility()
        pad_num = facility.instrument(instr).zeroPadding(int(run_number))
        zero_padding = '0' * (pad_num - len(run_number))

        run_name = instr + zero_padding + run_number
        log_filename = run_name.upper() + '.log'

        run = mtd[ws_name].getRun()

        if self._sample_log_name == 'Position':
            # Look for sample changer position in logs in workspace
            if self._sample_log_name in run:
                tmp = run[self._sample_log_name].value
                value_action = {'last_value': lambda x: x[len(x) - 1],
                                'average': lambda x: x.mean()}
                position = value_action['last_value'](tmp)
                if position == 0:
                    self._sample_log_name = 'Bot_Can_Top'
                if position == 1:
                    self._sample_log_name = 'Middle_Can_Top'
                if position == 2:
                    self._sample_log_name = 'Top_Can_Top'
            else:
                logger.information('Position not found in workspace.')

        if self._sample_log_name in run:
            # Look for sample unit in logs in workspace
            tmp = run[self._sample_log_name].value
            value_action = {'last_value': lambda x: x[len(x) - 1],
                            'average': lambda x: x.mean()}
            sample = value_action[self._sample_log_value](tmp)
            unit = run[self._sample_log_name].units
            logger.information('%d %s found for run: %s' % (sample, unit, run_name))
            return sample, unit

        else:
            # Logs not in workspace, try loading from file
            logger.information('Log parameter not found in workspace. Searching for log file.')
            log_path = FileFinder.getFullPath(log_filename)

            if log_path != '':
                # Get temperature from log file
                LoadLog(Workspace=ws_name, Filename=log_path)
                run_logs = mtd[ws_name].getRun()
                if self._sample_log_name in run_logs:
                    tmp = run_logs[self._sample_log_name].value
                    sample = tmp[len(tmp) - 1]
                    unit = run[self._sample_log_name].units
                    logger.debug('%d %s found for run: %s' % (sample, unit, run_name))
                    return sample, unit
                else:
                    logger.warning('Log entry %s for run %s not found' % (self._sample_log_name, run_name))
            else:
                logger.warning('Log file for run %s not found' % run_name)

        # Can't find log file
        logger.warning('No sample units found for run: %s' % run_name)
        return None, None


# Register algorithm with Mantid
AlgorithmFactory.subscribe(ElasticWindowMultiple)
