# Peak 1.07
# All peaks   0.54411, 0.56414, 0.5947, 0.63073, 0.68665, 0.7283 , 0.81854, 0.89198, 1.07577,
# import mantid algorithms
from mantid.simpleapi import (mtd, LoadNexusProcessed, FitPeaks)
from matplotlib import pyplot as plt
import numpy as np


# FIXME - FitPeaks is unable to 'Fit from right' by using the fitted parameter from the previously (right) fitted result
# TODO  - Fix FitPeaks
# TODO  - Temporary solution:
#         (1) Fit 3 peaks a time;
#         (2) Using the leftmost fitted peak as the starting value of the next 3 peaks to fit
nxs = '/home/wzz/Projects/Mantid/mantid/scripts/vulcan/calibration/LatestTestPD2Round/final_tests/VULCAN_164960_diamond_3banks.nxs'
LoadNexusProcessed(Filename=nxs, OutputWorkspace='VULCAN_164960_Final_3Banks')

input_ws_name = 'VULCAN_164960_Final_3Banks'

# peakparnames = 'I, A, B, X0, S'
peakparnames = 'A, B, S'

ws_index = 0, 1


exp_centers = [0.54411, 0.56414, 0.60309, 0.63073, 0.68665, 0.7283 , 0.81854, 0.89198, 1.07577]
min_x =       [0.530,   0.556,   0.592,   0.621,   0.673,   0.709,   0.806,   0.877,   1.05   ]
max_x =       [0.553,   0.578,   0.614,   0.641,   0.703,   0.747,   0.839,   0.910,   1.15   ]
fit_window_list = [x for boundary_pair in zip(min_x, max_x) for x in boundary_pair]
print(f'Fit window pair: {fit_window_list}')
out_ws_name = 'Bank3_Peaks'
param_ws_name = f'{out_ws_name}_Params'
error_ws_name = f'{out_ws_name}_FitErrors'
model_ws_name = f'{out_ws_name}_Model'

# peakparvalues = f'102176394 , 2500 , 1275 , {exp_center} , 0.000566765'
peakparvalues = f'2500 , 1275 , 0.000566765'

FitPeaks(InputWorkspace=input_ws_name,
         StartWorkspaceIndex=ws_index,
         StopWorkspaceIndex=ws_index,
         PeakFunction="BackToBackExponential",
         BackgroundType="Linear",
         PeakCenters=exp_centers,
         FitWindowBoundaryList=fit_window_list,
         PeakParameterNames=peakparnames, PeakParameterValues=peakparvalues,
         FitFromRight=True,
         HighBackground=False,
         OutputWorkspace=out_ws_name,
         OutputPeakParametersWorkspace=param_ws_name,
         OutputParameterFitErrorsWorkspace=error_ws_name,
         FittedPeaksWorkspace=model_ws_name)

# Process peak fitting result
peak_pos_ws = mtd[out_ws_name]
fitted_pos_vec = peak_pos_ws.extractY()[0]

param_ws = mtd[param_ws_name]
error_ws = mtd[error_ws_name]

vec_d = []
vec_x0 = []
vec_a = []
vec_b = []
vec_s = []

for pi in range(len(fitted_pos_vec)):
    # peak position
    fitted_pos = fitted_pos_vec[pi]
    if fitted_pos <= 0:
        print(f'[Fitting   Error] Error code = {fitted_pos}')
    else:
        print(f'[Fitting Success] Peak   pos = {param_ws.cell(pi, 5)} +/- some error {error_ws.cell(pi, 5)}')
        vec_d.append(exp_centers[pi])
        vec_a.append(param_ws.cell(pi, 3))
        vec_b.append(param_ws.cell(pi, 4))
        vec_x0.append(param_ws.cell(pi, 5))
        vec_s.append(param_ws.cell(pi, 6))

# build vectors for regression
vec_d = np.array(vec_d)  
vec_a = np.array(vec_a)
vec_b = np.array(vec_b)
vec_s = np.array(vec_s)
vec_x0 = np.array(vec_x0)

