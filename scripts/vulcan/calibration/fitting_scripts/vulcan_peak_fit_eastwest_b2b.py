# import mantid algorithms, numpy and matplotlib
from mantid.simpleapi import *
import matplotlib.pyplot as plt
import numpy as np
import os

# Load data
if True:
    # diamond_nxs = '/home/wzz/Projects/Mantid/mantid/scripts/vulcan/calibration/
    # pdcalib_0003/VULCAN_164960_diamond_3banks.nxs'
    # Specify diamond processed nexus and workspace name

    # CC mask
#     diamond_dir = '/home/wzz/Projects/Mantid/mantid/scripts/vulcan/calibration/cccalibration-mask'
#     diamond_ws_name = 'VULCAN_164960_CC_Mask_3banks'
#     title = f'Diamond Cross Correlation (Mask erroneous DIFC)'

    # CC fallback
#     diamond_dir = '/home/wzz/Projects/Mantid/mantid/scripts/vulcan/calibration/cccalibration-fallback'
#     diamond_ws_name = 'VULCAN_164960_CC_Fallback_3banks'
#     title = f'Diamond Cross Correlation (Fallback erroneous DIFC)'

    # PD
    diamond_dir = '/home/wzz/Projects/Mantid/mantid/scripts/vulcan/calibration/pdcalib_0003'
    diamond_ws_name = 'VULCAN_164960_PDCalibrationG_3banks'
    title = f'Diamond PDCalibration (Gaussian)'

    diamond_nxs = os.path.join(diamond_dir, 'VULCAN_164960_diamond_3banks.nxs')
    LoadNexusProcessed(Filename=diamond_nxs, OutputWorkspace=diamond_ws_name)
    diamond_ws = mtd[diamond_ws_name]

if False:
    LoadNexusProcessed(Filename='/home/wzz/Projects/Mantid/mantid/scripts/vulcan/calibration/VULCAN_164960_matrix.nxs',
                       OutputWorkspace='VULCAN_164960_raw_3banks')
    CreateGroupingWorkspace(InstrumentName='vulcan', GroupDetectorsBy='Group', OutputWorkspace='vulcan_3banks_group')
    DiffractionFocussing(InputWorkspace='VULCAN_164960_raw_3banks', OutputWorkspace='VULCAN_164960_raw_3banks',
                         GroupingWorkspace='vulcan_3banks_group', PreserveEvents=False)
    diamond_ws_name = 'VULCAN_164960_raw_3banks'
    diamond_ws = mtd[diamond_ws_name]
    title = f'Diamond Raw'

# Choose Bank
ws_index = 1


class Peak:
    def __init__(self):
        self.tag = None


def cal_fwhm(param_ws):
    param_ws = mtd[str(param_ws)]

    a = param_ws.cell(1, 1)
    b = param_ws.cell(2, 1)
    s = param_ws.cell(4, 1)
    # x0 = param_ws.cell(3, 1)

    # M_LN2 = 0.693147180559945309417
    # print(M_LN2)
    M_LN2 = np.log(2)

    w0 = M_LN2 * (a + b) / (a * b)
    fwhm = w0 * np.exp(-0.5 * M_LN2 * s / w0) + 2 * np.sqrt(2 * M_LN2) * s

    print(f'FWHM = {fwhm}')

    return fwhm


# Main .......

peak_info_list = list()

# 0.31173, 0.32571, 0.34987, 0.42049, 0.46451, 0.47679, 0.49961, 0.51499, 0.54411,
# 0.56414, 0.63073, .68665, .72830,

# Peak List: 0.63073, .68665, .72830, .81854, .89198, 1.07577, 1.26146,

# Peak 0
peak = Peak()
peak.tag = 'peak00'
peak.obs_pos = 1.26146
peak.start_x = 1.20
peak.end_x = 1.35
peak.intensity = 7.7859e+7
peak.a0_obs = 1.49818e+08
peak_info_list.append(peak)

# Peak 1
peak = Peak()
peak.tag = 'peak01'
peak.obs_pos = 1.07577
peak.start_x = 1.050
peak.end_x = 1.100
peak.intensity = 7.7859e+7
peak.a0_obs = 1.49818e+08
peak_info_list.append(peak)

# Peak 2
peak = Peak()
peak.tag = 'peak02'
peak.obs_pos = 0.89198
peak.start_x = 0.87
peak.end_x = 0.93
peak.intensity = 7.7859e+7
peak.a0_obs = 1.49818e+08
peak_info_list.append(peak)


# Peak 3
peak = Peak()
peak.tag = 'peak03'
peak.obs_pos = 0.81854
peak.start_x = 0.79306519523147423
peak.end_x = 0.84385086920977437
peak.intensity = 7.7859e+7
peak.a0_obs = 1.49818e+08
peak_info_list.append(peak)

# Peak 4
peak = Peak()
peak.tag = 'peak04'
peak.obs_pos = 0.72830
peak.start_x = 0.705
peak.end_x = 0.750
peak.intensity = 7.7859e+7
peak.a0_obs = 1.49818e+08
peak_info_list.append(peak)

# Peak 5
peak = Peak()
peak.tag = 'peak05'
peak.obs_pos = 0.68665
peak.start_x = 0.675
peak.end_x = 0.700
peak.intensity = 7.7859e+7
peak.a0_obs = 1.49818e+08
peak_info_list.append(peak)

