# Transmission main
#
from IndirectImport import *

from mantid.simpleapi import *
from mantid import config, logger, mtd
from IndirectCommon import StartTime, EndTime
import numpy, os.path

mp = import_mantidplot()

def UnwrapMon(inWS):
# Unwrap monitor - inWS contains M1,M2,S1  - outWS contains unwrapped Mon
#Unwrap s1>2 to L of S2 (M2) ie 38.76  Ouput is in wavelength
    out, join = UnwrapMonitor(InputWorkspace=inWS,LRef='37.86')
    outWS = 'out'
#Fill bad (dip) in spectrum
    RemoveBins(InputWorkspace=outWS, OutputWorkspace=outWS, Xmin=join-0.001, Xmax=join+0.001,
        Interpolation="Linear")
    FFTSmooth(InputWorkspace=outWS, OutputWorkspace=outWS, WorkspaceIndex=0)									# Smooth - FFT
    DeleteWorkspace(inWS)								# delete monWS
    return outWS

def TransMon(type,file,verbose):
    if verbose:
        logger.notice('Raw file : '+file)
    LoadRaw(Filename=file,OutputWorkspace='__m1',SpectrumMin=1,SpectrumMax=1)
    LoadRaw(Filename=file,OutputWorkspace='__m2',SpectrumMin=2,SpectrumMax=2)		
    LoadRaw(Filename=file,OutputWorkspace='__det',SpectrumMin=3,SpectrumMax=3)		
# Check for single or multiple time regimes
    MonTCBstart = mtd['__m1'].readX(0)[0]
    SpecTCBstart = mtd['__det'].readX(0)[0]	
    DeleteWorkspace('__det')								# delete monWS
    monWS = '__Mon'
    if (SpecTCBstart == MonTCBstart):
        monWS = UnwrapMon('__m1')	# unwrap the monitor spectrum and convert to wavelength
        RenameWorkspace(InputWorkspace=monWS, OutputWorkspace='__Mon1')		
    else:
        ConvertUnits(InputWorkspace='__m1', OutputWorkspace='__Mon1', Target="Wavelength")
    ConvertUnits(InputWorkspace='__m2', OutputWorkspace='__Mon2', Target="Wavelength")		
    DeleteWorkspace('__m2')								# delete monWS
    Xin = mtd['__Mon1'].readX(0)
    xmin1 = mtd['__Mon1'].readX(0)[0]
    xmax1 = mtd['__Mon1'].readX(0)[len(Xin)-1]
    Xin = mtd['__Mon2'].readX(0)
    xmin2 = mtd['__Mon2'].readX(0)[0]
    xmax2 = mtd['__Mon2'].readX(0)[len(Xin)-1]
    wmin = max(xmin1,xmin2)
    wmax = min(xmax1,xmax2)
    CropWorkspace(InputWorkspace='__Mon1', OutputWorkspace='__Mon1', XMin=wmin, XMax=wmax)
    RebinToWorkspace(WorkspaceToRebin='__Mon2', WorkspaceToMatch='__Mon1', OutputWorkspace='__Mon2')
    monWS = file[0:8] +'_'+ type
    Divide(LHSWorkspace='__Mon2', RHSWorkspace='__Mon1', OutputWorkspace=monWS)
    DeleteWorkspace('__Mon1')								# delete monWS
    DeleteWorkspace('__Mon2')								# delete monWS
	
def TransPlot(inputWS):
    tr_plot=mp.plotSpectrum(inputWS,0)

def IDATransStart(sfile,cfile,Verbose=False,Plot=True):
    StartTime('Transmission')
    TransMon('Sam',sfile,Verbose)
    TransMon('Can',cfile,Verbose)
    samWS = sfile[0:8] + '_Sam'
    canWS =cfile[0:8] + '_Can'
    trWS = sfile[0:8] + '_Tr'
    Divide(LHSWorkspace=samWS, RHSWorkspace=canWS, OutputWorkspace=trWS)
    trans = numpy.average(mtd[trWS].readY(0))
    transWS = sfile[0:8] + '_Trans'
    workdir = config['defaultsave.directory']
    group = samWS +','+ canWS +','+ trWS
    GroupWorkspaces(InputWorkspaces=group,OutputWorkspace=transWS)
    path = os.path.join(workdir,transWS+'.nxs')
    SaveNexusProcessed(InputWorkspace=transWS, Filename=path)
    if Verbose:
        logger.notice('Transmission : '+str(trans))
        logger.notice('Output file created : '+path)
	if Plot:
		TransPlot(transWS)
    EndTime('Transmission')	
