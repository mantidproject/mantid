# This script is to prototype the round-2 calibration, i.e., based on the fitted peaks
# in focused diffraction patten, fit
# d = a + b * d', where d' is the fitted peaks' position with round 1 calibration
# Cost function C = |d - exp_d|

from scipy import stats
from mantid.simpleapi import *
import matplotlib.pyplot as plt
import numpy as np

def main():
    """Test data
    West:
    [0.63073,  0.6303803847550561,   -0.0003496152449439238,    553577.6291306695,  0.0025397920738331925],
    [0.68665,  0.6863339476077304,   -0.0003160523922696168,   1334944.8526758517,  0.002943060794951285 ],
    [0.7283 ,  0.7280080223203926,   -0.0002919776796073137,   2475559.1870459486,  0.0031989291071790418],
    [0.81854,  0.8182250538488576,   -0.0003149461511424700,   2368725.6804186157,  0.0037373046872910378],
    [0.89198,  0.8916369838561887,   -0.00034301614381127  ,   1711014.2874182279,  0.004183869530891603 ],
    [1.07577,  1.0752923197526465,   -0.0004776802473533958,    5144107.263454953,  0.005301084450801467 ],
    [1.26146,  1.2608123836195135,   -0.0006476163804864932,    6105345.550936011,  0.0064128286662222915]
    
    East:
    [0.63073,  0.6302079728683675,   -0.0005220271316325187,    506135.12426777213,  -0.0005387387205723602],
    [0.68665,  0.6861197496412712,   -0.0005302503587287788,   1200154.2134833944 ,   0.0029087908163280165],
    [0.7283 ,  0.7278010121202007,   -0.0004989878797992953,   2233293.1515103644 ,   0.0031590097068082126],
    [0.81854,  0.8179917488396206,   -0.0005482511603794871,   2118262.788997767  ,   0.003681624512538406 ],
    [0.89198,  0.8913692482022341,   -0.0006107517977659294,   1491387.395178894  ,   0.004117032811035757 ],
    [1.07577,  1.0750533996758378,   -0.0007166003241620977,   4594537.432480814  ,   0.0051959641453001556],
    [1.26146,  1.2607046147470984,   -0.0007553852529016414,   5363235.786713449  ,   0.0063126764960063616],
    
    # High angle 
    [0.54411,  0.5437223211405720,   -0.00038767885942803115, 2133004.7822663253,  0.0010026679918045452,  5.7558893608886600e-07],
    [0.56414,  0.5637762108934349,   -0.00036378910656509333,   4898888.50066852,  0.0010447548950161113,  3.619113727588304e-07 ],
    [0.63073,  0.6303822394091168,   -0.0003477605908832615 ,  4198374.582353895,  0.0011776888621865616,  4.7636487552369486e-07],
    [0.68665,  0.686298608109717 ,   -0.00035139189028299267,  8256714.360593585,  0.001281542961031785 ,  3.5837820452911264e-07],
    [0.7283 ,  0.7279426143694034,   -0.0003573856305965073 , 14162260.660233488,  0.0013638154999138785,  2.7945650521171733e-07],
    [0.81854,  0.8181502385602287,   -0.00038976143977131894,  10853301.23312393,  0.0015244506497475016,  3.94817547325995e-07  ],
    [0.89198,  0.8915720486296965,   -0.00040795137030347206,  5964867.915248313, -0.0001302392985283653,  6.627982521449077e-07 ],
    [1.07577,  1.0752509282311888,   -0.0005190717688110524 ,  15183443.85147539,  0.001978951533954768 ,  4.7056051878319607e-07],
    """

    # Source calibration file
    source_cal_h5 = '/SNS/users/wzz/Mantid_Project/mantid/scripts/vulcan/calibration/LatestTestPD2Round/VULCAN_pdcalibration.h5'
    output_calib_file = 'VULCAN_calibration_pd2.h5'

    # Set up inputs
    west_bank_dataset = np.array([
            [0.63073,  0.6303803847550561,   -0.0003496152449439238,    553577.6291306695,  0.0025397920738331925],
            [0.68665,  0.6863339476077304,   -0.0003160523922696168,   1334944.8526758517,  0.002943060794951285 ],
            [0.7283 ,  0.7280080223203926,   -0.0002919776796073137,   2475559.1870459486,  0.0031989291071790418],
            [0.81854,  0.8182250538488576,   -0.0003149461511424700,   2368725.6804186157,  0.0037373046872910378],
            [0.89198,  0.8916369838561887,   -0.00034301614381127  ,   1711014.2874182279,  0.004183869530891603 ],
            [1.07577,  1.0752923197526465,   -0.0004776802473533958,    5144107.263454953,  0.005301084450801467 ],
            [1.26146,  1.2608123836195135,   -0.0006476163804864932,    6105345.550936011,  0.0064128286662222915]])

    # update_calibration_table(res_west)
    east_bank_dataset = np.array([
            [0.63073,  0.6302079728683675,   -0.0005220271316325187,    506135.12426777213,  -0.0005387387205723602],
            [0.68665,  0.6861197496412712,   -0.0005302503587287788,   1200154.2134833944 ,   0.0029087908163280165],
            [0.7283 ,  0.7278010121202007,   -0.0004989878797992953,   2233293.1515103644 ,   0.0031590097068082126],
            [0.81854,  0.8179917488396206,   -0.0005482511603794871,   2118262.788997767  ,   0.003681624512538406 ],
            [0.89198,  0.8913692482022341,   -0.0006107517977659294,   1491387.395178894  ,   0.004117032811035757 ],
            [1.07577,  1.0750533996758378,   -0.0007166003241620977,   4594537.432480814  ,   0.0051959641453001556],
            [1.26146,  1.2607046147470984,   -0.0007553852529016414,   5363235.786713449  ,   0.0063126764960063616]])

    highangle_bank_dataset = np.array([
            [0.54411,  0.5437223211405720,   -0.00038767885942803115, 2133004.7822663253,  0.0010026679918045452,  5.7558893608886600e-07],
            [0.56414,  0.5637762108934349,   -0.00036378910656509333,   4898888.50066852,  0.0010447548950161113,  3.619113727588304e-07 ],
            [0.63073,  0.6303822394091168,   -0.0003477605908832615 ,  4198374.582353895,  0.0011776888621865616,  4.7636487552369486e-07],
            [0.68665,  0.686298608109717 ,   -0.00035139189028299267,  8256714.360593585,  0.001281542961031785 ,  3.5837820452911264e-07],
            [0.7283 ,  0.7279426143694034,   -0.0003573856305965073 , 14162260.660233488,  0.0013638154999138785,  2.7945650521171733e-07],
            [0.81854,  0.8181502385602287,   -0.00038976143977131894,  10853301.23312393,  0.0015244506497475016,  3.94817547325995e-07  ],
            [0.89198,  0.8915720486296965,   -0.00040795137030347206,  5964867.915248313, -0.0001302392985283653,  6.627982521449077e-07 ],
            [1.07577,  1.0752509282311888,   -0.0005190717688110524 ,  15183443.85147539,  0.001978951533954768 ,  4.7056051878319607e-07]])

    # Load calibration file 
    calib_tuple = LoadDiffCal(Filename=source_cal_h5, InstrumentName='vulcan', WorkspaceName='DiffCal_Vulcan')
    print(f'type of returns from LoadDiffCal: {type(calib_tuple)}')
    print(f'dir for LoadDiffCal: {dir(calib_tuple)}')
    calib_ws_name = str(calib_tuple.OutputCalWorkspace)

    # 2nd-round calibration
    for bank_dataset, start_index, end_index in [(west_bank_dataset, 0, 3234),
                                                 (east_bank_dataset, 3234, 6468),
                                                 (highangle_bank_dataset, 6469, 24900)]:

        # linear fitting
        residual = calibrate_peak_positions(bank_dataset, plot=False)

        # update calibration table
        update_calibration_table(calib_tuple.OutputCalWorkspace, residual, start_index, end_index)

    # Save to new diffraction calibration file
    SaveDiffCal(CalibrationWorkspace=calib_ws_name,
                GroupingWorkspace=calib_tuple.OutputGroupingWorkspace,
                MaskWorkspace=calib_tuple.OutputMaskWorkspace,
                Filename=output_calib_file)


