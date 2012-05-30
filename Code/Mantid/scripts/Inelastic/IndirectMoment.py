# Transmission main
#
from mantid.simpleapi import *
import mantidplot as mp
from mantid import config, logger, mtd
from IndirectCommon import StartTime, EndTime, GetThetaQ
import numpy, os.path

def MomentRun(samWS,Verbose,Plot,Save):
    StartTime('Moments')
    workdir = config['defaultsave.directory']
    nq = mtd[samWS].getNumberHistograms()       # no. of hist/groups in sample
    if Verbose:
        logger.notice('Sample '+samWS+' has '+str(nq)+' Q values')
    axis = mtd[samWS].getAxis(1)
    Q = []
    e0 = []
    for i in range(0,nq):
        Q.append(float(axis.label(i)))
        e0.append(0.0)
    Xin = mtd[samWS].readX(0)
    nw = len(Xin)-1
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

def MomentStart(sname,Verbose,Plot,Save):
    workdir = config['defaultsave.directory']
    spath = os.path.join(workdir, sname+'.nxs')		# path name for sample nxs file
    LoadNexusProcessed(Filename=spath, OutputWorkspace=sname)
    MomentRun(sname,Verbose,Plot,Save)