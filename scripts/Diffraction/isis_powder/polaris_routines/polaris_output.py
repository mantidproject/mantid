from __future__ import (absolute_import, division, print_function)

import mantid.simpleapi as mantid
import os


def save_polaris_focused_data(d_spacing_group, tof_group, output_paths, run_number):
    mantid.SaveGSS(InputWorkspace=tof_group, Filename=output_paths["gss_filename"], SplitFiles=False, Append=False)
    mantid.SaveNexusProcessed(InputWorkspace=tof_group, Filename=output_paths["nxs_filename"], Append=False)

    _save_xye(ws_group=d_spacing_group, ws_units="d_spacing", run_number=run_number,
              output_folder=output_paths["output_folder"])
    _save_xye(ws_group=tof_group, ws_units="TOF", run_number=run_number,
              output_folder=output_paths["output_folder"])


def _save_xye(ws_group, ws_units, run_number, output_folder):
    bank_index = 1
    for ws in ws_group:
        outfile_name = str(run_number) + "-b_" + str(bank_index) + "-" + ws_units + ".dat"
        bank_index += 1
        full_file_path = os.path.join(output_folder, outfile_name)

        mantid.SaveFocusedXYE(InputWorkspace=ws, Filename=full_file_path, SplitFiles=False, IncludeHeader=False)
