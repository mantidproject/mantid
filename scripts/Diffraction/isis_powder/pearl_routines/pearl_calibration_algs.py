from __future__ import (absolute_import, division, print_function)

import os
import mantid.simpleapi as mantid
import isis_powder.routines.common as common
from isis_powder.routines.common_enums import INPUT_BATCHING

# TODO this entire file needs cleaning and refactoring


def create_calibration(self, calibration_runs, offset_file_name, grouping_file_name):
    input_ws_list = common.load_current_normalised_ws_list(run_number_string=calibration_runs, instrument=self,
                                                           input_batching=INPUT_BATCHING.Summed)
    input_ws = input_ws_list[0]
    run_details = self._get_run_details(calibration_runs)

    if run_details.instrument_version == "new" or run_details.instrument_version == "new2":
        input_ws = mantid.Rebin(InputWorkspace=input_ws, Params="100,-0.0006,19950")

    d_spacing_cal = mantid.ConvertUnits(InputWorkspace=input_ws, Target="dSpacing")
    d_spacing_cal = mantid.Rebin(InputWorkspace=d_spacing_cal, Params="1.8,0.002,2.1")

    if run_details.instrument_version == "new2":
        cross_cor_ws = mantid.CrossCorrelate(InputWorkspace=d_spacing_cal, ReferenceSpectra=20,
                                             WorkspaceIndexMin=9, WorkspaceIndexMax=1063, XMin=1.8, XMax=2.1)

    elif run_details.instrument_version == "new":
        cross_cor_ws = mantid.CrossCorrelate(InputWorkspace=d_spacing_cal, ReferenceSpectra=20,
                                             WorkspaceIndexMin=9, WorkspaceIndexMax=943, XMin=1.8, XMax=2.1)
    else:
        cross_cor_ws = mantid.CrossCorrelate(InputWorkspace=d_spacing_cal, ReferenceSpectra=500,
                                             WorkspaceIndexMin=1, WorkspaceIndexMax=1440, XMin=1.8, XMax=2.1)
    if self._old_api_uses_full_paths:  # Workaround for old API setting full paths
        grouping_file_path = grouping_file_name
        offset_file_path = offset_file_name
    else:
        offset_file_path = os.path.join(self.calibration_dir, offset_file_name)
        grouping_file_path = os.path.join(self.calibration_dir, grouping_file_name)

    # Ceo Cell refined to 5.4102(3) so 220 is 1.912795
    offset_output_path = mantid.GetDetectorOffsets(InputWorkspace=cross_cor_ws, Step=0.002, DReference=1.912795,
                                                   XMin=-200, XMax=200, GroupingFileName=offset_file_path)
    del offset_output_path  # This isn't used so delete it to keep linters happy
    aligned_ws = mantid.AlignDetectors(InputWorkspace=input_ws, CalibrationFile=offset_file_path)
    cal_grouped_ws = mantid.DiffractionFocussing(InputWorkspace=aligned_ws, GroupingFileName=grouping_file_path)

    common.remove_intermediate_workspace(d_spacing_cal)
    common.remove_intermediate_workspace(cross_cor_ws)
    common.remove_intermediate_workspace(aligned_ws)
    common.remove_intermediate_workspace(cal_grouped_ws)


def do_silicon_calibration(self, runs_to_process, cal_file_name, grouping_file_name):
    # TODO fix all of this as the script is too limited to be useful
    create_si_ws = common.load_current_normalised_ws_list(run_number_string=runs_to_process, instrument=self)
    cycle_details = self._get_label_information(runs_to_process)
    instrument_version = cycle_details["instrument_version"]

    if instrument_version == "new" or instrument_version == "new2":
        create_si_ws = mantid.Rebin(InputWorkspace=create_si_ws, Params="100,-0.0006,19950")

    create_si_d_spacing_ws = mantid.ConvertUnits(InputWorkspace=create_si_ws, Target="dSpacing")

    if instrument_version == "new2":
        create_si_d_spacing_rebin_ws = mantid.Rebin(InputWorkspace=create_si_d_spacing_ws, Params="1.71,0.002,2.1")
        create_si_cross_corr_ws = mantid.CrossCorrelate(InputWorkspace=create_si_d_spacing_rebin_ws,
                                                        ReferenceSpectra=20, WorkspaceIndexMin=9,
                                                        WorkspaceIndexMax=1063, XMin=1.71, XMax=2.1)
    elif instrument_version == "new":
        create_si_d_spacing_rebin_ws = mantid.Rebin(InputWorkspace=create_si_d_spacing_ws, Params="1.85,0.002,2.05")
        create_si_cross_corr_ws = mantid.CrossCorrelate(InputWorkspace=create_si_d_spacing_rebin_ws,
                                                        ReferenceSpectra=20, WorkspaceIndexMin=9,
                                                        WorkspaceIndexMax=943, XMin=1.85, XMax=2.05)
    elif instrument_version == "old":
        create_si_d_spacing_rebin_ws = mantid.Rebin(InputWorkspace=create_si_d_spacing_ws, Params="3,0.002,3.2")
        create_si_cross_corr_ws = mantid.CrossCorrelate(InputWorkspace=create_si_d_spacing_rebin_ws,
                                                        ReferenceSpectra=500, WorkspaceIndexMin=1,
                                                        WorkspaceIndexMax=1440, XMin=3, XMax=3.2)
    else:
        raise NotImplementedError("The instrument version is not supported for creating a silicon calibration")

    common.remove_intermediate_workspace(create_si_d_spacing_ws)
    common.remove_intermediate_workspace(create_si_d_spacing_rebin_ws)

    calibration_output_path = self.calibration_dir + cal_file_name
    create_si_offsets_ws = mantid.GetDetectorOffsets(InputWorkspace=create_si_cross_corr_ws,
                                                     Step=0.002, DReference=1.920127251, XMin=-200, XMax=200,
                                                     GroupingFileName=calibration_output_path)
    create_si_aligned_ws = mantid.AlignDetectors(InputWorkspace=create_si_ws,
                                                 CalibrationFile=calibration_output_path)
    grouping_output_path = self.calibration_dir + grouping_file_name
    create_si_grouped_ws = mantid.DiffractionFocussing(InputWorkspace=create_si_aligned_ws,
                                                       GroupingFileName=grouping_output_path)
    del create_si_offsets_ws, create_si_grouped_ws
