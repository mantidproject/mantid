from __future__ import (absolute_import, division, print_function)

import mantid.simpleapi as mantid
import os


def split_into_tof_d_spacing_groups(run_details, processed_spectra):
    d_spacing_output = []
    tof_output = []
    run_number = str(run_details.output_run_string)
    for name_index, ws in enumerate(processed_spectra):
        d_spacing_out_name = run_number + "-ResultD-" + str(name_index + 1)
        tof_out_name = run_number + "-ResultTOF-" + str(name_index + 1)

        d_spacing_output.append(mantid.ConvertUnits(InputWorkspace=ws, OutputWorkspace=d_spacing_out_name,
                                                    Target="dSpacing"))
        tof_output.append(mantid.ConvertUnits(InputWorkspace=ws, OutputWorkspace=tof_out_name, Target="TOF"))

    # Group the outputs
    d_spacing_group_name = run_number + "-Results-D-Grp"
    d_spacing_group = mantid.GroupWorkspaces(InputWorkspaces=d_spacing_output, OutputWorkspace=d_spacing_group_name)
    tof_group_name = run_number + "-Results-TOF-Grp"
    tof_group = mantid.GroupWorkspaces(InputWorkspaces=tof_output, OutputWorkspace=tof_group_name)

    return d_spacing_group, tof_group


def save_focused_data(d_spacing_group, tof_group, output_paths, run_number_string, inst_prefix):
    mantid.SaveGSS(InputWorkspace=tof_group, Filename=output_paths["gss_filename"], SplitFiles=False, Append=False)
    mantid.SaveNexusProcessed(InputWorkspace=tof_group, Filename=output_paths["nxs_filename"], Append=False)

    dat_folder_name = "dat_files"
    dat_file_destination = os.path.join(output_paths["output_folder"], dat_folder_name)
    if not os.path.exists(dat_file_destination):
        os.makedirs(dat_file_destination)

    _save_xye(ws_group=d_spacing_group, ws_units="d", run_number=run_number_string,
              output_folder=dat_file_destination, inst_prefix=inst_prefix)
    _save_xye(ws_group=tof_group, ws_units="TOF", run_number=run_number_string,
              output_folder=dat_file_destination, inst_prefix=inst_prefix)


def _save_xye(ws_group, ws_units, run_number, output_folder, inst_prefix):
    bank_index = 1
    prefix_filename = str(inst_prefix) + str(run_number)
    for ws in ws_group:
        outfile_name = prefix_filename + "-b_" + str(bank_index) + "-" + ws_units + ".dat"
        bank_index += 1
        full_file_path = os.path.join(output_folder, outfile_name)

        mantid.SaveFocusedXYE(InputWorkspace=ws, Filename=full_file_path, SplitFiles=False, IncludeHeader=False)
