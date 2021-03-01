# Check calibration by
# 1. Load event data (single or multiple files)
# 2. Optionally align detectors
#  or Reload instrument (raw)
# 3. Export unfocused data
# 4. Diffraction focus and export
import os
from mantid.simpleapi import (LoadInstrument, CreateGroupingWorkspace)
from .lib_analysis import (align_focus_event_ws)
from .mantid_helper import load_calibration_file
from typing import List, Tuple, Union


def make_group_workspace(template_ws_name: str,
                         group_ws_name: str,
                         grouping_plan: List[Tuple[int, int, int]]):
    """Create a GroupWorkspace with user specified group strategy

    Returns
    -------
    mantid.dataobjects.GroupingWorkspace
        Instance of the GroupingWorkspace generated

    """
    # Create an empty GroupWorkspace
    group_ws = CreateGroupingWorkspace(InputWorkspace=template_ws_name,
                                       GroupDetectorsBy='Group',
                                       OutputWorkspace=group_ws_name)
    group_ws = group_ws.OutputWorkspace

    # Set customized group to each pixel
    group_index = 1
    for start_index, step_size, end_index in grouping_plan:
        for ws_index in range(start_index, end_index, step_size):
            # set values
            for ws_shift in range(step_size):
                group_ws.dataY(ws_index + ws_shift)[0] = group_index
            # promote group index
            group_index += 1

    return group_ws


def reduce_calibration(event_ws_name: str,
                       calibration_file: str,
                       idf_file=None,
                       apply_mask=True,
                       align_detectors=True,
                       customized_group_ws_name: Union[str, None] = None,
                       output_dir: str = os.getcwd()) -> Tuple[str, str]:
    """Reduce data to test calibration

    If a customized group workspace is specified, the native 3-bank will be still focused and saved.
    But the return value will be focused on the customized groups

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
    customized_group_ws_name: str, None
        Name of customized GroupWorkspace (other than standard 3 banks)
    output_dir: str
        Directory for output files

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
                                         customized_group_ws_name,
                                         output_dir=output_dir)

    return focused_tuple
