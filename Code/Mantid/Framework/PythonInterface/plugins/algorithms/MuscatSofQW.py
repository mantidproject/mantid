#pylint: disable=no-init,too-many-instance-attributes,too-many-arguments

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
        return "Workflow\\MIDAS;PythonAlgorithms"


    def summary(self):
        return "Calculates an S(Q,w) from fitted parameters for use in Muscat."


    def PyInit(self):
        self.declareProperty(MatrixWorkspaceProperty('SampleWorkspace', '',
                                                     direction=Direction.Input),
                             doc="Name for the input Sample workspace.")

        self.declareProperty(MatrixWorkspaceProperty('ResolutionWorkspace', '',
                                                     direction=Direction.Input),
                             doc="Name for the input Resolution workspace.")

        self.declareProperty(MatrixWorkspaceProperty('ParametersWorkspace', '',
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
        self._load_input()
        self._get_conv_fit_result()


    def _setup(self):
        self._sam_ws = self.getPropertyValue('SampleWorkspace')
        self._res_ws = self.getPropertyValue('ResolutionWorkspace')
        self._par_ws = self.getPropertyValue('ParametersWorkspace')

        self._emax = self.getProperty('EnergyMax').value
        self._einc = self.getProperty('EnergyInc').value

        self._out_ws_name = self.getPropertyValue('OutputWorkspace')


    def _create_conv_fit_fun(self, ip, D1, H1, W1, H2, W2):
        pk_1 = '(composite=Convolution,FixResolution=true,NumDeriv=true;name=Resolution, Workspace="'+self._res_rebin+'"'
        if  self._lor >= 1:
            lor_fun = ('composite=ProductFunction,NumDeriv=false;name=Lorentzian,Amplitude='+str(H1[ip])+',PeakCentre=0.0,FWHM='+str(W1[ip]))
        if self._lor == 2:
            funcIndex = 1 if self._delta else 0
            lor_2 = 'name=Lorentzian,Amplitude='+str(H2[ip])+',PeakCentre=0.0,FWHM='+str(W2[ip])
            lor_fun = lor_fun +';'+ lor_2 +';ties=(f'+str(funcIndex)+'.PeakCentre=f'+str(funcIndex+1)+'.PeakCentre)'
        if self._delta:
            delta_fun = 'name=DeltaFunction,Height='+str(D1[ip])
            lor_fun = delta_fun +';' + lor_fun
        func = pk_1 +';('+ lor_fun +'))'
        return func


    def _get_conv_fit_result(self):
        sam_ws = mtd[self._sam_ws]
        x = sam_ws.readX(0)
        xmax = max(abs(x[0]),x[len(x)-1])
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
        Q = par_ws.readX(0)
        specMax = len(Q)

        if self._delta:
            D1 = par_ws.readY(0) #delta
        else:
            D1 = []

        if self._lor >= 1:
            H1 = par_ws.readY(0) #height1
            W1 = par_ws.readY(1) #width1
            H2 = []
            W2 = []
        if self._lor == 2:
            H2 = par_ws.readY(2) #height2
            W2 = par_ws.readY(3) #width2

        for i in range(specMax):
            func = self._create_conv_fit_fun(i, D1, H1, W1, H2, W2)
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
                x = dataX
                y = dataY
                e = dataE
            else:
                x = np.append(x, dataX)
                y = np.append(y, dataY)
                e = np.append(e, dataE)

            DeleteWorkspace(fit_output_name + '_Workspace')
            DeleteWorkspace(fit_output_name + '_NormalisedCovarianceMatrix')
            DeleteWorkspace(fit_output_name + '_Parameters')

        CreateWorkspace(OutputWorkspace=self._out_ws_name,
                        DataX=x,
                        DataY=y,
                        DataE=e,
                        Nspec=specMax,
                        UnitX='Energy',
                        VerticalAxisUnit='MomentumTransfer',
                        VerticalAxisValues=Q)
        CopyLogs(InputWorkspace=self._par_ws,
                 OutputWorkspace=self._out_ws_name)

        self.setProperty('OutputWorkspace', self._out_ws_name)


    def _load_input(self):
        logger.information('Sample run : %s' % self._sam_ws)
        logger.information('Resolution run : %s' % self._res_ws)
        logger.information('Parameter file : %s' % self._par_ws)

        inGR = mtd[self._par_ws].getRun()

        program = inGR.getLogData('fit_program').value
        if program != 'ConvFit':
            raise ValueError('Fit program MUST be ConvFit')

        self._delta = inGR.getLogData('delta_function').value
        logger.information('delta_function : %s' % self._delta)

        if 'lorentzians' in inGR:
            lor = inGR.getLogData('lorentzians').value
            self._lor = int(lor)
            logger.information('lorentzians : %i' % self._lor)
        else:
            raise ValueError('Fit MUST be Lorentzians')


AlgorithmFactory.subscribe(MuscatSofQW)
