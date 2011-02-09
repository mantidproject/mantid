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

import math
import os.path

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
    det = ws.getDetector(0)
    try:
        efixed = det.getNumberParameter('Efixed')[0]
    except AttributeError:
        ids = det.getDetectorIDs()
        det = ws.getInstrument().getDetector(ids[0])
        efixed = det.getNumberParameter('Efixed')[0]
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

def CubicFit(inputWS, spec):
    '''Uses the Mantid Fit Algorithm to fit a cubic function to the inputWS
    parameter. Returns a list containing the fitted parameter values.'''
    function = 'name=UserFunction, Formula=A0+A1*x+A2*x*x, A0=1, A1=0, A2=0'
    fit = Fit(inputWS, spec, Function=function)
    return fit.getPropertyValue('Parameters')

def applyCorrections(inputWS, cannisterWS, corrections, efixed):
    '''Through the PolynomialCorrection algorithm, makes corrections to the
    input workspace based on the supplied correction values.'''
    # Corrections are applied in Lambda (Wavelength)
    ConvertUnits(inputWS, inputWS, 'Wavelength', 'Indirect',
        EFixed=efixed)
    if cannisterWS != '':
        ConvertUnits(cannisterWS, cannisterWS, 'Wavelength', 'Indirect',
            EFixed=efixed)
    nHist = mtd[inputWS].getNumberHistograms()
    # Check that number of histograms in each corrections workspace matches
    # that of the input (sample) workspace
    for ws in corrections:
        if ( mtd[ws].getNumberHistograms() != nHist ):
            raise ValueError('Mismatch: num of spectra in '+ws+' and inputWS')
    # Workspaces that hold intermediate results
    CorrectedWorkspace = '__corrected'
    CorrectedSampleWorkspace = '__csamws'
    CorrectedCanWorkspace = '__cancorrected'
    for i in range(0, nHist): # Loop through each spectra in the inputWS
        ExtractSingleSpectrum(inputWS, CorrectedSampleWorkspace, i)
        if ( len(corrections) == 1 ):
            Ass = CubicFit(corrections[0], i)
            PolynomialCorrection(CorrectedSampleWorkspace, 
                CorrectedSampleWorkspace, Ass, 'Divide')
            if ( i == 0 ):
                CloneWorkspace(CorrectedSampleWorkspace, CorrectedWorkspace)
            else:
                ConjoinWorkspaces(CorrectedWorkspace, CorrectedSampleWorkspace)
        else:
            ExtractSingleSpectrum(cannisterWS, CorrectedCanWorkspace, i)
            Acc = CubicFit(corrections[3], i)
            PolynomialCorrection(CorrectedCanWorkspace, CorrectedCanWorkspace,
                Acc, 'Divide')
            Acsc = CubicFit(corrections[2], i)
            PolynomialCorrection(CorrectedCanWorkspace, CorrectedCanWorkspace,
                Acsc, 'Multiply')
            Minus(CorrectedSampleWorkspace, CorrectedCanWorkspace,
                CorrectedSampleWorkspace)
            Assc = CubicFit(corrections[1], i)
            PolynomialCorrection(CorrectedSampleWorkspace, 
                CorrectedSampleWorkspace, Assc, 'Divide')
            if ( i == 0 ):
                CloneWorkspace(CorrectedSampleWorkspace, CorrectedWorkspace)
            else:
                ConjoinWorkspaces(CorrectedWorkspace, CorrectedSampleWorkspace)
    ConvertUnits(CorrectedWorkspace, CorrectedWorkspace+"en", 'DeltaE', 'Indirect',
        EFixed=efixed)
    if cannisterWS != '':
        ConvertUnits(CorrectedCanWorkspace, CorrectedCanWorkspace, 'DeltaE',
            'Indirect', EFixed=efixed)
                
def correctionsFeeder(sampleFile, cannisterFile, geom):
    '''Load up the necessary files and then passes them into the main
    applyCorrections routine.'''
    sample = loadNexus(sampleFile)
    ## Files named: (ins)(runNo)_(geom)_(suffix)
    ws = mtd[sample]
    ins = ws.getInstrument().getName()
    ins = ConfigService().facility().instrument(ins).shortName().lower()
    run = ws.getRun().getLogData('run_number').value
    name = ins + run + '_' + geom + '_'
    corrections = [loadNexus(name+'ass.nxs')]
    if cannisterFile != '':
        cannister = loadNexus(cannisterFile)
        corrections.append(loadNexus(name+'assc.nxs'))
        corrections.append(loadNexus(name+'acsc.nxs'))
        corrections.append(loadNexus(name+'acc.nxs'))
    else: # No can
        cannister = ''
    # Get efixed
    det = ws.getDetector(0)
    try:
        efixed = det.getNumberParameter('Efixed')[0]
    except AttributeError:
        ids = det.getDetectorIDs()
        det = ws.getInstrument().getDetector(ids[0])
        efixed = det.getNumberParameter('Efixed')[0]
    # Fire off main routine
    try:
        applyCorrections(sample, cannister, corrections, efixed)
    except ValueError:
        print """Number of histograms in corrections workspaces do not match
            the sample workspace."""
        raise

def loadNexus(filename):
    '''Loads a Nexus file into a workspace with the name based on the
    filename. Convenience function for not having to play around with paths
    in every function.'''
    name = os.path.splitext( os.path.split(filename)[1] )[0]
    LoadNexus(filename, name)
    return name