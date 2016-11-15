from __future__ import (absolute_import, division, print_function)
import mantid.simpleapi as mantid

# --- Public API --- #


def focus(number, instrument, attenuate=True, van_norm=True):
    return _run_pearl_focus(run_number=number, perform_attenuation=attenuate,
                            perform_vanadium_norm=van_norm, instrument=instrument)


def create_calibration_by_names(calibration_runs, startup_objects, grouping_file_name, group_names):
    _create_blank_cal_file(calibration_runs=calibration_runs, group_names=group_names,
                           out_grouping_file_name=grouping_file_name, instrument=startup_objects)


def create_vanadium(startup_object, vanadium_runs, empty_runs, output_file_name,
                    num_of_spline_coefficients=60, do_absorb_corrections=True, generate_absorb_corrections=False):
    _create_van(instrument=startup_object, van=vanadium_runs, empty=empty_runs,
                output_van_file_name=output_file_name, num_of_splines=num_of_spline_coefficients,
                absorb=do_absorb_corrections, gen_absorb=generate_absorb_corrections)


def set_debug(debug_on=False):
    global g_debug
    g_debug = debug_on


def remove_intermediate_workspace(workspace_name):
    _remove_ws(ws_to_remove=workspace_name)

# --- Private Implementation --- #

# Please note these functions can change in any way at any time.
# For this reason please do not call them directly and instead use the Public API provided.

# If this doesn't quite provide what you need please let a developer know so we can create
# another API which will not change without notice

# This section is holds several counters which work around the fact that Mantid
# takes the alias and uses it as the WS name and on subsequent calls overrides it
# when the Python API truly implements anonymous pointers this can be removed

_read_pearl_ws_count = 0
global g_ads_workaround
g_ads_workaround = {"read_pearl_ws" : _read_pearl_ws_count}


def _create_blank_cal_file(calibration_runs, out_grouping_file_name, instrument, group_names):
    input_ws = _read_ws(calibration_runs, instrument)
    calibration_d_spacing_ws = mantid.ConvertUnits(InputWorkspace=input_ws, Target="dSpacing")
    mantid.CreateCalFileByNames(InstrumentWorkspace=calibration_d_spacing_ws,
                                GroupingFileName=out_grouping_file_name, GroupNames=group_names)
    remove_intermediate_workspace(calibration_d_spacing_ws)
    remove_intermediate_workspace(input_ws)


def _create_van(instrument, van, empty, output_van_file_name, num_of_splines=60, absorb=True, gen_absorb=False):
    cycle_information = instrument._get_cycle_information(van)

    input_van_ws = _read_ws(number=van, instrument=instrument)
    input_empty_ws = _read_ws(number=empty, instrument=instrument)

    corrected_van_ws = mantid.Minus(LHSWorkspace=input_van_ws, RHSWorkspace=input_empty_ws)

    remove_intermediate_workspace(input_empty_ws)
    remove_intermediate_workspace(input_van_ws)

    calibration_full_paths = instrument._get_calibration_full_paths(cycle=cycle_information["cycle"])
    tof_binning = instrument._get_create_van_tof_binning()

    if absorb:
        corrected_van_ws = _apply_absorb_corrections(calibration_full_paths, corrected_van_ws, gen_absorb)

    corrected_van_ws = mantid.ConvertUnits(InputWorkspace=corrected_van_ws, Target="TOF")
    corrected_van_ws = mantid.Rebin(InputWorkspace=corrected_van_ws, Params=tof_binning["1"])

    corrected_van_ws = mantid.AlignDetectors(InputWorkspace=corrected_van_ws,
                                             CalibrationFile=calibration_full_paths["calibration"])

    focused_van_file = mantid.DiffractionFocussing(InputWorkspace=corrected_van_ws,
                                                   GroupingFileName=calibration_full_paths["grouping"])

    focused_van_file = mantid.ConvertUnits(InputWorkspace=focused_van_file, Target="TOF")

    focused_van_file = mantid.Rebin(InputWorkspace=focused_van_file, Params=tof_binning["2"])
    focused_van_file = mantid.ConvertUnits(InputWorkspace=focused_van_file, Target="dSpacing")

    remove_intermediate_workspace(corrected_van_ws)

    splined_ws_list = instrument._spline_background(focused_van_file, num_of_splines,
                                                    cycle_information["instrument_version"])

    if instrument._PEARL_use_full_path():
        out_van_file_path = output_van_file_name
    else:
        out_van_file_path = instrument.calibration_dir + output_van_file_name

    append = False
    for ws in splined_ws_list:
        mantid.SaveNexus(Filename=out_van_file_path, InputWorkspace=ws, Append=append)
        remove_intermediate_workspace(ws)
        append = True

    mantid.LoadNexus(Filename=out_van_file_path, OutputWorkspace="Van_data")


