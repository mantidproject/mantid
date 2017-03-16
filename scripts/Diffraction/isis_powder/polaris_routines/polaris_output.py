from __future__ import (absolute_import, division, print_function)

import mantid.simpleapi as mantid
import os


def save_polaris_focused_data(d_spacing_group, tof_group, output_paths, run_number_string):
    mantid.SaveGSS(InputWorkspace=tof_group, Filename=output_paths["gss_filename"], SplitFiles=False, Append=False)
    mantid.SaveNexusProcessed(InputWorkspace=tof_group, Filename=output_paths["nxs_filename"], Append=False)

    dat_folder_name = "dat_files"
    dat_file_destination = os.path.join(output_paths["output_folder"], dat_folder_name)
    if not os.path.exists(dat_file_destination):
        os.makedirs(dat_file_destination)

    _save_xye(ws_group=d_spacing_group, ws_units="d", run_number=run_number_string,
              output_folder=dat_file_destination)
    _save_xye(ws_group=tof_group, ws_units="TOF", run_number=run_number_string,
              output_folder=dat_file_destination)


def _save_xye(ws_group, ws_units, run_number, output_folder):
    bank_index = 1
    prefix_filename = "POL" + str(run_number)
    for ws in ws_group:
        outfile_name = prefix_filename + "-b_" + str(bank_index) + "-" + ws_units + ".dat"
        bank_index += 1
        full_file_path = os.path.join(output_folder, outfile_name)

        mantid.SaveFocusedXYE(InputWorkspace=ws, Filename=full_file_path, SplitFiles=False, IncludeHeader=False)
