# This script is to prototype the round-2 calibration, i.e., based on the fitted peaks
# in focused diffraction patten, fit
# d = a + b * d', where d' is the fitted peaks' position with round 1 calibration
# Cost function C = |d - exp_d|

from scipy import stats
from mantid.simpleapi import *
import matplotlib.pyplot as plt
import numpy as np

def main():
    res_west = calibrate_west_bank()

    update_calibration_table(res_west)


def update_calibration_table(res_west, rest_east, rest_highangle):
    # import mantid algorithms, numpy and matplotlib
    source_cal_h5 = '/home/wzz/Projects/Mantid/mantid/scripts/vulcan/calibration/diagnostic_history/pdcalibration/VULCAN_pdcalibration.h5'
    out = LoadDiffCal(Filename=source_cal_h5, InstrumentName='vulcan', WorkspaceName='DiffCal_Vulcan')

    cal_table = mtd['DiffCal_Vulcan_cal']
    print(cal_table.getColumnNames())
    print(cal_table.rowCount())

    # theory
    # DIFC^(2)(i) = DIFC(i) / b
    # T0^(2)(i) = - DIFC(i) * a

    # west bank
    print('before: ', cal_table.cell(0, 1), cal_table.cell(0, 3))
    # This is the linear regression for focused peaks from PDCalibration
    # slope:
    b = 1.0005157396059512
    # interception:
    a = -5.642400138239356e-05
    for i_r in range(3234):
        # original difc
        difc = cal_table.cell(i_r, 1)
        tzero = cal_table.cell(i_r, 3)
        if abs(tzero) > 1E-6:
            print('..... bug bug ....')
        # apply 2nd round correction
        new_difc = difc / b
        tzero = - difc * a
        # set
        cal_table.setCell(i_r, 1, new_difc)
        cal_table.setCell(i_r, 3, tzero)

    print('after: ', cal_table.cell(0, 1), cal_table.cell(0, 3))

    SaveDiffCal(CalibrationWorkspace=f'DiffCal_Vulcan_cal', GroupingWorkspace=f'DiffCal_Vulcan_group',
                MaskWorkspace=f'DiffCal_Vulcan_mask', Filename='vulcan_pd0003_round2.h5')


def calibrate_west_bank():
    print()

    # West Bank PD Calibration
    pdcali_dataset = np.array([
        [0.63073, 0.6303807020787756, -0.0003492979212244007, 553585.1532951221, -0.00043600094837896],
        [0.68665, 0.6863333957275309, -0.0003166042724690454, 1334892.12795973, 0.002943373084875997],
        [0.7283, 0.72800799447662, -0.0002920055233799345, 2475652.703519975, 0.003198845057664761],
        [0.81854, 0.8182249919999077, -0.0003150080000923205, 2368601.200305329, 0.003737520580334892],
        [0.89198, 0.8916361075538068, -0.0003438924461931503, 1710967.4781655986, 0.004184149879801434],
        [1.07577, 1.0752880720224502, -0.0004819279775496454, 5142679.879816529, 0.005303074272502532],
        [1.26146, 1.2608124973076278, -0.0006475026923722371, 6105366.223368138, 0.006412806308394916]])

    y = pdcali_dataset[:, 0]
    x = pdcali_dataset[:, 1]

    # linear regression
    res = stats.linregress(x, y)
    print(f'slope:        b = {res.slope}')
    print(f'interception: a = {res.intercept}')

    # plot
    plt.plot(x, y, 'o', label='original data')
    plt.plot(x, res.intercept + res.slope * x, 'r', label='fitted line')
    plt.legend()
    plt.show()

    # Check
    new_x = res.slope * x + res.intercept
    percent_relative_diff = (new_x - y) / y * 100.
    for i in range(len(new_x)):
        print(f'{y[i]}: {percent_relative_diff[i]:.5f}%')

    return res


if __name__ == '__main__':
    main()