def _apply_absorb_corrections(calibration_full_paths, corrected_van_ws, gen_absorb):
    corrected_van_ws = mantid.ConvertUnits(InputWorkspace=corrected_van_ws, Target="Wavelength")

    if gen_absorb:
        raise NotImplementedError("Generating absorption corrections is not currently working correctly")
        # TODO look into this and see what is missing from the original script based on the current
        # TODO generated NXS file history
        absorb_ws = _generate_vanadium_absorb_corrections(calibration_full_paths, corrected_van_ws)
    else:
        absorb_ws = _load_van_absorb_corr(calibration_full_paths)

    corrected_van_ws = mantid.RebinToWorkspace(WorkspaceToRebin=corrected_van_ws, WorkspaceToMatch=absorb_ws)
    corrected_van_ws = mantid.Divide(LHSWorkspace=corrected_van_ws, RHSWorkspace=absorb_ws)
    remove_intermediate_workspace(absorb_ws)
    return corrected_van_ws


def _generate_vanadium_absorb_corrections(calibration_full_paths, ws_to_match):
    # TODO are these values applicable to all instruments
    shape_ws = mantid.CloneWorkspace(InputWorkspace=ws_to_match)
    mantid.CreateSampleShape(InputWorkspace=shape_ws, ShapeXML='<sphere id="sphere_1"> <centre x="0" y="0" z= "0" />\
                                                      <radius val="0.005" /> </sphere>')

    absorb_ws = \
        mantid.AbsorptionCorrection(InputWorkspace=shape_ws, AttenuationXSection="5.08",
                                    ScatteringXSection="5.1", SampleNumberDensity="0.072",
                                    NumberOfWavelengthPoints="25", ElementSize="0.05")
    mantid.SaveNexus(Filename=calibration_full_paths["vanadium_absorption"],
                     InputWorkspace=absorb_ws, Append=False)
    remove_intermediate_workspace(shape_ws)
    return absorb_ws


def _load_van_absorb_corr(calibration_full_paths):
    absorption_ws = mantid.LoadNexus(Filename=calibration_full_paths["vanadium_absorption"])
    return absorption_ws


def _load_monitor(number, input_dir, instrument):
    if isinstance(number, int):
        full_file_path = instrument._generate_input_full_path(run_number=number, input_dir=input_dir)
        mspectra = instrument._get_monitor_spectra(number)
        load_monitor_ws = mantid.LoadRaw(Filename=full_file_path, SpectrumMin=mspectra, SpectrumMax=mspectra,
                                         LoadLogFiles="0")
    else:
        load_monitor_ws = _load_monitor_sum_range(files=number, input_dir=input_dir, instrument=instrument)

    return load_monitor_ws


def _load_monitor_sum_range(files, input_dir, instrument):
    loop = 0
    num = files.split("_")
    frange = list(range(int(num[0]), int(num[1]) + 1))
    mspectra = instrument._get_monitor_spectra(int(num[0]))
    for i in frange:
        file_path = instrument._generate_input_full_path(i, input_dir)
        outwork = "mon" + str(i)
        mantid.LoadRaw(Filename=file_path, OutputWorkspace=outwork, SpectrumMin=mspectra, SpectrumMax=mspectra,
                       LoadLogFiles="0")
        loop += 1
        if loop == 2:
            firstwk = "mon" + str(i - 1)
            secondwk = "mon" + str(i)
            load_monitor_summed = mantid.Plus(LHSWorkspace=firstwk, RHSWorkspace=secondwk)
            mantid.mtd.remove(firstwk)
            mantid.mtd.remove(secondwk)
        elif loop > 2:
            secondwk = "mon" + str(i)
            load_monitor_summed = mantid.Plus(LHSWorkspace=load_monitor_summed, RHSWorkspace=secondwk)
            mantid.mtd.remove(secondwk)

    return load_monitor_summed


