#pylint: disable=no-init,too-many-instance-attributes,too-many-arguments

from __future__ import (absolute_import, division, print_function)
from mantid.simpleapi import *
from mantid.api import (DataProcessorAlgorithm, AlgorithmFactory, MatrixWorkspaceProperty)
from mantid.kernel import (Direction, logger)
import numpy as np


class MuscatSofQW(DataProcessorAlgorithm):

    _sam_ws = None
    _res_ws = None
    _par_ws = None
    _par_file = None
    _emax = 0.5
    _einc = 0.005
    _lor = 1
    _delta = False
    _sam_rebin = None
    _res_rebin = None
    _out_ws_name = None
    _para_name = None

    def category(self):
        return "Workflow\\MIDAS"

    def summary(self):
        return "Calculates an S(Q, w) from fitted parameters for use in Muscat."

    def PyInit(self):
        self.declareProperty(MatrixWorkspaceProperty('SampleWorkspace', '',
                                                     direction=Direction.Input),
                             doc="Name for the input Sample workspace.")

        self.declareProperty(MatrixWorkspaceProperty('ResolutionWorkspace', '',
                                                     direction=Direction.Input),
                             doc="Name for the input Resolution workspace.")

        self.declareProperty(MatrixWorkspaceProperty('ParameterWorkspace', '',
                                                     direction=Direction.Input),
                             doc="Name for the input Parameters workspace.")

        self.declareProperty(name='EnergyMax', defaultValue=0.5,
                             doc='Energy maximum')
        self.declareProperty(name='EnergyInc', defaultValue=0.005,
                             doc='Energy increment')

        self.declareProperty(MatrixWorkspaceProperty('OutputWorkspace', '',
                                                     direction=Direction.Output),
                             doc='Output workspace in S(Q, w)')

    def PyExec(self):
        self._setup()
        self._cache_parameter_data()
        self._get_conv_fit_result()

    def _setup(self):
        self._sam_ws = self.getPropertyValue('SampleWorkspace')
        self._res_ws = self.getPropertyValue('ResolutionWorkspace')
        self._par_ws = self.getPropertyValue('ParameterWorkspace')

        self._emax = self.getProperty('EnergyMax').value
        self._einc = self.getProperty('EnergyInc').value

        self._out_ws_name = self.getPropertyValue('OutputWorkspace')

    def _create_conv_fit_fun(self, peak_idx, delta_1, l_height_1, l_width_1, l_height_2, l_width_2):
        """
        Creates a fit function string based on the ConvFit parameters.

        @param peak_idx Peak index
        @param delta_1 Delta function value
        @param l_height_1 Height of Lorentzian 1
        @param l_width_1 FWHM of Lorentzian 1
        @param l_height_2 Height of Lorentzian 2
        @param l_width_2 FWHM of Lorentzian 2
        @return Fit function string
        """

        pk_1 = '(composite=Convolution,FixResolution=true,NumDeriv=true;name=Resolution, Workspace="{0}"'.format(self._res_rebin)

        if self._lor >= 1:
            lor_fun = 'composite=ProductFunction,NumDeriv=false;name=Lorentzian,'+\
                      'Amplitude={0},PeakCentre=0.0,FWHM={1}'.format(l_height_1[peak_idx], l_width_1[peak_idx])
        elif self._lor == 2:
            funcIndex = 1 if self._delta else 0
            lor_2 = 'name=Lorentzian,Amplitude='+str(l_height_2[peak_idx])+',PeakCentre=0.0,FWHM='+str(l_width_2[peak_idx])
            lor_fun = lor_fun +';'+ lor_2 +';ties=(f'+str(funcIndex)+'.PeakCentre=f'+str(funcIndex+1)+'.PeakCentre)'

        if self._delta:
            delta_fun = 'name=DeltaFunction,Height='+str(delta_1[peak_idx])
            lor_fun = delta_fun +';' + lor_fun

        func = '{0};({1}))'.format(pk_1, lor_fun)
        return func

    def _get_conv_fit_result(self):
        sam_ws = mtd[self._sam_ws]
        x_data = sam_ws.readX(0)
        xmax = max(abs(x_data[0]), x_data[-1])
        rebin = str(-self._emax)+','+str(self._einc)+','+str(self._emax)
        self._sam_rebin = '__sam_rebin'
        Rebin(InputWorkspace=self._sam_ws,
              OutputWorkspace=self._sam_rebin,
              Params=rebin)
        self._res_rebin = '__res_rebin'
        Rebin(InputWorkspace=self._res_ws,
              OutputWorkspace=self._res_rebin,
              Params=rebin)
        logger.information('energy range ; input : %f rebin : %f' % (xmax, self._emax))
        par_ws = mtd[self._par_ws]
        q_values = par_ws.readX(0)
        specMax = len(q_values)

        if self._delta:
            delta_1 = par_ws.readY(0) #delta
        else:
            delta_1 = []

        if self._lor >= 1:
            l_height_1 = par_ws.readY(0) #height1
            l_width_1 = par_ws.readY(1) #width1
            l_height_2 = []
            l_width_2 = []
        if self._lor == 2:
            l_height_2 = par_ws.readY(2) #height2
            l_width_2 = par_ws.readY(3) #width2

        for i in range(specMax):
            func = self._create_conv_fit_fun(i, delta_1, l_height_1, l_width_1, l_height_2, l_width_2)
            logger.information('Fit func : %s' % func)
            fit_output_name = '%s_%i' % (self._out_ws_name, i)

            Fit(Function=func,
                InputWorkspace=self._sam_rebin,
                WorkspaceIndex=i,
                Output=fit_output_name,
                MaxIterations=0)

            dataX=np.array(mtd[fit_output_name + '_Workspace'].readX(1))
            dataY=np.array(mtd[fit_output_name + '_Workspace'].readY(1))
            dataE=np.array(mtd[fit_output_name + '_Workspace'].readE(1))

            if i == 0:
                x_data = dataX
                y_data = dataY
                e_data = dataE
            else:
                x_data = np.append(x_data, dataX)
                y_data = np.append(y_data, dataY)
                e_data = np.append(e_data, dataE)

            DeleteWorkspace(fit_output_name + '_Workspace')
            DeleteWorkspace(fit_output_name + '_NormalisedCovarianceMatrix')
            DeleteWorkspace(fit_output_name + '_Parameters')

        CreateWorkspace(OutputWorkspace=self._out_ws_name,
                        DataX=x_data,
                        DataY=y_data,
                        DataE=e_data,
                        Nspec=specMax,
                        UnitX='Energy',
                        VerticalAxisUnit='MomentumTransfer',
                        VerticalAxisValues=q_values,
                        ParentWorkspace=self._sam_ws)

        CopyLogs(InputWorkspace=self._par_ws,
                 OutputWorkspace=self._out_ws_name)

        self.setProperty('OutputWorkspace', self._out_ws_name)

    def _cache_parameter_data(self):
        """
        Validates and caches data from parameter workspace.
        """

        sample_run = mtd[self._par_ws].getRun()

        program = sample_run.getLogData('fit_program').value
        if program != 'ConvFit':
            raise ValueError('Fit program MUST be ConvFit')

        self._delta = sample_run.getLogData('delta_function').value
        logger.information('delta_function : %s' % self._delta)

        if 'lorentzians' in sample_run:
            lor = sample_run.getLogData('lorentzians').value
            self._lor = int(lor)
            logger.information('lorentzians : %i' % self._lor)
        else:
            raise ValueError('Fit MUST be Lorentzians')


AlgorithmFactory.subscribe(MuscatSofQW)
