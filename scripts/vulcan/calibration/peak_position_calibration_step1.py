# Peak positions calibration (round 2 calibration): Step 1
# Task: For each bank
# 1. Fit diamond peaks
# 2. Report deviations from expected positions
# 3. Fit peak positions with linear (or in future quadratic) function
# 4. Predict the 2nd-round calibration result
import os
import numpy as np
from mantid.simpleapi import LoadNexusProcessed, mtd, FitPeaks, LoadDiffCal, SaveDiffCal
from matplotlib import pyplot as plt
import time


def main(focused_diamond_nxs, figure_title, ws_tag, src_diff_cal_h5, target_diff_cal_h5):

    # Load focused diamond diffraction data from process NeXus file
    base_name = os.path.basename(focused_diamond_nxs).split('.')[0]
    diamond_ws_name = f'{base_name}_{ws_tag}'
    print(focused_diamond_nxs, diamond_ws_name)
    LoadNexusProcessed(Filename=focused_diamond_nxs, OutputWorkspace=diamond_ws_name)
    diamond_ws = mtd[diamond_ws_name]
    assert diamond_ws

    # Fit west bank
    west_res = fit_diamond_peaks(diamond_ws_name, 0)
    # Fit east bank
    east_res = fit_diamond_peaks(diamond_ws_name, 1)
    # Fit high angle bank
    high_angel_res = fit_diamond_peaks(diamond_ws_name, 2)

    # apply 2nd round calibration to diffraction calibration file
    if src_diff_cal_h5:
        # Load calibration file
        calib_outputs = LoadDiffCal(Filename=src_diff_cal_h5,
                                    InputWorkspace=diamond_ws_name,
                                    #nstrumentName='vulcan',
                                    WorkspaceName='DiffCal_Vulcan')
        diff_cal_table_name = str(calib_outputs.OutputCalWorkspace)

        # Update calibration table and save
        apply_peaks_positions_calibration(diff_cal_table_name, [west_res, east_res, high_angel_res])

        # Save to new diffraction calibration file
        SaveDiffCal(CalibrationWorkspace=diff_cal_table_name,
                    GroupingWorkspace=calib_outputs.OutputGroupingWorkspace,
                    MaskWorkspace=calib_outputs.OutputMaskWorkspace,
                    Filename=target_diff_cal_h5)


def apply_peaks_positions_calibration(diff_cal_table_name, residual_list):
    west_bank_residual, east_bank_residual, high_angle_bank_residual = residual_list

    # 2nd-round calibration
    for residual, start_index, end_index in [(west_bank_residual, 0, 81920),
                                             (east_bank_residual, 81920, 163840),
                                             (high_angle_bank_residual, 163840, 200704)]:

        # update calibration table
        update_calibration_table(mtd[diff_cal_table_name], residual, start_index, end_index)


def update_calibration_table(cal_table, residual, start_index, end_index):

    # theory
    # TOF = DIFC^(1) * d': first round calibration
    # d = a + b * d': 2nd round calibration
    # TOF = DIFC * (d / b - a / b)
    #     = -DIFC * a / b + DIFC / b * d

    # DIFC^(2)(i) = DIFC / b
    # T0^(2)(i) = - DIFC * a  / b

    # west bank
    # slope:
    b = residual.slope   # 1.0005157396059512
    # interception:
    a = residual.intercept  # -5.642400138239356e-05

    print(f'Calibrate with from d = {b} * d\' + {a}')

    for i_r in range(start_index, end_index):
        # original difc
        difc = cal_table.cell(i_r, 1)
        tzero = cal_table.cell(i_r, 3)
        if abs(tzero) > 1E-6:
            raise RuntimeError(f'Calibration table row {i_r}, Found non-zero TZERO {tzero}')
        # apply 2nd round correction
        new_difc = difc / b
        tzero = - difc * a / b
        # set
        cal_table.setCell(i_r, 1, new_difc)
        cal_table.setCell(i_r, 3, tzero)


def generate_90_degree_bank_parameters():
    # diamond peak positions
    exp_centers = [0.60309, 0.63073, 0.68665, 0.7283, 0.81854, 0.89198, 1.07577, 1.26146]
    # manually determined fit windows
    fit_window_list = [0.58800, 0.61600, 0.61600, 0.64500, 0.67500, 0.7000, 0.7050, 0.750, 0.79300, 0.84300,
                       0.87000, 0.93000, 1.05000, 1.100, 1.20, 1.35]
    # bank specific staring back-to-back exponential peak profile value
    rightmost_peak_param_values = f'1200 , 800 , 0.000366765'

    return exp_centers, fit_window_list, rightmost_peak_param_values


