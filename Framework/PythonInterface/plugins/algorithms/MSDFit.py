#pylint: disable=no-init
from __future__ import (absolute_import, division, print_function)
from mantid.simpleapi import *
from mantid.api import *
from mantid.kernel import *
from six.moves import range #pylint: disable=redefined-builtin


class MSDFit(DataProcessorAlgorithm):
    _output_fit_ws = None
    _spec_range = None
    _x_range = None
    _input_ws = None
    _output_param_ws = None
    _output_msd_ws = None

    def category(self):
        return 'Workflow\\MIDAS'

    def summary(self):
        return 'Fits log(intensity) vs Q-squared to obtain the mean squared displacement.'

    def PyInit(self):
        self.declareProperty(MatrixWorkspaceProperty('InputWorkspace', '',direction=Direction.Input),
                             doc='Sample input workspace')

        self.declareProperty(name='XStart', defaultValue=0.0,
                             doc='Start of fitting range')
        self.declareProperty(name='XEnd', defaultValue=0.0,
                             doc='End of fitting range')

        self.declareProperty(name='SpecMin', defaultValue=0,
                             doc='Start of spectra range to be fit')
        self.declareProperty(name='SpecMax', defaultValue=0,
                             doc='End of spectra range to be fit')

        self.declareProperty(WorkspaceGroupProperty('OutputWorkspace', '',direction=Direction.Output),
                             doc='Output mean squared displacement')

        self.declareProperty(ITableWorkspaceProperty('ParameterWorkspace', '',
                                                     direction=Direction.Output,
                                                     optional=PropertyMode.Optional),
                             doc='Output fit parameters table')

        self.declareProperty(WorkspaceGroupProperty('FitWorkspaces', '',
                                                    direction=Direction.Output,
                                                    optional=PropertyMode.Optional),
                             doc='Output fitted workspaces')

    def validateInputs(self):
        issues = dict()

        workspace = self.getProperty('InputWorkspace').value
        x_data = workspace.readX(0)

        # Validate X axis fitting range
        x_min = self.getProperty('XStart').value
        x_max = self.getProperty('XEnd').value

        if x_min > x_max:
            msg = 'XStart must be less then XEnd'
            issues['XStart'] = msg
            issues['XEnd'] = msg

        if x_min < x_data[0]:
            issues['XStart'] = 'Must be greater than minimum X value in workspace'

        if x_max > x_data[-1]:
            issues['XEnd'] = 'Must be less than maximum X value in workspace'

        # Validate specta fitting range
        spec_min = self.getProperty('SpecMin').value
        spec_max = self.getProperty('SpecMax').value

        if spec_min < 0:
            issues['SpecMin'] = 'Minimum spectrum number must be greater than or equal to 0'

        if spec_max > workspace.getNumberHistograms():
            issues['SpecMax'] = 'Maximum spectrum number must be less than number of spectra in workspace'

        if spec_min > spec_max:
            msg = 'SpecMin must be less then SpecMax'
            issues['SpecMin'] = msg
            issues['SpecMax'] = msg

        return issues

    def PyExec(self):
        self._setup()

        # Fit line to each of the spectra
        function = 'name=LinearBackground, A0=0, A1=0'
        input_params = [self._input_ws + ',i%d' % i for i in range(self._spec_range[0],
                                                                   self._spec_range[1] + 1)]
        input_params = ';'.join(input_params)
        PlotPeakByLogValue(Input=input_params,
                           OutputWorkspace=self._output_msd_ws,
                           Function=function,
                           StartX=self._x_range[0],
                           EndX=self._x_range[1],
                           FitType='Sequential',
                           CreateOutput=True)

        DeleteWorkspace(self._output_msd_ws + '_NormalisedCovarianceMatrices')
        DeleteWorkspace(self._output_msd_ws + '_Parameters')
        RenameWorkspace(self._output_msd_ws,
                        OutputWorkspace=self._output_param_ws)

        params_table = mtd[self._output_param_ws]

        # MSD value should be positive, but the fit output is negative
        msd = params_table.column('A1')
        for i, value in enumerate(msd):
            params_table.setCell('A1', i, value * -1)

        # Create workspaces for each of the parameters
        parameter_ws_group = []

        # A0 workspace
        ws_name = self._output_msd_ws + '_A0'
        parameter_ws_group.append(ws_name)
        ConvertTableToMatrixWorkspace(self._output_param_ws, OutputWorkspace=ws_name,
                                      ColumnX='axis-1', ColumnY='A0', ColumnE='A0_Err')
        xunit = mtd[ws_name].getAxis(0).setUnit('Label')
        xunit.setLabel('Temperature', 'K')

        # A1 workspace
        ws_name = self._output_msd_ws + '_A1'
        parameter_ws_group.append(ws_name)
        ConvertTableToMatrixWorkspace(self._output_param_ws, OutputWorkspace=ws_name,
                                      ColumnX='axis-1', ColumnY='A1', ColumnE='A1_Err')
        xunit = mtd[ws_name].getAxis(0).setUnit('Label')
        xunit.setLabel('Temperature', 'K')
        SortXAxis(ws_name, OutputWorkspace=ws_name)

        # Group parameter workspaces
        GroupWorkspaces(InputWorkspaces=','.join(parameter_ws_group),
                        OutputWorkspace=self._output_msd_ws)

        # Rename fit workspace group
        original_fit_ws_name = self._output_msd_ws + '_Workspaces'
        if original_fit_ws_name != self._output_fit_ws:
            RenameWorkspace(InputWorkspace=self._output_msd_ws + '_Workspaces',
                            OutputWorkspace=self._output_fit_ws)

        # Add sample logs to output workspace
        CopyLogs(InputWorkspace=self._input_ws, OutputWorkspace=self._output_msd_ws)
        CopyLogs(InputWorkspace=self._output_msd_ws + '_A0', OutputWorkspace=self._output_fit_ws)

        self.setProperty('OutputWorkspace', self._output_msd_ws)
        self.setProperty('ParameterWorkspace', self._output_param_ws)
        self.setProperty('FitWorkspaces', self._output_fit_ws)

    def _setup(self):
        """
        Gets algorithm properties.
        """
        self._input_ws = self.getPropertyValue('InputWorkspace')
        self._output_msd_ws = self.getPropertyValue('OutputWorkspace')

        self._output_param_ws = self.getPropertyValue('ParameterWorkspace')
        if self._output_param_ws == '':
            self._output_param_ws = self._output_msd_ws + '_Parameters'

        self._output_fit_ws = self.getPropertyValue('FitWorkspaces')
        if self._output_fit_ws == '':
            self._output_fit_ws = self._output_msd_ws + '_Workspaces'

        self._x_range = [self.getProperty('XStart').value,
                         self.getProperty('XEnd').value]

        self._spec_range = [self.getProperty('SpecMin').value,
                            self.getProperty('SpecMax').value]

AlgorithmFactory.subscribe(MSDFit)
