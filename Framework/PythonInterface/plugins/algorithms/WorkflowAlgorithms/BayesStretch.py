#pylint: disable=invalid-name,too-many-instance-attributes,too-many-branches,no-init
from IndirectImport import *

from mantid.api import (PythonAlgorithm, AlgorithmFactory, MatrixWorkspaceProperty,
                        WorkspaceGroupProperty, Progress)
from mantid.kernel import StringListValidator, Direction
from mantid.simpleapi import *
from mantid import config, logger
import os
import numpy as np

if is_supported_f2py_platform():
    Que     = import_f2py("Quest")

class BayesStretch(PythonAlgorithm):

    _sam_name = None
    _sam_ws = None
    _res_name = None
    _e_min = None
    _e_max = None
    _sam_bins = None
    _elastic = None
    _background = None
    _nbet = None
    _nsig = None
    _loop = None

    _erange = None
    _nbins = None


    def category(self):
        return "Workflow\\MIDAS"

    def summary(self):
        return "This is a variation of the stretched exponential option of Quasi."

    def PyInit(self):
        self.declareProperty(MatrixWorkspaceProperty('SampleWorkspace', '', direction=Direction.Input),
                             doc='Name of the Sample input Workspace')

        self.declareProperty(MatrixWorkspaceProperty('ResolutionWorkspace', '', direction=Direction.Input),
                             doc='Name of the resolution input Workspace')

        self.declareProperty(name='EMin', defaultValue=-0.2,
                             doc='The start of the fit range. Default=-0.2')

        self.declareProperty(name='EMax', defaultValue=0.2,
                             doc='The end of the fit range. Default=0.2')

        self.declareProperty(name='SampleBins', defaultValue=1,
                             doc='The number of sample bins')

        self.declareProperty(name='Elastic', defaultValue=True,
                             doc='Fit option for using the elastic peak')

        self.declareProperty(name='Background', defaultValue='Flat',
                             validator=StringListValidator(['Sloping','Flat','Zero']),
                             doc='Fit option for the type of background')

        self.declareProperty(name='NumberSigma', defaultValue=50,
                             doc='Number of sigma values. Default=50')
        self.declareProperty(name='NumberBeta', defaultValue=30,
                             doc='Number of beta values. Default=30')

        self.declareProperty(name='Loop', defaultValue=True, doc='Switch Sequential fit On/Off')

        self.declareProperty(WorkspaceGroupProperty('OutputWorkspaceFit', '', direction=Direction.Output),
                             doc='The name of the fit output workspaces')

        self.declareProperty(WorkspaceGroupProperty('OutputWorkspaceContour', '', direction=Direction.Output),
                             doc='The name of the contour output workspaces')


    def validateInputs(self):
        self._get_properties()
        issues = dict()

        # Validate fitting range in energy
        if self._e_min > self._e_max:
            issues['EMax'] = 'Must be less than EnergyMin'

        # Validate fitting range within data range
        data_min = self._sam_ws.readX(0)[0]
        if self._e_min < data_min:
            issues['EMin'] = 'EMin must be more than the minimum x range of the data.'
        data_max = self._sam_ws.readX(0)[-1]
        if self._e_max > data_max:
            issues['EMax'] = 'EMax must be less than the maximum x range of the data'

        return issues


    #pylint: disable=too-many-locals,too-many-branches
    def PyExec(self):
        run_f2py_compatibility_test()

        from IndirectBayes import (CalcErange, GetXYE)
        from IndirectCommon import (CheckXrange, CheckAnalysers, getEfixed, GetThetaQ,
                                    CheckHistZero)
        setup_prog = Progress(self, start=0.0, end=0.3, nreports = 5)
        self.log().information('BayesStretch input')

        setup_prog.report('Converting to binary for Fortran')
        #convert true/false to 1/0 for fortran
        o_el = 1 if self._elastic else 0

        #fortran code uses background choices defined using the following numbers
        setup_prog.report('Encoding input options')
        if self._background == 'Sloping':
            o_bgd = 2
        elif self._background == 'Flat':
            o_bgd = 1
        elif self._background == 'Zero':
            o_bgd = 0

        fitOp = [o_el, o_bgd, 0, 0]
        nbs = [self._nbet, self._nsig]

        setup_prog.report('Establishing save path')
        workdir = config['defaultsave.directory']
        if not os.path.isdir(workdir):
            workdir = os.getcwd()
            logger.information('Default Save directory is not set. Defaulting to current working Directory: ' + workdir)

        array_len = 4096                           # length of array in Fortran
        setup_prog.report('Checking X Range')
        CheckXrange(self._erange, 'Energy')

        nbin,nrbin = self._nbins[0], 1

        logger.information('Sample is %s' % self._sam_name)
        logger.information('Resolution is %s' % self._res_name)

        setup_prog.report('Checking Analysers')
        CheckAnalysers(self._sam_name, self._res_name)
        setup_prog.report('Obtaining EFixed, theta and Q')
        efix = getEfixed(self._sam_name)
        theta, Q = GetThetaQ(self._sam_name)

        nsam,ntc = CheckHistZero(self._sam_name)

        #check if we're performing a sequential fit
        if self._loop != True:
            nsam = 1

        setup_prog.report('Checking Histograms')
        prog = 'Stretch'
        logger.information('Version is Stretch')
        logger.information('Number of spectra = %s ' % nsam)
        logger.information('Erange : %f to %f ' % (self._erange[0], self._erange[1]))

        setup_prog.report('Establishing output workspace name')
        fname = self._sam_name[:-4] + '_'+ prog
        wrks=os.path.join(workdir, self._sam_name[:-4])
        logger.information(' lptfile : %s_Qst.lpt' % wrks)
        lwrk=len(wrks)
        wrks.ljust(140, ' ')
        wrkr=self._res_name
        wrkr.ljust(140, ' ')
        Nbet, Nsig = nbs[0], nbs[1]
        eBet0 = np.zeros(Nbet)                  # set errors to zero
        eSig0 = np.zeros(Nsig)                  # set errors to zero
        rscl = 1.0
        Qaxis = ''

        group = ''
        workflow_prog = Progress(self, start=0.3, end=0.7, nreports=nsam*3)
        for m in range(nsam):
            logger.information('Group %i at angle %f' % (m, theta[m]))
            nsp = m + 1
            nout, bnorm, Xdat, Xv, Yv, Ev = CalcErange(self._sam_name, m, self._erange, nbin)
            Ndat = nout[0]
            Imin = nout[1]
            Imax = nout[2]
            if prog == 'QLd':
                mm = m
            else:
                mm = 0
            Nb, Xb, Yb, _ = GetXYE(self._res_name, mm, array_len)     # get resolution data
            numb = [nsam, nsp, ntc, Ndat, nbin, Imin, Imax, Nb, nrbin, Nbet, Nsig]
            rscl = 1.0
            reals = [efix, theta[m], rscl, bnorm]

            workflow_prog.report('Processing spectrum number %i' % m)
            xsout, ysout, xbout, ybout, zpout=Que.quest(numb, Xv, Yv, Ev, reals, fitOp,
                                                        Xdat, Xb, Yb, wrks, wrkr, lwrk)
            dataXs = xsout[:Nsig]               # reduce from fixed Fortran array
            dataYs = ysout[:Nsig]
            dataXb = xbout[:Nbet]
            dataYb = ybout[:Nbet]
            zpWS = fname + '_Zp' +str(m)
            if m > 0:
                Qaxis += ','
            Qaxis += str(Q[m])

            dataXz = []
            dataYz = []
            dataEz = []

            for n in range(Nsig):
                yfit_list = np.split(zpout[:Nsig*Nbet], Nsig)
                dataYzp = yfit_list[n]

                dataXz = np.append(dataXz, xbout[:Nbet])
                dataYz = np.append(dataYz, dataYzp[:Nbet])
                dataEz = np.append(dataEz, eBet0)

            zpWS = fname + '_Zp' + str(m)
            CreateWorkspace(OutputWorkspace=zpWS,
                            DataX=dataXz, DataY=dataYz,
                            DataE=dataEz, Nspec=Nsig,
                            UnitX='MomentumTransfer',
                            VerticalAxisUnit='MomentumTransfer',
                            VerticalAxisValues=dataXs)

            unitx = mtd[zpWS].getAxis(0).setUnit("Label")
            unitx.setLabel('beta' , '')
            unity = mtd[zpWS].getAxis(1).setUnit("Label")
            unity.setLabel('sigma' , '')

            if m == 0:
                xSig = dataXs
                ySig = dataYs
                eSig = eSig0
                xBet = dataXb
                yBet = dataYb
                eBet = eBet0
                groupZ = zpWS
            else:
                xSig = np.append(xSig,dataXs)
                ySig = np.append(ySig,dataYs)
                eSig = np.append(eSig,eSig0)
                xBet = np.append(xBet,dataXb)
                yBet = np.append(yBet,dataYb)
                eBet = np.append(eBet,eBet0)
                groupZ = groupZ +','+ zpWS

        #create workspaces for sigma and beta
        workflow_prog.report('Creating OutputWorkspace')
        self._create_workspace(fname + '_Sigma', [xSig, ySig, eSig], nsam, Qaxis)
        self._create_workspace(fname + '_Beta', [xBet, yBet, eBet], nsam, Qaxis)

        group = fname + '_Sigma,' + fname + '_Beta'
        fit_ws = fname + '_Fit'
        GroupWorkspaces(InputWorkspaces=group,
                        OutputWorkspace=fit_ws)
        contour_ws = fname + '_Contour'
        GroupWorkspaces(InputWorkspaces=groupZ,
                        OutputWorkspace=contour_ws)

        log_prog = Progress(self, start=0.8, end =1.0, nreports=8)
        #Add some sample logs to the output workspaces
        log_prog.report('Copying Logs to Fit workspace')
        CopyLogs(InputWorkspace=self._sam_name,
                 OutputWorkspace=fit_ws)
        log_prog.report('Adding Sample logs to Fit workspace')
        self._add_sample_logs(fit_ws, self._erange, nbin)
        log_prog.report('Copying logs to Contour workspace')
        CopyLogs(InputWorkspace=self._sam_name,
                 OutputWorkspace=contour_ws)
        log_prog.report('Adding sample logs to Contour workspace')
        self._add_sample_logs(contour_ws, self._erange, nbin)
        log_prog.report('Finialising log copying')

        self.setProperty('OutputWorkspaceFit', fit_ws)
        self.setProperty('OutputWorkspaceContour', contour_ws)
        log_prog.report('Setting workspace properties')