# Plot a - 1/d as a(d) = a1 * (1/d)
plt.plot(1/vec_d, vec_a, label='b2b: a')
plt.show()

# Plot b - 1/d^4 as b(d) = b0 + b1/d^4
plt.plot(1/vec_d**4, vec_b, label='b2b: b', linestyle='None', marker='o')
plt.show()

# Plot s**2 = s0**2 + s1**2 * d**2 + s2**2 * d**4
plt.plot(vec_d**2, vec_s**2, label='b2b: s^2', linestyle='None', marker='o')
plt.show()

# Plot deviation of X
relative_deviation = (vec_x0 - vec_d) / vec_d
print(f'delta(d)/d : sigma = {np.std(relative_deviation)}')
plt.plot(vec_d, relative_deviation, linestyle='None', marker='o')
plt.show()

# Difference model
vec_model_x = mtd[model_ws_name].extractX()[ws_index]
vec_model_y = mtd[model_ws_name].extractY()[ws_index]
vec_data_x = mtd[input_ws_name].extractX()[ws_index]
np.testing.assert_allclose(vec_data_x, vec_model_x)
vec_data_y = mtd[input_ws_name].extractY()[ws_index]
vec_diff_y = vec_model_y - vec_data_y
plt.plot(vec_data_x[:-1], vec_data_y, color='black', linestyle='None', marker='.')
plt.plot(vec_model_x[:-1], vec_model_y, color='red')
plt.plot(vec_data_x[:-1], vec_diff_y, color='green')
plt.show()


def fit_high_angle_bank_peaks(input_ws_name = 'VULCAN_164960_Final_3Banks'):
    # successful

    # peakparnames = 'I, A, B, X0, S'
    peakparnames = 'A, B, S'

    ws_index = 2

    exp_centers = [0.54411, 0.56414, 0.60309, 0.63073, 0.68665, 0.7283, 0.81854, 0.89198, 1.07577]
    min_x = [0.530, 0.556, 0.592, 0.621, 0.673, 0.709, 0.806, 0.877, 1.05]
    max_x = [0.553, 0.578, 0.614, 0.641, 0.703, 0.747, 0.839, 0.910, 1.15]
    fit_window_list = [x for boundary_pair in zip(min_x, max_x) for x in boundary_pair]
    print(f'Fit window pair: {fit_window_list}')
    out_ws_name = 'Bank3_Peaks'
    param_ws_name = f'{out_ws_name}_Params'
    error_ws_name = f'{out_ws_name}_FitErrors'
    model_ws_name = f'{out_ws_name}_Model'

    # peakparvalues = f'102176394 , 2500 , 1275 , {exp_center} , 0.000566765'
    peakparvalues = f'2500 , 1275 , 0.000566765'

    FitPeaks(InputWorkspace=input_ws_name,
             StartWorkspaceIndex=ws_index,
             StopWorkspaceIndex=ws_index,
             PeakFunction="BackToBackExponential",
             BackgroundType="Linear",
             PeakCenters=exp_centers,
             FitWindowBoundaryList=fit_window_list,
             PeakParameterNames=peakparnames, PeakParameterValues=peakparvalues,
             FitFromRight=True,
             HighBackground=False,
             OutputWorkspace=out_ws_name,
             OutputPeakParametersWorkspace=param_ws_name,
             OutputParameterFitErrorsWorkspace=error_ws_name,
             FittedPeaksWorkspace=model_ws_name)

    # Process peak fitting result
    peak_pos_ws = mtd[out_ws_name]
    fitted_pos_vec = peak_pos_ws.extractY()[0]

    param_ws = mtd[param_ws_name]
    error_ws = mtd[error_ws_name]

    print(fitted_pos)
    print(type(fitted_pos))

    vec_d = []
    vec_x0 = []
    vec_a = []
    vec_b = []
    vec_s = []

    for pi in range(len(fitted_pos_vec)):
        # peak position
        fitted_pos = fitted_pos_vec[pi]
        if fitted_pos <= 0:
            print(f'[Fitting   Error] Error code = {fitted_pos}')
        else:
            print(f'[Fitting Success] Peak   pos = {param_ws.cell(pi, 5)} +/- some error {error_ws.cell(pi, 5)}')
            vec_d.append(exp_centers[pi])
            vec_a.append(param_ws.cell(pi, 3))
            vec_b.append(param_ws.cell(pi, 4))
            vec_x0.append(param_ws.cell(pi, 5))
            vec_s.append(param_ws.cell(pi, 6))

    # build vectors for regression
    vec_d = np.array(vec_d)
    vec_a = np.array(vec_a)
    vec_b = np.array(vec_b)
    vec_s = np.array(vec_s)
    vec_x0 = np.array(vec_x0)

    # Plot a - 1/d as a(d) = a1 * (1/d)
    plt.plot(1 / vec_d, vec_a, label='b2b: a')
    plt.show()

    # Plot b - 1/d^4 as b(d) = b0 + b1/d^4
    plt.plot(1 / vec_d ** 4, vec_b, label='b2b: b', linestyle='None', marker='o')
    plt.show()

    # Plot s**2 = s0**2 + s1**2 * d**2 + s2**2 * d**4
    plt.plot(vec_d ** 2, vec_s ** 2, label='b2b: s^2', linestyle='None', marker='o')
    plt.show()

    # Plot deviation of X
    relative_deviation = (vec_x0 - vec_d) / vec_d
    print(f'delta(d)/d : sigma = {np.std(relative_deviation)}')
    plt.plot(vec_d, relative_deviation, linestyle='None', marker='o')
    plt.show()

    # Difference model
    vec_model_x = mtd[model_ws_name].extractX()[ws_index]
    vec_model_y = mtd[model_ws_name].extractY()[ws_index]
    vec_data_x = mtd[input_ws_name].extractX()[ws_index]
    np.testing.assert_allclose(vec_data_x, vec_model_x)
    vec_data_y = mtd[input_ws_name].extractY()[ws_index]
    vec_diff_y = vec_model_y - vec_data_y
    plt.plot(vec_data_x[:-1], vec_data_y, color='black', linestyle='None', marker='.')
    plt.plot(vec_model_x[:-1], vec_model_y, color='red')
    plt.plot(vec_data_x[:-1], vec_diff_y, color='green')
    plt.show()

