from __future__ import (absolute_import, division, print_function)
import mantid.simpleapi as mantid

# --- Public API --- #


def focus(number, instrument, fmode="trans", atten=True, van_norm=True):
    # TODO support other extensions
    return _run_pearl_focus(run_number=number, fmode=fmode, atten=atten, van_norm=van_norm,
                            instrument=instrument)


def create_calibration_by_names(calruns, startup_objects, ngroupfile, ngroup):
    _create_blank_cal_file(calibration_runs=calruns, group_names=ngroup, out_grouping_file_name=ngroupfile,
                           instrument=startup_objects)


def create_calibration(startup_object, calibration_runs, offset_file_path, grouping_file_name):
    _create_calibration(calibration_runs=calibration_runs, offset_file_name=offset_file_path,
                        grouping_file_name=grouping_file_name, instrument=startup_object)


def create_vanadium(startup_object, vanadium_runs, empty_runs, output_file_name,
                    num_of_spline_coefficients=60, do_absorp_corrections=True, generate_abosrp_corrections=False):
    _create_van(instrument=startup_object, van=vanadium_runs, empty=empty_runs,
                nvanfile=output_file_name, nspline=num_of_spline_coefficients,
                absorb=do_absorp_corrections, gen_absorb=generate_abosrp_corrections)


def create_calibration_si(startup_object, calibration_runs, out_file_name):
    _create_calibration_si(calibration_runs=calibration_runs, output_file_name=out_file_name,
                           instrument=startup_object)


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

    cycle_information = instrument.get_cycle_information(calibration_runs)

    input_ws = _read_ws(calibration_runs, instrument)
    calibration_d_spacing_ws = mantid.ConvertUnits(InputWorkspace=input_ws, Target="dSpacing")
    mantid.CreateCalFileByNames(InstrumentWorkspace=calibration_d_spacing_ws,
                                GroupingFileName=out_grouping_file_name, GroupNames=group_names)
    remove_intermediate_workspace(calibration_d_spacing_ws)
    remove_intermediate_workspace(input_ws)


def _create_calibration(calibration_runs, offset_file_name, grouping_file_name, instrument):
    input_ws = _read_ws(number=calibration_runs, instrument=instrument)
    cycle_information = instrument.get_cycle_information(calibration_runs)

    # TODO move these hard coded params to instrument specific
    if cycle_information["instrument_version"] == "new" or cycle_information["instrument_version"] == "new2":
        input_ws = mantid.Rebin(InputWorkspace=input_ws, Params="100,-0.0006,19950")

    d_spacing_cal = mantid.ConvertUnits(InputWorkspace=input_ws, Target="dSpacing")
    d_spacing_cal = mantid.Rebin(InputWorkspace=d_spacing_cal, Params="1.8,0.002,2.1")

    if cycle_information["instrument_version"] == "new2":
        cross_cor_ws = mantid.CrossCorrelate(InputWorkspace=d_spacing_cal, ReferenceSpectra=20,
                                             WorkspaceIndexMin=9, WorkspaceIndexMax=1063, XMin=1.8, XMax=2.1)

    elif cycle_information["instrument_version"] == "new":
        cross_cor_ws = mantid.CrossCorrelate(InputWorkspace=d_spacing_cal, ReferenceSpectra=20,
                                             WorkspaceIndexMin=9, WorkspaceIndexMax=943, XMin=1.8, XMax=2.1)
    else:
        cross_cor_ws = mantid.CrossCorrelate(InputWorkspace=d_spacing_cal, ReferenceSpectra=500,
                                             WorkspaceIndexMin=1, WorkspaceIndexMax=1440, XMin=1.8, XMax=2.1)
    calib_dir = instrument.calibration_dir
    offset_file_path = calib_dir + offset_file_name
    grouping_file_path = calib_dir + grouping_file_name
    # Ceo Cell refined to 5.4102(3) so 220 is 1.912795
    offset_output_path = mantid.GetDetectorOffsets(InputWorkspace=cross_cor_ws, Step=0.002, DReference=1.912795,
                                                   XMin=-200, XMax=200, GroupingFileName=offset_file_path)
    aligned_ws = mantid.AlignDetectors(InputWorkspace=input_ws, CalibrationFile=offset_file_path)
    cal_grouped_ws = mantid.DiffractionFocussing(InputWorkspace=aligned_ws, GroupingFileName=grouping_file_path)

    remove_intermediate_workspace(d_spacing_cal)
    remove_intermediate_workspace(cross_cor_ws)
    remove_intermediate_workspace(aligned_ws)
    remove_intermediate_workspace(cal_grouped_ws)


