from mantid.simpleapi import *
from mantid.kernel import *
from mantid.api import *


class ElasticWindowMultiple(DataProcessorAlgorithm):

    def category(self):
        return 'Workflow\\Inelastic;PythonAlgorithms;Inelastic'


    def summary(self):
        return 'Performs the ElasticWindow algorithm over multiple input workspaces'


    def PyInit(self):
        self.declareProperty(WorkspaceGroupProperty('InputWorkspaces', '', Direction.Input),
                             doc='Resolution workspace')

        self.declareProperty(name='Range1Start', defaultValue=0.0, doc='Range 1 start')
        self.declareProperty(name='Range1End', defaultValue=0.0, doc='Range 1 end')

        self.declareProperty(name='Range2Start', defaultValue='', doc='Range 2 start')
        self.declareProperty(name='Range2End', defaultValue='', doc='Range 2 end')

        self.declareProperty(WorkspaceProperty('OutputInQ', '', Direction.Output),
                             doc='Output workspace in Q')

        self.declareProperty(WorkspaceProperty('OutputInQSquared', '', Direction.Output),
                             doc='Output workspace in Q Squared')

        self.declareProperty(WorkspaceProperty('OutputELF', '', Direction.Output, PropertyMode.Optional),
                             doc='Output workspace')

        self.declareProperty(name='Plot', defaultValue=False, doc='Plot result spectra')


    def validateInputs(self):
        issues = dict()

        input_workspaces = self.getProperty('InputWorkspaces').value
        if len(input_workspaces.getNames()) < 2:
            issues['InputWorkspaces'] = 'Input workspace group must contain at least 2 workspaces'

        range_2_start = self.getPropertyValue('Range2Start')
        range_2_end = self.getPropertyValue('Range2End')

        if range_2_start != '' and range_2_end == '':
            issues['Range2End'] = 'If range 2 start was given and range 2 end must also be provided.'

        if range_2_start == '' and range_2_end != '':
            issues['Range2Start'] = 'If range 2 end was given and range 2 start must also be provided.'

        if range_2_start != '':
            try:
                val = float(range_2_start)
            except ValueError:
                issues['Range2Start'] = 'Range 2 start is not a double number'

        if range_2_end != '':
            try:
                val = float(range_2_end)
            except ValueError:
                issues['Range2End'] = 'Range 2 end is not a double number'

        return issues


    def PyExec(self):
        from IndirectCommon import getInstrRun
        from IndirectImport import import_mantidplot

        # Do setup
        self._setup()

        # Lists of input and output workspaces
        input_workspace_names = self._input_workspaces.getNames()
        q_workspaces = list()
        q2_workspaces = list()
        run_numbers = list()

        # Perform the ElasticWindow algorithms
        for input_ws in input_workspace_names:
            q_ws = input_ws + '_q'
            q2_ws = input_ws + '_q2'

            if self._range_2_start != '' and self._range_2_end != '':
                ElasticWindow(InputWorkspace=input_ws, OutputInQ=q_ws, OutputInQSquared=q2_ws,
                              Range1Start=self._range_1_start, Range1End=self._range_1_end,
                              Range2Start=float(self._range_2_start), Range2End=float(self._range_2_end))
            else:
                ElasticWindow(InputWorkspace=input_ws, OutputInQ=q_ws, OutputInQSquared=q2_ws,
                              Range1Start=self._range_1_start, Range1End=self._range_1_end)

            q_workspaces.append(q_ws)
            q2_workspaces.append(q2_ws)

            # Get the run number
            run_no = getInstrRun(input_ws)[1]
            run_numbers.append(run_no)

        # Must have two of each type of workspace to continue
        if len(q_workspaces) < 2:
            raise RuntimeError('Have less than 2 result workspaces in Q')
        if len(q2_workspaces) < 2:
            raise RuntimeError('Hvae less than 2 result workspaces in Q^2')

        # Append the spectra of the first two workspaces
        AppendSpectra(InputWorkspace1=q_workspaces[0], InputWorkspace2=q_workspaces[1], OutputWorkspace=self._q_workspace)
        AppendSpectra(InputWorkspace1=q2_workspaces[0], InputWorkspace2=q2_workspaces[1], OutputWorkspace=self._q2_workspace)

        # Append to the spectra of each remaining workspace
        for idx in range(2, len(input_workspace_names)):
            AppendSpectra(InputWorkspace1=self._q_workspace, InputWorkspace2=q_workspaces[idx], OutputWorkspace=self._q_workspace)
            AppendSpectra(InputWorkspace1=self._q2_workspace, InputWorkspace2=q2_workspaces[idx], OutputWorkspace=self._q2_workspace)

        # Delete the output workspaces from the ElasticWindow algorithms
        for q_ws in q_workspaces:
            DeleteWorkspace(q_ws)
        for q2_ws in q2_workspaces:
            DeleteWorkspace(q2_ws)

        # Set the verical axis axis
        unit = ('Run No', 'last 3 digits')

        q_ws_axis = mtd[self._q_workspace].getAxis(1)
        q_ws_axis.setUnit("Label").setLabel(unit[0], unit[1])

        q2_ws_axis = mtd[self._q2_workspace].getAxis(1)
        q2_ws_axis.setUnit("Label").setLabel(unit[0], unit[1])

        for idx in range(0, len(run_numbers)):
            q_ws_axis.setValue(idx, float(run_numbers[idx][-3:]))
            q2_ws_axis.setValue(idx, float(run_numbers[idx][-3:]))

        # Process the ELF workspace
        if self._elf_workspace != '':
            Transpose(InputWorkspace=self._q_workspace, OutputWorkspace=self._elf_workspace)
            SortXAxis(InputWorkspace=self._elf_workspace, OutputWorkspace=self._elf_workspace)
            self.setProperty('OutputELF', self._elf_workspace)

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


    def _setup(self):
        """
        Gets algorithm properties.
        """

        self._plot = self.getProperty('Plot').value

        self._input_workspaces = self.getProperty('InputWorkspaces').value
        self._q_workspace = self.getPropertyValue('OutputInQ')
        self._q2_workspace = self.getPropertyValue('OutputInQSquared')
        self._elf_workspace = self.getPropertyValue('OutputELF')

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


# Register algorithm with Mantid
AlgorithmFactory.subscribe(ElasticWindowMultiple)
