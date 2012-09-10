from MantidFramework import *
mtd.initialise()
from mantidsimple import *

import os
import boostmpi as mpi
import sys


cal_file  = "./mantid_5427.cal"
char_file = "./NOM_characterizations.txt"
binning = (300,-0.0004,16667)
runNumber = sys.argv[1]
sam_back = sys.argv[2]
van      = sys.argv[3]
van_back = 0
outputDir='/home/vel/scripts'
if mpi.world.rank == 0:
    print str(mpi.world.size)," mpi tasks"

SNSPowderReduction(Instrument="NOM", RunNumber=runNumber, Extension="_event.nxs",
                   PreserveEvents=False, PushDataPositive='AddMinimum',
                   CalibrationFile=cal_file, CharacterizationRunsFile=char_file,
                   BackgroundNumber=sam_back, VanadiumNumber=van, 
                   VanadiumBackgroundNumber=van_back, RemovePromptPulseWidth=50,
                   Binning=binning, BinInDspace=False, FilterBadPulses=True,
                   SaveAs="gsas and NeXus", OutputDirectory=outputDir,
                   NormalizeByCurrent=True, FinalDataUnits="MomentumTransfer")
