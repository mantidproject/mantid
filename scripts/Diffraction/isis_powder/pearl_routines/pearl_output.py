from __future__ import (absolute_import, division, print_function)

import mantid.simpleapi as mantid

import isis_powder.routines.common as common

# This file generates the various outputs for the PEARL instruments and saves them to their respective files


def generate_and_save_focus_output(instrument, processed_spectra, run_details, perform_attenuation, focus_mode=None):
    output_file_paths = instrument._generate_out_file_paths(run_details=run_details)
    cycle_information = instrument._get_label_information(run_details.run_number)
    unused, save_range = instrument._get_instrument_alg_save_ranges(cycle_information["instrument_version"])

    if focus_mode == "all":
        processed_nexus_files = _focus_mode_all(output_file_paths, processed_spectra)

    elif focus_mode == "groups":
        processed_nexus_files = _focus_mode_groups(cycle_information, output_file_paths, save_range,
                                                   processed_spectra)

    elif focus_mode == "trans":

        processed_nexus_files = _focus_mode_trans(output_file_paths, perform_attenuation, instrument, processed_spectra)

    elif focus_mode == "mods":

        processed_nexus_files = _focus_mode_mods(output_file_paths, processed_spectra)

    else:
        raise ValueError("Focus mode unknown")

    return processed_nexus_files


def _focus_mode_mods(output_file_paths, calibrated_spectra):
    index = 1
    append = False
    output_list = []
    for ws in calibrated_spectra:

        mantid.SaveGSS(InputWorkspace=ws, Filename=output_file_paths["gss_filename"], Append=append, Bank=index)
        output_name = output_file_paths["output_name"] + "_mod" + str(index)
        dspacing_ws = mantid.ConvertUnits(InputWorkspace=ws, OutputWorkspace=output_name, Target="dSpacing")
        output_list.append(dspacing_ws)
        mantid.SaveNexus(Filename=output_file_paths["nxs_filename"], InputWorkspace=dspacing_ws, Append=append)

        append = True
        index += 1
    return output_list


def _focus_mode_trans(output_file_paths, atten, instrument, calibrated_spectra):
    summed_ws = mantid.CloneWorkspace(InputWorkspace=calibrated_spectra[0])
    for i in range(1, 9):  # Add workspaces 2-9 to workspace 1
        summed_ws = mantid.Plus(LHSWorkspace=summed_ws, RHSWorkspace=calibrated_spectra[i])

    summed_ws = mantid.Scale(InputWorkspace=summed_ws, Factor=0.111111111111111)

    if atten:
        # Clone a workspace which is not attenuated
        no_att = output_file_paths["output_name"] + "_noatten"
        mantid.CloneWorkspace(InputWorkspace=summed_ws, OutputWorkspace=no_att)

        summed_ws = mantid.ConvertUnits(InputWorkspace=summed_ws, Target="dSpacing")
        summed_ws = instrument._attenuate_workspace(summed_ws)
        summed_ws = mantid.ConvertUnits(InputWorkspace=summed_ws, Target="TOF")

    mantid.SaveGSS(InputWorkspace=summed_ws, Filename=output_file_paths["gss_filename"], Append=False, Bank=1)
    mantid.SaveFocusedXYE(InputWorkspace=summed_ws, Filename=output_file_paths["tof_xye_filename"],
                          Append=False, IncludeHeader=False)

    summed_ws = mantid.ConvertUnits(InputWorkspace=summed_ws, Target="dSpacing")

    # Rename to user friendly name:
    summed_ws_name = output_file_paths["output_name"] + "_mods1-9"
    summed_ws = mantid.RenameWorkspace(InputWorkspace=summed_ws, OutputWorkspace=summed_ws_name)

    mantid.SaveFocusedXYE(InputWorkspace=summed_ws, Filename=output_file_paths["dspacing_xye_filename"],
                          Append=False, IncludeHeader=False)
    mantid.SaveNexus(InputWorkspace=summed_ws, Filename=output_file_paths["nxs_filename"], Append=False)

    output_list = [summed_ws]

    for i in range(0, 9):
        workspace_name = output_file_paths["output_name"] + "_mod" + str(i + 1)
        to_save = mantid.ConvertUnits(InputWorkspace=calibrated_spectra[i], Target="dSpacing",
                                      OutputWorkspace=workspace_name)
        output_list.append(to_save)
        mantid.SaveNexus(Filename=output_file_paths["nxs_filename"], InputWorkspace=to_save, Append=True)

    return output_list


