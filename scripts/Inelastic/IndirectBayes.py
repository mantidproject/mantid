#pylint: disable=invalid-name,too-many-arguments,too-many-locals

"""
Bayes routines
Fortran programs use fixed length arrays whereas Python has variable lenght lists
Input : the Python list is padded to Fortrans length using procedure PadArray
Output : the Fortran numpy array is sliced to Python length using dataY = yout[:ny]
"""

from IndirectImport import *
if is_supported_f2py_platform():
    QLr     = import_f2py("QLres")
    QLd     = import_f2py("QLdata")
    Qse     = import_f2py("QLse")
    Que     = import_f2py("Quest")
    resnorm = import_f2py("ResNorm")
else:
    unsupported_message()

from mantid.simpleapi import *
from mantid import config, logger, mtd
from IndirectCommon import *
import sys, platform, math, os.path, numpy as np
MTD_PLOT = import_mantidplot()

def CalcErange(inWS,ns,erange,binWidth):
    #length of array in Fortran
    array_len = 4096

    binWidth = int(binWidth)
    bnorm = 1.0/binWidth

    #get data from input workspace
    _,X,Y,E = GetXYE(inWS,ns,array_len)
    Xdata = mtd[inWS].readX(0)

    #get all x values within the energy range
    rangeMask = (Xdata >= erange[0]) & (Xdata <= erange[1])
    Xin = Xdata[rangeMask]

    #get indicies of the bounds of our energy range
    minIndex = np.where(Xdata==Xin[0])[0][0]+1
    maxIndex = np.where(Xdata==Xin[-1])[0][0]

    #reshape array into sublists of bins
    Xin = Xin.reshape(len(Xin)/binWidth, binWidth)

    #sum and normalise values in bins
    Xout = [sum(bin_val) * bnorm for bin_val in Xin]

    #count number of bins
    nbins = len(Xout)

    nout = [nbins, minIndex, maxIndex]

     #pad array for use in Fortran code
    Xout = PadArray(Xout,array_len)

    return nout,bnorm,Xout,X,Y,E

def GetXYE(inWS,n,array_len):
    Xin = mtd[inWS].readX(n)
    N = len(Xin)-1                            # get no. points from length of x array
    Yin = mtd[inWS].readY(n)
    Ein = mtd[inWS].readE(n)
    X=PadArray(Xin,array_len)
    Y=PadArray(Yin,array_len)
    E=PadArray(Ein,array_len)
    return N,X,Y,E

# Quest programs
def CheckBetSig(nbs):
    Nsig = int(nbs[1])
    if Nsig == 0:
        raise ValueError('Number of sigma points is Zero')
    if Nsig > 200:
        raise ValueError('Max number of sigma points is 200')

    Nbet = int(nbs[0])
    if Nbet == 0:
        raise ValueError('Number of beta points is Zero')
    if Nbet > 200:
        raise ValueError('Max number of beta points is 200')

    return Nbet,Nsig

