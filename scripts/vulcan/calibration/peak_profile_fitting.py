# High angle bank TOF fitting result
from mantid.simpleapi import FitPeaks, mtd, CloneWorkspace, LoadNexusProcessed
import os
import matplotlib.pyplot as plt
import numpy as np
import time

# This is VULCAN variables
STARTING_PROFILE_PARAMETERS = {'A': [1030., 752., 2535],
                               'B': [1030., 752., 1282],
                               'S': [0.002, 0.002, 0.00057],
                               'W': [5, 5, 1]}


def load_data(focused_diamond_nexus):
    diamond_ws_name = 'DiamondD3Banks'
    LoadNexusProcessed(Filename=focused_diamond_nexus, OutputWorkspace=diamond_ws_name)
    return diamond_ws_name


def peak_width(d):
    """Estimate peak width by FWHM(d)^2 = w0 + w1 * d^2 + w2 * d^4

    Parameters
    ----------
    d: float
        peak position in dSpacing

    Returns
    -------
    tuple
        peak width, square of peak width

    """
    # The following set is for Bank 5.  Thus for Bank 1 and Bank 2, a factor must be multiplied
    w0 = -2.259378321115203e-07
    w1 = 1.233167702630151e-06
    w2 = 5.7816197222790225e-08
    width_sq = w0 + w1 * d ** 2 + w2 * d ** 4
    width = np.sqrt(width_sq)
    return width, width_sq


