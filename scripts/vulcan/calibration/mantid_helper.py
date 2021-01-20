from typing import Union
from mantid.api import AnalysisDataService as ADS
from mantid.simpleapi import (ConvertUnits, CreateWorkspace, LoadNexusLogs, Load, LoadEventNexus,
                              LoadDetectorsGroupingFile)
import os


def mtd_convert_units(ws_name: str,
                      target_unit: str,
                      out_ws_name: Union[str, None]):
    """
    Convert the unit of a workspace.
    Guarantees: if the original workspace is point data, then the output must be point data
    :param ws_name:
    :param target_unit:
    :param out_ws_name: str, None
        output workspace name
    :return:
    """
    # Check inputs
    if not workspace_does_exist(ws_name):
        raise RuntimeError('Workspace {} does not exist in ADS'.format(ws_name))

    if out_ws_name is None:
        out_ws_name = ws_name

    # Correct target unit
    if target_unit.lower() == 'd' or target_unit.lower().count('spac') == 1:
        target_unit = 'dSpacing'
    elif target_unit.lower() == 'tof':
        target_unit = 'TOF'
    elif target_unit.lower() == 'q':
        target_unit = 'MomentumTransfer'

    workspace = retrieve_workspace(ws_name, True)
    if workspace.id() == 'WorkspaceGroup':
        workspace = workspace[0]
    if workspace.getAxis(0).getUnit().unitID() == target_unit:
        print('[INFO] Workspace {} has unit {} already. No need to convert'.format(ws_name, target_unit))
        return

    # Convert to Histogram, convert unit (must work on histogram) and convert back to point data
    ConvertUnits(InputWorkspace=ws_name,
                 OutputWorkspace=out_ws_name,
                 Target=target_unit,
                 EMode='Elastic',
                 ConvertFromPointData=True)

    # Check output
    if not workspace_does_exist(out_ws_name):
        raise RuntimeError('Output workspace {0} cannot be retrieved!'.format(ws_name))

    return


def load_calibration_file(calib_file_name: str,
                          output_name: str,
                          ref_ws_name: str,
                          load_cal=False):
    """

    Note:
    - output_name:  this is NOT calibration workspace name but a base name for multiple calibration-related
    workspaces

    Parameters
    ----------
    calib_file_name
    output_name: str
        base name for calib, mask and group
    ref_ws_name
    load_cal

    Returns
    -------
    ~tuple
        1) output workspaces (2) output offsets workspace (as LoadDiffCal_returns cannot have an arbitrary member)

    """
    # check
    assert os.path.exists(calib_file_name)

    # determine file names
    if calib_file_name.endswith('.h5'):
        diff_cal_file = calib_file_name
    else:
        raise RuntimeError('Calibration file {} does not end with .h5 or .dat.  Unable to support'
                           ''.format(calib_file_name))

    # Load files: new diff calib file
    outputs = mantidapi.LoadDiffCal(InputWorkspace=ref_ws_name,
                                    Filename=diff_cal_file,
                                    WorkspaceName=output_name)

    # # set up output
    # if outputs is None:
    #     outputs = outputs_cal
    #     offset_ws = outputs_cal.OutputOffsetsWorkspace
    # elif outputs_cal is not None:
    #     offset_ws = outputs_cal.OutputOffsetsWorkspace
    # else:
    #     offset_ws = None

    return outputs


def load_grouping_file(grouping_file_name: str,
                       grouping_ws_name: str):
    """
    Load a detector grouping file (saved by SaveDetectorsGroupingFile) to a GroupingWorkspace
    :param grouping_file_name:
    :param grouping_ws_name:
    :return:
    """
    # check input
    # datatypeutility.check_file_name(grouping_file_name, check_exist=True,
    #                                 check_writable=False, is_dir=False, note='Detector grouping file')
    # datatypeutility.check_string_variable('Nmae of GroupingWorkspace to load {} to'.format(grouping_file_name),
    #                                       grouping_ws_name)

    LoadDetectorsGroupingFile(InputFile=grouping_file_name,  OutputWorkspace=grouping_ws_name)

    return


def load_nexus(data_file_name, output_ws_name, meta_data_only, max_time=None):
    """ Load NeXus file
    :param data_file_name:
    :param output_ws_name:
    :param meta_data_only:
    :param max_time: relative max time (stop time) to load from Event Nexus file in unit of seconds
    :return: 2-tuple
    """
    if meta_data_only:
        # load logs to an empty workspace
        out_ws = CreateWorkspace(DataX=[0], DataY=[0], DataE=[
                                           0], NSpec=1, OutputWorkspace=output_ws_name)
        try:
            LoadNexusLogs(Workspace=output_ws_name,
                          Filename=data_file_name, OverwriteLogs=True)
        except RuntimeError as run_err:
            return False, 'Unable to load Nexus (log) file {} due to {}'.format(data_file_name, run_err)
    else:
        # regular workspace with data
        try:
            if not data_file_name.endswith('.h5'):
                out_ws = Load(Filename=data_file_name,
                              OutputWorkspace=output_ws_name,
                              MetaDataOnly=False)
            elif max_time is None:
                out_ws = LoadEventNexus(Filename=data_file_name,
                                        OutputWorkspace=output_ws_name,
                                        MetaDataOnly=False)
            else:
                out_ws = LoadEventNexus(Filename=data_file_name,
                                        OutputWorkspace=output_ws_name,
                                        FilterByTimeStop=max_time)

        except RuntimeError as e:
            return False, 'Unable to load Nexus file %s due to %s' % (data_file_name, str(e))
    # END-IF-ELSE

    return True, out_ws


def retrieve_workspace(ws_name: str,
                       raise_if_not_exist: bool = False):
    """
    Retrieve workspace from AnalysisDataService
    Purpose:
        Get workspace from Mantid's analysis data service

    Requirements:
        workspace name is a string
    Guarantee:
        return the reference to the workspace or None if it does not exist
    :param ws_name:
    :param raise_if_not_exist:
    :return: workspace instance (1)
    """
    assert isinstance(ws_name, str), 'Input ws_name %s is not of type string, but of type %s.' % (str(ws_name),
                                                                                                  str(type(
                                                                                                      ws_name)))

    if ADS.doesExist(ws_name) is False:
        if raise_if_not_exist:
            raise RuntimeError('ADS does not exist workspace named as {0}.'.format(ws_name))
        else:
            return None

    return ADS.retrieve(ws_name)


def workspace_does_exist(workspace_name):
    """ Check whether a workspace exists in analysis data service by its name
    Requirements: input workspace name must be a non-empty string
    :param workspace_name:
    :return: boolean
    """
    # Check
    assert isinstance(workspace_name, str), 'Workspace name must be string but not %s.' % str(
        type(workspace_name))
    assert len(workspace_name) > 0, 'It is impossible to for a workspace with empty string as name.'

    #
    does_exist = ADS.doesExist(workspace_name)

    return does_exist

