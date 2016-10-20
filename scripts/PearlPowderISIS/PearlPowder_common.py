from __future__ import (absolute_import, division, print_function)
import mantid.simpleapi as mantid

# --- Public API --- #


def focus(number, startup_object, ext="raw", fmode="trans", ttmode="TT70", atten=True, van_norm=True):

    return _run_pearl_focus(run_number=number, ext=ext, fmode=fmode, ttmode=ttmode, atten=atten, van_norm=van_norm,
                            instrument=startup_object)


def create_calibration_by_names(calruns, startup_objects, ngroupfile, ngroup="bank1,bank2,bank3,bank4"):
    _create_blank_cal_file(calibration_runs=calruns, user_input=startup_objects, group_names=ngroup,
                           instrument=startup_objects, out_grouping_file_name=ngroupfile)


def create_calibration(startup_object, calibration_runs, offset_file_path, grouping_file_path):
    _create_calibration(calruns=calibration_runs, noffsetfile=offset_file_path, groupfile=grouping_file_path,
                        user_params=startup_object)


def create_vanadium(startup_object, vanadium_runs, empty_runs, output_file_name, tt_mode="TT88",
                    num_of_spline_coefficients=60, do_absorp_corrections=True):
    # TODO do we support different extensions?
    _create_van(user_input=startup_object, van=vanadium_runs, empty=empty_runs, nvanfile=output_file_name,
                nspline=num_of_spline_coefficients, absorb=do_absorp_corrections, ttmode=tt_mode)


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


def _create_blank_cal_file(calibration_runs, out_grouping_file_name, instrument, user_input=None,
                           group_names="bank1,bank2,bank3,bank4"):

    cycle_information = _get_cycle_info(calibration_runs, instrument=instrument)

    input_ws = "cal_raw"
    _read_pearl_ws(calibration_runs, "raw", input_ws, user_input.raw_data_dir, cycle_information["cycle"])
    calibration_dspacing_ws = mantid.ConvertUnits(InputWorkspace=input_ws, Target="dSpacing")
    mantid.CreateCalFileByNames(InstrumentWorkspace=calibration_dspacing_ws,
                                GroupingFileName=out_grouping_file_name, GroupNames=group_names)
    remove_intermediate_workspace(calibration_dspacing_ws)
    remove_intermediate_workspace(input_ws)
    return


def _get_cycle_info(run_number, instrument):
    cycle, instrument_version = instrument.get_cycle_information(run_number)

    cycle_information = {'cycle': cycle,
                         'instrument_version':  instrument_version}
    return cycle_information


def _generate_cycle_dir(raw_data_dir, run_cycle):
    str_run_cycle = str(run_cycle)
    # Append current cycle to raw data directory
    generated_dir = raw_data_dir + str_run_cycle
    if raw_data_dir.endswith('\\'):
        generated_dir += '\\'
    elif raw_data_dir.endswith('/'):
        generated_dir += '/'
    else:
        raise ValueError("Path :" + raw_data_dir + "\n Does not end with a \\ or / character")
    return generated_dir


def _get_calib_files_full_paths(cycle, in_tt_mode, instrument):

    calibration_dir = instrument.calibration_dir

    calibration_file, grouping_file, van_absorb, van_file, instrument_ver =\
        instrument.get_calibration_file_names(cycle=cycle, tt_mode=in_tt_mode)

    calibration_full_path = calibration_dir + calibration_file
    grouping_full_path = calibration_dir + grouping_file
    van_absorb_full_path = calibration_dir + van_absorb
    van_file_full_path = calibration_dir + van_file

    calibration_details = {"calibration": calibration_full_path,
                           "grouping": grouping_full_path,
                           "vanadium_absorption": van_absorb_full_path,
                           "vanadium": van_file_full_path,
                           "instrument_version": instrument_ver}

    return calibration_details


def _load_monitor(number, input_dir, instrument):
    _load_monitor_out_ws = None
    if isinstance(number, int):
        full_file_path = instrument.generate_input_full_path(run_number=number, input_dir=input_dir)
        mspectra = instrument.get_monitor_spectra(number)
        _load_monitor_out_ws = mantid.LoadRaw(Filename=full_file_path, SpectrumMin=mspectra, SpectrumMax=mspectra,
                                              LoadLogFiles="0")
    else:
        _load_monitor_out_ws = _load_monitor_sum_range(files=number, input_dir=input_dir, instrument=instrument, ext=ext)

    return _load_monitor_out_ws


