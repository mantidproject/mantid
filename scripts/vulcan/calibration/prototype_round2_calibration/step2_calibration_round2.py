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

"""
West:
0.63073  0.6303803847550561   -0.0003496152449439238   553577.6291306695  0.0025397920738331925
0.68665  0.6863339476077304   -0.0003160523922696168   1334944.8526758517  0.002943060794951285
0.7283  0.7280080223203926   -0.0002919776796073137   2475559.1870459486  0.0031989291071790418
0.81854  0.8182250538488576   -0.00031494615114247004   2368725.6804186157  0.0037373046872910378
0.89198  0.8916369838561887   -0.00034301614381127   1711014.2874182279  0.004183869530891603
1.07577  1.0752923197526465   -0.00047768024735339587   5144107.263454953  0.005301084450801467
1.26146  1.2608123836195135   -0.0006476163804864932   6105345.550936011  0.0064128286662222915

East:
0.63073  0.6302079728683675   -0.0005220271316325187   506135.12426777213  -0.0005387387205723602
0.68665  0.6861197496412712   -0.0005302503587287788   1200154.2134833944  0.0029087908163280165
0.7283  0.7278010121202007   -0.0004989878797992953   2233293.1515103644  0.0031590097068082126
0.81854  0.8179917488396206   -0.0005482511603794871   2118262.788997767  0.003681624512538406
0.89198  0.8913692482022341   -0.0006107517977659294   1491387.395178894  0.004117032811035757
1.07577  1.0750533996758378   -0.0007166003241620977   4594537.432480814  0.0051959641453001556
1.26146  1.2607046147470984   -0.0007553852529016414   5363235.786713449  0.0063126764960063616


0.54411  0.543722321140572   -0.00038767885942803115   2133004.7822663253  0.0010026679918045452  5.75588936088866e-07
0.56414  0.5637762108934349   -0.00036378910656509333   4898888.50066852  0.0010447548950161113  3.619113727588304e-07
0.63073  0.6303822394091168   -0.0003477605908832615   4198374.582353895  0.0011776888621865616  4.7636487552369486e-07
0.68665  0.686298608109717   -0.00035139189028299267   8256714.360593585  0.001281542961031785  3.5837820452911264e-07
0.7283  0.7279426143694034   -0.0003573856305965073   14162260.660233488  0.0013638154999138785  2.7945650521171733e-07
0.81854  0.8181502385602287   -0.00038976143977131894   10853301.23312393  0.0015244506497475016  3.94817547325995e-07
0.89198  0.8915720486296965   -0.00040795137030347206   5964867.915248313  -0.00013023929852836531  6.627982521449077e-07
1.07577  1.0752509282311888   -0.0005190717688110524   15183443.85147539  0.001978951533954768  4.7056051878319607e-07


"""

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
