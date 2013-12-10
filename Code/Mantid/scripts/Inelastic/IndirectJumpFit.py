from mantid.simpleapi import *
from mantid import config, logger, mtd
from IndirectCommon import *
from IndirectImport import import_mantidplot
import sys, os.path
mp = import_mantidplot()

# Jump programs
def JumpRun(samWS,jumpFunc,width,qmin,qmax,Verbose=False,Plot=False,Save=False):
	StartTime('Jump fit : '+jumpFunc+' ; ')

	workdir = getDefaultWorkingDirectory()

	#select the width we wish to fit
	spectumWs = "__" + samWS
	ExtractSingleSpectrum(InputWorkspace=samWS, OutputWorkspace=spectumWs, WorkspaceIndex=width)

	#crop the workspace between the given ranges
	if Verbose:
		logger.notice('Cropping from Q= ' + str(qmin) +' to '+ str(qmax))

	CropWorkspace(InputWorkspace=spectumWs, OutputWorkspace=spectumWs,XMin=qmin, XMax=qmax)

	#give the user some extra infromation is required
	if Verbose:
		inGR = mtd[samWS].getRun()
		log = inGR.getLogData('Fit Program')
		
		if log:
			val = log.value
			logger.notice('Fit program was : '+val)

		logger.notice('Parameters in ' + samWS)

	x = mtd[samWS].readX(0)
	y = mtd[samWS].readY(0)
	xmax = x[len(x)-1]
	diff = (y[2]-y[0])/((x[2]-x[0])*(x[2]-x[0]))
	
	#select fit function to use
	if jumpFunc == 'CE':
		# Chudley-Elliott: HWHM=(1-sin*(Q*L)/(Q*L))/Tau
		# for Q->0 W=Q^2*L^2/(6*Tau)

		tval = 1.0/xmax
		lval = 1.5
		func = 'name=ChudleyElliot, Tau='+str(tval)+', L='+str(lval)

	elif jumpFunc == 'SS':
		# Singwi-Sjolander: HWHM=(1-exp(-L*Q^2))/Tau
		# for Q->0 W=A*Q^2*r

		tval = 1.0/xmax
		lval = 1.5
		func = 'name=SingwiSjolander, Tau='+str(tval)+', L='+str(lval)

	elif jumpFunc == 'Fick':
		# Fick: HWHM=D*Q^2

		func = 'name=FickDiffusion, D='+str(diff)

	elif jumpFunc == 'Teixeira':
		# Teixeira: HWHM=Q^2*L/((1+Q^2*L)*tau)
		# for Q->0 W=

		tval = 1.0/xmax
		lval = 1.5
		func = 'name=TeixeiraWater, Tau='+str(tval)+', L='+str(lval)
		
	else:
		sys.exit("Error in Jump Fit: Invalid fit function supplied.")
		return

	#run fit function
	fitWS = samWS[:-10] +'_'+ jumpFunc +'fit'
	Fit(Function=func, InputWorkspace=spectumWs, CreateOutput=True, Output=fitWS)

	#process output options
	if Save:
		fit_path = os.path.join(workdir,fitWS+'_Workspace.nxs')
		SaveNexusProcessed(InputWorkspace=fitWS+'_Workspace', Filename=fit_path)
		
		if Verbose:
			logger.notice('Fit file is ' + fit_path)

	if Plot:
		JumpPlot(fitWS+'_Workspace')

	EndTime('Jump fit : '+jumpFunc+' ; ')

def JumpPlot(inputWS):
    mp.plotSpectrum(inputWS,[0,1,2],True)