def generate_high_angle_bank_parameters():
    # diamond peak positions
    exp_centers = [0.54411, 0.56414, 0.60309, 0.63073, 0.68665, 0.7283, 0.81854, 0.89198, 1.07577]
    # manually determined fit windows
    min_x = [0.530, 0.556, 0.592, 0.621, 0.673, 0.709, 0.806, 0.877, 1.05]
    max_x = [0.553, 0.578, 0.614, 0.641, 0.703, 0.747, 0.839, 0.910, 1.15]
    fit_window_list = [x for boundary_pair in zip(min_x, max_x) for x in boundary_pair]
    # bank specific staring back-to-back exponential peak profile value
    rightmost_peak_param_values = f'2500 , 1275 , 0.000566765'

    return exp_centers, fit_window_list, rightmost_peak_param_values


def fit_diamond_peaks(diamond_ws_name, bank_index):

    # Generate peak fitting parameters for back-to-back exponential
    if bank_index in [0, 1]:
        exp_centers, fit_window_list, rightmost_peak_param_values = generate_90_degree_bank_parameters()
    elif bank_index in [2]:
        exp_centers, fit_window_list, rightmost_peak_param_values = generate_high_angle_bank_parameters()
    else:
        raise RuntimeError(f'Bank with index {bank_index} is not supported')

    # Prepare output workspaces
    # Set up workspace names
    out_ws_name = f'Bank{bank_index}_Peaks'
    param_ws_name = f'{out_ws_name}_Params'
    error_ws_name = f'{out_ws_name}_FitErrors'
    model_ws_name = f'{out_ws_name}_Model'

    # Fit peaks
    peak_param_names = 'A, B, S'

    FitPeaks(InputWorkspace=diamond_ws_name,
             StartWorkspaceIndex=bank_index,
             StopWorkspaceIndex=bank_index,
             PeakFunction="BackToBackExponential",
             BackgroundType="Linear",
             PeakCenters=exp_centers,
             FitWindowBoundaryList=fit_window_list,
             PeakParameterNames=peak_param_names,
             PeakParameterValues=rightmost_peak_param_values,
             FitFromRight=True,
             HighBackground=False,
             OutputWorkspace=out_ws_name,
             OutputPeakParametersWorkspace=param_ws_name,
             OutputParameterFitErrorsWorkspace=error_ws_name,
             FittedPeaksWorkspace=model_ws_name)

    # process fitting information
    param_value_dict, param_error_dict = process_fit_result(out_ws_name, param_ws_name, error_ws_name)

    # report
    if True:
        report_calibrated_diamond_data(param_value_dict, param_error_dict, diamond_ws_name, model_ws_name, bank_index)

    # 2nd round calibration
    calib_model, calib_res = calibrate_peak_positions(exp_pos_vec=param_value_dict['ExpectedX0'],
                                                      calibrated_pos_vec=param_value_dict['X0'],
                                                      poly_order=1)

    plot_predicted_calibrated_peak_positions(param_value_dict['ExpectedX0'],
                                             calib_model, param_value_dict['X0'],
                                             calib_res, bank_index)

    return calib_res


class Res(object):
    def __init__(self):
        self.intercept = None
        self.slope = None


def calibrate_peak_positions(exp_pos_vec, calibrated_pos_vec, poly_order=1):

    # TODO can be changed to 2 only after we find out how to do the math to superpose with DIFC
    if poly_order != 1:
        raise NotImplementedError(f'Polynomial fitting order {poly_order} is not supported')

    y = exp_pos_vec
    x = calibrated_pos_vec

    # polynomial fit
    my_model = np.poly1d(np.polyfit(x, y, poly_order))

    # set to res(idual) instance
    res = Res()
    res.intercept = my_model.coefficients[1]
    res.slope = my_model.coefficients[0]

    return my_model, res


