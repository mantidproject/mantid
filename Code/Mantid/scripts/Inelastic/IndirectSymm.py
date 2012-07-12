# SYMMetrise 
#
from mantid.simpleapi import *
from mantid import config, logger, mtd
from IndirectCommon import *
from IndirectImport import import_mantidplot
import sys, platform, math, os.path, numpy as np
mp = import_mantidplot()

def SymmRun(sample,cut,Verbose,Plot,Save):
	workdir = config['defaultsave.directory']
	symWS = sample[:-3] + 'sym'
	hist = mtd[sample].getNumberHistograms()       # no. of hist/groups in sam
	Xin = mtd[sample].readX(0)
	npt = len(Xin)-1
	delx = Xin[1]-Xin[0]
	for n in range(0,npt):
		x = Xin[n]-cut
		if math.fabs(x) < delx:
			ineg = n
	for n in range(0,npt):
		x = Xin[n]+Xin[ineg]
		if math.fabs(x) < delx:
			ipos = n
	ncut = npt-ipos+1
	if Verbose:
		logger.notice('No. points = '+str(npt))
		logger.notice('Negative : at i ='+str(ineg)+' ; x = '+str(Xin[ineg]))
		logger.notice('Positive : at i ='+str(ipos)+' ; x = '+str(Xin[ipos]))
		logger.notice('Cut points = '+str(ncut))
	for m in range(0,hist):
		Xin = mtd[sample].readX(m)
		Yin = mtd[sample].readY(m)
		Ein = mtd[sample].readE(m)
		Xout = []
		Yout = []
		Eout = []
		for n in range(0,ncut):
			icut = npt-n-1
			Xout.append(-Xin[icut])
			Yout.append(Yin[icut])
			Eout.append(Ein[icut])
		for n in range(ncut,npt):
			Xout.append(Xin[n])
			Yout.append(Yin[n])
			Eout.append(Ein[n])
		if m == 0:
			CreateWorkspace(OutputWorkspace=symWS, DataX=Xout, DataY=Yout, DataE=Eout,
				Nspec=1, UnitX='DeltaE')
		else:
			CreateWorkspace(OutputWorkspace='__tmp', DataX=Xout, DataY=Yout, DataE=Eout,
				Nspec=1, UnitX='DeltaE')
			ConjoinWorkspaces(InputWorkspace1=symWS, InputWorkspace2='__tmp',CheckOverlapping=False)				
#		Nspec=nt, UnitX='MomentumTransfer', VerticalAxisUnit='Text', VerticalAxisValues='Taxis')
# start output
	if Save:
		path = os.path.join(workdir,symWS+'.nxs')
		SaveNexusProcessed(InputWorkspace=symWS, Filename=path)
		if Verbose:
			logger.notice('Output file : ' + path)
	if Plot:
		plotSymm('','',Plot)

def SymmStart(inType,sname,cut,Verbose,Plot,Save):
	StartTime('Symmetrise')
	workdir = config['defaultsave.directory']
	if inType == 'File':
		spath = os.path.join(workdir, sname+'.nxs')		# path name for sample nxs file
		LoadNexusProcessed(FileName=spath, OutputWorkspace=sname)
		message = 'Input from File : '+spath
	else:
		message = 'Input from Workspace : '+sname
	if Verbose:
		logger.notice(message)
	SymmRun(sname,cut,Verbose,Plot,Save)
	EndTime('Symmetrise')

def plotSymm(inWS,spec_list,Plot):
	if (Plot == 'Totals' or Plot == 'All'):
		tot_plot=mp.plotSpectrum(inWS+'_Totals',spec_list)
	if (Plot == 'Scat1' or Plot == 'All'):
		mp.importMatrixWorkspace(inWS+'_1').plotGraph2D()
