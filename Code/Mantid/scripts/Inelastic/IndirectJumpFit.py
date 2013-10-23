from mantid.simpleapi import *
from mantid import config, logger, mtd
from IndirectCommon import *
from IndirectImport import import_mantidplot
import sys, os.path
mp = import_mantidplot()

# Jump programs
def JumpRun(samWS,jumpFunc,width,qmin,qmax,Verbose=False,Plot=False,Save=False):
	StartTime('Jump fit : '+jumpFunc+' ; ')
	workdir = config['defaultsave.directory']
	
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

		logger.notice('Parameters in ' + samWs)

	x = mtd[samWS].readX(0)
	xmax = x[len(x)-1]

	#select fit function to use
	if jumpFunc == 'CE':
		# Chudley-Elliott: HWHM=A*(1-sin*(Q*K)/(Q*K))
		# for Q->0 W=A*Q^2*K^2/6

		aval = xmax
		bval = 1.5
		func = 'name=UserFunction, Formula=a*(1-(sin(x*b))/(x*b)), a='+str(aval)+', b='+str(bval)
	elif jumpFunc == 'SS':
		# Singwi-Sjolander: HWHM=A*(1-exp(-r*Q^2))
		# for Q->0 W=A*Q^2*r

		aval = xmax
		bval = 0.24
		func = 'name=UserFunction, Formula=a*(1-exp(-x*x*b)), a='+str(aval)+', b='+str(bval)
	else:
		sys.exit("Error in Jump Fit: Invalid fit function supplied.")
		return

	#run fit function
	fitWS = samWS +'_'+jumpFunc +'fit'
	Fit(Function=func, InputWorkspace=spectumWs, CreateOutput=True, Output=fitWS)

	#process output options
	if Save:
		fit_path = os.path.join(workdir,fitWS+'.nxs')
		SaveNexusProcessed(InputWorkspace=fitWS, Filename=fit_path)
		
		if Verbose:
			logger.notice('Fit file is ' + fit_path)

	if Plot:
		JumpPlot(fitWS+'_Workspace')

	EndTime('Jump fit : '+jumpFunc+' ; ')

def JumpPlot(inputWS):
    j_plot=mp.plotSpectrum(inputWS,[0,1,2],True)