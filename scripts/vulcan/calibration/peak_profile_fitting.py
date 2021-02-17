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


def test_main():
    # User setup
    nexus_file = '/home/wzz/Projects/Mantid/mantid/scripts/vulcan/calibration/testdata/VULCAN_164960_diamond_3banks.nxs'
    bank_id = 0

    # Load data
    diamond_ws_name = load_data(nexus_file)
    # Fit diamond peaks
    vec_exp_d, vec_a, bec_b, vec_s, vec_width = fit_diamond_peaks(diamond_ws_name, bank_id)
    # Fit Back-to-back exponential profile parameters: A, B and S
    fit_vulcan_profile(bank_id, vec_exp_d, vec_a, bec_b, vec_s, vec_width)


def load_data(focused_diamond_nexus):
    diamond_ws_name = 'DiamondD3Banks'
    LoadNexusProcessed(Filename=focused_diamond_nexus, OutputWorkspace=diamond_ws_name)
    return diamond_ws_name


def peak_width(d):
    w0 = -2.259378321115203e-07
    w1 = 1.233167702630151e-06
    w2 = 5.7816197222790225e-08
    width_sq = w0 + w1 * d ** 2 + w2 * d ** 4
    width = np.sqrt(width_sq)
    return width, width_sq
    

def fit_diamond_peaks(diamond_ws_name, bank_index):
    # import mantid algorithms, numpy and matplotlib
    # Fit A, B, S for dSpacing

    
    # Fill
    exp_d_centers = np.array([0.60309, 0.63073, 0.68665, 0.7283, 0.81854, 0.89198, 1.07577, 1.26146])
    
    # @ d = 1.075
    A = STARTING_PROFILE_PARAMETERS['A'][bank_index]
    B = STARTING_PROFILE_PARAMETERS['B'][bank_index]
    S = STARTING_PROFILE_PARAMETERS['S'][bank_index]
    width_factor = STARTING_PROFILE_PARAMETERS['W'][bank_index]

    
    width_vec, width_sq_vec = peak_width(exp_d_centers)
    print(f'{width_vec}')
    
    if bank_index in [0, 1]:
        exp_fit_d_centers = exp_d_centers[:]
    else:        
        exp_fit_d_centers = exp_d_centers[:-1]
    
    fit_window_list = ''
    for i_peak, d_center in enumerate(exp_fit_d_centers):
        left = d_center - 6 * width_vec[i_peak] * width_factor
        right = d_center + 8 * width_vec[i_peak] * width_factor
        print('determine window:', left, d_center, right)
    
        if len(fit_window_list) > 0:
            fit_window_list += ', '
        fit_window_list += f'{left}, {right}'
    print(fit_window_list)
    
    rightmost_peak_param_values = f'{A}, {B}, {S}'
    
    peak_param_names = 'A, B, S'
    
    peakpos_ws_name = f'{diamond_ws_name}_position_bank{bank_index}_{len(exp_fit_d_centers)}Peaks'
    param_ws_name = f'{diamond_ws_name}_param_value_bank{bank_index}_{len(exp_fit_d_centers)}Peaks'
    error_ws_name = f'{diamond_ws_name}_param_error_bank{bank_index}_{len(exp_fit_d_centers)}Peaks'
    eff_value_ws_name = f'{diamond_ws_name}_eff_value_bank{bank_index}_{len(exp_fit_d_centers)}Peaks'
    model_ws_name = f'{diamond_ws_name}_model_bank{bank_index}_{len(exp_fit_d_centers)}Peaks'
    
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
    
        op1 = f'd = {d_center:5f}: {f_a:5f} +/- {err_a:5f},  {f_b:5f} +/- {err_b:5f},  {f_s:5f} +/- {err_s:5f},  chi^2 = {chi2:5f}'
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


def fit_fwhm():
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

        if False:
            plt.plot(vec_d ** 2, vec_fwhm ** 2, linestyle='None', marker='o', color='black')
            plt.plot(vec_d ** 2, fitted_sq_fwhm_vec, linestyle=':', marker='d', color='red')
            plt.show()
        else:
            plt.plot(vec_d, vec_fwhm, linestyle='None', marker='o', color='black')
            plt.plot(vec_d, fitted_fwhm_vec, linestyle=':', marker='d', color='red')
            plt.show()

        return


