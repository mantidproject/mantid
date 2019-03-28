# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#pylint: disable=invalid-name,too-many-arguments,too-many-locals

"""
Bayes routines
Fortran programs use fixed length arrays whereas Python has variable length lists
Input : the Python list is padded to Fortrans length using procedure PadArray
Output : the Fortran numpy array is sliced to Python length using dataY = yout[:ny]
"""

from __future__ import (absolute_import, division, print_function)
from IndirectImport import *
if is_supported_f2py_platform(): # noqa
    QLr     = import_f2py("QLres")
    QLd     = import_f2py("QLdata")
    Qse     = import_f2py("QLse")
    resnorm = import_f2py("ResNorm")
else:
    unsupported_message()

from mantid.simpleapi import *
from mantid import logger, mtd
from IndirectCommon import *
import os.path
import numpy as np
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

    #get indices of the bounds of our energy range
    minIndex = np.where(Xdata==Xin[0])[0][0]+1
    maxIndex = np.where(Xdata==Xin[-1])[0][0]

    #reshape array into sublists of bins
    Xin = Xin.reshape((len(Xin)//binWidth, binWidth))

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


# ResNorm programs
def ResNormRun(vname,rname,erange,nbin,Plot='None',Save=False):
    StartTime('ResNorm')

    workdir = getDefaultWorkingDirectory()

    array_len = 4096                                    # length of Fortran array
    CheckXrange(erange,'Energy')
    CheckAnalysersOrEFixed(vname,rname)
    nvan,ntc = CheckHistZero(vname)
    theta = GetThetaQ(vname)[0]
    efix = getEfixed(vname)
    print("beginning erange calc")
    nout,bnorm,Xdat,Xv,Yv,Ev = CalcErange(vname,0,erange,nbin)
    print("end of erange calc")
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
        nd,xout,yout,eout,yfit,pfit=resnorm.resnorm(numb,Xv,Yv,Ev,reals,
                                                    Xdat,Xb,Yb,wrks,wrkr,lwrk)
        message = ' Fit paras : '+str(pfit[0])+' '+str(pfit[1])
        logger.information(message)
        dataX = xout[:nd]
        dataX = np.append(dataX,2*xout[nd-1]-xout[nd-2])
        if m == 0:
            yPar1 = np.array([pfit[0]])
            yPar2 = np.array([pfit[1]])
            CreateWorkspace(OutputWorkspace='Data', DataX=dataX, DataY=yout[:nd], DataE=eout[:nd],
                            NSpec=1, UnitX='DeltaE')
            CreateWorkspace(OutputWorkspace='Fit', DataX=dataX, DataY=yfit[:nd], DataE=np.zeros(nd),
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

    CreateWorkspace(OutputWorkspace=resnorm_intesity, DataX=xPar, DataY=yPar1, DataE=xPar,
                    NSpec=1, UnitX='MomentumTransfer')
    CreateWorkspace(OutputWorkspace=resnorm_stretch, DataX=xPar, DataY=yPar2, DataE=xPar,
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
