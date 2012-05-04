# IDA F2PY Absorption Corrections Wrapper
## Handle selection of .pyd files for absorption corrections
import platform, sys
operatingenvironment = platform.system()+platform.architecture()[0]
if ( operatingenvironment == 'Windows32bit' ):
    import fltabs_win32 as fltabs, cylabs_win32 as cylabs
else:
    if ( operatingenvironment == 'Linux64bit' ):
        import fltabs_lnx64 as fltabs, cylabs_lnx64 as cylabs
    else:
        sys.exit('F2Py Absorption Corrections programs NOT available on ' + operatingenvironment)
##

## Other imports
from mantidsimple import *
import mantidplot as mp
from IndirectCommon import *
from mantid import config, logger
import math, os.path

def Array2List(n,Array):
	List = []
	for m in range(0,n):
		List.append(Array[m])
	return List

def WaveRange(inWS, efixed):
    oWS = '__WaveRange'
    ExtractSingleSpectrum(InputWorkspace=inWS, OutputWorkspace=oWS, WorkspaceIndex=0)
    ConvertUnits(InputWorkspace=oWS, OutputWorkspace=oWS, Target='Wavelength',
        EMode='Indirect', EFixed=efixed)
    Xin = mtd[oWS].readX(0)
    xmin = mtd[oWS].readX(0)[0]
    xmax = mtd[oWS].readX(0)[len(Xin)-1]
    ebin = 0.5
    nw1 = int(xmin/ebin)
    nw2 = int(xmax/ebin)+1
    w1 = nw1*ebin
    w2 = nw2*ebin
    wave = []
    nw = 10
    ebin = (w2-w1)/(nw-1)
    for l in range(0,nw):
        wave.append(w1+l*ebin)
    DeleteWorkspace(oWS)
    return wave