def _create_calibration_si(calibration_runs, output_file_name, instrument):
    wcal = "cal_raw"
    PEARL_read(calruns, "raw", wcal)

    if instver == "new" or instver == "new2":
        mantid.Rebin(InputWorkspace=wcal, OutputWorkspace=wcal, Params="100,-0.0006,19950")

    ConvertUnits(InputWorkspace=wcal, OutputWorkspace="cal_inD", Target="dSpacing")

    if instver == "new2":
        mantid.Rebin(InputWorkspace="cal_inD", OutputWorkspace="cal_Drebin", Params="1.71,0.002,2.1")
        mantid.CrossCorrelate(InputWorkspace="cal_Drebin", OutputWorkspace="crosscor", ReferenceSpectra=20,
                       WorkspaceIndexMin=9, WorkspaceIndexMax=1063, XMin=1.71, XMax=2.1)
    elif instver == "new":
        mantid.Rebin(InputWorkspace="cal_inD", OutputWorkspace="cal_Drebin", Params="1.85,0.002,2.05")
        mantid.CrossCorrelate(InputWorkspace="cal_Drebin", OutputWorkspace="crosscor", ReferenceSpectra=20,
                       WorkspaceIndexMin=9, WorkspaceIndexMax=943, XMin=1.85, XMax=2.05)
    else:
        mantid.Rebin(InputWorkspace="cal_inD", OutputWorkspace="cal_Drebin", Params="3,0.002,3.2")
        mantid.CrossCorrelate(InputWorkspace="cal_Drebin", OutputWorkspace="crosscor", ReferenceSpectra=500,
                       WorkspaceIndexMin=1, WorkspaceIndexMax=1440, XMin=3, XMax=3.2)

    mantid.GetDetectorOffsets(InputWorkspace="crosscor", OutputWorkspace="OutputOffsets", Step=0.002,
                       DReference=1.920127251, XMin=-200, XMax=200, GroupingFileName=noffsetfile)
    mantid.AlignDetectors(InputWorkspace=wcal, OutputWorkspace="cal_aligned", CalibrationFile=noffsetfile)
    mantid.DiffractionFocussing(InputWorkspace="cal_aligned", OutputWorkspace="cal_grouped", GroupingFileName=groupfile)


def _create_van(instrument, van, empty, nvanfile, nspline=60, absorb=True, gen_absorb=False):

    cycle_information = instrument.get_cycle_information(van)

    input_van_ws = _read_ws(number=van, instrument=instrument)
    input_empty_ws = _read_ws(number=empty, instrument=instrument)

    corrected_van_ws = mantid.Minus(LHSWorkspace=input_van_ws, RHSWorkspace=input_empty_ws)

    remove_intermediate_workspace(input_empty_ws)
    remove_intermediate_workspace(input_van_ws)

    calibration_full_paths = instrument.get_calibration_full_paths(cycle=cycle_information["cycle"])

    if absorb:
        corrected_van_ws = _apply_absorb_corrections(calibration_full_paths, corrected_van_ws, gen_absorb)

    corrected_van_ws = mantid.ConvertUnits(InputWorkspace=corrected_van_ws, Target="TOF")
    corrected_van_ws = mantid.Rebin(InputWorkspace=corrected_van_ws, Params=instrument.get_create_van_tof_binning())

    corrected_van_ws = mantid.AlignDetectors(InputWorkspace=corrected_van_ws,
                                             CalibrationFile=calibration_full_paths["calibration"])

    focused_van_file = mantid.DiffractionFocussing(InputWorkspace=corrected_van_ws,
                                                   GroupingFileName=calibration_full_paths["grouping"])

    focused_van_file = mantid.ConvertUnits(InputWorkspace=focused_van_file, Target="TOF")

    focused_van_file = mantid.Rebin(InputWorkspace=focused_van_file, Params=instrument.get_create_van_tof_binning())
    focused_van_file = mantid.ConvertUnits(InputWorkspace=focused_van_file, Target="dSpacing")

    remove_intermediate_workspace(corrected_van_ws)

    splined_ws_list = instrument.spline_background(focused_van_file, nspline, cycle_information["instrument_version"])

    append = False
    for ws in splined_ws_list:
        mantid.SaveNexus(Filename=nvanfile, InputWorkspace=ws, Append=append)
        remove_intermediate_workspace(ws)
        append = True

    mantid.LoadNexus(Filename=nvanfile, OutputWorkspace="Van_data")


