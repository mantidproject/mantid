# NIST SANS reduction

from MantidFramework import *
mtd.initialise(False)
from reduction.instruments.sans.ngsans_command_interface import *

NGSANS()
SolidAngle()
NoSensitivityCorrection()
#NoNormalization()
TimeNormalization()
#Mask(nx_low=3, nx_high=3, ny_low=3, ny_high=3)
AzimuthalAverage(n_bins=100, n_subpix=1, log_binning=False)
#DirectBeamCenter("Aug10001.SA3_ICE_I231")
DirectBeamCenter("Aug10301.SA3_ICE_I531")
DataPath("/home/mantid/workspace/mantid/Test/Data/SANS2D/")
AppendDataFile("Aug10301.SA3_ICE_I531")
SetTransmission(0.837725102901, .01)
ThetaDependentTransmission(True)
SaveIqAscii()
Reduce1D()