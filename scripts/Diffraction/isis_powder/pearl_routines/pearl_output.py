# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import mantid.simpleapi as mantid
from mantid.kernel import logger

from isis_powder.routines.common_enums import WORKSPACE_UNITS
from isis_powder.pearl_routines import pearl_algs

# This file generates the various outputs for the PEARL instruments and saves them to their respective files


def generate_and_save_focus_output(instrument, processed_spectra, run_details, attenuation_filepath, focus_mode):
    output_file_paths = instrument._generate_out_file_paths(run_details=run_details)
    if instrument._inst_settings.tt_mode == "custom" and len(processed_spectra) != 14:
        logger.warning("Custom grouping file does not contain 14 groups so switching to focus_mode=Mods")
        processed_nexus_files = _focus_mode_mods(output_file_paths=output_file_paths, calibrated_spectra=processed_spectra)
    elif focus_mode == "all":
        processed_nexus_files = _focus_mode_all(
            output_file_paths=output_file_paths, processed_spectra=processed_spectra, attenuation_filepath=attenuation_filepath
        )
    elif focus_mode == "groups":
        processed_nexus_files = _focus_mode_groups(output_file_paths=output_file_paths, calibrated_spectra=processed_spectra)
    elif "trans" in focus_mode:
        imods, suffix = instrument.get_trans_module_indices()
        processed_nexus_files = _focus_mode_trans(
            output_file_paths=output_file_paths,
            calibrated_spectra=processed_spectra,
            attenuation_filepath=attenuation_filepath,
            imods=imods,
            suffix=suffix,
        )
    elif focus_mode == "mods":
        processed_nexus_files = _focus_mode_mods(output_file_paths=output_file_paths, calibrated_spectra=processed_spectra)
    else:
        raise ValueError("Focus mode '" + str(focus_mode) + "' unknown.")

    return processed_nexus_files


def _focus_mode_all(output_file_paths, processed_spectra, attenuation_filepath):
    summed_spectra_name = output_file_paths["output_name"] + "_mods1-9"
    summed_spectra = mantid.MergeRuns(InputWorkspaces=processed_spectra[:9], OutputWorkspace=summed_spectra_name)
    xList = summed_spectra.readX(0)

    summed_spectra = mantid.CropWorkspace(InputWorkspace=summed_spectra, XMin=xList[1], Xmax=xList[-2], OutputWorkspace=summed_spectra_name)
    summed_spectra = mantid.Scale(InputWorkspace=summed_spectra, Factor=0.111111111111111, OutputWorkspace=summed_spectra_name)
    if attenuation_filepath:
        summed_spectra = pearl_algs.attenuate_workspace(attenuation_file_path=attenuation_filepath, ws_to_correct=summed_spectra)

    summed_spectra = mantid.ConvertUnits(InputWorkspace=summed_spectra, Target="TOF", OutputWorkspace=summed_spectra_name)

    mantid.SaveGSS(InputWorkspace=summed_spectra, Filename=output_file_paths["gss_filename"], Append=False, Bank=1)
    mantid.SaveFocusedXYE(
        InputWorkspace=summed_spectra_name,
        Filename=output_file_paths["tof_xye_filename"],
        Append=False,
        IncludeHeader=False,
        SplitFiles=False,
    )

    summed_spectra = mantid.ConvertUnits(InputWorkspace=summed_spectra, Target="dSpacing", OutputWorkspace=summed_spectra_name)
    mantid.SaveNexus(Filename=output_file_paths["nxs_filename"], InputWorkspace=summed_spectra, Append=False)

    output_list = [summed_spectra]
    for i in range(0, 5):
        spectra_index = i + 9  # Compensate for 0 based index
        ws_to_save = processed_spectra[spectra_index]  # Save out workspaces 10/11/12
        output_name = output_file_paths["output_name"] + "_mod" + str(spectra_index + 1)

        ws_to_save = mantid.ConvertUnits(InputWorkspace=ws_to_save, OutputWorkspace=ws_to_save, Target="TOF")
        mantid.SaveGSS(InputWorkspace=ws_to_save, Filename=output_file_paths["gss_filename"], Append=True, Bank=i + 2)
        splits = output_file_paths["tof_xye_filename"].split(".")
        tof_xye_name = splits[0] + "-" + str(i + 10) + "." + splits[1]
        mantid.SaveFocusedXYE(InputWorkspace=ws_to_save, Filename=tof_xye_name, Append=False, IncludeHeader=False, SplitFiles=False)
        ws_to_save = mantid.ConvertUnits(InputWorkspace=ws_to_save, OutputWorkspace=output_name, Target="dSpacing")
        mantid.SaveNexus(Filename=output_file_paths["nxs_filename"], InputWorkspace=ws_to_save, Append=True)

        output_list.append(ws_to_save)

    return output_list


