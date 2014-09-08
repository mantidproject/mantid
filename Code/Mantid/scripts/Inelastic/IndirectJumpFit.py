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
    spectrumWs = "__" + samWS
    ExtractSingleSpectrum(InputWorkspace=samWS, OutputWorkspace=spectrumWs, WorkspaceIndex=width)

    #convert to HWHM
    Scale(InputWorkspace=spectrumWs, Factor=0.5, OutputWorkspace=spectrumWs)

    #crop the workspace between the given ranges
    if Verbose:
    	logger.notice('Cropping from Q= ' + str(qmin) +' to '+ str(qmax))

    #give the user some extra infromation if required
    if Verbose:
    	inGR = mtd[samWS].getRun()
    	try:
    		log = inGR.getLogData('fit_program')
    		if log:
    			val = log.value
    			logger.notice('Fit program was : '+val)
    	except RuntimeError:
    		#if we couldn't find the fit program, just pass
    		pass

    	logger.notice('Parameters in ' + samWS)

    x = mtd[samWS].readX(0)
    xmax = x[-1]

    #select fit function to use
    if jumpFunc == 'CE':
    	# Chudley-Elliott: HWHM=(1-sin*(Q*L)/(Q*L))/Tau
    	# for Q->0 W=Q^2*L^2/(6*Tau)

    	tval = 1.0/xmax
    	lval = 1.5
    	func = 'name=ChudleyElliot, Tau='+str(tval)+', L='+str(lval)

    elif jumpFunc == 'HallRoss':
    	# Hall-Ross: HWHM=(1-exp(-L*Q^2))/Tau
    	# for Q->0 W=A*Q^2*r

    	tval = 1.0/xmax
    	lval = 1.5
    	func = 'name=HallRoss, Tau='+str(tval)+', L='+str(lval)

    elif jumpFunc == 'Fick':
    	# Fick: HWHM=D*Q^2

    	y = mtd[samWS].readY(0)
    	diff = (y[2]-y[0])/((x[2]-x[0])*(x[2]-x[0]))
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
    fit_workspace_base = samWS[:-10] +'_'+ jumpFunc +'fit'
    Fit(Function=func, InputWorkspace=spectrumWs, CreateOutput=True, Output=fit_workspace_base, StartX=qmin, EndX=qmax)
    fit_workspace = fit_workspace_base + '_Workspace'

    CopyLogs(InputWorkspace=samWS, OutputWorkspace=fit_workspace)
    AddSampleLog(Workspace=fit_workspace, LogName="jump_function", LogType="String", LogText=jumpFunc)
    AddSampleLog(Workspace=fit_workspace, LogName="q_min", LogType="Number", LogText=str(qmin))
    AddSampleLog(Workspace=fit_workspace, LogName="q_max", LogType="Number", LogText=str(qmax))

    #process output options
    if Save:
    	fit_path = os.path.join(workdir,fit_workspace+'.nxs')
    	SaveNexusProcessed(InputWorkspace=fit_workspace, Filename=fit_path)

    	if Verbose:
    		logger.notice('Fit file is ' + fit_path)

    if Plot:
    	JumpPlot(fit_workspace)

    EndTime('Jump fit : '+jumpFunc+' ; ')

def JumpPlot(inputWS):
    mp.plotSpectrum(inputWS,[0,1,2],True)