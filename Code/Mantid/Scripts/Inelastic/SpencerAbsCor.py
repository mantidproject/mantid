# Spencer's F2PY Absorption Corrections Wrapper

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
import os.path as op

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

def Array2List(ncan,n,A1,A2,A3,A4):
    Y1 = []
    Y2 = []
    Y3 = []
    Y4 = []
    for m in range(0,n):
        Y1.append(A1[m])
        if ncan > 1:
            Y2.append(A2[m])
            Y3.append(A3[m])
            Y4.append(A4[m])
    return Y1,Y2,Y3,Y4

def SaveWork(inWS,workdir,prefix,run,prog,verbose=True):
    file = prefix + run + '_' + prog + '.nxs'
    path = op.join(workdir, file)
    SaveNexusProcessed(inWS,path)
    if verbose:
        print 'Ouput file created : '+path

def PlotAngle(det,workspace,bin):
    nBin = mtd[workspace].getNumberHistograms()
    dataX = det
    dataY = []
    dataE = []
    for i in range(0, nBin):
        dataY.append(mtd[workspace].readY(i)[bin])
        dataE.append(mtd[workspace].readE(i)[bin])
    CreateWorkspace('Angle', dataX, dataY, dataE, NSpec=1)
    graph = plotSpectrum('Angle', 0)
    l = graph.activeLayer()
    l.setAxisTitle(Layer.Bottom, "Angle")

## workdir = defaultsave.directory
## prefix = instrument short name in lower case
## sam = run number
def AbsRun(workdir, prefix, sam, geom, beam, ncan, size, density, sigs, siga,
        avar, efixed, verbose=False, samWS='Sam'):
    det = GetWSangles(samWS)
    ndet = len(det)
    wavelas = math.sqrt(81.787/efixed) # elastic wavelength
    waves = WaveRange(samWS,0.0) # get wavelengths
    nw = len(waves)
    DataX = []
    DataE = []
    for n in range(0,nw):
        DataX.append(waves[n])
        DataE.append(0)
    assWS = '__Ass'
    asscWS = '__Assc'
    acscWS = '__Acsc'
    accWS = '__Acc'
    wrk = workdir + prefix + sam
    lwrk = len(wrk)
    wrk.ljust(120,' ')
    lwrk = 0 # value 0 means fltabs/cylabs will not save files
    plot_list = []
    dataY_a1ws = []
    dataY_a2ws = []
    dataY_a3ws = []
    dataY_a4ws = []
    for n in range(0,ndet):
        if geom == 'flt':
            angles = [avar, det[n]]
            kill, A1, A2, A3, A4 = fltabs.fltabs(ncan, size, density, sigs,
                siga, angles, waves, n, wrk, lwrk)
        if geom == 'cyl':
            astep = avar
            angle = det[n]
            kill, A1, A2, A3, A4 = cylabs.cylabs(astep, beam, ncan, size,
                density, sigs, siga, angle, wavelas, waves, n, wrk, lwrk)
        if kill == 0:
            print 'Detector '+str(n)+' at angle : '+str(det[n])+' * successful'
            DataY1,DataY2,DataY3,DataY4 = Array2List(ncan,nw,A1,A2,A3,A4)
            dataY_a1ws += DataY1
            if ncan > 1:
                dataY_a2ws += DataY2
                dataY_a3ws += DataY3
                dataY_a4ws += DataY4
            plot_list.append(n)
        else:
            print 'Detector '+str(n)+' at angle : '+str(det[n])+' *** failed : Error code '+str(kill)
    ## Create the workspaces
    nspec = len(plot_list)
    DataX = DataX * nspec
    DataE = DataE * nspec
    CreateWorkspace(assWS, DataX, dataY_a1ws, DataE,nspec,'Wavelength')
    if ( ncan > 1 ):
        CreateWorkspace(asscWS, DataX, dataY_a2ws, DataE,nspec,'Wavelength')
        CreateWorkspace(acscWS, DataX, dataY_a3ws, DataE,nspec,'Wavelength')
        CreateWorkspace(accWS, DataX, dataY_a4ws, DataE,nspec,'Wavelength')
    ## Plotting
    ass_graph=plotSpectrum(assWS,plot_list)
    if ncan > 1:
        all_list = [assWS,asscWS,acscWS,accWS]
        all_graph = plotSpectrum(all_list,0)
    if ndet > 1:
        PlotAngle(det,assWS,0)
    ## Tidy up & Save output
    SaveWork(assWS,workdir,prefix,sam,geom+'_ass',verbose)
    if ncan > 1:
        SaveWork(asscWS,workdir,prefix,sam,geom+'_assc',verbose)
        SaveWork(acscWS,workdir,prefix,sam,geom+'_acsc',verbose)
        SaveWork(accWS,workdir,prefix,sam,geom+'_acc',verbose)

def AbsRunFeeder(inputws, geom, beam, ncan, size, density, sigs, siga, avar):
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
    AbsRun(workdir, prefix, sam, geom, beam, ncan, size, density, sigs, siga, 
        avar, efixed, samWS=inputws)