def _load_raw_files(run_number, instrument, input_dir):
    if isinstance(run_number, int):
        infile = instrument._generate_input_full_path(run_number=run_number, input_dir=input_dir)
        load_raw_ws = mantid.LoadRaw(Filename=infile, LoadLogFiles="0")
    else:
        load_raw_ws = _load_raw_file_range(run_number, input_dir, instrument)
    return load_raw_ws


def _load_raw_file_range(files, input_dir, instrument):
    loop = 0
    num = files.split("_")
    frange = list(range(int(num[0]), int(num[1]) + 1))
    out_ws = None
    for i in frange:
        file_path = instrument._generate_input_full_path(i, input_dir)
        outwork = "run" + str(i)
        mantid.LoadRaw(Filename=file_path, OutputWorkspace=outwork, LoadLogFiles="0")
        loop += 1
        if loop == 2:
            firstwk = "run" + str(i - 1)
            secondwk = "run" + str(i)
            out_ws = mantid.Plus(LHSWorkspace=firstwk, RHSWorkspace=secondwk)
            mantid.mtd.remove(firstwk)
            mantid.mtd.remove(secondwk)
        elif loop > 2:
            secondwk = "run" + str(i)
            out_ws = mantid.Plus(LHSWorkspace=out_ws, RHSWorkspace=secondwk)
            mantid.mtd.remove(secondwk)
    return out_ws


def _read_ws(number, instrument):
    cycle_information = instrument._get_cycle_information(run_number=number)
    input_dir = instrument._generate_raw_data_cycle_dir(cycle_information["cycle"])
    read_in_ws = _load_raw_files(run_number=number, instrument=instrument, input_dir=input_dir)
    # TODO move this into instrument specific
    read_ws = mantid.ConvertUnits(InputWorkspace=read_in_ws, Target="Wavelength")
    remove_intermediate_workspace(read_in_ws)

    _read_pearl_monitor = instrument._get_monitor(run_number=number, input_dir=input_dir, spline_terms=20)

    read_ws = mantid.NormaliseToMonitor(InputWorkspace=read_ws, MonitorWorkspace=_read_pearl_monitor,
                                        IntegrationRangeMin=0.6, IntegrationRangeMax=5.0)
    output_name = "read_ws_output-" + str(g_ads_workaround["read_pearl_ws"])
    g_ads_workaround["read_pearl_ws"] += 1
    output_ws = mantid.ConvertUnits(InputWorkspace=read_ws,
                                    OutputWorkspace=output_name, Target="TOF")

    remove_intermediate_workspace(_read_pearl_monitor)
    remove_intermediate_workspace(read_ws)
    return output_ws


def _run_pearl_focus(instrument, run_number, perform_attenuation, perform_vanadium_norm):

    cycle_information = instrument._get_cycle_information(run_number=run_number)

    alg_range, save_range = instrument._get_instrument_alg_save_ranges(cycle_information["instrument_version"])

    input_file_paths = instrument._get_calibration_full_paths(cycle=cycle_information["cycle"])

    output_file_paths = instrument._generate_out_file_paths(run_number, instrument.output_dir)
    read_ws = _read_ws(number=run_number, instrument=instrument)
    input_workspace = mantid.Rebin(InputWorkspace=read_ws, Params=instrument._get_focus_tof_binning())
    input_workspace = mantid.AlignDetectors(InputWorkspace=input_workspace, CalibrationFile=input_file_paths["calibration"])
    input_workspace = mantid.DiffractionFocussing(InputWorkspace=input_workspace, GroupingFileName=input_file_paths["grouping"])

    calibrated_spectra = _focus_load(alg_range, input_workspace, input_file_paths, instrument, perform_vanadium_norm)

    remove_intermediate_workspace(read_ws)
    remove_intermediate_workspace(input_workspace)

    focus_mode = instrument.focus_mode
    if focus_mode == "all":
        processed_nexus_files = _focus_mode_all(output_file_paths, calibrated_spectra)

    elif focus_mode == "groups":
        processed_nexus_files = _focus_mode_groups(cycle_information, output_file_paths, save_range,
                                                   calibrated_spectra)

    elif focus_mode == "trans":

        processed_nexus_files = _focus_mode_trans(output_file_paths, perform_attenuation, instrument, calibrated_spectra)

    elif focus_mode == "mods":

        processed_nexus_files = _focus_mode_mods(output_file_paths, calibrated_spectra)

    else:
        raise ValueError("Focus mode unknown")

    for ws in calibrated_spectra:
        remove_intermediate_workspace(ws)

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
        remove_intermediate_workspace(ws)
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
        remove_intermediate_workspace(ws)
    return output_list