def _apply_absorb_corrections(calibration_full_paths, corrected_van_ws, gen_absorb):
    corrected_van_ws = mantid.ConvertUnits(InputWorkspace=corrected_van_ws, Target="Wavelength")

    if gen_absorb:
        absorb_ws = _generate_vanadium_absorb_corrections(calibration_full_paths, corrected_van_ws)
    else:
        absorb_ws = _load_van_absorb_corr(calibration_full_paths)

    corrected_van_ws = mantid.RebinToWorkspace(WorkspaceToRebin=corrected_van_ws, WorkspaceToMatch=absorb_ws)
    corrected_van_ws = mantid.Divide(LHSWorkspace=corrected_van_ws, RHSWorkspace=absorb_ws)
    remove_intermediate_workspace(absorb_ws)
    return corrected_van_ws


def _generate_vanadium_absorb_corrections(calibration_full_paths, ws_to_match):
    # TODO are these values applicable to all instruments
    import pydevd
    pydevd.settrace('localhost', port=23000, stdoutToServer=True, stderrToServer=True)
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
    load_monitor_ws = None
    if isinstance(number, int):
        full_file_path = instrument.generate_input_full_path(run_number=number, input_dir=input_dir)
        mspectra = instrument.get_monitor_spectra(number)
        load_monitor_ws = mantid.LoadRaw(Filename=full_file_path, SpectrumMin=mspectra, SpectrumMax=mspectra,
                                         LoadLogFiles="0")
    else:
        load_monitor_ws = _load_monitor_sum_range(files=number, input_dir=input_dir, instrument=instrument)

    return load_monitor_ws


def _load_monitor_sum_range(files, input_dir, instrument):
    loop = 0
    num = files.split("_")
    frange = list(range(int(num[0]), int(num[1]) + 1))
    mspectra = instrument.get_monitor_spectra(int(num[0]))
    for i in frange:
        file_path = instrument.generate_input_full_path(i, input_dir)
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
    ext = instrument.default_input_ext
    if isinstance(run_number, int):
        if ext[0] == 's':
            # TODO deal with liveData in higher class
            raise NotImplementedError()

        infile = instrument.generate_input_full_path(run_number=run_number, input_dir=input_dir)
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
        file_path = instrument.generate_input_full_path(i, input_dir)
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
    raw_data_dir = instrument.raw_data_dir
    cycle_information = instrument.get_cycle_information(run_number=number)
    input_dir = instrument.generate_cycle_dir(raw_data_dir, cycle_information["cycle"])
    read_in_ws = _load_raw_files(run_number=number, instrument=instrument, input_dir=input_dir)
    # TODO move this into instrument specific
    read_ws = mantid.ConvertUnits(InputWorkspace=read_in_ws, Target="Wavelength")
    remove_intermediate_workspace(read_in_ws)
    _read_pearl_monitor = instrument.get_monitor(run_number=number, input_dir=input_dir, spline_terms=20)
    read_ws = mantid.NormaliseToMonitor(InputWorkspace=read_ws,
                                                      MonitorWorkspace=_read_pearl_monitor,
                                                      IntegrationRangeMin=0.6, IntegrationRangeMax=5.0)
    output_name = "read_ws-" + str(g_ads_workaround["read_pearl_ws"])
    g_ads_workaround["read_pearl_ws"] += 1
    output_ws = mantid.ConvertUnits(InputWorkspace=read_ws,
                                    OutputWorkspace=output_name, Target="TOF")

    remove_intermediate_workspace(_read_pearl_monitor)
    remove_intermediate_workspace(read_ws)
    return output_ws


