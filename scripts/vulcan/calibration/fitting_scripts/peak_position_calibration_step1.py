# Peak positions calibration (round 2 calibration): Step 1
# Task: For each bank
# 1. Fit diamond peaks
# 2. Report deviations from expected positions
# 3. Fit peak positions with linear (or in future quadratic) function
# 4. Predict the 2nd-round calibration result
import os
from mantid.simpleapi import LoadNexusProcessed, mtd


def main(focused_diamond_nxs, figure_title, ws_tag):

    # Load focused diamond diffraction data from process NeXus file
    base_name = os.path.basename(focused_diamond_nxs).split('.')[0]
    diamond_ws_name = f'{base_name}_{ws_tag}'
    LoadNexusProcessed(Filename=diamond_nxs, OutputWorkspace=diamond_ws_name)
    diamond_ws = mtd[diamond_ws_name]
    assert diamond_ws

    # Fit west and east bank

    # Fit high angle banks

    # Fit west bank
    fit_90degree_banks(diamond_ws_name, title, 0)

    # Fit east bank
    fit_90degree_banks(diamond_ws_name, title, 1)

    # Fit high angle bank
    fit_high_angle_bank(diamond_ws_name, title)


if __name__ == '__main__':
    diamond_dir = '/SNS/users/wzz/Mantid_Project/mantid/scripts/vulcan/calibration/LatestTestPD2Round'
    diamond_nxs = os.path.join(diamond_dir, 'VULCAN_164960_diamond_3banks.nxs')
    title = f'Diamond PDCalibration (Fit by Gaussian)'
    tag = 'PDCalibrated'
    main(diamond_nxs, title, tag)