def _focus_mode_all(output_file_paths, calibrated_spectra):
    first_spectrum = calibrated_spectra[0]
    summed_spectra = mantid.CloneWorkspace(InputWorkspace=first_spectrum)

    for i in range(1, 9):  # TODO why is this 1-8
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


def _focus_load(alg_range, focused_ws, input_file_paths, instrument, van_norm):
    processed_spectra = []

    for index in range(0, alg_range):
        if van_norm:
            vanadium_ws = mantid.LoadNexus(Filename=input_file_paths["vanadium"], EntryNumber=index + 1)
            van_rebinned = mantid.Rebin(InputWorkspace=vanadium_ws, Params=instrument._get_focus_tof_binning())

            processed_spectra.append(calc_calibration_with_vanadium(focused_ws, index, van_rebinned, instrument))

            remove_intermediate_workspace(vanadium_ws)
            remove_intermediate_workspace(van_rebinned)
        else:
            processed_spectra.append(calc_calibration_without_vanadium(focused_ws, index, instrument))

    return processed_spectra


def calc_calibration_without_vanadium(focused_ws, index, instrument):
    focus_spectrum = mantid.ExtractSingleSpectrum(InputWorkspace=focused_ws, WorkspaceIndex=index)
    focus_spectrum = mantid.ConvertUnits(InputWorkspace=focus_spectrum, Target="TOF")
    focus_spectrum = mantid.Rebin(InputWorkspace=focus_spectrum, Params=instrument.tof_binning)
    focus_calibrated = mantid.CropWorkspace(InputWorkspace=focus_spectrum, XMin=0.1)
    return focus_calibrated


def calc_calibration_with_vanadium(focused_ws, index, vanadium_ws, instrument):
    data_ws = mantid.ExtractSingleSpectrum(InputWorkspace=focused_ws, WorkspaceIndex=index)
    data_ws = mantid.ConvertUnits(InputWorkspace=data_ws, Target="TOF")
    data_ws = mantid.Rebin(InputWorkspace=data_ws, Params=instrument._get_focus_tof_binning())

    data_processed = "van_processed" + str(index)  # Workaround for Mantid overwriting the WS in a loop

    mantid.Divide(LHSWorkspace=data_ws, RHSWorkspace=vanadium_ws, OutputWorkspace=data_processed)
    mantid.CropWorkspace(InputWorkspace=data_processed, XMin=0.1, OutputWorkspace=data_processed)
    mantid.Scale(InputWorkspace=data_processed, Factor=10, OutputWorkspace=data_processed)

    remove_intermediate_workspace(data_ws)

    return data_processed


def _remove_ws(ws_to_remove):
    """
    Removes any intermediate workspaces if debug is set to false
        @param ws_to_remove: The workspace to remove from the ADS
    """
    try:
        if not g_debug:
            _remove_ws_wrapper(ws=ws_to_remove)
    except NameError:  # If g_debug has not been set
        _remove_ws_wrapper(ws=ws_to_remove)


def _remove_ws_wrapper(ws):
    mantid.DeleteWorkspace(ws)
    del ws  # Mark it as deleted so that Python can throw before Mantid preserving more information