def _focus_mode_groups(cycle_information, output_file_paths, save_range, calibrated_spectra):
    output_list = []
    to_save = _sum_groups_of_three_ws(calibrated_spectra, output_file_paths)

    workspaces_4_to_9_name = output_file_paths["output_name"] + "_mods4-9"
    workspaces_4_to_9 = mantid.Plus(LHSWorkspace=to_save[1], RHSWorkspace=to_save[2])
    workspaces_4_to_9 = mantid.Scale(InputWorkspace=workspaces_4_to_9, Factor=0.5,
                                     OutputWorkspace=workspaces_4_to_9_name)
    to_save.append(workspaces_4_to_9)
    append = False
    index = 1
    for ws in to_save:
        if cycle_information["instrument_version"] == "new":
            mantid.SaveGSS(InputWorkspace=ws, Filename=output_file_paths["gss_filename"], Append=append,
                           Bank=index)
        elif cycle_information["instrument_version"] == "new2":
            mantid.SaveGSS(InputWorkspace=ws, Filename=output_file_paths["gss_filename"], Append=False,
                           Bank=index)

        workspace_names = ws.name()
        dspacing_ws = mantid.ConvertUnits(InputWorkspace=ws, OutputWorkspace=workspace_names, Target="dSpacing")
        common.remove_intermediate_workspace(ws)
        output_list.append(dspacing_ws)
        mantid.SaveNexus(Filename=output_file_paths["nxs_filename"], InputWorkspace=dspacing_ws, Append=append)
        append = True
        index += 1

    for i in range(0, save_range):
        monitor_ws_name = output_file_paths["output_name"] + "_mod" + str(i + 10)

        monitor_ws = calibrated_spectra[i + 9]
        to_save = mantid.CloneWorkspace(InputWorkspace=monitor_ws, OutputWorkspace=monitor_ws_name)

        mantid.SaveGSS(InputWorkspace=to_save, Filename=output_file_paths["gss_filename"], Append=True, Bank=i + 5)
        to_save = mantid.ConvertUnits(InputWorkspace=to_save, OutputWorkspace=monitor_ws_name, Target="dSpacing")
        mantid.SaveNexus(Filename=output_file_paths["nxs_filename"], InputWorkspace=to_save, Append=True)

        output_list.append(to_save)

    return output_list


def _focus_mode_all(output_file_paths, calibrated_spectra):
    first_spectrum = calibrated_spectra[0]
    summed_spectra = mantid.CloneWorkspace(InputWorkspace=first_spectrum)

    for i in range(1, 9):
        summed_spectra = mantid.Plus(LHSWorkspace=summed_spectra, RHSWorkspace=calibrated_spectra[i])

    summed_spectra_name = output_file_paths["output_name"] + "_mods1-9"

    summed_spectra = mantid.Scale(InputWorkspace=summed_spectra, Factor=0.111111111111111,
                                  OutputWorkspace=summed_spectra_name)
    mantid.SaveGSS(InputWorkspace=summed_spectra, Filename=output_file_paths["gss_filename"], Append=False, Bank=1)

    summed_spectra = mantid.ConvertUnits(InputWorkspace=summed_spectra, Target="dSpacing",
                                         OutputWorkspace=summed_spectra_name)
    mantid.SaveNexus(Filename=output_file_paths["nxs_filename"], InputWorkspace=summed_spectra, Append=False)

    output_list = [summed_spectra]
    for i in range(0, 3):
        spectra_index = (i + 9)  # We want workspaces 10/11/12 so compensate for 0 based index
        ws_to_save = calibrated_spectra[spectra_index]  # Save out workspaces 10/11/12
        output_name = output_file_paths["output_name"] + "_mod" + str(spectra_index + 1)
        mantid.SaveGSS(InputWorkspace=ws_to_save, Filename=output_file_paths["gss_filename"], Append=True, Bank=i + 2)
        ws_to_save = mantid.ConvertUnits(InputWorkspace=ws_to_save, OutputWorkspace=output_name, Target="dSpacing")
        output_list.append(ws_to_save)
        mantid.SaveNexus(Filename=output_file_paths["nxs_filename"], InputWorkspace=ws_to_save, Append=True)

    return output_list


def _sum_groups_of_three_ws(calibrated_spectra, output_file_names):
    workspace_list = []
    output_list = []
    for outer_loop_count in range(0, 3):
        # First clone workspaces 1/4/7
        pass_multiplier = (outer_loop_count * 3)
        workspace_names = "focus_mode_groups-" + str(pass_multiplier + 1)
        workspace_list.append(mantid.CloneWorkspace(InputWorkspace=calibrated_spectra[pass_multiplier],
                                                    OutputWorkspace=workspace_names))
        # Then add workspaces 1+2+3 / 4+5+6 / 7+8+9
        for i in range(1, 3):
            input_ws_index = i + pass_multiplier  # Workspaces 2/3 * n
            inner_workspace_names = "focus_mode_groups-" + str(input_ws_index)
            workspace_list[outer_loop_count] = mantid.Plus(LHSWorkspace=workspace_list[outer_loop_count],
                                                           RHSWorkspace=calibrated_spectra[input_ws_index],
                                                           OutputWorkspace=inner_workspace_names)

        # Finally scale the output workspaces
        mod_first_number = str((outer_loop_count * 3) + 1)  # Generates 1/4/7
        mod_last_number = str((outer_loop_count + 1) * 3)  # Generates 3/6/9
        workspace_names = output_file_names["output_name"] + "_mod" + mod_first_number + '-' + mod_last_number
        output_list.append(mantid.Scale(InputWorkspace=workspace_list[outer_loop_count],
                                        OutputWorkspace=workspace_names, Factor=0.333333333333))
    for ws in workspace_list:
        common.remove_intermediate_workspace(ws)
    return output_list