# Peak 6
peak = Peak()
peak.tag = 'peak06'
peak.obs_pos = 0.63073
peak.start_x = 0.620
peak.end_x = 0.645
peak.intensity = 7.7859e+7
peak.a0_obs = 1.49818e+08
peak_info_list.append(peak)

# # Peak 7
# peak = Peak()
# peak.tag = 'peak07'
# peak.obs_pos = 0.56414
# peak.start_x = 0.555
# peak.end_x = 0.575
# peak.intensity = 7.7859e+07
# peak.a0_obs = 1.49818e+09
# peak_info_list.append(peak)
#
# # Peak 8
# peak = Peak()
# peak.tag = 'peak08'
# peak.obs_pos = 0.54411
# peak.start_x = 0.530
# peak.end_x = 0.555
# peak.intensity = 7.7859e+07
# peak.a0_obs = 1.49818e+09
# peak_info_list.append(peak)


param_dict = dict()

# Overiew plot
cal_vec_x_list = []
cal_vec_y_list = []
cal_vec_diff_list = []

for pi in peak_info_list:

    # West: fit_function = f'name=BackToBackExponential,I={pi.intensity},A=1300.58,B=882.86,X0={pi.obs_pos},
    # S=0.0023;name=LinearBackground,A0={pi.a0_obs},A1=0.'

    fit_function = f'name=BackToBackExponential,I={pi.intensity},A=1700.58,B=882.86,X0={pi.obs_pos},' \
                   f'S=0.0019;name=LinearBackground,A0={pi.a0_obs},A1=0.'

    # fit_function = f'name=Gaussian, Height={pi.intensity}, PeakCentre={pi.obs_pos},
    # Sigma=0.002324;name=LinearBackground,A0={pi.a0_obs},A1=0.'

    out = Fit(Function=fit_function,
              InputWorkspace=diamond_ws,
              Output=f'VULCAN_164960_pd0003_3banks_{pi.tag}',
              OutputCompositeMembers=True,
              WorkspaceIndex=ws_index,
              StartX=pi.start_x,
              EndX=pi.end_x,
              Normalise=True)

    # summarize the fitting result
    fit_chi2 = out.OutputChi2overDoF
    fit_status = out.OutputStatus
    print(f'Fit {pi.obs_pos}: {fit_status}, Chi2 = {fit_chi2}')

    if fit_status.lower().count('success') == 0 and fit_status.lower().count('too small') == 0:
        continue

    cov_matrix_table_ws = out.OutputNormalisedCovarianceMatrix
    fit_param_table_ws = out.OutputParameters
    fitted_data_ws = out.OutputWorkspace
    fitted_data_ws = ConvertToPointData(InputWorkspace=fitted_data_ws, OutputWorkspace=str(fitted_data_ws))

    # collect fitted peak values
    if True:
        # back to back
        fwhm = cal_fwhm(fit_param_table_ws)
        x0 = fit_param_table_ws.cell(3, 1)
    else:
        # gaussian
        fwhm = fit_param_table_ws.cell(2, 1)
        x0 = fit_param_table_ws.cell(1, 1)

    # maximum value and peak range
    max_y = np.max(fitted_data_ws.extractY()[1])

    # output
    param_dict[pi.obs_pos] = x0, max_y, fwhm

    # overview plot
    cal_vec_x_list.append(fitted_data_ws.readX(1))
    cal_vec_y_list.append(fitted_data_ws.readY(1))
    cal_vec_diff_list.append(fitted_data_ws.readY(2))

    print(f'{pi.obs_pos}: max model Y: {np.max(fitted_data_ws.readY(1))}')

    # plot result
    plt.cla()
    for plot_index, color_label in enumerate([('black', 'Data'), ('red', 'Calc'), ('green', 'Diff')]):
        color, label = color_label
        plt.plot(fitted_data_ws.extractX()[plot_index], fitted_data_ws.extractY()[plot_index],
                 color=color, label=label)
    plt.legend()
    plt.title(f'{pi.tag} Peak Pos = {pi.obs_pos}')
    plt.savefig(f'bank{plot_index}_{pi.tag}.png')

# output fitted parameters
print('\n\n')
for exp_pos in sorted(param_dict.keys()):
    obs_pos, max_y, obs_fwhm = param_dict[exp_pos]
    diff = obs_pos - exp_pos
    print(f'{exp_pos}  {obs_pos}   {diff}   {max_y}  {obs_fwhm}')

# Plot overview
diamond_pt_ws = ConvertToPointData(InputWorkspace=diamond_ws, OutputWorkspace='diamond_point_data')

plt.cla()
plt.plot(diamond_pt_ws.readX(ws_index), diamond_pt_ws.readY(ws_index), color='black', linestyle='None',
         marker='.', label=diamond_ws_name)
plt.plot(np.concatenate(cal_vec_x_list), np.concatenate(cal_vec_y_list), color='red', label='Fitted')
plt.plot(np.concatenate(cal_vec_x_list), np.concatenate(cal_vec_diff_list), color='green', label='Diff')
plt.title(title)
plt.xlabel('dSpacing')
plt.ylabel('Intensity')
plt.legend()
plt.savefig(f'overview_fit_bank{ws_index+1}.png')
plt.show()

print(np.max(np.concatenate(cal_vec_y_list)))
print(np.argmax(np.concatenate(cal_vec_y_list)))
print(np.concatenate(cal_vec_x_list)[287])
