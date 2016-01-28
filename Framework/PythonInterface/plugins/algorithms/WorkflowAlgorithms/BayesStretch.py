#pylint: disable=invalid-name,too-many-instance-attributes,too-many-branches,no-init
from IndirectImport import *

from mantid.api import (PythonAlgorithm, AlgorithmFactory, MatrixWorkspaceProperty, PropertyMode,
                        WorkspaceGroupProperty, Progress)
from mantid.kernel import StringListValidator, Direction
from mantid.simpleapi import *
from mantid import config, logger
import os
import numpy as np

if is_supported_f2py_platform():
    Que     = import_f2py("Quest")

class BayesStretch(PythonAlgorithm):

    _program = 'Que'
    _samWS = None
    _resWS = None
    _resnormWS = None
    _e_min = None
    _e_max = None
    _sam_bins = None
    _res_bins = 1
    _elastic = None
    _background = None
    _width = False
    _res_norm = None
    _wfile = ''
    _loop = None
    _save = None
    _plot = None

    def category(self):
        return "Workflow\\MIDAS"

    def summary(self):
        return "This is a variation of the stretched exponential option of Quasi."

    def PyInit(self):
        self.declareProperty(MatrixWorkspaceProperty('SampleWorkspace', '', direction=Direction.Input),
                             doc='Name of the Sample input Workspace')

        self.declareProperty(MatrixWorkspaceProperty('ResolutionWorkspace', '', direction=Direction.Input),
                             doc='Name of the resolution input Workspace')

        self.declareProperty(name='MinRange', defaultValue=-0.2,
                             doc='The start of the fit range. Default=-0.2')

        self.declareProperty(name='MaxRange', defaultValue=0.2,
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

        self.declareProperty(name='Plot', defaultValue='', doc='Plot options')

        self.declareProperty(name='Save', defaultValue=False, doc='Switch Save result to nxs file Off/On')

        self.declareProperty(WorkspaceGroupProperty('OutputWorkspaceFit', '', direction=Direction.Output),
                             doc='The name of the fit output workspaces')

        self.declareProperty(WorkspaceGroupProperty('OutputWorkspaceContour', '', direction=Direction.Output),
                             doc='The name of the contour output workspaces')

    def validateInputs(self):
        self._get_properties()
        issues = dict()

        # Validate fitting range in energy
        if self._e_min > self._e_max:
            issues['MaxRange'] = 'Must be less than EnergyMin'

        return issues


    def _get_properties(self):
        self._program = 'Quest'
        self._sam_ws = self.getPropertyValue('SampleWorkspace')
        self._res_ws = self.getPropertyValue('ResolutionWorkspace')
        self._res_norm = False
        self._resnorm_ws = ''
        self._e_min = self.getProperty('MinRange').value
        self._e_max = self.getProperty('MaxRange').value
        self._sam_bins = self.getPropertyValue('SampleBins')
        self._res_bins = 1
        self._elastic = self.getProperty('Elastic').value
        self._background = self.getPropertyValue('Background')
        self._width = False
        self._wfile = ''
        self._nbet = self.getProperty('NumberBeta').value
        self._nsig = self.getProperty('NumberSigma').value
        self._loop = self.getProperty('Loop').value
        self._save = self.getProperty('Save').value
        self._plot = self.getPropertyValue('Plot')

    #pylint: disable=too-many-locals,too-many-branches
    def PyExec(self):
        run_f2py_compatibility_test()

        from IndirectBayes import (CalcErange, GetXYE, ReadNormFile,
                                   ReadWidthFile, QLAddSampleLogs, QuestPlot)
        from IndirectCommon import (CheckXrange, CheckAnalysers, getEfixed, GetThetaQ,
                                    CheckHistZero, CheckHistSame)
        setup_prog = Progress(self, start=0.0, end=0.3, nreports = 5)
        self.log().information('BayesStretch input')

        erange = [self._e_min, self._e_max]
        nbins = [self._sam_bins, self._res_bins]
        setup_prog.report('Converting to binary for Fortran')
        #convert true/false to 1/0 for fortran
        o_el = 1 if self._elastic else 0
        o_w1 = 1 if self._width else 0
        o_res = 1 if self._res_norm else 0

        #fortran code uses background choices defined using the following numbers
        setup_prog.report('Encoding input options')
        if self._background == 'Sloping':
            o_bgd = 2
        elif self._background == 'Flat':
            o_bgd = 1
        elif self._background == 'Zero':
            o_bgd = 0

        fitOp = [o_el, o_bgd, o_w1, o_res]
        nbs = [self._nbet, self._nsig]

        setup_prog.report('Establishing save path')
        workdir = config['defaultsave.directory']
        if not os.path.isdir(workdir):
            workdir = os.getcwd()
            logger.information('Default Save directory is not set. Defaulting to current working Directory: ' + workdir)

        array_len = 4096                           # length of array in Fortran
        setup_prog.report('Checking X Range')
        CheckXrange(erange, 'Energy')

        nbin,nrbin = nbins[0], nbins[1]

        logger.information('Sample is %s' % self._sam_ws)
        logger.information('Resolution is %s' % self._res_ws)

        setup_prog.report('Checking Analysers')
        CheckAnalysers(self._sam_ws, self._res_ws)
        setup_prog.report('Obtaining EFixed, theta and Q')
        efix = getEfixed(self._sam_ws)
        theta, Q = GetThetaQ(self._sam_ws)

        nsam,ntc = CheckHistZero(self._sam_ws)

        totalNoSam = nsam

        #check if we're performing a sequential fit
        if self._loop != True:
            nsam = 1

        nres = CheckHistZero(self._res_ws)[0]

        setup_prog.report('Checking Histograms')
        prog = 'Qst'
        logger.information('Version is Qst')
        logger.information(' Number of spectra = %s ' % nsam)
        logger.information(' Erange : %f to %f ' % (erange[0], erange[1]))

        setup_prog.report('Establishing output workspace name')
        fname = self._sam_ws[:-4] + '_'+ prog
        fitWS = fname + '_Fit'
        wrks=os.path.join(workdir, self._sam_ws[:-4])
        logger.information(' lptfile : %s_Qst.lpt' % wrks)
        lwrk=len(wrks)
        wrks.ljust(140, ' ')
        wrkr=self._res_ws
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
            nout, bnorm, Xdat, Xv, Yv, Ev = CalcErange(self._sam_ws, m, erange, nbin)
            Ndat = nout[0]
            Imin = nout[1]
            Imax = nout[2]
            if prog == 'QLd':
                mm = m
            else:
                mm = 0
            Nb, Xb, Yb, Eb = GetXYE(self._res_ws, mm, array_len)     # get resolution data
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
            if (m > 0):
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
                            DataX=dataXz,
                            DataY=dataYz,
                            DataE=dataEz,
                            Nspec=Nsig,
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
        CreateWorkspace(OutputWorkspace=fname + '_Sigma',
                        DataX=xSig,
                        DataY=ySig,
                        DataE=eSig,
                        Nspec=nsam,
                        UnitX='',
                        VerticalAxisUnit='MomentumTransfer',
                        VerticalAxisValues=Qaxis)
        unitx = mtd[fname + '_Sigma'].getAxis(0).setUnit("Label")
        unitx.setLabel('sigma' , '')

        CreateWorkspace(OutputWorkspace=fname + '_Beta',
                        DataX=xBet,
                        DataY=yBet,
                        DataE=eBet,
                        Nspec=nsam,
                        UnitX='',
                        VerticalAxisUnit='MomentumTransfer',
                        VerticalAxisValues=Qaxis)
        unitx = mtd[fname + '_Beta'].getAxis(0).setUnit("Label")
        unitx.setLabel('beta' , '')

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
        CopyLogs(InputWorkspace=self._sam_ws,
                 OutputWorkspace=fit_ws)
        log_prog.report('Adding Sample logs to Fit workspace')
        QLAddSampleLogs(fit_ws, self._res_ws, prog, self._background, self._elastic, erange,
                        (nbin, nrbin), self._resnormWS, self._wfile)
        log_prog.report('Copying logs to Contour workspace')
        CopyLogs(InputWorkspace=self._sam_ws,
                 OutputWorkspace=contour_ws)
        log_prog.report('Adding sample logs to Contour workspace')
        QLAddSampleLogs(contour_ws, self._res_ws, prog, self._background, self._elastic, erange,
                        (nbin, nrbin), self._resnormWS, self._wfile)
        log_prog.report('Finialising log copying')

        if self._save:
            log_prog.report('Saving workspaces')
            fit_path = os.path.join(workdir, fit_ws + '.nxs')
            SaveNexusProcessed(InputWorkspace=fit_ws,
                               Filename=fit_path)
            logger.information('Output Fit file created : %s' % fit_path)
            contour_path = os.path.join(workdir, contour_ws + '.nxs')                    # path name for nxs file
            SaveNexusProcessed(InputWorkspace=contour_ws,
                               Filename=contour_path)
            logger.information('Output Contour file created : %s' % contour_path)
            log_prog.report('Files Saved')

        self.setProperty('OutputWorkspaceFit', fit_ws)
        self.setProperty('OutputWorkspaceContour', contour_ws)
        log_prog.report('Setting workspace properties')

AlgorithmFactory.subscribe(BayesStretch)         # Register algorithm with Mantid