def _run_pearl_focus(instrument, run_number, fmode="trans", atten=True, van_norm=True):

    cycle_information = instrument.get_cycle_information(run_number=run_number)

    alg_range, save_range = instrument.get_instrument_alg_save_ranges(cycle_information["instrument_version"])

    input_file_paths = instrument.get_calibration_full_paths(cycle=cycle_information["cycle"])

    output_file_names = instrument.generate_out_file_paths(run_number, instrument.output_dir)
    read_ws = _read_ws(number=run_number, instrument=instrument)
    input_workspace = mantid.Rebin(InputWorkspace=read_ws, Params=instrument.get_focus_tof_binning())
    input_workspace = mantid.AlignDetectors(InputWorkspace=input_workspace, CalibrationFile=input_file_paths["calibration"])
    input_workspace = mantid.DiffractionFocussing(InputWorkspace=input_workspace, GroupingFileName=input_file_paths["grouping"])

    calibrated_spectra = _focus_load(alg_range, input_workspace, input_file_paths, instrument, van_norm)

    remove_intermediate_workspace(read_ws)
    remove_intermediate_workspace(input_workspace)

    if fmode == "all":
        processed_nexus_files = _focus_mode_all(output_file_names, calibrated_spectra)

    elif fmode == "groups":
        processed_nexus_files = _focus_mode_groups(alg_range, cycle_information, output_file_names, save_range,
                                                   calibrated_spectra)

    elif fmode == "trans":

        processed_nexus_files = _focus_mode_trans(output_file_names, atten, instrument, calibrated_spectra)

    elif fmode == "mods":

        processed_nexus_files = _focus_mode_mods(output_file_names, calibrated_spectra)

    else:
        raise ValueError("Focus mode unknown")

    for ws in calibrated_spectra:
        remove_intermediate_workspace(ws)

    return processed_nexus_files


def _focus_mode_mods(output_file_names, calibrated_spectra):
    index = 1
    append = False
    output_list = []
    for ws in calibrated_spectra:

        if ws == calibrated_spectra[0]:
            # Skip WS group
            continue

        mantid.SaveGSS(InputWorkspace=ws, Filename=output_file_names["gss_filename"], Append=append, Bank=index)
        output_name = "focus_mode_mods-" + str(index)
        dspacing_ws = mantid.ConvertUnits(InputWorkspace=ws, OutputWorkspace=output_name, Target="dSpacing")
        output_list.append(dspacing_ws)
        mantid.SaveNexus(Filename=output_file_names["nxs_filename"], InputWorkspace=dspacing_ws, Append=append)

        append = True
        index += 1
    return output_list


def _focus_mode_trans(output_file_names, atten, instrument, calibrated_spectra):
    summed_ws = mantid.CloneWorkspace(InputWorkspace=calibrated_spectra[1])
    for i in range(2, 10):  # Add workspaces 2-9
        summed_ws = mantid.Plus(LHSWorkspace=summed_ws, RHSWorkspace=calibrated_spectra[i])

    summed_ws = mantid.Scale(InputWorkspace=summed_ws, Factor=0.111111111111111)

    in_atten_ws = summed_ws
    if atten:
        no_att = output_file_names["output_name"] + "_noatten"

        mantid.CloneWorkspace(InputWorkspace=summed_ws, OutputWorkspace=no_att)
        in_atten_ws = mantid.ConvertUnits(InputWorkspace=in_atten_ws, Target="dSpacing")
        atten_ws = instrument.attenuate_workspace(in_atten_ws)
        attenuated_workspace = mantid.ConvertUnits(InputWorkspace=atten_ws, Target="TOF")
        remove_intermediate_workspace(atten_ws)
        remove_intermediate_workspace(in_atten_ws)
        in_atten_ws = attenuated_workspace  # Reuse name for later

    mantid.SaveGSS(InputWorkspace=in_atten_ws, Filename=output_file_names["gss_filename"], Append=False, Bank=1)
    mantid.SaveFocusedXYE(InputWorkspace=in_atten_ws, Filename=output_file_names["tof_xye_filename"],
                          Append=False, IncludeHeader=False)

    focus_mode_trans_atten_ws = mantid.ConvertUnits(InputWorkspace=in_atten_ws, Target="dSpacing")
    remove_intermediate_workspace(in_atten_ws)
    mantid.SaveFocusedXYE(InputWorkspace=focus_mode_trans_atten_ws, Filename=output_file_names["dspacing_xye_filename"],
                          Append=False, IncludeHeader=False)
    mantid.SaveNexus(InputWorkspace=focus_mode_trans_atten_ws, Filename=output_file_names["nxs_filename"], Append=False)

    output_list = [focus_mode_trans_atten_ws]

    for i in range(1, 10):
        workspace_name = "focus_mode_trans-dspacing" + str(i)
        to_save = mantid.ConvertUnits(InputWorkspace=calibrated_spectra[i], Target="dSpacing",
                                      OutputWorkspace=workspace_name)
        output_list.append(to_save)
        mantid.SaveNexus(Filename=output_file_names["nxs_filename"], InputWorkspace=to_save, Append=True)

    remove_intermediate_workspace(summed_ws)

    return output_list