def _load_monitor_sum_range(files, input_dir, instrument, ext):
    loop = 0
    num = files.split("_")
    frange = list(range(int(num[0]), int(num[1]) + 1))
    mspectra = instrument.get_monitor_spectra(int(num[0]))
    out_ws = None
    for i in frange:
        file_name = instrument.generate_inst_file_name(i, ext)
        infile = input_dir + file_name
        outwork = "mon" + str(i)
        mantid.LoadRaw(Filename=infile, OutputWorkspace=outwork, SpectrumMin=mspectra, SpectrumMax=mspectra,
                       LoadLogFiles="0")
        loop += 1
        if loop == 2:
            firstwk = "mon" + str(i - 1)
            secondwk = "mon" + str(i)
            out_ws = mantid.Plus(LHSWorkspace=firstwk, RHSWorkspace=secondwk)
            mantid.mtd.remove(firstwk)
            mantid.mtd.remove(secondwk)
        elif loop > 2:
            secondwk = "mon" + str(i)
            out_ws = mantid.Plus(LHSWorkspace=out_ws, RHSWorkspace=secondwk)
            mantid.mtd.remove(secondwk)

    return out_ws


def _load_raw_files(run_number, ext, instrument, input_dir):
    out_ws = None
    if isinstance(run_number, int):
        if ext[0] == 's':
            # TODO deal with liveData in higher class
            raise NotImplementedError()

        infile = instrument.generate_input_full_path(run_number=run_number, input_dir=input_dir)
        out_ws = mantid.LoadRaw(Filename=infile, LoadLogFiles="0")
    else:
        out_ws = load_raw_file_range(ext, run_number, input_dir)
    return out_ws


def load_raw_file_range(ext, files, input_dir, instrument):
    loop = 0
    num = files.split("_")
    frange = list(range(int(num[0]), int(num[1]) + 1))
    out_ws = None
    for i in frange:
        file_name = instrument.generate_inst_file_name(i, ext)
        file_path = input_dir + file_name
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


def _read_pearl_ws(number, ext, run_cycle, instrument):
    raw_data_dir = instrument.raw_data_dir
    input_dir = _generate_cycle_dir(raw_data_dir, run_cycle)
    input_ws = _load_raw_files(run_number=number, ext=ext, instrument=instrument, input_dir=input_dir)

    _read_pearl_workspace = mantid.ConvertUnits(InputWorkspace=input_ws, Target="Wavelength")
    _read_pearl_monitor = instrument.get_monitor(run_number=number, input_dir=input_dir, spline_terms=20)
    _read_pearl_workspace = mantid.NormaliseToMonitor(InputWorkspace=_read_pearl_workspace,
                                                      MonitorWorkspace=_read_pearl_monitor,
                                                      IntegrationRangeMin=0.6, IntegrationRangeMax=5.0)
    output_ws = mantid.ConvertUnits(InputWorkspace=_read_pearl_workspace, Target="TOF")

    remove_intermediate_workspace(_read_pearl_monitor)
    remove_intermediate_workspace(_read_pearl_workspace)
    return output_ws


