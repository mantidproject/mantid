# import mantid algorithms, numpy and matplotlib
from mantid.simpleapi import FitPeaks, mtd
import matplotlib.pyplot as plt
import numpy as np

if False:
    LoadNexusProcessed(Filename='/home/wzz/Projects/Mantid/mantid/scripts/vulcan/calibration/demo/VULCAN_164960_diamond_3banks.nxs', OutputWorkspace='VULCAN_164960_diamond_3banks_PDCalibrated_2ndRoundCalibrated')
    EditInstrumentGeometry(Workspace='VULCAN_164960_diamond_3banks_PDCalibrated_2ndRoundCalibrated', PrimaryFlightPath=42, SpectrumIDs='1-3', L2='2,2,2', Polar='89.9284,90.0716,150.059', Azimuthal='0,0,0', DetectorIDs='1-3', InstrumentName='vulcan_3bank')
    ConvertUnits(InputWorkspace='VULCAN_164960_diamond_3banks_PDCalibrated_2ndRoundCalibrated', OutputWorkspace='VULCAN_164960_diamond_3banks_PDCalibrated_2ndRoundCalibrated_TOF', Target='TOF')
    CloneWorkspace(InputWorkspace='VULCAN_164960_diamond_3banks_PDCalibrated_2ndRoundCalibrated_TOF', OutputWorkspace='Test3Bank')

# Fill
diamond_ws_name = 'Test3Bank'

exp_d_centers = np.array([0.60309, 0.63073, 0.68665, 0.7283, 0.81854, 0.89198, 1.07577, 1.26146])
difc = 21489.498991239565
# exp_tof_centers = exp_centers * difc

exp_tof_centers = [12960.10194663, 13554.07169874, 14755.76448233, 15650.80211532, 17590.01450429,
                   19168.20331021, 23117.75832981, 27108.14339749]

# @ d = 1.075
# TOF = tofs[-2]  # 23101.211415582533
A = 0.9302325581395349
B = 0.057703264259136466
S = 34.19706599544208


bank_index = 2
exp_fit_d_centers = exp_d_centers[:-1]
exp_fit_centers = exp_tof_centers[:-1]

fit_window_list = ''
for i_peak, TOF in enumerate(exp_fit_centers):
    left = TOF - 6 * S * exp_fit_d_centers[i_peak]
    right = TOF + 8 * S * exp_fit_d_centers[i_peak]
    print(left, TOF, right)

    if len(fit_window_list) > 0:
        fit_window_list += ', '
    fit_window_list += f'{left}, {right}'
print(fit_window_list)

rightmost_peak_param_values = f'{A}, {B}, {S * 0.3}'

peak_param_names = 'A, B, S'

peakpos_ws_name = f'{diamond_ws_name}_position_bank{bank_index}_{len(exp_fit_centers)}Peaks'
param_ws_name = f'{diamond_ws_name}_param_value_bank{bank_index}_{len(exp_fit_centers)}Peaks'
error_ws_name = f'{diamond_ws_name}_param_error_bank{bank_index}_{len(exp_fit_centers)}Peaks'
eff_value_ws_name = f'{diamond_ws_name}_eff_value_bank{bank_index}_{len(exp_fit_centers)}Peaks'
model_ws_name = f'{diamond_ws_name}_model_bank{bank_index}_{len(exp_fit_centers)}Peaks'

FitPeaks(InputWorkspace=diamond_ws_name,
         StartWorkspaceIndex=bank_index,
         StopWorkspaceIndex=bank_index,
         PeakFunction="BackToBackExponential",
         BackgroundType="Linear",
         PeakCenters=exp_fit_centers,
         FitWindowBoundaryList=fit_window_list,
         PeakParameterNames=peak_param_names,
         PeakParameterValues=rightmost_peak_param_values,
         FitFromRight=True,
         HighBackground=False,
         OutputWorkspace=peakpos_ws_name,
         OutputPeakParametersWorkspace=param_ws_name,
         OutputParameterFitErrorsWorkspace=error_ws_name,
         FittedPeaksWorkspace=model_ws_name)
FitPeaks(InputWorkspace=diamond_ws_name,
         StartWorkspaceIndex=bank_index,
         StopWorkspaceIndex=bank_index,
         PeakFunction="BackToBackExponential",
         BackgroundType="Linear",
         PeakCenters=exp_fit_centers,
         FitWindowBoundaryList=fit_window_list,
         PeakParameterNames=peak_param_names,
         PeakParameterValues=rightmost_peak_param_values,
         FitFromRight=True,
         HighBackground=False,
         OutputWorkspace=peakpos_ws_name,
         OutputPeakParametersWorkspace=eff_value_ws_name,
         RawPeakParameters=False)


# get fitted value and etc.
output1 = ''
output2 = ''
for i_peak, d_center in enumerate(exp_fit_d_centers):
    peak_pos = mtd[peakpos_ws_name].readY(0)[i_peak]
    peak_pos_err = mtd[error_ws_name].cell(i_peak, 5)
    f_a = mtd[param_ws_name].cell(i_peak, 3)
    f_b = mtd[param_ws_name].cell(i_peak, 4)
    f_s = mtd[param_ws_name].cell(i_peak, 6)
    chi2 = mtd[param_ws_name].cell(i_peak, 9)
    err_a = mtd[error_ws_name].cell(i_peak, 3)
    err_b = mtd[error_ws_name].cell(i_peak, 4)
    err_s = mtd[error_ws_name].cell(i_peak, 6)
    width = mtd[eff_value_ws_name].cell(i_peak, 3)
    height = mtd[eff_value_ws_name].cell(i_peak, 4)

    op1 = f'd = {d_center:5f}: {f_a:5f} +/- {err_a:5f},  {f_b:5f} +/- {err_b:5f},  {f_s:5f} +/- {err_s:5f},  chi^2 = {chi2:5f}'
    op2 = f'd = {d_center:5f}: X = {peak_pos:.8f} +/- {peak_pos_err:.8f}, FWHM = {width}, H = {height:5f}'

    output1 += op1 + '\n'
    output2 += op2 + '\n'

print(output1)
print()
print(output2)