#----------------------------- Helper functions -----------------------------

    def _create_workspace(self, name, xye, num_spec, vert_axis):
        """
        Creates a workspace from FORTRAN data

        @param name         :: Full name of outputworkspace
        @param xye          :: List of axis data [x, y , e]
        @param num_spec     :: Number of spectra
        @param vert_axis    :: The values on the vertical axis
        """

        CreateWorkspace(OutputWorkspace=name,
                        DataX=xye[0], DataY=xye[1], DataE=xye[2],
                        Nspec=num_spec, UnitX='',
                        VerticalAxisUnit='MomentumTransfer',
                        VerticalAxisValues=vert_axis)

        unitx = mtd[name].getAxis(0).setUnit("Label")
        if name[:4] == 'Beta':
            unitx.setLabel('beta' , '')
        else:
            unitx.setLabel('sigma', '')


    def _add_sample_logs(self, workspace, erange, sample_binning):
        """
        Add the Bayes Stretch specific values to the sample logs
        """
        energy_min, energy_max = erange

        AddSampleLog(Workspace=workspace, LogName="res_file",
                     LogType="String", LogText=self._res_name)
        AddSampleLog(Workspace=workspace, LogName="background",
                     LogType="String", LogText=str(self._background))
        AddSampleLog(Workspace=workspace, LogName="elastic_peak",
                     LogType="String", LogText=str(self._elastic))
        AddSampleLog(Workspace=workspace, LogName="energy_min",
                     LogType="Number", LogText=str(energy_min))
        AddSampleLog(Workspace=workspace, LogName="energy_max",
                     LogType="Number", LogText=str(energy_max))
        AddSampleLog(Workspace=workspace, LogName="sample_binning",
                     LogType="Number", LogText=str(sample_binning))


    def _get_properties(self):
        self._sam_name = self.getPropertyValue('SampleWorkspace')
        self._sam_ws = self.getProperty('SampleWorkspace').value
        self._res_name = self.getPropertyValue('ResolutionWorkspace')
        self._e_min = self.getProperty('EMin').value
        self._e_max = self.getProperty('EMax').value
        self._sam_bins = self.getPropertyValue('SampleBins')
        self._elastic = self.getProperty('Elastic').value
        self._background = self.getPropertyValue('Background')
        self._nbet = self.getProperty('NumberBeta').value
        self._nsig = self.getProperty('NumberSigma').value
        self._loop = self.getProperty('Loop').value

        self._erange = [self._e_min, self._e_max]
        self._nbins = [self._sam_bins, 1]


AlgorithmFactory.subscribe(BayesStretch)         # Register algorithm with Mantid
