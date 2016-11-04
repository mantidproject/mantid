from __future__ import (absolute_import, division, print_function)
import mantid.simpleapi as mantid


# --- Public API --- #

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
# another API or update an existing one


_read_ws_count = 0
global g_ads_workaround
g_ads_workaround = {"read_ws": _read_ws_count}

def _create_blank_cal_file(calibration_runs, out_grouping_file_name, instrument, group_names):
    input_ws = _load_current_normalised_ws(calibration_runs, instrument)
    calibration_d_spacing_ws = mantid.ConvertUnits(InputWorkspace=input_ws, Target="dSpacing")
    mantid.CreateCalFileByNames(InstrumentWorkspace=calibration_d_spacing_ws,
                                GroupingFileName=out_grouping_file_name, GroupNames=group_names)
    remove_intermediate_workspace(calibration_d_spacing_ws)
    remove_intermediate_workspace(input_ws)


def _create_van(instrument, van, empty, output_van_file_name, num_of_splines=60, absorb=True, gen_absorb=False):
    cycle_information = instrument._get_cycle_information(van)

    input_van_ws = _load_current_normalised_ws(number=van, instrument=instrument)
    input_empty_ws = _load_current_normalised_ws(number=empty, instrument=instrument)

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

    if instrument._PEARL_filename_is_full_path():
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
    # TODO refactor this - it probably doesn't work at the moment
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


def _load_current_normalised_ws(number, instrument):
    cycle_information = instrument._get_cycle_information(run_number=number)
    input_dir = instrument._generate_raw_data_cycle_dir(cycle_information["cycle"])

    read_in_ws = _load_raw_files(run_number=number, instrument=instrument, input_dir=input_dir)
    load_monitor_ws = _load_monitor(number=number, input_dir=input_dir, instrument=instrument)

    read_ws = instrument._normalise_ws(ws_to_correct=read_in_ws, monitor_ws=load_monitor_ws, spline_terms=20)

    output_name = "read_ws_output-" + str(g_ads_workaround["read_ws"])
    g_ads_workaround["read_ws"] += 1
    read_ws = mantid.RenameWorkspace(InputWorkspace=read_ws, OutputWorkspace=output_name)

    remove_intermediate_workspace(read_in_ws)
    return read_ws


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
