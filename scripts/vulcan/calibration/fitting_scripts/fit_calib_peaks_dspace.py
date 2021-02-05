# import mantid algorithms, numpy and matplotlib
from mantid.simpleapi import *
import matplotlib.pyplot as plt
import numpy as np

# Load data
if False:
    LoadEventNexus(Filename='/SNS/VULCAN/IPTS-21356/nexus/VULCAN_164960.nxs.h5', OutputWorkspace='VULCAN_164960_events')
    ConvertUnits(InputWorkspace='VULCAN_164960_events', OutputWorkspace='VULCAN_164960_events', Target='dSpacing')
    Rebin(InputWorkspace='VULCAN_164960_events', OutputWorkspace='VULCAN_164960_events', Params='-0.0003', FullBinsOnly='1')
    diamond_ws = ConvertToMatrixWorkspace(InputWorkspace='VULCAN_164960_events', OutputWorkspace='VULCAN_164960_matrix')
    # SaveNexusProcessed(InputWorkspace='VULCAN_164960_matrix', Filename='/SNS/users/wzz/VULCAN_164960_matrix.nxs', Title='Diamond run in dSpacing')
else:
    diamond_ws = LoadNexusProcessed(Filename='/home/wzz/Projects/Mantid/mantid/scripts/vulcan/calibration/VULCAN_164960_matrix.nxs', OutputWorkspace='VULCAN_164960_diamond')
    # diamond_ws = ConvertUnits(InputWorkspace='VULCAN_164960_diamond', OutputWorkspace='VULCAN_164960_diamond', Target='TOF')

input_ws_name = 'VULCAN_164960_diamond'

# Calculate DIFC
# difc_table = CalculateDIFC(InputWorkspace='VULCAN_164960_diamond', OutputWorkspace='vulcan_difc')


# Fit peaks
peakparnames = 'I, A, B, X0, S'
peakparvalues = '30000 , 1200., 800. , 1.26, 0.002'
# peakparvalues = '30000 , 0.11 , 6550000 , 20663, 45.33'

# West	 539	TOF	20663.8	0.1133	6.55+06	45.33

peak_d_list = [0.54411, 0.56414, 0.63073, 0.68665, 0.72830, 0.81854, 0.89198, 1.07577, 1.26146]
peak_d_boundaries = [0.535, 0.555,
                     0.555, 0.576,
                     0.620, 0.650,
                     0.670, 0.706,
                     0.711, 0.754,
                     0.790, 0.840, 0.870, 0.919, 1.030, 1.14, 1.20, 1.33]


print((1.26174 - 1.26146) / 1.26146 * 100)