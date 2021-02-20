# Check calibration by
# 1. Load event data (single or multiple files)
# 2. Optionally align detectors
#  or Reload instrument (raw)
# 3. Export unfocused data
# 4. Diffraction focus and export
import os
from mantid.simpleapi import LoadEventNexus, Plus, LoadInstrument
from lib_analysis import (align_focus_event_ws)
from mantid_helper import load_calibration_file
from typing import List, Tuple, Union


def reduce_calibration(event_ws_name: str,
                       calibration_file: str,
                       idf_file=None,
                       apply_mask=True,
                       align_detectors=True,
                       output_dir=os.getcwd()) -> Tuple[str, str]:
    """

    Parameters
    ----------
    event_ws_name: str
        Name of EventWorkspace to reduce from
    calibration_file
    idf_file: str, None
        If give, use IDF to reload instrument and automatically disable align detector
    apply_mask
    align_detectors: bool
        Flag to align detector or not

    Returns
    -------
    ~tuple
        focused workspace name, path to processed nexus file saved from focused workspace

    """
    # Load calibration file
    calib_tuple = load_calibration_file(calibration_file, 'VulcanX_PD_Calib', event_ws_name)
    calib_cal_ws = calib_tuple.OutputCalWorkspace
    calib_group_ws = calib_tuple.OutputGroupingWorkspace
    calib_mask_ws = calib_tuple.OutputMaskWorkspace

    # Load instrument
    if idf_file:
        LoadInstrument(Workspace=event_ws_name,
                       Filename=idf_file,
                       InstrumentName='VULCAN',
                       RewriteSpectraMap=True)
        # auto disable align detector
        align_detectors = False

    # Align, focus and export
    focused_tuple = align_focus_event_ws(event_ws_name,
                                         str(calib_cal_ws) if align_detectors else None,
                                         str(calib_group_ws),
                                         str(calib_mask_ws) if apply_mask else None,
                                         output_dir=output_dir)

    return focused_tuple
