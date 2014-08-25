# Transmission main
#
from IndirectImport import *
from mantid.simpleapi import *
from mantid import config, logger, mtd
from IndirectCommon import *
import math, numpy, os.path
mp = import_mantidplot()

def CheckElimits(erange,Xin):
    nx = len(Xin)-1
    if math.fabs(erange[0]) < 1e-5:
        error = 'Elimits - input emin ( '+str(erange[0])+' ) is Zero'
        logger.notice('ERROR *** ' + error)
        sys.exit(error)
    if erange[0] < Xin[0]:
        error = 'Elimits - input emin ( '+str(erange[0])+' ) < data emin ( '+str(Xin[0])+' )'
        logger.notice('ERROR *** ' + error)
        sys.exit(error)
    if math.fabs(erange[1]) < 1e-5:
        error = 'Elimits - input emax ( '+str(erange[1])+' ) is Zero'
        logger.notice('ERROR *** ' + error)
        sys.exit(error)
    if erange[1] > Xin[nx]:
        error = 'Elimits - input emax ( '+str(erange[1])+' ) > data emax ( '+str(Xin[nx])+' )'
        logger.notice('ERROR *** ' + error)
        sys.exit(error)
    if erange[1] < erange[0]:
        error = 'Elimits - input emax ( '+str(erange[1])+' ) < emin ( '+erange[0]+' )'
        logger.notice('ERROR *** ' + error)
        sys.exit(error)

def MomentRun(samWS,erange,factor,Verbose,Plot,Save):
    StartTime('Moments')
    workdir = config['defaultsave.directory']
    nq,nw = CheckHistZero(samWS)
    if Verbose:
        logger.notice('Sample '+samWS+' has '+str(nq)+' Q values & '+str(nw)+' w values')
    axis = mtd[samWS].getAxis(1)
    Q = []
    e0 = []
    for i in range(0,nq):
        Q.append(float(axis.label(i)))
        e0.append(0.0)
    Xin = mtd[samWS].readX(0)
    CheckElimits(erange,Xin)
    CropWorkspace(InputWorkspace=samWS, OutputWorkspace=samWS, XMin=erange[0], XMax=erange[1])
    Xin = mtd[samWS].readX(0)
    nw = len(Xin)-1
    if Verbose:
        logger.notice('Energy range is '+str(Xin[0])+' to '+str(Xin[nw]))
    if factor > 0.0:
        Scale(InputWorkspace=samWS, OutputWorkspace=samWS, Factor=factor, Operation='Multiply')
        if Verbose:
            logger.notice('S(q,w) scaled by '+str(factor))
    w = Xin[:nw]
    yM0 = []
    yM1 = []
    yM2 = []
    yM4 = []
    for m in range(0,nq):
        if Verbose:
            logger.notice('Group '+str(m+1)+' at Q = '+str(Q[m]))
        S = mtd[samWS].readY(m)
        m0 = sum(S)
        wS = w*S
        m1 = sum(wS)/m0
        w2S = w*wS
        m2 = sum(w2S)/m0
        w4S = w*w*w2S
        m4 = sum(w4S)/m0
        text = 'M0 = '+str(m0)+' ; M2 = '+str(m2)+' ; M4 = '+str(m4)
        logger.notice(text)
        yM0.append(m0)
        yM1.append(m1)
        yM2.append(m2)
        yM4.append(m4)
    fname = samWS[:-3] + 'Moments'
    CreateWorkspace(OutputWorkspace=fname+'_M0', DataX=Q, DataY=yM0, DataE=e0,
        Nspec=1, UnitX='MomentumTransfer')
    CreateWorkspace(OutputWorkspace=fname+'_M1', DataX=Q, DataY=yM1, DataE=e0,
        Nspec=1, UnitX='MomentumTransfer')
    CreateWorkspace(OutputWorkspace=fname+'_M2', DataX=Q, DataY=yM2, DataE=e0,
        Nspec=1, UnitX='MomentumTransfer')
    CreateWorkspace(OutputWorkspace=fname+'_M4', DataX=Q, DataY=yM4, DataE=e0,
        Nspec=1, UnitX='MomentumTransfer')
    group = fname+'_M0,'+fname+'_M1,'+fname+'_M2,'+fname+'_M4'
    GroupWorkspaces(InputWorkspaces=group,OutputWorkspace=fname)
    if Save:
        opath = os.path.join(workdir,fname+'.nxs')
        SaveNexusProcessed(InputWorkspace=fname, Filename=opath)
        if Verbose:
    		logger.notice('Output file : ' + opath)
    if (Plot != 'None'):
        MomentPlot(fname,Plot)
    EndTime('Moments')

def MomentPlot(inputWS,Plot):
    m0_plot=mp.plotSpectrum(inputWS+'_M0',0)
    m2_plot=mp.plotSpectrum([inputWS+'_M2',inputWS+'_M4'],0)

def MomentStart(inType,sname,erange,factor,Verbose,Plot,Save):
    workdir = config['defaultsave.directory']
    if inType == 'File':
        spath = os.path.join(workdir, sname+'.nxs')    	# path name for sample nxs file
        logger.notice('Input from File : '+spath)
        LoadNexusProcessed(Filename=spath, OutputWorkspace=sname)
    else:
        logger.notice('Input from Workspace : '+sname)
    MomentRun(sname,erange,factor,Verbose,Plot,Save)