def plot_predicted_calibrated_peak_positions(exp_pos_vec, poly_model, raw_pos_vec, residual, ws_index):
    # calculate the optimized positions
    predicted_pos_vec = poly_model(raw_pos_vec)

    percent_relative_diff = (predicted_pos_vec - exp_pos_vec) / exp_pos_vec
    prev_percent_relative_diff = (raw_pos_vec - exp_pos_vec) / exp_pos_vec
    for i in range(len(predicted_pos_vec)):
        print(f'{exp_pos_vec[i]:.5f}: {percent_relative_diff[i]:.3f}   {prev_percent_relative_diff[i]:.3f}')

    # plot
    time.sleep(1)
    plt.cla()
    plt.plot(exp_pos_vec, prev_percent_relative_diff,
             linestyle='None',
             marker='o',
             color='black',
             label='Observed peak positions')
    plt.plot(exp_pos_vec, percent_relative_diff,
             linestyle='None',
             marker='D',
             color='red',
             label='Expected calibrated peak positions')
    plt.xlabel('d (A): expected peak position')
    plt.ylabel('Relative difference on peak position (d_obs - d_exp) / d_exp')
    plt.title(f'Regression: d = {residual.intercept:.3E} + {residual.slope} x d\'')

    # set VULCAN criteria
    plt.plot([exp_pos_vec[0] - 0.1, exp_pos_vec[-1] + 0.1], [-0.0001, -0.0001], linestyle=':', color='green')
    plt.plot([exp_pos_vec[0] - 0.1, exp_pos_vec[-1] + 0.1], [0.0001, 0.0001], linestyle=':', color='green')

    # set canvas size
    plt.xlim(exp_pos_vec[0] - 0.1, exp_pos_vec[-1] + 0.1)
    y_limit = np.max(np.abs(prev_percent_relative_diff)) * 1.2
    plt.ylim(-max(y_limit, 0.00012), max(y_limit, 0.00012))

    plt.legend()
    plt.savefig(os.path.join('/tmp', f'predicted_position_bank{ws_index}'))
    plt.show()
    time.sleep(1)


def process_fit_result(peak_pos_ws_name, param_ws_name, param_error_ws_name):

    # work include
    # 1. get peak positions and errors
    # 2. get fitted peak parameters (A, B, S)
    # 3. report unsuccessful fit peaks
    # 4. return in labeled numpy array

    # Process peak fitting result
    peak_pos_ws = mtd[peak_pos_ws_name]
    vec_x0 = peak_pos_ws.extractY()[0]

    param_ws = mtd[param_ws_name]
    error_ws = mtd[param_error_ws_name]

    # hard code the parameter dictionary structure and error structure

    param_dict_query = {'A': 3, 'B': 4, 'S': 6, 'Chi2': 7}
    error_dict_query = {'A': 3, 'B': 4, 'S': 6}

    param_value_dict = dict()
    param_error_dict = dict()

    for param_name in param_dict_query.keys():
        param_value_dict[param_name] = list()
    for param_name in error_dict_query.keys():
        param_error_dict[param_name] = list()

    # Retrieve information from parameter value and error table workspace
    # assume that there is only 1 spectrum that is fitted a time
    for pi in range(len(vec_x0)):
        # peak position
        fitted_pos = vec_x0[pi]
        if fitted_pos <= 0:
            print(f'[Fitting   Error] Expected position = {peak_pos_ws.extractX()[0][pi]} Error code = {fitted_pos}')
        else:
            # print(f'[Fitting Success] Peak   pos = {param_ws.cell(pi, 5)} +/- some error {error_ws.cell(pi, 5)}')
            for param_name in param_dict_query.keys():
                # set value for parameter value
                param_value_dict[param_name].append(param_ws.cell(pi, param_dict_query[param_name]))
            for param_name in error_dict_query.keys():
                # set fitting error for parameter
                param_error_dict[param_name].append(error_ws.cell(pi, error_dict_query[param_name]))

    # convert to array
    for param_name in param_dict_query.keys():
        # set value for parameter value
        param_value_dict[param_name] = np.array(param_value_dict[param_name])
    for param_name in error_dict_query.keys():
        # set fitting error for parameter
        param_error_dict[param_name] = np.array(param_error_dict[param_name])

    # peak positions
    vec_x0 = vec_x0
    vec_expected_pos = peak_pos_ws.extractX()[0][vec_x0 > 0]
    # FIXME - error from workspace vecE is not correct!
    vec_x0_error = peak_pos_ws.extractE()[0][vec_x0 > 0]
    vec_x0 = vec_x0[vec_x0 > 0]
    param_value_dict['X0'] = vec_x0
    param_value_dict['ExpectedX0'] = vec_expected_pos
    param_error_dict['X0'] = vec_x0_error

    return param_value_dict, param_error_dict