def _run_pearl_focus(instrument, run_number, ext="raw", fmode="trans", ttmode="TT70", atten=True, van_norm=True):

    cycle_information = _get_cycle_info(run_number=run_number, instrument=instrument)

    alg_range, save_range = instrument.get_instrument_alg_save_ranges(cycle_information["instrument_version"])

    input_file_paths = _get_calib_files_full_paths(cycle=cycle_information["cycle"], in_tt_mode=ttmode,
                                                   instrument=instrument)

    output_file_names = instrument.generate_out_file_paths(run_number, instrument.output_dir)
    input_workspace = _read_pearl_ws(number=run_number, ext=ext, instrument=instrument, run_cycle=cycle_information["cycle"])
    input_workspace = mantid.Rebin(InputWorkspace=input_workspace, Params=instrument.get_tof_binning())
    input_workspace = mantid.AlignDetectors(InputWorkspace=input_workspace, CalibrationFile=input_file_paths["calibration"])
    input_workspace = mantid.DiffractionFocussing(InputWorkspace=input_workspace, GroupingFileName=input_file_paths["grouping"])

    calibrated_spectra = _focus_calibrate(alg_range, input_workspace, input_file_paths, instrument, van_norm)

    remove_intermediate_workspace(input_workspace)

    if fmode == "all":

        focus_mode_all(alg_range, output_file_names, calibrated_spectra)

    elif (fmode == "groups"):

        name = []
        name.extend((output_file_names["output_name"] + "_mods1-3", output_file_names["output_name"] + "_mods4-6",
                     output_file_names["output_name"] + "_mods7-9", output_file_names["output_name"] + "_mods4-9"))

        input = []
        input.extend((output_file_names["output_name"] + "_mod1", output_file_names["output_name"] + "_mod4", output_file_names["output_name"] + "_mod7"))

        for i in range(0, 3):
            mantid.CloneWorkspace(InputWorkspace=input[i], OutputWorkspace=name[i])

        for i in range(1, 3):
            toadd = output_file_names["output_name"] + "_mod" + str(i + 1)
            mantid.Plus(LHSWorkspace=name[0], RHSWorkspace=toadd, OutputWorkspace=name[0])

        mantid.Scale(InputWorkspace=name[0], OutputWorkspace=name[0], Factor=0.333333333333)

        for i in range(1, 3):
            toadd = output_file_names["output_name"] + "_mod" + str(i + 4)
            mantid.Plus(LHSWorkspace=name[1], RHSWorkspace=toadd, OutputWorkspace=name[1])

        mantid.Scale(InputWorkspace=name[1], OutputWorkspace=name[1], Factor=0.333333333333)

        for i in range(1, 3):
            toadd = output_file_names["output_name"] + "_mod" + str(i + 7)
            mantid.Plus(LHSWorkspace=name[2], RHSWorkspace=toadd, OutputWorkspace=name[2])

        mantid.Scale(InputWorkspace=name[2], OutputWorkspace=name[2], Factor=0.333333333333)
        #
        #       Sum left and right 90degree bank modules, i.e. modules 4-9...
        #
        mantid.Plus(LHSWorkspace=name[1], RHSWorkspace=name[2], OutputWorkspace=name[3])
        mantid.Scale(InputWorkspace=name[3], OutputWorkspace=name[3], Factor=0.5)

        for i in range(0, 4):
            append = True
            if i is 0:
                append = False

            if cycle_information["instrument_version"] == "new":
                mantid.SaveGSS(InputWorkspace=name[i], Filename=output_file_names["gss_filename"], Append=append, Bank=i + 1)
            elif cycle_information["instrument_version"] == "new2":
                mantid.SaveGSS(InputWorkspace=name[i], Filename=output_file_names["gss_filename"], Append=False, Bank=i + 1)

            mantid.ConvertUnits(InputWorkspace=name[i], OutputWorkspace=name[i], Target="dSpacing")
            mantid.SaveNexus(Filename=output_file_names["nxs_filename"], InputWorkspace=name[i], Append=append)

        for i in range(0, save_range):
            tosave = output_file_names["output_name"] + "_mod" + str(i + 10)

            mantid.SaveGSS(InputWorkspace=tosave, Filename=output_file_names["gss_filename"], Append=True, Bank=i + 5)

            mantid.ConvertUnits(InputWorkspace=tosave, OutputWorkspace=tosave, Target="dSpacing")

            mantid.SaveNexus(Filename=output_file_names["nxs_filename"], InputWorkspace=tosave, Append=True)

        for i in range(0, alg_range):
            _remove_ws(ws_to_remove=(output_file_names["output_name"] + "_mod" + str(i + 1)))
            _remove_ws(ws_to_remove=("van" + str(i + 1)))
            _remove_ws(ws_to_remove=("rdata" + str(i + 1)))
            _remove_ws(ws_to_remove=output)
        for i in range(1, 4):
            _remove_ws(ws_to_remove=name[i])

    elif (fmode == "trans"):

        name = output_file_names["output_name"] + "_mods1-9"

        input = output_file_names["output_name"] + "_mod1"

        mantid.CloneWorkspace(InputWorkspace=input, OutputWorkspace=name)

        for i in range(1, 9):
            toadd = output_file_names["output_name"] + "_mod" + str(i + 1)
            mantid.Plus(LHSWorkspace=name, RHSWorkspace=toadd, OutputWorkspace=name)

        mantid.Scale(InputWorkspace=name, OutputWorkspace=name, Factor=0.111111111111111)

        if (atten):
            no_att = output_file_names["output_name"] + "_noatten"

            mantid.ConvertUnits(InputWorkspace=name, OutputWorkspace=name, Target="dSpacing")
            mantid.CloneWorkspace(InputWorkspace=name, OutputWorkspace=no_att)

            attenuated_workspace = instrument.attenuate_workspace(name)

            name = mantid.ConvertUnits(InputWorkspace=attenuated_workspace, Target="TOF")
            remove_intermediate_workspace(attenuated_workspace)

        mantid.SaveGSS(InputWorkspace=name, Filename=output_file_names["gss_filename"], Append=False, Bank=1)
        mantid.SaveFocusedXYE(InputWorkspace=name, Filename=output_file_names["tof_xye_filename"], Append=False, IncludeHeader=False)

        mantid.ConvertUnits(InputWorkspace=name, OutputWorkspace=name, Target="dSpacing")

        mantid.SaveFocusedXYE(InputWorkspace=name, Filename=output_file_names["dspacing_xye_filename"], Append=False, IncludeHeader=False)
        mantid.SaveNexus(Filename=output_file_names["nxs_filename"], InputWorkspace=name, Append=False)

        for i in range(0, 9):
            tosave = output_file_names["output_name"] + "_mod" + str(i + 1)
            # SaveGSS(tosave,Filename=gssfile,Append=True,Bank=i+2)
            mantid.ConvertUnits(InputWorkspace=tosave, OutputWorkspace=tosave, Target="dSpacing")
            mantid.SaveNexus(Filename=output_file_names["nxs_filename"], InputWorkspace=tosave, Append=True)

        for i in range(0, alg_range):
            _remove_ws(ws_to_remove=(output_file_names["output_name"] + "_mod" + str(i + 1)))
            _remove_ws(ws_to_remove=("van" + str(i + 1)))
            _remove_ws(ws_to_remove=("rdata" + str(i + 1)))
            _remove_ws(ws_to_remove=output)
        _remove_ws(ws_to_remove=name)

    elif (fmode == "mods"):

        for i in range(0, alg_range):

            output = output_file_names["output_name"] + "_mod" + str(i + 1)

            van = "van" + str(i + 1)

            rdata = "rdata" + str(i + 1)

            status = True

            if (i == 0):
                status = False

            mantid.SaveGSS(InputWorkspace=output, Filename=output_file_names["gss_filename"], Append=status, Bank=i + 1)

            mantid.ConvertUnits(InputWorkspace=output, OutputWorkspace=output, Target="dSpacing")

            mantid.SaveNexus(Filename=output_file_names["nxs_filename"], InputWorkspace=output, Append=status)

            _remove_ws(ws_to_remove=rdata)
            _remove_ws(ws_to_remove=van)
            _remove_ws(ws_to_remove=output)

    else:
        print("Sorry I don't know that mode", fmode)
        return

    mantid.LoadNexus(Filename=output_file_names["nxs_filename"], OutputWorkspace=output_file_names["output_name"])
    return output_file_names["output_name"]


