# Check the quality of calibration file
# including
# 1. masked detector
# 2. etc.
from lib_cross_correlation import (verify_vulcan_difc)
from lib_analysis import report_masked_pixels
from mantid.simpleapi import (LoadNexusProcessed)
from mantid_helper import load_calibration_file
import os

"""
Total = 81920 * 2 + (24900 - 6468) * 2 = 200704
Bank 1: 81920 .. center = 40,960 - ??? ...  40960
Bank 2: 81920 ... center = 61,140  ...  61140
Bank 5: 36864 ... center = 182,272 ...182272
"""
VULCAN_X_PIXEL_RANGE = {'Bank1': (0, 81920),  # 160 tubes
                        'Bank2': (81920, 163840),   # 160 tubes
                        'Bank5': (163840, 200704)   # 72 tubes
                        }


def main_report_calibration(calib_file, diamond_file):
    """Import a calibration file and make reports

    Parameters
    ----------
    calib_file
    diamond_file

    Returns
    -------

    """
    # Load counts for each pixel
    diamond_count_ws = LoadNexusProcessed(Filename=diamond_file, OutputWorkspace='Diamond_Counts')

    # Load calibration file
    calib_ws_tuple = load_calibration_file(calib_file,
                                           output_name='VULCAN_calibration',
                                           ref_ws_name=str(diamond_count_ws))

    # Report masks
    for bank_id in [1, 2, 5]:
        bank = f'Bank{bank_id}'
        print(f'[MASK] Bank {bank}')
        report = report_masked_pixels(diamond_count_ws, calib_ws_tuple.OutputMaskWorkspace,
                                      VULCAN_X_PIXEL_RANGE[bank][0], VULCAN_X_PIXEL_RANGE[bank][1])
        print(report)

    # Report offsets
    verify_vulcan_difc(ws_name=str(diamond_count_ws),
                       cal_table_name=str(calib_ws_tuple.OutputCalWorkspace),
                       mask_ws_name=str(calib_ws_tuple.OutputMaskWorkspace),
                       fallback_incorrect_difc_pixels=False,
                       mask_incorrect_difc_pixels=False,
                       output_dir=os.getcwd())


if __name__ == '__main__':
    # sys.argv[1], sys.argv[2])
    calibration_file='/SNS/VULCAN/shared/wzz/pd_4runs/VULCAN_pdcalibration.h5'
    diamond_nxs = 'VULCAN_192227_192230_Cal.nxs'
    main_report_calibration(calibration_file, diamond_nxs)