def _focus_mode_groups(alg_range, cycle_information, output_file_names, save_range, calibrated_spectra):
    output_list = []
    to_save = _sum_groups_of_three_ws(calibrated_spectra)

    workspaces_4_to_9 = mantid.Plus(LHSWorkspace=to_save[1], RHSWorkspace=to_save[2])
    workspaces_4_to_9 = mantid.Scale(InputWorkspace=workspaces_4_to_9, Factor=0.5)
    to_save.append(workspaces_4_to_9)
    append = False
    index = 1
    for ws in to_save:
        if cycle_information["instrument_version"] == "new":
            mantid.SaveGSS(InputWorkspace=ws, Filename=output_file_names["gss_filename"], Append=append,
                           Bank=index)
        elif cycle_information["instrument_version"] == "new2":
            mantid.SaveGSS(InputWorkspace=ws, Filename=output_file_names["gss_filename"], Append=False,
                           Bank=index)

        workspace_names = "focus_mode_groups_grouped-" + str(index)
        dspacing_ws = mantid.ConvertUnits(InputWorkspace=ws, OutputWorkspace=workspace_names, Target="dSpacing")
        remove_intermediate_workspace(ws)
        output_list.append(dspacing_ws)
        mantid.SaveNexus(Filename=output_file_names["nxs_filename"], InputWorkspace=dspacing_ws, Append=append)
        append = True
        index += 1

    for i in range(0, save_range):
        workspace_names = "focus_mode_groups_ws-" + str(i + 9)

        tosave = calibrated_spectra[i + 9]

        mantid.SaveGSS(InputWorkspace=tosave, Filename=output_file_names["gss_filename"], Append=True, Bank=i + 5)

        output_list.append(mantid.ConvertUnits(InputWorkspace=tosave,
                                               OutputWorkspace=workspace_names, Target="dSpacing"))

        mantid.SaveNexus(Filename=output_file_names["nxs_filename"], InputWorkspace=tosave, Append=True)
    return output_list


def _sum_groups_of_three_ws(calibrated_spectra, ):
    workspace_list = []
    to_scale_list = []
    output_list = []
    for outer_loop_count in range(0, 3):
        # First clone workspaces 1/4/7
        pass_multiplier = (outer_loop_count * 3)
        first_ws_index = pass_multiplier + 1
        workspace_names = "focus_mode_groups-" + str(first_ws_index)
        workspace_list.append(mantid.CloneWorkspace(InputWorkspace=calibrated_spectra[first_ws_index],
                                                    OutputWorkspace=workspace_names))
        # Then add workspaces 1+2+3 / 4+5+6 / 7+8+9
        for i in range(2, 4):
            input_ws_index = i + pass_multiplier  # Workspaces 2/3
            inner_workspace_names = "focus_mode_groups-" + str(input_ws_index)

            to_scale_list.append(mantid.Plus(LHSWorkspace=workspace_list[outer_loop_count],
                                             RHSWorkspace=calibrated_spectra[input_ws_index],
                                             OutputWorkspace=inner_workspace_names))

        # Finally scale the output workspaces
        workspace_names = "focus_mode_groups_scaled-" + str(outer_loop_count)
        output_list.append(mantid.Scale(InputWorkspace=to_scale_list[outer_loop_count],
                                        OutputWorkspace=workspace_names, Factor=0.333333333333))
    for ws in to_scale_list:
        remove_intermediate_workspace(ws)
    return output_list


