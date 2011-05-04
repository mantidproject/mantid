#Make the reduction module available
from ISISCommandInterface import *

# Set a default data path where we look for raw data
DataPath("/home/m2d/workspace/mantid/Test/Data/SANS2D")
# Set a user path where we look for the mask file
UserPath("/home/m2d/workspace/mantid/Test/Data/SANS2D")
# Set instrument. SANS2D() or LOQ()
SANS2D()
#Set reduction to 1D (note that if this is left out, 1D is the default)
Set1D()

# Set the detector
# Options:
# SANS: rear-detector, front-detector
# LOQ: main-detector-bank, HAB
Detector("rear-detector")

# Read a mask file
MaskFile('MASKSANS2D.091A')

# Change default limits if we want to
#LimitsR(50.,170.)
#   Here the arguments are min, max, step, step type
#LimitsWav(4.,8.,0.125, 'LIN')
#LimitsQXY(0, 0.1, 0.002, 'LIN')

# Q limits (binning) can be specified in 2 ways
# A simple min,delta,max and binning type
#LimitsQ(0.02, 1.0, 0.02, 'LOG')

# Or a full string that is passed directly to rebin as is 
#LimitsQ("0.02, -0.02,1.0")

# Consider Gravity true/false (default off)
Gravity(True)

# Alter the trans fit type and range. First parameter can be 'OFF','LOG',LIN'
TransFit('LOG',3.0,8.0)

# Assign run numbers (.nxs for nexus)
AssignSample('992.raw')
TransmissionSample('988.raw', '987.raw')
AssignCan('993.raw')
TransmissionCan('989.raw', '987.raw')
#TransWorkspace('988_trans_sample_2.0_14.0', '989_trans_can_2.0_14.0')

# Update the centre coordinates
# Arguments are rmin,rmax, niterations
#FindBeamCentre(50., 170., 2)

# Do the reduction
# WavRangeReduction runs the reduction for the specfied wavelength range

# The final argument can either be DefaultTrans or CalcTrans where
#  (a) DefaultTrans calculates the transmission for the whole range specified by L/WAV 
#      and then crops it to the current range and
#  (b) CalcTrans calculates the transmission for the specifed range
#
# The returned value is the name of the fully reduced workspace
#reduced = WavRangeReduction(2.0, 14.0, NewTrans)
#plotSpectrum(reduced,0)

#  ... OR ...

# Looping over other ranges
wav1 = 2.0
wav2 = wav1 + 2.0
reduced = WavRangeReduction(wav1, wav2, DefaultTrans)
#ws_list = [reduced]
#plt = plotSpectrum(reduced, 0)
#for i in range(2,7):
#    wav1 = i*2.0
#    wav2 = wav1 + 2.0
#    reduced = WavRangeReduction(wav1, wav2, DefaultTrans)
#    ws_list.append(reduced)
#plotSpectrum(ws_list,0)
