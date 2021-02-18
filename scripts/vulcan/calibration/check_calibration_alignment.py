# Check calibration by
# 1. Load event data (single or multiple files)
# 2. Optionally align detectors
#  or Reload instrument (raw)
# 3. Export unfocused data
# 4. Diffraction focus and export
from mantid.simpleapi import LoadEventNexus, Plus, LoadInstrument
from lib_analysis import (align_focus_event_ws)
from mantid_helper import load_calibration_file
from typing import List


def reduce_calibration(diamond_runs: List[int],
                       calibration_file: str,
                       idf_file=None,
                       apply_mask=True,
                       align_detectors=True):
    """

    Parameters
    ----------
    diamond_runs
    calibration_file
    idf_file: str, None
        If give, use IDF to reload instrument and automatically disable align detector
    apply_mask
    align_detectors: bool
        Flag to align detector or not

    Returns
    -------

    """

    # Load event data from single or multiple file
    dia_wksp = "%s_%d" % ('VULCAN', dia_runs[0])

    # Load full size data
    LoadEventNexus(Filename=dia_wksp, OutputWorkspace=dia_wksp)

    # Load more files
    for file_index in range(1, len(diamond_runs)):
        # Load next file
        dia_wksp_i = "%s_%d" % ('VULCAN', dia_runs[file_index])
        LoadEventNexus(Filename=dia_wksp_i, OutputWorkspace=dia_wksp_i)
        # Merge
        Plus(LHSWorkspace=dia_wksp,
             RHSWorkspace=dia_wksp_i,
             OutputWorkspace=dia_wksp, ClearRHSWorkspace=True)

    # Load calibration file
    calib_tuple = load_calibration_file(calibration_file, 'VulcanX_PD_Calib', dia_wksp)
    calib_cal_ws = calib_tuple.OutputCalWorkspace
    calib_group_ws = calib_tuple.OutputGroupingWorkspace
    calib_mask_ws = calib_tuple.OutputMaskWorkspace

    # Load instrument
    if idf_file:
        LoadInstrument(Workspace=dia_wksp,
                       Filename=idf_file,
                       MonitorList='-3--2', InstrumentName='VULCAN', RewriteSpectraMap=True)
        # auto disable align detector
        align_detectors = False

    # Align, focus and export
    align_focus_event_ws(dia_wksp,
                         str(calib_cal_ws) if align_detectors else None,
                         str(calib_group_ws),
                         str(calib_mask_ws) if apply_mask else None)


if __name__ == '__main__':
    dia_runs = [192227, 192228, 192229, 192230]

    test_calib_file='/SNS/VULCAN/shared/wzz/pd_4runs/VULCAN_pdcalibration.h5',
    test_calib_file='/SNS/users/wzz/Mantid_Project/mantid/scripts/vulcan/calibration/VULCAN_Calibration_CC.h5'

    reduce_calibration(dia_runs[0:1],
                       calibration_file=test_calib_file,
                       idf_file=None,  #  'data/VULCAN_Definition_pete02.xml',
                       apply_mask=True,
                       align_detectors=True)