def fit_diamond_peaks(diamond_ws_name, bank_index):
    """Fit diamond peaks on a single bank

    Parameters
    ----------
    diamond_ws_name: str
        Focused diamond peak workspace's name
    bank_index: int
        Workspace index for the bank to fit.  Note starting from 0.

    Returns
    -------
    tuple

    """
    # Fit A, B, S for dSpacing
    # Hard code expected diamond peak positions in dspacing
    exp_d_centers = np.array([0.60309, 0.63073, 0.68665, 0.7283, 0.81854, 0.89198, 1.07577, 1.26146])

    # Obtain peak parameters' starting values for fitting
    A = STARTING_PROFILE_PARAMETERS['A'][bank_index]
    B = STARTING_PROFILE_PARAMETERS['B'][bank_index]
    S = STARTING_PROFILE_PARAMETERS['S'][bank_index]
    width_factor = STARTING_PROFILE_PARAMETERS['W'][bank_index]

    # Calculate peak width
    width_vec, width_sq_vec = peak_width(exp_d_centers)

    # Bank 5 has 1 less peak
    if bank_index in [0, 1]:
        exp_fit_d_centers = exp_d_centers[:]
    else:        
        exp_fit_d_centers = exp_d_centers[:-1]

    fit_window_list = ''
    for i_peak, d_center in enumerate(exp_fit_d_centers):
        left = d_center - 6 * width_vec[i_peak] * width_factor
        right = d_center + 8 * width_vec[i_peak] * width_factor
        print('Proposed window:', left, d_center, right)

        if len(fit_window_list) > 0:
            fit_window_list += ', '
        fit_window_list += f'{left}, {right}'

    # Set up for peak fitting
    rightmost_peak_param_values = f'{A}, {B}, {S}'
    peak_param_names = 'A, B, S'

    peakpos_ws_name = f'{diamond_ws_name}_position_bank{bank_index}_{len(exp_fit_d_centers)}Peaks'
    param_ws_name = f'{diamond_ws_name}_param_value_bank{bank_index}_{len(exp_fit_d_centers)}Peaks'
    error_ws_name = f'{diamond_ws_name}_param_error_bank{bank_index}_{len(exp_fit_d_centers)}Peaks'
    eff_value_ws_name = f'{diamond_ws_name}_eff_value_bank{bank_index}_{len(exp_fit_d_centers)}Peaks'
    model_ws_name = f'{diamond_ws_name}_model_bank{bank_index}_{len(exp_fit_d_centers)}Peaks'

    # Fit for raw S, A, and B
    FitPeaks(InputWorkspace=diamond_ws_name,
             StartWorkspaceIndex=bank_index,
             StopWorkspaceIndex=bank_index,
             PeakFunction="BackToBackExponential",
             BackgroundType="Linear",
             PeakCenters=exp_fit_d_centers,
             FitWindowBoundaryList=fit_window_list,
             PeakParameterNames=peak_param_names,
             PeakParameterValues=rightmost_peak_param_values,
             FitFromRight=True,
             HighBackground=False,
             OutputWorkspace=peakpos_ws_name,
             OutputPeakParametersWorkspace=param_ws_name,
             OutputParameterFitErrorsWorkspace=error_ws_name,
             FittedPeaksWorkspace=model_ws_name)

    # Fit for peak width
    FitPeaks(InputWorkspace=diamond_ws_name,
             StartWorkspaceIndex=bank_index,
             StopWorkspaceIndex=bank_index,
             PeakFunction="BackToBackExponential",
             BackgroundType="Linear",
             PeakCenters=exp_fit_d_centers,
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

    vec_exp_d = list()
    vec_a = list()
    vec_b = list()
    vec_s = list()
    vec_width = list()

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

        op1 = f'd = {d_center:5f}: {f_a:5f} +/- {err_a:5f},  {f_b:5f} +/- {err_b:5f}, ' \
              f'{f_s:5f} +/- {err_s:5f},  chi^2 = {chi2:5f}'
        op2 = f'd = {d_center:5f}: X = {peak_pos:.8f} +/- {peak_pos_err:.8f}, FWHM = {width}, H = {height:5f}'

        output1 += op1 + '\n'
        output2 += op2 + '\n'

        if chi2 < 1E10:
            vec_exp_d.append(d_center)
            vec_a.append(f_a)
            vec_b.append(f_b)
            vec_s.append(f_s)
            vec_width.append(width)

    print(output1)
    print()
    print(output2)

    # return
    vec_exp_d = np.array(vec_exp_d)
    vec_a = np.array(vec_a)
    vec_b = np.array(vec_b)
    vec_s = np.array(vec_s)
    vec_width = np.array(vec_width)

    return vec_exp_d, vec_a, vec_b, vec_s, vec_width


def fit_fwhm(plot_linear):
    FIT_FWHM_CURVE = False

    # vec_d = np.array([0.603090, 0.630730, 0.686650, 0.728300, 0.818540, 0.891980, 1.075770])

    if FIT_FWHM_CURVE:

        vec_d = np.array([0.630730, 0.686650, 0.728300, 0.818540, 0.891980, 1.075770])

        vec_fwhm = np.array([0.0005322292795359265, 0.0006031970155189648, 0.0006597971580518967, 0.0007877106113513953,
                             0.0008966306384946079, 0.0011297544020837992])

        # Fit FWHM like Sigma^2: Quadratic fit on d**2
        model_fwhm = np.poly1d(np.polyfit(vec_d ** 2, vec_fwhm ** 2, 2))
        w0 = model_fwhm.coefficients[2]
        w1 = model_fwhm.coefficients[1]
        w2 = model_fwhm.coefficients[0]
        fitted_sq_fwhm_vec = model_fwhm(vec_d ** 2)
        fitted_fwhm_vec = np.sqrt(fitted_sq_fwhm_vec)

        print(f'w0 = {w0}, w1 = {w1}, w2 = {w2}')

        if plot_linear:
            plt.plot(vec_d ** 2, vec_fwhm ** 2, linestyle='None', marker='o', color='black')
            plt.plot(vec_d ** 2, fitted_sq_fwhm_vec, linestyle=':', marker='d', color='red')
            plt.show()
        else:
            plt.plot(vec_d, vec_fwhm, linestyle='None', marker='o', color='black')
            plt.plot(vec_d, fitted_fwhm_vec, linestyle=':', marker='d', color='red')
            plt.show()

        return


def fit_vulcan_profile(bank_index, vec_exp_d, vec_a, vec_b, vec_s, vec_width,
                       output_dir):

    print(f'Bank on workspace {bank_index}')

    vec_d = vec_exp_d

    # Fit Alpha
    # A(d) = alph0 + alpha1 x (1/d)
    model_a = np.poly1d(np.polyfit(1./vec_d, vec_a, 1))
    alpha0 = model_a.coefficients[1]
    alpha1 = model_a.coefficients[0]
    fitted_a_vec = model_a(1./vec_d)

    plt.cla()
    time.sleep(1)
    plt.plot(1./vec_d, vec_a, linestyle='None', marker='o', color='black', label='Observed')
    plt.plot(1./vec_d, fitted_a_vec, linestyle=':', marker='d', color='red', label='A(d) = alpha0 + alpha1 / d')
    plt.legend()
    plt.savefig(os.path.join(output_dir, f'alpha_bank{bank_index}.png'))

    # Fit Beta
    # B(d) = beta0 + beta1 x (1/d**4)
    model_b = np.poly1d(np.polyfit(1./vec_d**4, vec_b, 1))
    beta0 = model_b.coefficients[1]
    beta1 = model_b.coefficients[0]
    fitted_b_vec = model_b(1./vec_d**4)

    plt.cla()
    time.sleep(1)
    plt.plot(1./vec_d**4, vec_b, linestyle='None', marker='o', color='black', label='Observed')
    plt.plot(1./vec_d**4, fitted_b_vec, linestyle=':', marker='d', color='red', label='B(d) = beta0 + beta1 / d^4')
    plt.legend()
    plt.savefig(os.path.join(output_dir, f'beta_bank{bank_index}.png'))

    # Fit Sigma
    # S**2 = sig0 + sig1 x d**2 + sig2 x d**4
    model_s = np.poly1d(np.polyfit(vec_d**2, vec_s**2, 2))
    sig0 = model_s.coefficients[2]
    sig1 = model_s.coefficients[1]
    sig2 = model_s.coefficients[0]
    fitted_sq_s_vec = model_s(vec_d**2)

    plt.cla()
    time.sleep(1)
    plt.plot(vec_d**2, vec_s**2, linestyle='None', marker='o', color='black',
             label='Observed')
    plt.plot(vec_d**2, fitted_sq_s_vec, linestyle=':', marker='d', color='red',
             label='S(d) = sqrt(sig0 + sig1 * d^2 + sig2 * d^4')
    plt.legend()
    plt.savefig(os.path.join(output_dir, f'sigma_bank{bank_index}.png'))


def test_main():
    # User setup
    nexus_file = '/home/wzz/Projects/Mantid/mantid/scripts/vulcan/calibration/vulcan-x-beta/' \
                 'VULCAN_192227_192230_CalMasked_3banks.nxs'

    # Load data
    diamond_ws_name = load_data(nexus_file)

    for bank_id in [0, 1, 2]:
        # Fit diamond peaks
        vec_exp_d, vec_a, bec_b, vec_s, vec_width = fit_diamond_peaks(diamond_ws_name, bank_id)
        # Fit Back-to-back exponential profile parameters: A, B and S
        fit_vulcan_profile(bank_id, vec_exp_d, vec_a, bec_b, vec_s, vec_width,
                           output_dir=os.path.expanduser('~/Desktop'))


test_main()
