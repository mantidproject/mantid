# import mantid algorithms, numpy and matplotlib
from mantid.simpleapi import *
import matplotlib.pyplot as plt
import numpy as np
import os
import h5py

dir = '/SNS/users/wzz/Mantid_Project/mantid/scripts/vulcan/calibration'
dir2 = '/SNS/users/wzz/Mantid_Project/mantid/scripts/vulcan/calibration/full_diamond_calibration'

focused_nxs = 'VULCAN_164960_diamond_3banks.nxs'
aligned_nxs = 'VULCAN_164960_diamond_aligned.nxs'

focused_c_ws = LoadNexusProcessed(Filename=os.path.join(dir2, focused_nxs), OutputWorkspace='diamond_focused')
aligned_c_ws = LoadNexusProcessed(Filename=os.path.join(dir2, aligned_nxs), OutputWorkspace='diamond_aligned')

calibration_ws = mtd['calibration_cal']
print(calibration_ws.getColumnNames())

print(type(calibration_ws))

for row in range(10):
    print(calibration_ws.cell(row, 1))

# check offset workspace
high_angle_offset = LoadNexusProcessed(Filename='offset_VULCAN_164960_diamond_high_angle_1fit.nxs', OutputWorkspace='Bank3_Offset')

# calibration workspace (loaded)
calib_ws = mtd['diamond_cal_cal']
print(calib_ws.getColumnNames())

difc_h5 = h5py.File('VULCAN_164960_diamond_DIFC.h5', 'r')
bank1_data = difc_h5['west']['DIFC_cal_raw_diff']
print(type(bank1_data))

for i in range(0, 3000, 100):
    print(bank1_data[i][0], bank1_data[i][1], calib_ws.cell(i, 1))