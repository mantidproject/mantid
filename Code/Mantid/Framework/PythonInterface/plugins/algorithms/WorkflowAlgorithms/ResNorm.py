#pylint: disable=no-init
from mantid.api import PythonAlgorithm, AlgorithmFactory, MatrixWorkspaceProperty
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

        self.declareProperty(MatrixWorkspaceProperty('OutputWorkspace', '',
                                                     direction=Direction.Output),
                             doc='Fitted parameter output')


    def validateInputs(self):
        self._get_properties()
        issues = dict()

        # Validate fitting range in energy
        if self._e_min > self._e_max:
            issues['EnergyMax'] = 'Must be less than EnergyMin'

        # Resolution should only have one histogram
        if mtd[self._res_ws].getNumberHistograms() != 1:
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

        van_ws = mtd[self._van_ws]
        num_hist = van_ws.getNumberHistograms()

        v_values = van_ws.getAxis(1).extractValues()
        v_unit = van_ws.getAxis(1).getUnit().unitID()

        padded_res_ws = self._pad_res_ws(num_hist)

        input_str = ''
        for idx in range(num_hist):
            input_str += '%s,i%d;' % (padded_res_ws, idx)

        out_name = getWSprefix(self._res_ws) + 'ResNorm_Fit'

        fit_params = PlotPeakByLogValue(Input=input_str,
                                        OutputWorkspace=out_name,
                                        Function='name=TabulatedFunction,Workspace=irs26173_graphite002_red,Scaling=1.0,Shift=0.0',
                                        FitType='Individual',
                                        PassWSIndexToFunction=True,
                                        CreateOutput=self._create_output,
                                        StartX=self._e_min,
                                        EndX=self._e_max)

        self._process_fit_params(fit_params, v_values, v_unit)
        self.setProperty('OutputWorkspace', self._out_ws)

        DeleteWorkspace(padded_res_ws)
        if not self._create_output:
            DeleteWorkspace(fit_params)


    def _pad_res_ws(self, num_hist):
        """
        Generate a resolution workspaes with the same number of histograms
        as the vanadium run.

        @param num_hist Number of histograms required
        @return Padded workspace
        """

        ws_name = '__%s_%dspec' % (self._res_ws, num_hist)

        for idx in range(num_hist):
            input_ws_1 = ws_name
            if idx == 0:
                input_ws_1 = self._res_ws

            AppendSpectra(InputWorkspace1=input_ws_1,
                          InputWorkspace2=self._res_ws,
                          OutputWorkspace=ws_name)

        return mtd[ws_name]


    def _process_fit_params(self, fit_params, x_axis, x_unit):
        """
        Generate the output workspace containing fit parameters using the
        fit parameter table from PlotPeakByLogValue.

        @param fit_params Fit parameters as table workspace
        @param x_axis Values for X axis of output workspace
        @param x_unit Unit for X axis of output workspace
        """

        col_names = fit_params.getColumnNames()

        vert_axis_values = ['Scaling', 'Shift']

        y_values = []
        e_values = []

        for name in vert_axis_values:
            y_values.extend(fit_params.column(col_names.index(name)))
            e_values.extend(fit_params.column(col_names.index(name + '_Err')))

        CreateWorkspace(OutputWorkspace=self._out_ws,
                        DataX=x_axis,
                        DataY=y_values,
                        DataE=e_values,
                        NSpec=len(vert_axis_values),
                        UnitX=x_unit,
                        VerticalAxisUnit='Text',
                        VerticalAxisValues=vert_axis_values)


# Register algorithm with Mantid
AlgorithmFactory.subscribe(ResNorm)