def report_calibrated_diamond_data(param_value_dict, param_error_dict, data_ws_name, model_ws_name, ws_index):
    """Do a series of report on fitted data including
    1. figure include multiple subplots
      - (d - d_exp) / d_exp * 10000
      - 1/d - A
      - 1/d^4 - B
      - d^2 - S
    2. comparison between data and model (fitted)

    Parameters
    ----------
    param_value_dict
    param_error_dict
    data_ws_name
    model_ws_name
    ws_index

    Returns
    -------

    """
    # Initialize figure with multiple subplots
    time.sleep(1)
    plt.cla()
    param_figure = plt.figure()

    # common data
    exp_pos_vec = param_value_dict['ExpectedX0']

    # plot d_exp ~ d
    # FIXME - change to 111 now for first round conceptual proof
    axis = param_figure.add_subplot(111)
    # axis.errorbar(exp_pos_vec, (param_value_dict['X0'] - exp_pos_vec) / exp_pos_vec,
    #               param_error_dict['X0'] / exp_pos_vec, linestyle='None', marker='o')
    axis.plot(exp_pos_vec, (param_value_dict['X0'] - exp_pos_vec) / exp_pos_vec * 1E4,
              linestyle='None', marker='o')
    axis.set_xlabel('d (A): expected peak position')
    axis.set_ylabel('Relative error on peak position (d_obs - d_exp) / d_exp x 1E4')
    axis.legend()

    # plot 1/d ~ A
    # FIXME - diable plot
    # axis = param_figure.add_subplot(412)
    # print(1/exp_pos_vec, (1/exp_pos_vec).shape)
    # axis.errorbar(1/exp_pos_vec, param_value_dict['A'], param_error_dict['A'], linestyle='None', marker='o')
    # axis.set_xlabel('1/d (A)')
    # axis.set_ylabel('Back-to-back Exponential: A')
    # axis.legend()

    # ...
    # plt.plot(1 / vec_d ** 4, vec_b, label='b2b: b', linestyle='None', marker='o')

    # ...
    #     plt.plot(vec_d ** 2, vec_s ** 2, label='b2b: s^2', linestyle='None', marker='o')

    plt.savefig(os.path.join('/tmp/', f'peak_param_bank{ws_index}.png'))
    plt.show()

    # Plot data, model and difference
    time.sleep(1)
    plt.cla()

    # 2. plot and save relative peak positions and errors
    # FIXME - the workspaces are not PointData
    data_vec_x = mtd[data_ws_name].extractX()[ws_index]
    data_vec_y = mtd[data_ws_name].extractY()[ws_index]
    print(data_vec_x[:-1].shape, data_vec_y.shape)
    plt.plot(data_vec_x[:-1], data_vec_y, color='black', linestyle='None', marker='.', label='data')

    model_vec_x = mtd[model_ws_name].extractX()[ws_index]
    model_vec_y = mtd[model_ws_name].extractY()[ws_index]
    plt.plot(model_vec_x[:-1], model_vec_y, color='red', label='fitted')

    if np.allclose(data_vec_x, model_vec_x):
        diff_y_vec = model_vec_y - data_vec_y

        # only plot the difference for the calculated peaks
        true_false_vec = model_vec_y > 0
        true_false_vec = true_false_vec.astype('int')
        diff_y_vec *= true_false_vec

        plt.plot(model_vec_x[:-1], diff_y_vec, color='green', label='diff')
    plt.legend()
    plt.xlim(0.3, 1.5)

    plt.savefig(os.path.join('/tmp', f'bank_{ws_index}.png'))
    plt.show()


def demo_calibration():
    diamond_dir = '/SNS/users/wzz/Mantid_Project/mantid/scripts/vulcan/calibration'
    if os.path.exists(diamond_dir) is False:
        diamond_dir = '/home/wzz/Projects/Mantid/mantid/scripts/vulcan/backup/calibration-real/LatestTestPD2Round/'
    diamond_nxs = os.path.join(diamond_dir, 'VULCAN_192227_CalMasked_3banks.nxs')
    title = f'Diamond PDCalibration (Fit by Gaussian)'
    tag = 'PDCalibrated'
    print(diamond_nxs)

    # calibration
    src_cal_h5 = os.path.join(diamond_dir, 'VULCAN_Calibration_CC_4runs.h5')
    target_cal_h5 = f'VULCAN_Calibration_CC_4runs_hybrid.h5'

    main(diamond_nxs, title, tag, src_cal_h5, target_cal_h5)


def demo_report():
    diamond_dir = '/SNS/users/wzz/Mantid_Project/mantid/scripts/vulcan/calibration'
    if os.path.exists(diamond_dir) is False:
        diamond_dir = '/home/wzz/Projects/Mantid/mantid/scripts/vulcan/backup/calibration-real/LatestTestPD2Round/'
    diamond_nxs = os.path.join(diamond_dir, 'VULCAN_192227_CalMasked_3banks.nxs')
    title = f'Diamond Cross-correlatin + Hybrid'
    tag = 'CC_2ndRoundCalibrated'

    main(diamond_nxs, title, tag, None, None)


if __name__ in ['__main__', 'mantidqt.widgets.codeeditor.execution']:
    if False:
        demo_calibration()
    else:
        demo_report()
