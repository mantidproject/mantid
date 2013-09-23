from mantid.simpleapi import *
from mantid import config, logger, mtd
from IndirectCommon import *
from IndirectImport import import_mantidplot
import os.path
mp = import_mantidplot()

# Jump programs
def JumpRun(samWS,jump,width,qmin,qmax,Verbose=False,Plot=False,Save=False):
# Chudley-Elliott    HWHM=A*(1-sin*(Q*K)/(Q*K))
# for Q->0 W=A*Q^2*K^2/6
# Singwi-Sjolander  HWHM=A*(1-exp(-r*Q^2))
# for Q->0 W=A*Q^2*r
	StartTime('Jump fit : '+jump+' ; ')

	ExtractSingleSpectrum(InputWorkspace=samWS, OutputWorkspace=samWS, WorkspaceIndex=width)

	if Verbose:
		logger.notice('Cropping from Q= ' + str(qmin) +' to '+ str(qmax))
	CropWorkspace(InputWorkspace=samWS, OutputWorkspace=samWS,XMin=qmin, XMax=qmax)

	if Verbose:
		inGR = mtd[samWS].getRun()
		log = inGR.getLogData('Fit Program')
		if log:
			val = log.value
			logger.notice('Fit program was : '+val)

	workdir = config['defaultsave.directory']
	if Verbose:
		logger.notice('Parameters in ' + samWS)
	x = mtd[samWS].readX(0)
	xmax = x[len(x)-1]
	if jump == 'CE':
		aval = xmax
		bval = 1.5
		func = 'name=UserFunction, Formula=a*(1-(sin(x*b))/(x*b)), a='+str(aval)+', b='+str(bval)
	if jump == 'SS':
		aval = xmax
		bval = 0.24
		func = 'name=UserFunction, Formula=a*(1-exp(-x*x*b)), a='+str(aval)+', b='+str(bval)
	fitWS = samWS +'_'+jump +'fit'
	Fit(Function=func, InputWorkspace=samWS, CreateOutput=True, Output=fitWS)
	if Save:
		fit_path = os.path.join(workdir,fitWS+'.nxs')
		SaveNexusProcessed(InputWorkspace=fitWS, Filename=fit_path)
		if Verbose:
			logger.notice('Fit file is ' + fit_path)
	if Plot:
		JumpPlot(fitWS+'_Workspace')
	EndTime('Jump fit : '+jump+' ; ')

def JumpPlot(inputWS):
    j_plot=mp.plotSpectrum(inputWS,[0,1,2],True)