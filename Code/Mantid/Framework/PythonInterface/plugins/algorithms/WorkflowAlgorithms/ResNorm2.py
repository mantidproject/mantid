#pylint: disable=no-init
from mantid.api import (PythonAlgorithm, AlgorithmFactory, MatrixWorkspaceProperty,
                        WorkspaceGroup, WorkspaceGroupProperty)
from mantid.kernel import Direction
from mantid.simpleapi import *


class ResNorm(PythonAlgorithm):

    _res_ws = None
    _van_ws = None
    _e_min = None
    _e_max = None
    _create_output = None
    _out_ws = None


    def category(self):
        return "Workflow\\MIDAS;PythonAlgorithms"


    def summary(self):
        return """Creates a group normalisation file by taking a resolution file
                  and fitting it to all the groups in the resolution (vanadium)
                  reduction."""

    def version(self):
        return 2


    def PyInit(self):
        self.declareProperty(MatrixWorkspaceProperty('ResolutionWorkspace', '',
                                                     direction=Direction.Input),
                             doc='Workspace containing resolution')

        self.declareProperty(MatrixWorkspaceProperty('VanadiumWorkspace', '',
                                                     direction=Direction.Input),
                             doc='Workspace containing reduction of vanadium run')

        self.declareProperty(name='EnergyMin',
                             defaultValue=-0.2,
                             doc='Minimum energy for fit. Default=-0.2')

        self.declareProperty(name='EnergyMax',
                             defaultValue=0.2,
                             doc='Maximum energy for fit. Default=0.2')

        self.declareProperty(name='CreateOutput',
                             defaultValue=False,
                             doc='Create additional fitting output')

        self.declareProperty(WorkspaceGroupProperty('OutputWorkspace', '',
                                                    direction=Direction.Output),
                             doc='Fitted parameter output')


    def validateInputs(self):
        self._get_properties()
        issues = dict()

        # Validate fitting range in energy
        if self._e_min > self._e_max:
            issues['EnergyMax'] = 'Must be less than EnergyMin'

        res_ws = mtd[self._res_ws]
        # Can't use a WorkspaceGroup for resolution
        if isinstance(res_ws, WorkspaceGroup):
            issues['ResolutionWorkspace'] = 'Must be a MatrixWorkspace'

        # Resolution should only have one histogram
        elif mtd[self._res_ws].getNumberHistograms() != 1:
            issues['ResolutionWorkspace'] = 'Must have exactly one histogram'

        return issues


    def _get_properties(self):
        self._res_ws = self.getPropertyValue('ResolutionWorkspace')
        self._van_ws = self.getPropertyValue('VanadiumWorkspace')
        self._e_min = self.getProperty('EnergyMin').value
        self._e_max = self.getProperty('EnergyMax').value
        self._create_output = self.getProperty('CreateOutput').value
        self._out_ws = self.getPropertyValue('OutputWorkspace')


    def PyExec(self):
        from IndirectCommon import getWSprefix

        # Process vanadium workspace
        van_ws = ConvertSpectrumAxis(InputWorkspace=self._van_ws,
                                     OutputWorkspace='__ResNorm_vanadium',
                                     Target='ElasticQ',
                                     EMode='Indirect')

        num_hist = van_ws.getNumberHistograms()

        v_values = van_ws.getAxis(1).extractValues()
        v_unit = van_ws.getAxis(1).getUnit().unitID()

        # Process resolution workspace
        padded_res_ws = self._process_res_ws(num_hist)

        input_str = ''
        for idx in range(num_hist):
            input_str += '%s,i%d;' % (padded_res_ws, idx)

        out_name = getWSprefix(self._res_ws) + 'ResNorm_Fit'
        function = 'name=TabulatedFunction,Workspace=%s,Scaling=1,Shift=0,XScaling=1,ties=(Shift=0)' % self._van_ws

        fit_params = PlotPeakByLogValue(Input=input_str,
                                        OutputWorkspace=out_name,
                                        Function=function,
                                        FitType='Individual',
                                        PassWSIndexToFunction=True,
                                        CreateOutput=self._create_output,
                                        StartX=self._e_min,
                                        EndX=self._e_max)

        params = {'XScaling':'Stretch', 'Scaling':'Intensity'}
        result_workspaces = []
        for param_name, output_name in params.items():
            result_workspaces.append(self._process_fit_params(fit_params, param_name, v_values, v_unit, output_name))

        GroupWorkspaces(InputWorkspaces=result_workspaces,
                        OutputWorkspace=self._out_ws)
        self.setProperty('OutputWorkspace', self._out_ws)

        DeleteWorkspace(van_ws)
        DeleteWorkspace(padded_res_ws)
        if not self._create_output:
            DeleteWorkspace(fit_params)


    def _process_res_ws(self, num_hist):
        """
        Generate a resolution workspaes with the same number of histograms
        as the vanadium run, with area normalised to 1.

        @param num_hist Number of histograms required
        @return Padded workspace
        """

        norm_res_ws = '__ResNorm_unityres'
        NormaliseToUnity(InputWorkspace=self._res_ws,
                         OutputWorkspace=norm_res_ws)

        ws_name = '__ResNorm_res_%s_%dspec' % (self._res_ws, num_hist)

        for idx in range(num_hist):
            input_ws_1 = ws_name
            if idx == 0:
                input_ws_1 = norm_res_ws

            AppendSpectra(InputWorkspace1=input_ws_1,
                          InputWorkspace2=norm_res_ws,
                          OutputWorkspace=ws_name)

        DeleteWorkspace(norm_res_ws)

        return mtd[ws_name]


    #pylint: disable=too-many-arguments
    def _process_fit_params(self, fit_params, parameter_name, x_axis, x_unit, workspace_suffix=None):
        """
        Generate the output workspace containing fit parameters using the
        fit parameter table from PlotPeakByLogValue.

        @param fit_params Fit parameters as table workspace
        @param parameter_name Parameter name to extract
        @param x_axis Values for X axis of output workspace
        @param x_unit Unit for X axis of output workspace
        @param workspace_suffix Suffix of result workspace name
        """

        if workspace_suffix is None:
            workspace_suffix = parameter_name

        col_names = fit_params.getColumnNames()

        y_values = []
        e_values = []

        y_values = fit_params.column(col_names.index(parameter_name))
        e_values = fit_params.column(col_names.index(parameter_name + '_Err'))

        ws_name = self._out_ws + '_' + workspace_suffix

        CreateWorkspace(OutputWorkspace=ws_name,
                        DataX=x_axis,
                        DataY=y_values,
                        DataE=e_values,
                        NSpec=1,
                        UnitX=x_unit,
                        VerticalAxisUnit='Text',
                        VerticalAxisValues=[parameter_name])

        return ws_name


# Register algorithm with Mantid
AlgorithmFactory.subscribe(ResNorm)