def update_calibration_table(cal_table, residual, start_index, end_index):

    # theory
    # DIFC^(2)(i) = DIFC(i) / b
    # T0^(2)(i) = - DIFC(i) * a

    # west bank
    # print('before: ', cal_table.cell(0, 1), cal_table.cell(0, 3))
    # This is the linear regression for focused peaks from PDCalibration
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
        tzero = - difc * a
        # set
        cal_table.setCell(i_r, 1, new_difc)
        cal_table.setCell(i_r, 3, tzero)

    # print('after: ', cal_table.cell(0, 1), cal_table.cell(0, 3))


def calibrate_peak_positions(bank_dataset, plot=False):

        
    y = bank_dataset[:, 0]
    x = bank_dataset[:, 1]

    if False:
        # linear regression: it shall be replaced by polyfit
        res = stats.linregress(x, y)
        print(f'slope:        b = {res.slope}')
        print(f'interception: a = {res.intercept}')

        # Check
        new_x = res.slope * x + res.intercept
    else:
        # polynomial regression
        poly_order = 1  # TODO can be changed to 2 only after we find out how to do the math to superpose with DIFC
        mymodel = np.poly1d(np.polyfit(x, y, poly_order))
        # print(type(mymodel))  numpy.poly1d
        # print(dir(mymodel))

        # 'c', 'coef', 'coefficients', 'coeffs', 'deriv', 'integ', 'o', 'order', 'r', 'roots', 'variable
        # print(type(mymodel.coefficients))  
        # print(mymodel.coefficients) numpy.ndarray

        print(f'slope:        b = {mymodel.coefficients[0]}')
        print(f'interception: a = {mymodel.coefficients[1]}')

        new_x = mymodel(x)

        class Res(object):
            def __init__(self):
                return

        res = Res()
        res.intercept = mymodel.coefficients[1]
        res.slope = mymodel.coefficients[0]

    percent_relative_diff = (new_x - y) / y * 10000.
    prev_percent_relative_diff = (x - y) / y * 10000.
    for i in range(len(new_x)):
        print(f'{y[i]:.5f}: {percent_relative_diff[i]:.3f}   {prev_percent_relative_diff[i]:.3f}')

    if plot:
        # plot
        plt.plot(x, y, 'o', label='original data')
        plt.plot(x, new_x, 'r', label='fitted line')
        plt.legend()
        plt.show()

    return res


if __name__ == '__main__':
    main()


"""
Note:

    - linear regression
        d_exp    diff(2)  diff(1)
        0.54411: -0.884   -7.125
        0.56414: -0.338   -6.449
        0.63073:  0.224   -5.514
        0.68665:  0.362   -5.117
        0.72830:  0.406   -4.907
        0.81854:  0.249   -4.762
        0.89198:  0.236   -4.574
        1.07577: -0.398   -4.825

     - order 2 polynomial
        0.54411: -0.252   -7.125
        0.56414:  0.093   -6.449
        0.63073:  0.174   -5.514
        0.68665:  0.082   -5.117
        0.72830:  0.035   -4.907
        0.81854: -0.139   -4.762
        0.89198: -0.026   -4.574
        1.07577:  0.028   -4.825
        
"""