def fit_single_peak_proto():

    input_ws_name = 'VULCAN_164960_Final_3Banks'

    # peakparnames = 'I, A, B, X0, S'
    peakparnames = 'A, B, X0, S'

    ws_index = 2

    exp_center = 1.07
    min_x = 1.05
    max_x = 1.15
    out_ws_name = 'Bank3_Peak_R0'
    param_ws_name = f'{out_ws_name}_Params'
    error_ws_name = f'{out_ws_name}_FitErrors'
    model_ws_name = f'{out_ws_name}_Model'

    # peakparvalues = f'102176394 , 2500 , 1275 , {exp_center} , 0.000566765'
    peakparvalues = f'2500 , 1275 , {exp_center} , 0.000566765'

    FitPeaks(InputWorkspace=input_ws_name,
             StartWorkspaceIndex=ws_index,
             StopWorkspaceIndex=ws_index,
             PeakFunction="BackToBackExponential",
             BackgroundType="Linear",
             PeakCenters=exp_center,
             FitWindowBoundaryList=[min_x, max_x],
             PeakParameterNames=peakparnames, PeakParameterValues=peakparvalues,
             FitFromRight=True,
             HighBackground=False,
             OutputWorkspace=out_ws_name,
             OutputPeakParametersWorkspace=param_ws_name,
             OutputParameterFitErrorsWorkspace=error_ws_name,
             FittedPeaksWorkspace=model_ws_name)

    # Process peak fitting result
    peak_pos_ws = mtd[out_ws_name]
    fitted_pos = peak_pos_ws.extractY()[0]

    param_ws = mtd[param_ws_name]
    error_ws = mtd[error_ws_name]

    if fitted_pos <= 0:
        print(f'[Fitting   Error] Error code = {fitted_pos}')
    else:
        print(f'[Fitting Success] Peak   pos = {param_ws.cell(0, 5)} +/- some error {error_ws.cell(0, 5)}')