def focus_mode_all(alg_range, output_file_names, calibrated_spectra):
    # Take first calibrated spectra
    first_spectrum = calibrated_spectra[0]
    summed_spectra = mantid.CloneWorkspace(InputWorkspace=first_spectrum)
    for i in range(1, 9):  # TODO why is this 1-8
        summed_spectra = mantid.Plus(LHSWorkspace=summed_spectra, RHSWorkspace=calibrated_spectra[i])
    summed_spectra = mantid.Scale(InputWorkspace=summed_spectra, Factor=0.111111111111111)
    mantid.SaveGSS(InputWorkspace=summed_spectra, Filename=output_file_names["gss_filename"], Append=False, Bank=1)
    summed_spectra = mantid.ConvertUnits(InputWorkspace=summed_spectra, Target="dSpacing")
    mantid.SaveNexus(Filename=output_file_names["nxs_filename"], InputWorkspace=summed_spectra, Append=False)
    for i in range(0, 3):
        tosave = calibrated_spectra[(i + 10)]

        mantid.SaveGSS(InputWorkspace=tosave, Filename=output_file_names["gss_filename"], Append=True, Bank=i + 2)

        mantid.ConvertUnits(InputWorkspace=tosave, OutputWorkspace=tosave, Target="dSpacing")

        mantid.SaveNexus(Filename=output_file_names["nxs_filename"], InputWorkspace=tosave, Append=True)
    for i in range(0, alg_range):
        remove_intermediate_workspace(calibrated_spectra[i])


def _focus_calibrate(alg_range, focused_ws, input_file_paths, instrument, van_norm):
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
    van_rebinned = mantid.Rebin(InputWorkspace=vanadium_ws, Params=instrument.get_tof_binning())

    van_spectrum = mantid.ExtractSingleSpectrum(InputWorkspace=focused_ws, WorkspaceIndex=index)
    van_spectrum = mantid.ConvertUnits(InputWorkspace=van_spectrum, Target="TOF")
    van_spectrum = mantid.Rebin(InputWorkspace=van_spectrum, Params=instrument.get_tof_binning())

    van_processed = "van_processed" + str(index)  # Workaround for Mantid overwriting the WS in a loop
    mantid.Divide(LHSWorkspace=van_spectrum, RHSWorkspace=van_rebinned, OutputWorkspace=van_processed)
    mantid.CropWorkspace(InputWorkspace=van_processed, XMin=0.1, OutputWorkspace=van_processed)
    mantid.Scale(InputWorkspace=van_processed, Factor=10, OutputWorkspace=van_processed)

    #remove_intermediate_workspace(van_spectrum)

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
    #mantid.DeleteWorkspace(ws)
    #del ws  # Mark it as deleted so that Python can throw before Mantid preserving more information
    print("skipping")