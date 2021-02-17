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
        

""" Report from VULCAN-x

Spectrum 0 (Bank1)
------------------

FitPeaks successful, Duration 0.05 seconds
d = 0.603090: 4688.629269 +/- 61.006472,  1140.582071 +/- 1.192618,  0.000550 +/- 0.000002,  chi^2 = 105.950250
d = 0.630730: 5217.608878 +/- 134.415045,  1145.180052 +/- 2.012122,  0.000637 +/- 0.000003,  chi^2 = 35.229695
d = 0.686650: 3573.659772 +/- 33.476861,  1098.524516 +/- 0.865098,  0.000743 +/- 0.000002,  chi^2 = 72.907671
d = 0.728300: 3003.363809 +/- 13.524445,  1066.160102 +/- 0.527464,  0.000818 +/- 0.000001,  chi^2 = 135.871153
d = 0.818540: 2946.494124 +/- 24.908868,  1052.024424 +/- 0.778622,  0.001011 +/- 0.000002,  chi^2 = 104.180765
d = 0.891980: 2199.117640 +/- 15.327119,  1029.378259 +/- 1.198184,  0.001130 +/- 0.000002,  chi^2 = 49.660174
d = 1.075770: 1946.485872 +/- 7.034173,  1006.838333 +/- 0.659716,  0.001491 +/- 0.000001,  chi^2 = 472.571845
d = 1.261460: 1953.437501 +/- 13.998276,  1005.559320 +/- 1.166352,  0.001885 +/- 0.000002,  chi^2 = 778.303131


d = 0.603090: X = 0.60264909 +/- 0.00000318, FWHM = 0.0018829942184107127, H = 1734204.977863
d = 0.630730: X = 0.63024919 +/- 0.00000585, FWHM = 0.002046736448304733, H = 1356671.249112
d = 0.686650: X = 0.68620205 +/- 0.00000288, FWHM = 0.002353399946237673, H = 3564304.401712
d = 0.728300: X = 0.72786303 +/- 0.00000161, FWHM = 0.0025640006561046505, H = 6856814.161810
d = 0.818540: X = 0.81803231 +/- 0.00000315, FWHM = 0.0029857437604975216, H = 6442882.841179
d = 0.891980: X = 0.89150280 +/- 0.00000336, FWHM = 0.003327009546712837, H = 3883610.643117
d = 1.075770: X = 1.07510836 +/- 0.00000204, FWHM = 0.004148352682798104, H = 14680710.267618
d = 1.261460: X = 1.26049381 +/- 0.00000436, FWHM = 0.004998277041702236, H = 15503120.549208

alpha0 = -1480.4936257457014, alpha1 = 3686.3709369617836
beta0 = 997.7750026275578, beta1 = 20.909915950121587
sig0 = -3.452604447419142e-07, sig1 = 1.6615817540949255e-06, sig2 = 4.937377061528497e-07


Spectrum 1 (Bank2)
------------------

FitPeaks successful, Duration 0.06 seconds
d = 0.603090: 4541.487576 +/- 60.600610,  1139.327338 +/- 1.257776,  0.000554 +/- 0.000002,  chi^2 = 113.742034
d = 0.630730: 4826.702427 +/- 125.661138,  1148.782074 +/- 2.118523,  0.000638 +/- 0.000004,  chi^2 = 41.204316
d = 0.686650: 3964.732492 +/- 55.202271,  1092.532822 +/- 1.025857,  0.000755 +/- 0.000002,  chi^2 = 77.809379
d = 0.728300: 4530.476444 +/- 66.311812,  1050.297745 +/- 0.796895,  0.000848 +/- 0.000002,  chi^2 = 179769313486231570814527423731704356798070567525844996598917476803157260780028538760589558632766878171540458953514382464234321326889464182768467546703537516986049910576551282076245490090389328944075868508455133942304583236903222948165808559332123348274797826204144723168738177180919299881250404026184124858368.000000
d = 0.818540: 4764.846911 +/- 93.584797,  996.767809 +/- 1.055006,  0.001021 +/- 0.000002,  chi^2 = 100.873557
d = 0.891980: 3850.468346 +/- 118.956034,  955.654245 +/- 1.822320,  0.001152 +/- 0.000004,  chi^2 = 57.810028
d = 1.075770: 3296.606354 +/- 50.053674,  888.301503 +/- 0.861163,  0.001483 +/- 0.000002,  chi^2 = 625.651934
d = 1.261460: 2682.431161 +/- 44.720691,  835.595309 +/- 1.064357,  0.001834 +/- 0.000003,  chi^2 = 986.788645


d = 0.603090: X = 0.60248440 +/- 0.00000334, FWHM = 0.0018952805759461662, H = 1497401.028826
d = 0.630730: X = 0.63010136 +/- 0.00000629, FWHM = 0.002058778354773117, H = 1168173.478360
d = 0.686650: X = 0.68604397 +/- 0.00000393, FWHM = 0.002363920770004744, H = 3083572.018007
d = 0.728300: X = -4.00000000 +/- 0.00000371, FWHM = 0.0025627776867232937, H = 5953860.249125
d = 0.818540: X = 0.81786269 +/- 0.00000490, FWHM = 0.002956337530358825, H = 5623749.113757
d = 0.891980: X = 0.89132401 +/- 0.00000952, FWHM = 0.003294765224152541, H = 3385571.153764
d = 1.075770: X = 1.07509579 +/- 0.00000548, FWHM = 0.004081994870506091, H = 12685469.396576
d = 1.261460: X = 1.26076940 +/- 0.00000749, FWHM = 0.004924334936275179, H = 13111255.778328

alpha0 = 1456.4225958484844, alpha1 = 2023.0764925200333
beta0 = 868.9774156128921, beta1 = 41.76267927224485
sig0 = -4.62242170957069e-07, sig1 = 2.0916067695736593e-06, sig2 = 1.934111744761891e-07

Spectrum 2 (Bank3)
------------------

d = 0.603090: 7777.715385 +/- 24.015570,  1421.197495 +/- 1.749807,  0.000225 +/- 0.000001,  chi^2 = 56.060688
d = 0.630730: 5436.044940 +/- 17.493442,  1406.260155 +/- 2.627166,  0.000225 +/- 0.000001,  chi^2 = 128.634445
d = 0.686650: 4711.964552 +/- 9.897260,  1386.570840 +/- 1.650444,  0.000258 +/- 0.000001,  chi^2 = 553.811582
d = 0.728300: 4278.124595 +/- 6.670252,  1375.211309 +/- 1.233588,  0.000283 +/- 0.000001,  chi^2 = 1169.735280
d = 0.818540: 3647.663211 +/- 6.537285,  1376.146795 +/- 1.694749,  0.000336 +/- 0.000001,  chi^2 = 1251.242083
d = 0.891980: 3097.396040 +/- 8.189854,  1370.678488 +/- 3.123900,  0.000382 +/- 0.000001,  chi^2 = 634.514471
d = 1.075770: 2797.446672 +/- 4.220778,  1325.445733 +/- 1.849737,  0.000481 +/- 0.000001,  chi^2 = 1418.904869


d = 0.603090: X = 0.60268316 +/- 0.00000072, FWHM = 0.0010341608381671868, H = 1656038.099883
d = 0.630730: X = 0.63036250 +/- 0.00000106, FWHM = 0.0010761409022941476, H = 912694.144281
d = 0.686650: X = 0.68629096 +/- 0.00000078, FWHM = 0.0011704484657733315, H = 1786322.275315
d = 0.728300: X = 0.72794594 +/- 0.00000064, FWHM = 0.0012420601195364336, H = 2871103.235602
d = 0.818540: X = 0.81818729 +/- 0.00000090, FWHM = 0.0013786849299019248, H = 2169651.573511
d = 0.891980: X = 0.89163853 +/- 0.00000165, FWHM = 0.001508785493849381, H = 1057660.105309
d = 1.075770: X = 1.07533798 +/- 0.00000111, FWHM = 0.001752759770628538, H = 2636638.492695

Bank 2
alpha0 = -3343.6129916632617, alpha1 = 5901.5043458038135
beta0 = 1338.1040313808196, beta1 = 11.13036765018546
sig0 = -2.8525605914634994e-08, sig1 = 1.9467468144840576e-07, sig2 = 2.6159047086806803e-08

"""


