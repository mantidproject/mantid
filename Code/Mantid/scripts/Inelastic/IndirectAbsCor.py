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
from mantidplot import *
from IndirectCommon import getEfixed, createQaxis
import math, os.path

def GetWSangles(inWS):
    nhist = mtd[inWS].getNumberHistograms() # get no. of histograms/groups
    sourcePos = mtd[inWS].getInstrument().getSource().getPos()
    samplePos = mtd[inWS].getInstrument().getSample().getPos()
    beamPos = samplePos - sourcePos
    angles = []                                 # will be list of angles
    for index in range(0, nhist):
        detector = mtd[inWS].getDetector(index) # get index
        twoTheta = detector.getTwoTheta(samplePos, beamPos)*180.0/math.pi # calc angle
        angles.append(twoTheta)                 # add angle
    return angles

def WaveRange(inWS, efixed):
    oWS = '__WaveRange'
    ExtractSingleSpectrum(inWS,oWS,0)
    ConvertUnits(oWS,oWS,'Wavelength','Indirect', efixed)
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

def AbsRun(inputWS, geom, beam, ncan, size, density, sigs, siga, avar, verbose):
    workdir = mantid.getConfigProperty('defaultsave.directory')
    det = GetWSangles(inputWS)
    ndet = len(det)
    efixed = getEfixed(inputWS)
    wavelas = math.sqrt(81.787/efixed) # elastic wavelength
    waves = WaveRange(inputWS, efixed) # get wavelengths
    nw = len(waves)
    if verbose:
        mtd.sendLogMessage('Sample run : '+inputWS)
        message = '  sigt = '+str(sigs[0])+' ; siga = '+str(siga[0])+' ; rho = '+str(density[0])
        mtd.sendLogMessage(message)
        if geom == 'cyl':
            message = '  inner radius = '+str(size[0])+' ; outer radius = '+str(size[1])
            mtd.sendLogMessage(message)
        if geom == 'flt':
            mtd.sendLogMessage('  thickness = '+str(size[0]))
        if ncan == 2:
            message = 'Can : sigt = '+str(sigs[1])+' ; siga = '+str(siga[1])+' ; rho = '+str(density[1])
            mtd.sendLogMessage(message)
            if geom == 'cyl':
                mtd.sendLogMessage(message)
                message = '  inner radius = '+str(size[1])+' ; outer radius = '+str(size[2])
            if geom == 'flt':
                mtd.sendLogMessage('  thickness = '+str(size[1]))
        mtd.sendLogMessage('Elastic lambda : '+str(wavelas))
        message = 'Lambda : '+str(nw)+' values from '+str(waves[0])+' to '+str(waves[nw-1])
        mtd.sendLogMessage(message)
        message = 'Detector angles : '+str(ndet)+' from '+str(det[0])+' to '+str(det[ndet-1])
        mtd.sendLogMessage(message)
    DataX = []
    DataE = []
    for n in range(0,nw):
        DataX.append(waves[n])
        DataE.append(0)
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
            if verbose:
                mtd.sendLogMessage('Detector '+str(n)+' at angle : '+str(det[n])+' * successful')
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
    DataX = DataX * nspec
    DataE = DataE * nspec
    qAxis = createQaxis(inputWS)
    CreateWorkspace(assWS, DataX, dataY_a1ws, DataE,nspec,'Wavelength',
            VerticalAxisUnit='MomentumTransfer', VerticalAxisValues=qAxis)
    CreateWorkspace(asscWS, DataX, dataY_a2ws, DataE,nspec,'Wavelength',
            VerticalAxisUnit='MomentumTransfer', VerticalAxisValues=qAxis)
    CreateWorkspace(acscWS, DataX, dataY_a3ws, DataE,nspec,'Wavelength',
            VerticalAxisUnit='MomentumTransfer', VerticalAxisValues=qAxis)
    CreateWorkspace(accWS, DataX, dataY_a4ws, DataE,nspec,'Wavelength',
            VerticalAxisUnit='MomentumTransfer', VerticalAxisValues=qAxis)
    ## Save output
    group = assWS +','+ asscWS +','+ acscWS +','+ accWS
    GroupWorkspaces(InputWorkspaces=group,OutputWorkspace=fname)
    SaveNexusProcessed(fname,os.path.join(workdir,fname+'.nxs'))
    if verbose:
        mtd.sendLogMessage('Output files created : '+workdir+fname+'.nxs')
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
        sigs, siga, avar, verbose=verbOp)
    if ( plotOpt == 'None' ):
        return
    if ( plotOpt == 'Wavelength' or plotOpt == 'Both' ):
        graph = plotSpectrum(workspaces, 0)
    if ( plotOpt == 'Angle' or plotOpt == 'Both' ):
        graph = plotTimeBin(workspaces, 0)
        graph.activeLayer().setAxisTitle(Layer.Bottom, 'Angle')