def _focus_mode_groups(output_file_paths, calibrated_spectra):
    output_list = []
    to_save = _sum_groups_of_three_ws(calibrated_spectra=calibrated_spectra, output_file_names=output_file_paths)

    workspaces_4_to_9_name = output_file_paths["output_name"] + "_mods4-9"
    workspaces_4_to_9 = mantid.MergeRuns(InputWorkspaces=calibrated_spectra[3:9], OutputWorkspace=workspaces_4_to_9_name)
    xList = workspaces_4_to_9.readX(0)

    workspaces_4_to_9 = mantid.CropWorkspace(InputWorkspace=workspaces_4_to_9, XMin=xList[1], Xmax=xList[-2])
    workspaces_4_to_9 = mantid.Scale(InputWorkspace=workspaces_4_to_9, Factor=0.5, OutputWorkspace=workspaces_4_to_9_name)
    to_save.append(workspaces_4_to_9)
    append = False
    index = 1
    for ws in to_save:
        ws = mantid.ConvertUnits(InputWorkspace=ws, OutputWorkspace=ws, Target="TOF")
        mantid.SaveGSS(InputWorkspace=ws, Filename=output_file_paths["gss_filename"], Append=False, Bank=index)
        mantid.SaveFocusedXYE(
            InputWorkspace=ws, Filename=output_file_paths["tof_xye_filename"], Append=False, IncludeHeader=False, SplitFiles=False
        )
        workspace_names = ws.name()
        ws = mantid.ConvertUnits(InputWorkspace=ws, OutputWorkspace=workspace_names, Target="dSpacing")
        output_list.append(ws)
        mantid.SaveNexus(Filename=output_file_paths["nxs_filename"], InputWorkspace=ws, Append=append)
        append = True
        index += 1

    save_range = 5
    for i in range(0, save_range):
        monitor_ws_name = output_file_paths["output_name"] + "_mod" + str(i + 10)

        monitor_ws = calibrated_spectra[i + 9]
        to_save = mantid.CloneWorkspace(InputWorkspace=monitor_ws, OutputWorkspace=monitor_ws_name)

        to_save = mantid.ConvertUnits(InputWorkspace=to_save, OutputWorkspace=to_save, Target="TOF")
        splits = output_file_paths["tof_xye_filename"].split(".")
        tof_xye_name = splits[0] + "-" + str(i + 10) + "." + splits[1]
        mantid.SaveGSS(InputWorkspace=to_save, Filename=output_file_paths["gss_filename"], Append=True, Bank=i + 5)
        mantid.SaveFocusedXYE(InputWorkspace=to_save, Filename=tof_xye_name, Append=False, IncludeHeader=False, SplitFiles=False)
        to_save = mantid.ConvertUnits(InputWorkspace=to_save, OutputWorkspace=monitor_ws_name, Target="dSpacing")
        mantid.SaveNexus(Filename=output_file_paths["nxs_filename"], InputWorkspace=to_save, Append=True)

        output_list.append(to_save)

    return output_list


