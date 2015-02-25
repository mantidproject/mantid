#pylint: disable=no-init
from mantid.simpleapi import *
from mantid.kernel import *
from mantid.api import *


def _normalize_to_lowest_temp(elt_ws_name):
    """
    Normalise a workspace to the lowest temperature run.

    @param elt_ws_name Name of the ELT workspace
    """

    num_hist = mtd[elt_ws_name].getNumberHistograms()

    # Normalize each spectrum in the workspace
    for idx in range(0, num_hist):
        y_vals = mtd[elt_ws_name].readY(idx)
        scale = 1.0 / y_vals[0]
        y_vals_scaled = scale * y_vals
        mtd[elt_ws_name].setY(idx, y_vals_scaled)


class ElasticWindowMultiple(DataProcessorAlgorithm):

    _plot = None
    _sample_log_name = None
    _input_workspaces = None
    _q_workspace = None
    _q2_workspace = None
    _elf_workspace = None
    _elt_workspace = None
    _range_1_start = None
    _range_1_end = None
    _range_2_start = None
    _range_2_end = None

    def category(self):
        return 'Workflow\\Inelastic;PythonAlgorithms;Inelastic'


    def summary(self):
        return 'Performs the ElasticWindow algorithm over multiple input workspaces'


    def PyInit(self):
        self.declareProperty(WorkspaceGroupProperty('InputWorkspaces', '', Direction.Input),
                             doc='Grouped input workspaces')

        self.declareProperty(name='Range1Start', defaultValue=0.0, doc='Range 1 start')
        self.declareProperty(name='Range1End', defaultValue=0.0, doc='Range 1 end')

        self.declareProperty(name='Range2Start', defaultValue='', doc='Range 2 start')
        self.declareProperty(name='Range2End', defaultValue='', doc='Range 2 end')

        self.declareProperty(name='SampleEnvironmentLogName', defaultValue='sample',
                             doc='Name of the sample environment log entry')

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

        self.declareProperty(name='Plot', defaultValue=False, doc='Plot result spectra')


    def validateInputs(self):
        issues = dict()

        range_2_start = self.getPropertyValue('Range2Start')
        range_2_end = self.getPropertyValue('Range2End')

        if range_2_start != '' and range_2_end == '':
            issues['Range2End'] = 'If range 2 start was given and range 2 end must also be provided.'

        if range_2_start == '' and range_2_end != '':
            issues['Range2Start'] = 'If range 2 end was given and range 2 start must also be provided.'

        if range_2_start != '':
            try:
                _ = float(range_2_start)
            except ValueError:
                issues['Range2Start'] = 'Range 2 start is not a double number'

        if range_2_end != '':
            try:
                _ = float(range_2_end)
            except ValueError:
                issues['Range2End'] = 'Range 2 end is not a double number'

        return issues


    def PyExec(self):
        from IndirectImport import import_mantidplot
        from IndirectCommon import getInstrRun

        # Do setup
        self._setup()

        logger.debug('in_ws:'+str(type(self._input_workspaces)))

        # Get input workspaces
        input_workspace_names = self._input_workspaces.getNames()

        # Lists of input and output workspaces
        q_workspaces = list()
        q2_workspaces = list()
        run_numbers = list()
        temperatures = list()

        # Perform the ElasticWindow algorithms
        for input_ws in input_workspace_names:
            logger.information('Running ElasticWindow for workspace: %s' % input_ws)

            q_ws = '__' + input_ws + '_q'
            q2_ws = '__' + input_ws + '_q2'

            if self._range_2_start != '' and self._range_2_end != '':
                ElasticWindow(InputWorkspace=input_ws,
                              OutputInQ=q_ws, OutputInQSquared=q2_ws,
                              Range1Start=self._range_1_start,
                              Range1End=self._range_1_end,
                              Range2Start=float(self._range_2_start),
                              Range2End=float(self._range_2_end))
            else:
                ElasticWindow(InputWorkspace=input_ws, OutputInQ=q_ws, OutputInQSquared=q2_ws,
                              Range1Start=self._range_1_start, Range1End=self._range_1_end)

            Logarithm(InputWorkspace=q2_ws, OutputWorkspace=q2_ws)

            q_workspaces.append(q_ws)
            q2_workspaces.append(q2_ws)

            # Get the run number
            run_no = getInstrRun(input_ws)[1]
            run_numbers.append(run_no)

            # Get the sample temperature
            temp = self._get_temperature(input_ws)
            if temp is not None:
                temperatures.append(temp)

        logger.information('Creating Q and Q^2 workspaces')

        if len(input_workspace_names) == 1:
            # Just rename single workspaces
            RenameWorkspace(InputWorkspace=q_workspaces[0], OutputWorkspace=self._q_workspace)
            RenameWorkspace(InputWorkspace=q2_workspaces[0], OutputWorkspace=self._q2_workspace)
        else:
            # Append the spectra of the first two workspaces
            AppendSpectra(InputWorkspace1=q_workspaces[0], InputWorkspace2=q_workspaces[1],
                          OutputWorkspace=self._q_workspace)
            AppendSpectra(InputWorkspace1=q2_workspaces[0], InputWorkspace2=q2_workspaces[1],
                          OutputWorkspace=self._q2_workspace)

            # Append to the spectra of each remaining workspace
            for idx in range(2, len(input_workspace_names)):
                AppendSpectra(InputWorkspace1=self._q_workspace,
                              InputWorkspace2=q_workspaces[idx],
                              OutputWorkspace=self._q_workspace)
                AppendSpectra(InputWorkspace1=self._q2_workspace,
                              InputWorkspace2=q2_workspaces[idx],
                              OutputWorkspace=self._q2_workspace)

            # Delete the output workspaces from the ElasticWindow algorithms
            for q_ws in q_workspaces:
                DeleteWorkspace(q_ws)
            for q2_ws in q2_workspaces:
                DeleteWorkspace(q2_ws)

        logger.information('Setting vertical axis units and values')

        # Set the verical axis units
        v_axis_is_temp = len(input_workspace_names) == len(temperatures)

        if v_axis_is_temp:
            logger.notice('Vertical axis is in temperature')
            unit = ('Temperature', 'K')
        else:
            logger.notice('Vertical axis is in run number')
            unit = ('Run No', 'last 3 digits')

        q_ws_axis = mtd[self._q_workspace].getAxis(1)
        q_ws_axis.setUnit("Label").setLabel(unit[0], unit[1])

        q2_ws_axis = mtd[self._q2_workspace].getAxis(1)
        q2_ws_axis.setUnit("Label").setLabel(unit[0], unit[1])

        # Set the vertical axis values
        for idx in range(0, len(input_workspace_names)):
            if v_axis_is_temp:
                q_ws_axis.setValue(idx, float(temperatures[idx]))
                q2_ws_axis.setValue(idx, float(temperatures[idx]))
            else:
                q_ws_axis.setValue(idx, float(run_numbers[idx][-3:]))
                q2_ws_axis.setValue(idx, float(run_numbers[idx][-3:]))

        # Process the ELF workspace
        if self._elf_workspace != '':
            logger.information('Creating ELF workspace')

            Transpose(InputWorkspace=self._q_workspace, OutputWorkspace=self._elf_workspace)
            SortXAxis(InputWorkspace=self._elf_workspace, OutputWorkspace=self._elf_workspace)

            self.setProperty('OutputELF', self._elf_workspace)

        # Do temperature normalisation
        if self._elt_workspace != '':
            logger.information('Creating ELT workspace')

            # If the ELT workspace was not already created then create it here,
            # otherwise just clone it
            if self._elf_workspace == '':
                Transpose(InputWorkspace=self._q_workspace, OutputWorkspace=self._elt_workspace)
                SortXAxis(InputWorkspace=self._elt_workspace, OutputWorkspace=self._elt_workspace)
            else:
                CloneWorkspace(InputWorkspace=self._elf_workspace,
                               OutputWorkspace=self._elt_workspace)

            _normalize_to_lowest_temp(self._elt_workspace)

            self.setProperty('OutputELT', self._elt_workspace)

        # Set the output workspace
        self.setProperty('OutputInQ', self._q_workspace)
        self.setProperty('OutputInQSquared', self._q2_workspace)

        # Plot spectra plots
        if self._plot:
            self._mtd_plot = import_mantidplot()

            self._plot_spectra(self._q_workspace)
            self._plot_spectra(self._q2_workspace)

            if self._elf_workspace != '':
                self._plot_spectra(self._elf_workspace)

            if self._elt_workspace != '':
                self._plot_spectra(self._elt_workspace)


    def _setup(self):
        """
        Gets algorithm properties.
        """

        self._plot = self.getProperty('Plot').value
        self._sample_log_name = self.getPropertyValue('SampleEnvironmentLogName')

        self._input_workspaces = self.getProperty('InputWorkspaces').value
        self._q_workspace = self.getPropertyValue('OutputInQ')
        self._q2_workspace = self.getPropertyValue('OutputInQSquared')
        self._elf_workspace = self.getPropertyValue('OutputELF')
        self._elt_workspace = self.getPropertyValue('OutputELT')

        self._range_1_start = self.getProperty('Range1Start').value
        self._range_1_end = self.getProperty('Range1End').value

        self._range_2_start = self.getPropertyValue('Range2Start')
        self._range_2_end = self.getPropertyValue('Range2End')


    def _plot_spectra(self, ws_name):
        """
        Plots up to the first 10 spectra from a workspace.

        @param ws_name Name of workspace to plot
        """

        num_hist = mtd[ws_name].getNumberHistograms()

        # Limit number of plotted histograms to 10
        if num_hist > 10:
            num_hist = 10

        # Build plot list
        plot_list = []
        for i in range(0, num_hist):
            plot_list.append(i)

        self._mtd_plot.plotSpectrum(ws_name, plot_list)


    def _get_temperature(self, ws_name):
        """
        Gets the sample temperature for a given workspace.

        @param ws_name Name of workspace
        @returns Temperature in Kelvin or None if not found
        """
        from IndirectCommon import getInstrRun

        instr, run_number = getInstrRun(ws_name)

        facility = config.getFacility()
        pad_num = facility.instrument(instr).zeroPadding(int(run_number))
        zero_padding = '0' * (pad_num - len(run_number))

        run_name = instr + zero_padding + run_number
        log_filename = run_name.upper() + '.log'

        run = mtd[ws_name].getRun()

        if self._sample_log_name in run:
            # Look for temperature in logs in workspace
            tmp = run[self._sample_log_name].value
            temp = tmp[len(tmp) - 1]
            logger.debug('Temperature %d K found for run: %s' % (temp, run_name))
            return temp

        else:
            # Logs not in workspace, try loading from file
            logger.information('Log parameter not found in workspace. Searching for log file.')
            log_path = FileFinder.getFullPath(log_filename)

            if log_path != '':
                # Get temperature from log file
                LoadLog(Workspace=ws_name, Filename=log_path)
                run_logs = mtd[ws_name].getRun()
                tmp = run_logs[self._sample_log_name].value
                temp = tmp[len(tmp) - 1]
                logger.debug('Temperature %d K found for run: %s' % (temp, run_name))
                return temp

            else:
                # Can't find log file
                logger.warning('Log file for run %s not found' % run_name)
                logger.warning('No temperature found for run: %s' % run_name)
                return None


# Register algorithm with Mantid
AlgorithmFactory.subscribe(ElasticWindowMultiple)