def fit_vulcan_profile(bank_index, vec_exp_d, vec_a, vec_b, vec_s, vec_width,
                       output_dir=os.path.expanduser('~/Desktop')):

    print(f'Bank {bank_index}')

    # Peaks with good fitting
    # vec_d = np.array([0.630730, 0.686650, 0.728300, 0.818540, 0.891980, 1.075770])
    vec_d = vec_exp_d
    
    # A(d) = alph0 + alpha1 x (1/d)
    # print(vec_a)
    # vec_a = np.array([4731.678496, 4164.784651, 3798.979360, 3419.096784, 2997.871497, 2525.416876])
    
    # Fit Alpha
    model_a = np.poly1d(np.polyfit(1./vec_d, vec_a, 1))
    alpha0 = model_a.coefficients[1]
    alpha1 = model_a.coefficients[0]
    fitted_a_vec = model_a(1./vec_d)
    
    print(f'alpha0 = {alpha0}, alpha1 = {alpha1}')
    
    plt.cla()
    time.sleep(1)
    plt.plot(1./vec_d, vec_a, linestyle='None', marker='o', color='black', label='Observed')
    plt.plot(1./vec_d, fitted_a_vec, linestyle=':', marker='d', color='red', label='A(d) = alpha0 + alpha1 / d')
    plt.legend()
    plt.savefig(os.path.join(output_dir, f'alpha_bank{bank_index}.png'))

    if False:
        plt.show()
    
    
    # B(d) = beta0 + beta1 x (1/d**4)
    # print(f'B: {vec_b}')
    # vec_b = np.array([1409.614474, 1373.530991, 1353.195948, 1333.428796, 1307.413265, 1261.874839])
    
    # Fit Beta
    model_b = np.poly1d(np.polyfit(1./vec_d**4, vec_b, 1))
    beta0 = model_b.coefficients[1]
    beta1 = model_b.coefficients[0]
    fitted_b_vec = model_b(1./vec_d**4)
    
    print(f'beta0 = {beta0}, beta1 = {beta1}')

    plt.cla()
    time.sleep(1)
    plt.plot(1./vec_d**4, vec_b, linestyle='None', marker='o', color='black', label='Observed')
    plt.plot(1./vec_d**4, fitted_b_vec, linestyle=':', marker='d', color='red', label='B(d) = beta0 + beta1 / d^4')
    plt.legend()
    plt.savefig(os.path.join(output_dir, f'beta_bank{bank_index}.png'))
    if False:
        plt.show()
        return
    
    
    # S**2 = sig0 + sig1 x d**2 + sig2 x d**4
    # vec_s = np.array([0.000266, 0.000302, 0.000330, 0.000394, 0.000448, 0.000565])
    
    # Fit Sigma
    model_s = np.poly1d(np.polyfit(vec_d**2, vec_s**2, 2))
    sig0 = model_s.coefficients[2]
    sig1 = model_s.coefficients[1]
    sig2 = model_s.coefficients[0]
    fitted_sq_s_vec = model_s(vec_d**2)
    
    print(f'sig0 = {sig0}, sig1 = {sig1}, sig2 = {sig2}')
    
    plt.cla()
    time.sleep(1)
    plt.plot(vec_d**2, vec_s**2, linestyle='None', marker='o', color='black',
             label='Observed')
    plt.plot(vec_d**2, fitted_sq_s_vec, linestyle=':', marker='d', color='red',
             label='S(d) = sqrt(sig0 + sig1 * d^2 + sig2 * d^4')
    plt.legend()
    plt.savefig(os.path.join(output_dir, f'sigma_bank{bank_index}.png'))
    if True:
        plt.show()
        return