"""  Report VULCAN (not X)

Bank 0
alpha0 = 176.38264715632278, alpha1 = 1091.4713417571072
beta0 = 837.5365298761925, beta1 = 32.98942380273425
sig0 = -8.188814338896528e-07, sig1 = 3.214616320064918e-06, sig2 = 4.6793989064934257e-07

Bank 2 (Refactored codes)
alpha0 = -737.3538882217031, alpha1 = 3494.440665792036
beta0 = 1326.1068826471871, beta1 = 16.193526421073575
sig0 = -5.383109227983116e-08, sig1 = 3.026148491708998e-07, sig2 = 3.393504763431537e-08

"""

def test_main():
    # User setup
    nexus_file = '/home/wzz/Projects/Mantid/mantid/scripts/vulcan/calibration/vulcan-x-beta/VULCAN_192227_192230_CalMasked_3banks.nxs'
    bank_id = 2

    # Load data
    diamond_ws_name = load_data(nexus_file)
    # Fit diamond peaks
    vec_exp_d, vec_a, bec_b, vec_s, vec_width = fit_diamond_peaks(diamond_ws_name, bank_id)
    # Fit Back-to-back exponential profile parameters: A, B and S
    fit_vulcan_profile(bank_id, vec_exp_d, vec_a, bec_b, vec_s, vec_width)


test_main()