def _focus_mode_mods(output_file_paths, calibrated_spectra):
    append = False
    output_list = []
    for index, ws in enumerate(calibrated_spectra):
        output_name = output_file_paths["output_name"] + "_mod" + str(index + 1)
        tof_ws = mantid.ConvertUnits(InputWorkspace=ws, OutputWorkspace=output_name, Target=WORKSPACE_UNITS.tof)
        mantid.SaveFocusedXYE(
            InputWorkspace=tof_ws, Filename=output_file_paths["tof_xye_filename"], Append=False, IncludeHeader=False, SplitFiles=False
        )
        mantid.SaveGSS(InputWorkspace=tof_ws, Filename=output_file_paths["gss_filename"], Append=append, Bank=index + 1)
        dspacing_ws = mantid.ConvertUnits(InputWorkspace=ws, OutputWorkspace=output_name, Target=WORKSPACE_UNITS.d_spacing)
        output_list.append(dspacing_ws)
        mantid.SaveNexus(Filename=output_file_paths["nxs_filename"], InputWorkspace=dspacing_ws, Append=append)

        append = True
    return output_list


def _focus_mode_trans(output_file_paths, attenuation_filepath, calibrated_spectra, imods, suffix):
    summed_ws = mantid.MergeRuns(InputWorkspaces=[calibrated_spectra[imod] for imod in imods])
    xList = summed_ws.readX(0)

    summed_ws = mantid.CropWorkspace(InputWorkspace=summed_ws, XMin=xList[1], Xmax=xList[-2])
    summed_ws = mantid.Scale(InputWorkspace=summed_ws, Factor=1.0 / len(imods))

    if attenuation_filepath:
        summed_ws = pearl_algs.attenuate_workspace(attenuation_file_path=attenuation_filepath, ws_to_correct=summed_ws)

    summed_ws = mantid.ConvertUnits(InputWorkspace=summed_ws, Target="TOF")
    mantid.SaveGSS(InputWorkspace=summed_ws, Filename=output_file_paths["gss_filename"], Append=False, Bank=1)

    mantid.SaveFocusedXYE(
        InputWorkspace=summed_ws, Filename=output_file_paths["tof_xye_filename"], Append=False, IncludeHeader=False, SplitFiles=False
    )

    summed_ws = mantid.ConvertUnits(InputWorkspace=summed_ws, Target="dSpacing")

    # Rename to user friendly name:
    summed_ws_name = output_file_paths["output_name"] + "_mods" + suffix
    summed_ws = mantid.RenameWorkspace(InputWorkspace=summed_ws, OutputWorkspace=summed_ws_name)
    mantid.SaveFocusedXYE(
        InputWorkspace=summed_ws, Filename=output_file_paths["dspacing_xye_filename"], Append=False, IncludeHeader=False, SplitFiles=False
    )
    mantid.SaveNexus(InputWorkspace=summed_ws, Filename=output_file_paths["nxs_filename"], Append=False)

    output_list = [summed_ws]

    for imod in imods:
        workspace_name = output_file_paths["output_name"] + "_mod" + str(imod + 1)
        to_save = mantid.ConvertUnits(InputWorkspace=calibrated_spectra[imod], Target="dSpacing", OutputWorkspace=workspace_name)
        output_list.append(to_save)
        mantid.SaveNexus(Filename=output_file_paths["nxs_filename"], InputWorkspace=to_save, Append=True)

    return output_list


def _sum_groups_of_three_ws(calibrated_spectra, output_file_names):
    output_list = []

    for i in range(3):
        ws_name = output_file_names["output_name"] + "_mods{}-{}".format(i * 3 + 1, (i + 1) * 3)
        summed_spectra = mantid.MergeRuns(InputWorkspaces=calibrated_spectra[i * 3 : (i + 1) * 3], OutputWorkspace=ws_name)
        xList = summed_spectra.readX(0)

        summed_spectra = mantid.CropWorkspace(InputWorkspace=summed_spectra, XMin=xList[1], Xmax=xList[-2])
        scaled = mantid.Scale(InputWorkspace=summed_spectra, Factor=1.0 / 3, OutputWorkspace=ws_name)
        output_list.append(scaled)

    return output_list