def AbsRun(inputWS, geom, beam, ncan, size, density, sigs, siga, avar, Verbose):
    StartTime('Absorption Correction')
    workdir = config['defaultsave.directory']
    if geom == 'flt':
        if(math.fabs(avar-90.0) < 10.0):
            error = 'ERROR ** can angle ' +str(avar)+ ' cannot be 90 +/- 10'
            logger.notice(error)
            exit(error)
        else:
            if Verbose:
                logger.notice(' Flat geom: can angle = '+str(avar))
    det = GetWSangles(inputWS)
    ndet = len(det)
    efixed = getEfixed(inputWS)
    wavelas = math.sqrt(81.787/efixed) # elastic wavelength
    waves = WaveRange(inputWS, efixed) # get wavelengths
    nw = len(waves)
    if Verbose:
        logger.notice('Sample run : '+inputWS)
        message = '  sigt = '+str(sigs[0])+' ; siga = '+str(siga[0])+' ; rho = '+str(density[0])
        logger.notice(message)
        if geom == 'cyl':
            message = ' Cyl geom: inner radius = '+str(size[0])+' ; outer radius = '+str(size[1])
            logger.notice(message)
        if geom == 'flt':
            logger.notice(' Flat geom: thickness = '+str(size[0]))
        if ncan == 2:
            message = 'Can : sigt = '+str(sigs[1])+' ; siga = '+str(siga[1])+' ; rho = '+str(density[1])
            logger.notice(message)
            if geom == 'cyl':
                logger.notice(message)
                message = ' Cyl geom: inner radius = '+str(size[1])+' ; outer radius = '+str(size[2])
            if geom == 'flt':
                logger.notice(' Flat geom: thickness = '+str(size[1]))
        logger.notice('Elastic lambda : '+str(wavelas))
        message = 'Lambda : '+str(nw)+' values from '+str(waves[0])+' to '+str(waves[nw-1])
        logger.notice(message)
        message = 'Detector angles : '+str(ndet)+' from '+str(det[0])+' to '+str(det[ndet-1])
        logger.notice(message)
    dataX = []
    dataE = []
    for n in range(0,nw):
        dataX.append(waves[n])
        dataE.append(0)
    name = inputWS[:-3] + geom
    assWS = name + '_ass'
    asscWS = name + '_assc'
    acscWS = name + '_acsc'
    accWS = name + '_acc'
    fname = name +'_Abs'
    wrk = workdir + inputWS[:-4]
    wrk.ljust(120,' ')
    plot_list = []
    dataY_a1ws = []
    dataY_a2ws = []
    dataY_a3ws = []
    dataY_a4ws = []
    for n in range(0,ndet):
        if geom == 'flt':
            angles = [avar, det[n]]
            kill, A1, A2, A3, A4 = fltabs.fltabs(ncan, size, density, sigs,
                siga, angles, waves, n, wrk, 0)
        if geom == 'cyl':
            astep = avar
            angle = det[n]
            kill, A1, A2, A3, A4 = cylabs.cylabs(astep, beam, ncan, size,
                density, sigs, siga, angle, wavelas, waves, n, wrk, 0)
        if kill == 0:
            if Verbose:
                logger.notice('Detector '+str(n)+' at angle : '+str(det[n])+' successful')
            dataY_a1ws += list(A1)
            dataY_a2ws += list(A2)
            dataY_a3ws += list(A3)
            dataY_a4ws += list(A4)              
            plot_list.append(n)
        else:
            error = 'Detector '+str(n)+' at angle : '+str(det[n])+' *** failed : Error code '+str(kill)
            exit(error)
    ## Create the workspaces
    nspec = len(plot_list)
    dataX = dataX * nspec
    dataE = dataE * nspec
    qAxis = createQaxis(inputWS)
    CreateWorkspace(OutputWorkspace=assWS, DataX=dataX, DataY=dataY_a1ws, DataE=dataE,
        NSpec=nspec, UnitX='Wavelength',
        VerticalAxisUnit='MomentumTransfer', VerticalAxisValues=qAxis)
    CreateWorkspace(OutputWorkspace=asscWS, DataX=dataX, DataY=dataY_a2ws, DataE=dataE,
        NSpec=nspec, UnitX='Wavelength',
        VerticalAxisUnit='MomentumTransfer', VerticalAxisValues=qAxis)
    CreateWorkspace(OutputWorkspace=acscWS, DataX=dataX, DataY=dataY_a3ws, DataE=dataE,
        NSpec=nspec, UnitX='Wavelength',
        VerticalAxisUnit='MomentumTransfer', VerticalAxisValues=qAxis)
    CreateWorkspace(OutputWorkspace=accWS, DataX=dataX, DataY=dataY_a4ws, DataE=dataE,
        NSpec=nspec, UnitX='Wavelength',
        VerticalAxisUnit='MomentumTransfer', VerticalAxisValues=qAxis)
    ## Save output
    group = assWS +','+ asscWS +','+ acscWS +','+ accWS
    GroupWorkspaces(InputWorkspaces=group,OutputWorkspace=fname)
    opath = os.path.join(workdir,fname+'.nxs')
    SaveNexusProcessed(InputWorkspace=fname, Filename=opath)
    if Verbose:
        logger.notice('Output file created : '+opath)
    EndTime('Absorption Correction')
    if ncan > 1:
        return [assWS, asscWS, acscWS, accWS]
    else:
        return [assWS]

def AbsRunFeeder(inputWS, geom, beam, ncan, size, density, sigs, siga, avar,
        plotOpt='None'):
    verbOp = True
    '''Handles the feeding of input and plotting of output for the F2PY
    absorption correction routine.'''
    workspaces = AbsRun(inputWS, geom, beam, ncan, size, density,
        sigs, siga, avar, Verbose=verbOp)
    if ( plotOpt == 'None' ):
        return
    if ( plotOpt == 'Wavelength' or plotOpt == 'Both' ):
        w_graph = mp.plotSpectrum(workspaces, 0)
    if ( plotOpt == 'Angle' or plotOpt == 'Both' ):
        a_graph = mp.plotTimeBin(workspaces, 0)
        a_graph.activeLayer().setAxisTitle(mp.Layer.Bottom, 'Angle')
