# Spencer's F2PY Absorption Corrections Wrapper
# and the application of these corrections
###############################################################################
## Handle selection of .pyd files for absorption corrections                 ##
import platform                                                              ##
opeteratingenvironment = platform.system()+platform.architecture()[0]        ##
if ( opeteratingenvironment == 'Windows32bit' ):                             ##
    import fltabs_win32 as fltabs                                            ##
    import cylabs_win32 as cylabs                                            ##
else:                                                                        ##
    sys.exit('F2PY absorption corrections only available on Win32')          ##
###############################################################################
###############################################################################

## Other imports
from mantidsimple import *
from mantidplot import *
from IndirectDataAnalysis import getEfixed
import math

def GetWSangles(inWS):
    tmp = mtd[inWS]
    nhist = tmp.getNumberHistograms() # get no. of histograms/groups
    sourcePos = tmp.getInstrument().getSource().getPos()
    samplePos = tmp.getInstrument().getSample().getPos()
    beamPos = samplePos - sourcePos
    group_angles = [] # will be list of angles
    for index in range(0, nhist):
        detector = tmp.getDetector(index) # get index
        twoTheta = detector.getTwoTheta(samplePos, beamPos)*180.0/math.pi # calc angle
        group_angles.append(twoTheta) # add angle
    return group_angles

def WaveRange(inWS,delw):
    oWS = '__spencer_WaveRange'
    ExtractSingleSpectrum(inWS,oWS,0)
    ConvertUnits(oWS,oWS,"Wavelength","Indirect")
    tempWS = mtd[oWS]
    Xin = tempWS.readX(0)
    npt = len(Xin)-1			
    xmin = tempWS.readX(0)[0]
    xmax = tempWS.readX(0)[npt]
    if delw == 0.0:
        ebin = 0.5
    else:
        ebin = delw
    nw1 = int(xmin/ebin)
    nw2 = int(xmax/ebin)+1
    w1 = nw1*ebin
    w2 = nw2*ebin
    wave = []
    if delw == 0.0:
        nw = 10
        ebin = (w2-w1)/(nw-1)
    else:
        nw = nw2-nw1+1
    for l in range(0,nw):
        wave.append(w1+l*ebin)
    DeleteWorkspace(oWS)
    return wave

## workdir = defaultsave.directory
## prefix = instrument short name in lower case
## sam = run number
def AbsRun(workdir, prefix, sam, geom, beam, ncan, size, density, sigs, siga,
        avar, efixed, verbose=False, inputWS='Sam'):
    det = GetWSangles(inputWS)
    ndet = len(det)
    wavelas = math.sqrt(81.787/efixed) # elastic wavelength
    waves = WaveRange(inputWS,0.0) # get wavelengths
    nw = len(waves)
    DataX = []
    DataE = []
    for n in range(0,nw):
        DataX.append(waves[n])
        DataE.append(0)
    name = prefix + sam + '_' + geom
    assWS = name + '_ass'
    asscWS = name + '_assc'
    acscWS = name + '_acsc'
    accWS = name + '_acc'
    wrk = workdir + prefix + sam
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
            print 'Detector '+str(n)+' at angle : '+str(det[n])+' * successful'
            dataY_a1ws += list(A1)
            if ncan > 1:
                dataY_a2ws += list(A2)
                dataY_a3ws += list(A3)
                dataY_a4ws += list(A4)              
            plot_list.append(n)
    ## Create the workspaces
    nspec = len(plot_list)
    DataX = DataX * nspec
    DataE = DataE * nspec
    CreateWorkspace(assWS, DataX, dataY_a1ws, DataE,nspec,'Wavelength')
    if ( ncan > 1 ):
        CreateWorkspace(asscWS, DataX, dataY_a2ws, DataE,nspec,'Wavelength')
        CreateWorkspace(acscWS, DataX, dataY_a3ws, DataE,nspec,'Wavelength')
        CreateWorkspace(accWS, DataX, dataY_a4ws, DataE,nspec,'Wavelength')
    ## Tidy up & Save output
    SaveNexusProcessed(assWS, assWS+'.nxs')
    if ncan > 1:
        SaveNexusProcessed(asscWS, asscWS+'.nxs')
        SaveNexusProcessed(acscWS, acscWS+'.nxs')
        SaveNexusProcessed(accWS, accWS+'.nxs')
    if ncan > 1:
        return [assWS, asscWS, acscWS, accWS]
    else:
        return [assWS]

def AbsRunFeeder(inputws, geom, beam, ncan, size, density, sigs, siga, avar,
        plotOpt='None'):
    '''Handles the feeding of input and plotting of output for the F2PY
    absorption correction routine.'''
    ws = mtd[inputws]
    ## workdir is default save directory
    workdir = mantid.getConfigProperty('defaultsave.directory')
    ## prefix is instrument short name
    prefix = ws.getInstrument().getName()
    prefix = ConfigService().facility().instrument(prefix).shortName().lower()
    ## sam = run number
    sam = ws.getRun().getLogData("run_number").value
    ## efixed can be taken from the instrument parameter file
    efixed = getEfixed(inputws)
    ## Run the routine
    workspaces = AbsRun(workdir, prefix, sam, geom, beam, ncan, size, density,
        sigs, siga, avar, efixed, inputWS=inputws)
    if ( plotOpt == 'None' ):
        return
    if ( plotOpt == 'Wavelength' or plotOpt == 'Both' ):
        graph = plotSpectrum(workspaces, 0)
    if ( plotOpt == 'Angle' or plotOpt == 'Both' ):
        graph = plotTimeBin(workspaces, 0)
        graph.activeLayer().setAxisTitle(Layer.Bottom, 'Angle')