def _focus_mode_all(output_file_names, calibrated_spectra):
    first_spectrum = calibrated_spectra[0]
    summed_spectra = mantid.CloneWorkspace(InputWorkspace=first_spectrum)

    for i in range(1, 9):  # TODO why is this 1-8
        summed_spectra = mantid.Plus(LHSWorkspace=summed_spectra, RHSWorkspace=calibrated_spectra[i])

    summed_spectra = mantid.Scale(InputWorkspace=summed_spectra, Factor=0.111111111111111)
    mantid.SaveGSS(InputWorkspace=summed_spectra, Filename=output_file_names["gss_filename"], Append=False, Bank=1)

    focus_mode_all_summed_ws = mantid.ConvertUnits(InputWorkspace=summed_spectra, Target="dSpacing")
    mantid.SaveNexus(Filename=output_file_names["nxs_filename"], InputWorkspace=focus_mode_all_summed_ws, Append=False)

    remove_intermediate_workspace(summed_spectra)
    output_list = [focus_mode_all_summed_ws]
    for i in range(0, 3):
        spectra_index = (i + 10)
        ws_to_save = calibrated_spectra[spectra_index]  # Save out workspaces 10/11/12
        output_name = "focus_mode_all_workspace" + str(spectra_index)
        mantid.SaveGSS(InputWorkspace=ws_to_save, Filename=output_file_names["gss_filename"], Append=True, Bank=i + 2)
        ws_to_save = mantid.ConvertUnits(InputWorkspace=ws_to_save, OutputWorkspace=output_name, Target="dSpacing")
        output_list.append(ws_to_save)
        mantid.SaveNexus(Filename=output_file_names["nxs_filename"], InputWorkspace=ws_to_save, Append=True)

    return output_list


def _focus_load(alg_range, focused_ws, input_file_paths, instrument, van_norm):
    processed_spectra = []
    if van_norm:
        vanadium_ws_list = mantid.LoadNexus(Filename=input_file_paths["vanadium"])

    for index in range(0, alg_range):
        if van_norm:
            processed_spectra.append(calc_calibration_with_vanadium(focused_ws, index,
                                                                    vanadium_ws_list[index + 1], instrument))
        else:
            processed_spectra.append(calc_calibration_without_vanadium(focused_ws, index, instrument))

    if van_norm:
        remove_intermediate_workspace(vanadium_ws_list[0]) # Delete the WS group

    return processed_spectra


def calc_calibration_without_vanadium(focused_ws, index, instrument):
    focus_spectrum = mantid.ExtractSingleSpectrum(InputWorkspace=focused_ws, WorkspaceIndex=index)
    focus_spectrum = mantid.ConvertUnits(InputWorkspace=focus_spectrum, Target="TOF")
    focus_spectrum = mantid.Rebin(InputWorkspace=focus_spectrum, Params=instrument.tof_binning)
    focus_calibrated = mantid.CropWorkspace(InputWorkspace=focus_spectrum, XMin=0.1)
    return focus_calibrated


def calc_calibration_with_vanadium(focused_ws, index, vanadium_ws, instrument):
    # Load in workspace containing vanadium run
    van_rebinned = mantid.Rebin(InputWorkspace=vanadium_ws, Params=instrument.get_focus_tof_binning())

    van_spectrum = mantid.ExtractSingleSpectrum(InputWorkspace=focused_ws, WorkspaceIndex=index)
    van_spectrum = mantid.ConvertUnits(InputWorkspace=van_spectrum, Target="TOF")
    van_spectrum = mantid.Rebin(InputWorkspace=van_spectrum, Params=instrument.get_focus_tof_binning())

    van_processed = "van_processed" + str(index)  # Workaround for Mantid overwriting the WS in a loop
    mantid.Divide(LHSWorkspace=van_spectrum, RHSWorkspace=van_rebinned, OutputWorkspace=van_processed)
    mantid.CropWorkspace(InputWorkspace=van_processed, XMin=0.1, OutputWorkspace=van_processed)
    mantid.Scale(InputWorkspace=van_processed, Factor=10, OutputWorkspace=van_processed)

    remove_intermediate_workspace(van_rebinned)
    remove_intermediate_workspace(van_spectrum)

    return van_processed


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