def QuestRun(samWS,resWS,nbs,erange,nbins,Fit,Loop,Plot,Save):
    StartTime('Quest')
    #expand fit options
    elastic, background, width, res_norm = Fit

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

    workdir = config['defaultsave.directory']
    if not os.path.isdir(workdir):
        workdir = os.getcwd()
        logger.information('Default Save directory is not set. Defaulting to current working Directory: ' + workdir)

    array_len = 4096                           # length of array in Fortran
    CheckXrange(erange,'Energy')
    nbin,nrbin = nbins[0],nbins[1]
    logger.information('Sample is ' + samWS)
    logger.information('Resolution is ' + resWS)
    CheckAnalysers(samWS,resWS)
    nsam,ntc = CheckHistZero(samWS)

    if Loop != True:
        nsam = 1

    efix = getEfixed(samWS)
    theta,Q = GetThetaQ(samWS)
    nres = CheckHistZero(resWS)[0]
    if nres == 1:
        prog = 'Qst'                        # res file
    else:
        raise ValueError('Stretched Exp ONLY works with RES file')
    logger.information(' Number of spectra = '+str(nsam))
    logger.information(' Erange : '+str(erange[0])+' to '+str(erange[1]))

    fname = samWS[:-4] + '_'+ prog
    wrks=os.path.join(workdir, samWS[:-4])
    logger.information(' lptfile : ' + wrks +'_Qst.lpt')
    lwrk=len(wrks)
    wrks.ljust(140,' ')
    wrkr=resWS
    wrkr.ljust(140,' ')
    Nbet,Nsig = nbs[0], nbs[1]
    eBet0 = np.zeros(Nbet)                  # set errors to zero
    eSig0 = np.zeros(Nsig)                  # set errors to zero
    rscl = 1.0
    Qaxis = ''
    for m in range(0,nsam):
        logger.information('Group ' +str(m)+ ' at angle '+ str(theta[m]))
        nsp = m+1
        nout,bnorm,Xdat,Xv,Yv,Ev = CalcErange(samWS,m,erange,nbin)
        Ndat = nout[0]
        Imin = nout[1]
        Imax = nout[2]
        Nb,Xb,Yb,_ = GetXYE(resWS,0,array_len)
        numb = [nsam, nsp, ntc, Ndat, nbin, Imin, Imax, Nb, nrbin, Nbet, Nsig]
        reals = [efix, theta[m], rscl, bnorm]
        xsout,ysout,xbout,ybout,zpout=Que.quest(numb,Xv,Yv,Ev,reals,fitOp,\
                                            Xdat,Xb,Yb,wrks,wrkr,lwrk)
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

        for n in range(0,Nsig):
            yfit_list = np.split(zpout[:Nsig*Nbet],Nsig)
            dataYzp = yfit_list[n]

            dataXz = np.append(dataXz,xbout[:Nbet])
            dataYz = np.append(dataYz,dataYzp[:Nbet])
            dataEz = np.append(dataEz,eBet0)

        CreateWorkspace(OutputWorkspace=zpWS, DataX=dataXz, DataY=dataYz, DataE=dataEz,
                        Nspec=Nsig, UnitX='MomentumTransfer',
                        VerticalAxisUnit='MomentumTransfer', VerticalAxisValues=dataXs)

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
    CreateWorkspace(OutputWorkspace=fname+'_Sigma', DataX=xSig, DataY=ySig, DataE=eSig,\
        Nspec=nsam, UnitX='', VerticalAxisUnit='MomentumTransfer', VerticalAxisValues=Qaxis)
    unitx = mtd[fname+'_Sigma'].getAxis(0).setUnit("Label")
    unitx.setLabel('sigma' , '')

    CreateWorkspace(OutputWorkspace=fname+'_Beta', DataX=xBet, DataY=yBet, DataE=eBet,\
        Nspec=nsam, UnitX='', VerticalAxisUnit='MomentumTransfer', VerticalAxisValues=Qaxis)
    unitx = mtd[fname+'_Beta'].getAxis(0).setUnit("Label")
    unitx.setLabel('beta' , '')

    group = fname + '_Sigma,'+ fname + '_Beta'

    fit_workspace = fname+'_Fit'
    contour_workspace = fname+'_Contour'
    GroupWorkspaces(InputWorkspaces=group,OutputWorkspace=fit_workspace)
    GroupWorkspaces(InputWorkspaces=groupZ,OutputWorkspace=contour_workspace)

    #add sample logs to the output workspaces
    CopyLogs(InputWorkspace=samWS, OutputWorkspace=fit_workspace)
    QuestAddSampleLogs(fit_workspace, resWS, background, elastic, erange, nbin, Nsig, Nbet)
    CopyLogs(InputWorkspace=samWS, OutputWorkspace=contour_workspace)
    QuestAddSampleLogs(contour_workspace, resWS, background, elastic, erange, nbin, Nsig, Nbet)

    if Save:
        fpath = os.path.join(workdir,fit_workspace+'.nxs')
        SaveNexusProcessed(InputWorkspace=fit_workspace, Filename=fpath)

        cpath = os.path.join(workdir,contour_workspace+'.nxs')
        SaveNexusProcessed(InputWorkspace=contour_workspace, Filename=cpath)

        logger.information('Output file for Fit : ' + fpath)
        logger.information('Output file for Contours : ' + cpath)

    if Plot != 'None' and Loop == True:
        QuestPlot(fname,Plot)
    EndTime('Quest')