"""  Report VULCAN (not X)

Bank 2 (Refactored codes)


Peak width: FWHM(d)^2 = w0 + w1 * d^2 + w2 * d^4
    w0 = -2.259378321115203e-07
    w1 = 1.233167702630151e-06
    w2 = 5.7816197222790225e-08

Alaph(d) = alpha0 + alpha1 / d
    alpha0 = -656.1985335370924
    alpha1 = 3326.9500707745465

Beta(d) = beta0 + beta1 / d^4
    beta0 = 1262.7403739861777
    beta1 = 24.4441779173347

Sigma(d) = sig0 + sig1 * d^2 + sig2 * d^4

    sig0 = -5.5813971121388484e-08
    sig1 = 3.064601120671658e-07
    sig2 = 1.5610078763158822e-08
    
    
alpha0 = -737.3538882217031, alpha1 = 3494.440665792036
beta0 = 1326.1068826471871, beta1 = 16.193526421073575
sig0 = -5.383109227983116e-08, sig1 = 3.026148491708998e-07, sig2 = 3.393504763431537e-08

From
d = 0.603090: X = -4.00000000 +/- 0.00000000, FWHM = 0.0019809490776722025, H = 2541361.865837
d = 0.630730: X = 0.63073847 +/- 0.00000055, FWHM = 0.0011781892453575735, H = 4067393.166818
d = 0.686650: X = 0.68667101 +/- 0.00000041, FWHM = 0.0012835477462595718, H = 7998219.081139
d = 0.728300: X = 0.72832916 +/- 0.00000032, FWHM = 0.0013671539249262453, H = 13720252.206207
d = 0.818540: X = 0.81856800 +/- 0.00000044, FWHM = 0.0015243958150369526, H = 10570980.125501
d = 0.891980: X = 0.89201150 +/- 0.00000073, FWHM = 0.0016757974063104923, H = 5709708.090450
d = 1.075770: X = 1.07574102 +/- 0.00000052, FWHM = 0.001983664301973602, H = 14778136.149454


Bank 0
-------
FitPeaks successful, Duration 0.03 seconds
d = 0.603090: 2110.637621 +/- 6.091554,  1050.365429 +/- 1.100552,  0.000651 +/- 0.000001,  chi^2 = 20.089635
d = 0.630730: 1908.697949 +/- 6.791700,  1033.963996 +/- 1.416354,  0.000736 +/- 0.000002,  chi^2 = 13.653711
d = 0.686650: 1705.409612 +/- 3.702353,  1014.238048 +/- 0.920666,  0.000902 +/- 0.000001,  chi^2 = 31.087241
d = 0.728300: 1583.959452 +/- 2.329005,  985.052288 +/- 0.646488,  0.001002 +/- 0.000001,  chi^2 = 71.863102
d = 0.818540: 1482.478699 +/- 2.897885,  955.454862 +/- 0.803338,  0.001234 +/- 0.000001,  chi^2 = 46.223532
d = 0.891980: 1367.316009 +/- 3.761635,  925.167734 +/- 1.135906,  0.001417 +/- 0.000002,  chi^2 = 41.153766
d = 1.075770: 1237.353596 +/- 2.264183,  855.806565 +/- 0.669586,  0.001891 +/- 0.000001,  chi^2 = 278.537155
d = 1.261460: 1080.638653 +/- 2.040876,  766.980909 +/- 0.628265,  0.002338 +/- 0.000001,  chi^2 = 670.612679


d = 0.603090: X = 0.60301026 +/- 0.00000144, FWHM = 0.002318764040282674, H = 644242.980312
d = 0.630730: X = 0.63067986 +/- 0.00000188, FWHM = 0.002539889721293265, H = 519998.071444
d = 0.686650: X = 0.68666208 +/- 0.00000130, FWHM = 0.0029426133680407794, H = 1274084.072858
d = 0.728300: X = 0.72834531 +/- 0.00000096, FWHM = 0.0032012379912310907, H = 2386916.960219
d = 0.818540: X = 0.81860315 +/- 0.00000133, FWHM = 0.003739586154045624, H = 2282496.736509
d = 0.891980: X = 0.89205219 +/- 0.00000198, FWHM = 0.004185496705364365, H = 1641392.920085
d = 1.075770: X = 1.07578982 +/- 0.00000148, FWHM = 0.0053023543252423335, H = 5056303.281902
d = 1.261460: X = 1.26137353 +/- 0.00000174, FWHM = 0.006420189599258519, H = 6035797.086629

Bank 0
alpha0 = 176.38264715632278, alpha1 = 1091.4713417571072
beta0 = 837.5365298761925, beta1 = 32.98942380273425
sig0 = -8.188814338896528e-07, sig1 = 3.214616320064918e-06, sig2 = 4.6793989064934257e-07


"""

test_main()
