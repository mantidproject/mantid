#pylint: disable=no-init
from mantid.api import PythonAlgorithm, AlgorithmFactory, MatrixWorkspaceProperty, PropertyMode
from mantid.kernel import StringListValidator, Direction
from mantid.simpleapi import *
from mantid import config, logger
import os

class QLRun(PythonAlgorithm):

    def category(self):
        return "Workflow\\MIDAS;PythonAlgorithms"

    def summary(self):
        return "This algorithm runs the Fortran QLines programs which fits a Delta function of"+\
               " amplitude 0 and Lorentzians of amplitude A(j) and HWHM W(j) where j=1,2,3. The"+\
               " whole function is then convoled with the resolution function."

    def version(self):
        return 1

    def PyInit(self):
        self.declareProperty(name='Program', defaultValue='QL',
                             validator=StringListValidator(['QL','QSe']),
                             doc='The type of program to run (either QL or QSe)')

        self.declareProperty(MatrixWorkspaceProperty('SampleWorkspace', '', direction=Direction.Input),
                             doc='Name of the Sample input Workspace')

        self.declareProperty(MatrixWorkspaceProperty('ResolutionWorkspace', '', direction=Direction.Input),
                             doc='Name of the resolution input Workspace')

        self.declareProperty(MatrixWorkspaceProperty('ResNormWorkspace', '',
                             optional=PropertyMode.Optional, direction=Direction.Input),
                             doc='Name of the ResNorm input Workspace')

        self.declareProperty(name='MinRange', defaultValue=-0.2,
                             doc='The start of the fit range. Default=-0.2')

        self.declareProperty(name='MaxRange', defaultValue=0.2,
                             doc='The end of the fit range. Default=0.2')

        self.declareProperty(name='SampleBins', defaultValue=1,
                             doc='The number of sample bins')

        self.declareProperty(name='ResolutionBins', defaultValue=1,
                             doc='The number of resolution bins')

        self.declareProperty(name='Elastic', defaultValue=True,
                             doc='Fit option for using the elastic peak')

        self.declareProperty(name='Background', defaultValue='Flat',
                             validator=StringListValidator(['Sloping','Flat','Zero']),
                             doc='Fit option for the type of background')

        self.declareProperty(name='FixedWidth', defaultValue=True,
                             doc='Fit option for using FixedWidth')

        self.declareProperty(name='UseResNorm', defaultValue=False,
                             doc='fit option for using ResNorm')

        self.declareProperty(name='WidthFile', defaultValue='', doc='The name of the fixedWidth file')

        self.declareProperty(name='Loop', defaultValue=True, doc='Switch Sequential fit On/Off')

        self.declareProperty(name='Plot', defaultValue='', doc='Plot options')

        self.declareProperty(name='Save', defaultValue=False, doc='Switch Save result to nxs file Off/On')

    def PyExec(self):
        from IndirectImport import run_f2py_compatibility_test, is_supported_f2py_platform

        if is_supported_f2py_platform():
            import IndirectBayes as Main

        run_f2py_compatibility_test()

        from IndirectBayes import *

        self.log().information('QLRun input')

        program = self.getPropertyValue('Program')
        samWS = self.getPropertyValue('SampleWorkspace')
        resWS = self.getPropertyValue('ResolutionWorkspace')
        resnormWS = self.getPropertyValue('ResNormWorkspace')
        e_min = self.getProperty('MinRange').value
        e_max = self.getProperty('MaxRange').value
        sam_bins = self.getPropertyValue('SampleBins')
        res_bins = self.getPropertyValue('ResolutionBins')
        elastic = self.getProperty('Elastic').value
        background = self.getPropertyValue('Background')
        width = self.getProperty('FixedWidth').value
        res_norm = self.getProperty('UseResNorm').value
        wfile = self.getPropertyValue('WidthFile')
        Loop = self.getProperty('Loop').value
        Save = self.getProperty('Save').value
        Plot = self.getPropertyValue('Plot')

        erange = [e_min, e_max]
        nbins = [sam_bins, res_bins]

        #convert true/false to 1/0 for fortran
        o_el = 1 if elastic else 0
        o_w1 = 1 if width else 0
        o_res = 1 if res_norm else 0

        #fortran code uses background choices defined using the following numbers
        if background == 'Sloping':
            o_bgd = 2
        elif background == 'Flat':
            o_bgd = 1
        elif background == 'Zero':
            o_bgd = 0

        fitOp = [o_el, o_bgd, o_w1, o_res]

        workdir = getDefaultWorkingDirectory()

        array_len = 4096                           # length of array in Fortran
        CheckXrange(erange,'Energy')

        nbin,nrbin = nbins[0], nbins[1]

        logger.information('Sample is ' + samWS)
        logger.information('Resolution is ' + resWS)

        CheckAnalysers(samWS,resWS)
        efix = getEfixed(samWS)
        theta, Q = GetThetaQ(samWS)

        nsam,ntc = CheckHistZero(samWS)

        totalNoSam = nsam

        #check if we're performing a sequential fit
        if Loop != True:
            nsam = 1

        nres = CheckHistZero(resWS)[0]

        if program == 'QL':
            if nres == 1:
                prog = 'QLr'                        # res file
            else:
                prog = 'QLd'                        # data file
                CheckHistSame(samWS,'Sample',resWS,'Resolution')
        elif program == 'QSe':
            if nres == 1:
                prog = 'QSe'                        # res file
            else:
                raise ValueError('Stretched Exp ONLY works with RES file')

        logger.information('Version is ' +prog)
        logger.information(' Number of spectra = '+str(nsam))
        logger.information(' Erange : '+str(erange[0])+' to '+str(erange[1]))

        Wy,We = ReadWidthFile(width,wfile,totalNoSam)
        dtn,xsc = ReadNormFile(res_norm,resnormWS,totalNoSam)

        fname = samWS[:-4] + '_'+ prog
        probWS = fname + '_Prob'
        fitWS = fname + '_Fit'
        wrks=os.path.join(workdir, samWS[:-4])
        logger.information(' lptfile : '+wrks+'_'+prog+'.lpt')
        lwrk=len(wrks)
        wrks.ljust(140,' ')
        wrkr=resWS
        wrkr.ljust(140,' ')

        # initialise probability list
        if program == 'QL':
            prob0 = []
            prob1 = []
            prob2 = []
        xQ = np.array([Q[0]])
        for m in range(1,nsam):
            xQ = np.append(xQ,Q[m])
        xProb = xQ
        xProb = np.append(xProb,xQ)
        xProb = np.append(xProb,xQ)
        eProb = np.zeros(3*nsam)

        group = ''
        for m in range(0,nsam):
            logger.information('Group ' +str(m)+ ' at angle '+ str(theta[m]))
            nsp = m+1
            nout,bnorm,Xdat,Xv,Yv,Ev = CalcErange(samWS,m,erange,nbin)
            Ndat = nout[0]
            Imin = nout[1]
            Imax = nout[2]
            if prog == 'QLd':
                mm = m
            else:
                mm = 0
            Nb,Xb,Yb,Eb = GetXYE(resWS,mm,array_len)     # get resolution data
            numb = [nsam, nsp, ntc, Ndat, nbin, Imin, Imax, Nb, nrbin]
            rscl = 1.0
            reals = [efix, theta[m], rscl, bnorm]

            if prog == 'QLr':
                nd,xout,yout,eout,yfit,yprob=QLr.qlres(numb,Xv,Yv,Ev,reals,fitOp,
                                                       Xdat,Xb,Yb,Wy,We,dtn,xsc,
                                                       wrks,wrkr,lwrk)
                message = ' Log(prob) : '+str(yprob[0])+' '+str(yprob[1])+' '+str(yprob[2])+' '+str(yprob[3])
                logger.information(message)
            if prog == 'QLd':
                nd,xout,yout,eout,yfit,yprob=QLd.qldata(numb,Xv,Yv,Ev,reals,fitOp,
                                                        Xdat,Xb,Yb,Eb,Wy,We,
                                                        wrks,wrkr,lwrk)
                message = ' Log(prob) : '+str(yprob[0])+' '+str(yprob[1])+' '+str(yprob[2])+' '+str(yprob[3])
                logger.information(message)
            if prog == 'QSe':
                nd,xout,yout,eout,yfit,yprob=Qse.qlstexp(numb,Xv,Yv,Ev,reals,fitOp,\
                                                        Xdat,Xb,Yb,Wy,We,dtn,xsc,\
                                                        wrks,wrkr,lwrk)
            dataX = xout[:nd]
            dataX = np.append(dataX,2*xout[nd-1]-xout[nd-2])
            yfit_list = np.split(yfit[:4*nd],4)
            dataF1 = yfit_list[1]
            if program == 'QL':
                dataF2 = yfit_list[2]
            dataG = np.zeros(nd)
            datX = dataX
            datY = yout[:nd]
            datE = eout[:nd]
            datX = np.append(datX,dataX)
            datY = np.append(datY,dataF1[:nd])
            datE = np.append(datE,dataG)
            res1 = dataF1[:nd] - yout[:nd]
            datX = np.append(datX,dataX)
            datY = np.append(datY,res1)
            datE = np.append(datE,dataG)
            nsp = 3
            names = 'data,fit.1,diff.1'
            res_plot = [0, 1, 2]
            if program == 'QL':
                datX = np.append(datX,dataX)
                datY = np.append(datY,dataF2[:nd])
                datE = np.append(datE,dataG)
                res2 = dataF2[:nd] - yout[:nd]
                datX = np.append(datX,dataX)
                datY = np.append(datY,res2)
                datE = np.append(datE,dataG)
                nsp += 2
                names += ',fit.2,diff.2'
                res_plot.append(4)
                prob0.append(yprob[0])
                prob1.append(yprob[1])
                prob2.append(yprob[2])

            # create result workspace
            fitWS = fname+'_Workspaces'
            fout = fname+'_Workspace_'+ str(m)

            CreateWorkspace(OutputWorkspace=fout, DataX=datX, DataY=datY, DataE=datE,\
                Nspec=nsp, UnitX='DeltaE', VerticalAxisUnit='Text', VerticalAxisValues=names)

            # append workspace to list of results
            group += fout + ','

        GroupWorkspaces(InputWorkspaces=group,OutputWorkspace=fitWS)

        if program == 'QL':
            yPr0 = np.array([prob0[0]])
            yPr1 = np.array([prob1[0]])
            yPr2 = np.array([prob2[0]])
            for m in range(1,nsam):
                yPr0 = np.append(yPr0,prob0[m])
                yPr1 = np.append(yPr1,prob1[m])
                yPr2 = np.append(yPr2,prob2[m])
            yProb = yPr0
            yProb = np.append(yProb,yPr1)
            yProb = np.append(yProb,yPr2)
            CreateWorkspace(OutputWorkspace=probWS, DataX=xProb, DataY=yProb, DataE=eProb,\
                Nspec=3, UnitX='MomentumTransfer')
            outWS = C2Fw(samWS[:-4],fname)
            if Plot != 'None':
                QuasiPlot(fname,Plot,res_plot,Loop)
        if program == 'QSe':
            outWS = C2Se(fname)
            if Plot != 'None':
                QuasiPlot(fname,Plot,res_plot,Loop)

        #Add some sample logs to the output workspaces
        CopyLogs(InputWorkspace=samWS, OutputWorkspace=outWS)
        QLAddSampleLogs(outWS, resWS, prog, background, elastic, erange,
                        (nbin, nrbin), resnormWS, wfile)
        CopyLogs(InputWorkspace=samWS, OutputWorkspace=fitWS)
        QLAddSampleLogs(fitWS, resWS, prog, background, elastic, erange,
                        (nbin, nrbin), resnormWS, wfile)

        if Save:
            fit_path = os.path.join(workdir,fitWS+'.nxs')
            SaveNexusProcessed(InputWorkspace=fitWS, Filename=fit_path)
            out_path = os.path.join(workdir, outWS+'.nxs')                    # path name for nxs file
            SaveNexusProcessed(InputWorkspace=outWS, Filename=out_path)
            logger.information('Output fit file created : ' + fit_path)
            logger.information('Output paramter file created : ' + out_path)



# Register algorithm with Mantid
AlgorithmFactory.subscribe(QLRun)