def QuestAddSampleLogs(workspace, res_workspace, background, elastic_peak, e_range, sample_binning, sigma, beta):
    energy_min, energy_max = e_range

    AddSampleLog(Workspace=workspace, LogName="res_file",
                 LogType="String", LogText=res_workspace)
    AddSampleLog(Workspace=workspace, LogName="background",
                 LogType="String", LogText=str(background))
    AddSampleLog(Workspace=workspace, LogName="elastic_peak",
                 LogType="String", LogText=str(elastic_peak))
    AddSampleLog(Workspace=workspace, LogName="energy_min",
                 LogType="Number", LogText=str(energy_min))
    AddSampleLog(Workspace=workspace, LogName="energy_max",
                 LogType="Number", LogText=str(energy_max))
    AddSampleLog(Workspace=workspace, LogName="sample_binning",
                 LogType="Number", LogText=str(sample_binning))
    AddSampleLog(Workspace=workspace, LogName="sigma",
                 LogType="Number", LogText=str(sigma))
    AddSampleLog(Workspace=workspace, LogName="beta",
                 LogType="Number", LogText=str(beta))


def QuestPlot(inputWS,Plot):
    if Plot == 'Sigma' or Plot == 'All':
        MTD_PLOT.importMatrixWorkspace(inputWS+'_Sigma').plotGraph2D()
    if Plot == 'Beta' or Plot == 'All':
        MTD_PLOT.importMatrixWorkspace(inputWS+'_Beta').plotGraph2D()

