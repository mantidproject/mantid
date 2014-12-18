
from ReflectometerCors import *

def quick(run):
    '''Mantid version of quick:lam'''

    # Some beamline constants Should migrate these to the def file
    MonitorBackground=[15.0,17.0]
    MonitorsToCorrect=[0,1,2]
    I0MonitorIndex=2

    # Load a data set. single period
    LoadRaw(Filename=run,OutputWorkspace="W",SpectrumMax="4",LoadMonitors="Separate")
    ConvertUnits(InputWorkspace="W_Monitors",OutputWorkspace="M",Target="Wavelength",AlignBins="1")
    CalculateFlatBackground(InputWorkspace="M",OutputWorkspace="M",WorkspaceIndexList=MonitorsToCorrect,StartX=MonitorBackground[0],EndX=MonitorBackground[1])
    ConvertUnits(InputWorkspace="W",OutputWorkspace="D",Target="Wavelength",AlignBins="1")

    heliumDetectorEff('D')
    monitor2Eff('M')

    RebinToWorkspace(WorkspaceToRebin="M",WorkspaceToMatch="D",OutputWorkspace="M")
    CropWorkspace(InputWorkspace="M",OutputWorkspace="I0",StartWorkspaceIndex=I0MonitorIndex)
    Divide(LHSWorkspace="D",RHSWorkspace="I0",OutputWorkspace="R")


#quick("D:/data/cycle_09_4/POLREF00003015.raw")
quick("D:/data/cycle_09_4/POLREF00003014.raw")



