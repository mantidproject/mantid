# Peak 1.07
# All peaks   0.54411, 0.56414, 0.5947, 0.63073, 0.68665, 0.7283 , 0.81854, 0.89198, 1.07577,
# import mantid algorithms
from mantid.simpleapi import *


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

ws_index = 2


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
fitted_pos = peak_pos_ws.extractY()[0]

param_ws = mtd[param_ws_name]
error_ws = mtd[error_ws_name]

if fitted_pos <= 0:
    print(f'[Fitting   Error] Error code = {fitted_pos}')
else:
    print(f'[Fitting Success] Peak   pos = {param_ws.cell(0, 5)} +/- some error {error_ws.cell(0, 5)}')


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