# ResNorm programs
def ResNormRun(vname,rname,erange,nbin,Plot='None',Save=False):
    StartTime('ResNorm')

    workdir = getDefaultWorkingDirectory()

    array_len = 4096                                    # length of Fortran array
    CheckXrange(erange,'Energy')
    CheckAnalysers(vname,rname)
    nvan,ntc = CheckHistZero(vname)
    theta = GetThetaQ(vname)[0]
    efix = getEfixed(vname)
    print "begining erange calc"
    nout,bnorm,Xdat,Xv,Yv,Ev = CalcErange(vname,0,erange,nbin)
    print "end of erange calc"
    Ndat = nout[0]
    Imin = nout[1]
    Imax = nout[2]
    wrks=os.path.join(workdir, vname[:-4])
    logger.information(' Number of spectra = '+str(nvan))
    logger.information(' lptfile : ' + wrks +'_resnrm.lpt')
    lwrk=len(wrks)
    wrks.ljust(140,' ')                              # pad for fioxed Fortran length
    wrkr=rname
    wrkr.ljust(140,' ')
    Nb,Xb,Yb,_ = GetXYE(rname,0,array_len)
    rscl = 1.0
    xPar = np.array([theta[0]])
    for m in range(1,nvan):
        xPar = np.append(xPar,theta[m])
    fname = vname[:-4]
    for m in range(0,nvan):
        logger.information('Group ' +str(m)+ ' at angle '+ str(theta[m]))
        ntc,Xv,Yv,Ev = GetXYE(vname,m,array_len)
        nsp = m+1
        numb = [nvan, nsp, ntc, Ndat, nbin, Imin, Imax, Nb]
        reals = [efix, theta[0], rscl, bnorm]
        nd,xout,yout,eout,yfit,pfit=resnorm.resnorm(numb,Xv,Yv,Ev,reals,\
                                    Xdat,Xb,Yb,wrks,wrkr,lwrk)
        message = ' Fit paras : '+str(pfit[0])+' '+str(pfit[1])
        logger.information(message)
        dataX = xout[:nd]
        dataX = np.append(dataX,2*xout[nd-1]-xout[nd-2])
        if m == 0:
            yPar1 = np.array([pfit[0]])
            yPar2 = np.array([pfit[1]])
            CreateWorkspace(OutputWorkspace='Data', DataX=dataX, DataY=yout[:nd], DataE=eout[:nd],\
                NSpec=1, UnitX='DeltaE')
            CreateWorkspace(OutputWorkspace='Fit', DataX=dataX, DataY=yfit[:nd], DataE=np.zeros(nd),\
                NSpec=1, UnitX='DeltaE')
        else:
            yPar1 = np.append(yPar1,pfit[0])
            yPar2 = np.append(yPar2,pfit[1])

            CreateWorkspace(OutputWorkspace='__datmp', DataX=dataX, DataY=yout[:nd],
                            DataE=eout[:nd], NSpec=1, UnitX='DeltaE')
            ConjoinWorkspaces(InputWorkspace1='Data', InputWorkspace2='__datmp',
                              CheckOverlapping=False)
            CreateWorkspace(OutputWorkspace='__f1tmp', DataX=dataX, DataY=yfit[:nd],
                            DataE=np.zeros(nd), NSpec=1, UnitX='DeltaE')
            ConjoinWorkspaces(InputWorkspace1='Fit', InputWorkspace2='__f1tmp',
                              CheckOverlapping=False)

    resnorm_intesity = fname+'_ResNorm_Intensity'
    resnorm_stretch = fname+'_ResNorm_Stretch'

    CreateWorkspace(OutputWorkspace=resnorm_intesity, DataX=xPar, DataY=yPar1, DataE=xPar,\
        NSpec=1, UnitX='MomentumTransfer')
    CreateWorkspace(OutputWorkspace=resnorm_stretch, DataX=xPar, DataY=yPar2, DataE=xPar,\
        NSpec=1, UnitX='MomentumTransfer')

    group = resnorm_intesity + ','+ resnorm_stretch

    resnorm_workspace = fname+'_ResNorm'
    resnorm_fit_workspace = fname+'_ResNorm_Fit'

    GroupWorkspaces(InputWorkspaces=group,OutputWorkspace=resnorm_workspace)
    GroupWorkspaces(InputWorkspaces='Data,Fit',OutputWorkspace=resnorm_fit_workspace)

    CopyLogs(InputWorkspace=vname, OutputWorkspace=resnorm_workspace)
    ResNormAddSampleLogs(resnorm_workspace, erange, nbin)

    CopyLogs(InputWorkspace=vname, OutputWorkspace=resnorm_fit_workspace)
    ResNormAddSampleLogs(resnorm_fit_workspace, erange, nbin)

    if Save:
        par_path = os.path.join(workdir,resnorm_workspace+'.nxs')
        SaveNexusProcessed(InputWorkspace=resnorm_workspace, Filename=par_path)

        fit_path = os.path.join(workdir,resnorm_fit_workspace+'.nxs')
        SaveNexusProcessed(InputWorkspace=resnorm_fit_workspace, Filename=fit_path)

        logger.information('Parameter file created : ' + par_path)
        logger.information('Fit file created : ' + fit_path)

    if Plot != 'None':
        ResNormPlot(fname,Plot)
    EndTime('ResNorm')

def ResNormAddSampleLogs(workspace, e_range, v_binning):
    energy_min, energy_max = e_range

    AddSampleLog(Workspace=workspace, LogName="energy_min",
                 LogType="Number", LogText=str(energy_min))
    AddSampleLog(Workspace=workspace, LogName="energy_max",
                 LogType="Number", LogText=str(energy_max))
    AddSampleLog(Workspace=workspace, LogName="van_binning",
                 LogType="Number", LogText=str(v_binning))

def ResNormPlot(inputWS,Plot):
    if Plot == 'Intensity' or Plot == 'All':
        iWS = inputWS + '_ResNorm_Intensity'
        MTD_PLOT.plotSpectrum(iWS,0,False)
    if Plot == 'Stretch' or Plot == 'All':
        sWS = inputWS + '_ResNorm_Stretch'
        MTD_PLOT.plotSpectrum(sWS,0,False)
    if Plot == 'Fit' or Plot == 'All':
        fWS = inputWS + '_ResNorm_Fit'
        MTD_PLOT.plotSpectrum(fWS,0,False)
