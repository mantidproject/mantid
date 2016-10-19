from __future__ import (absolute_import, division, print_function)
import mantid.simpleapi as mantid

import numpy as numpy

import pearl_cycle_factory # TODO move this into instrument class specific overrides
import pearl_calib_factory

# --- Public API --- #


def focus(number, startup_object, ext="raw", fmode="trans", ttmode="TT70", atten=True, van_norm=True):

    outwork = _run_pearl_focus(number, ext=ext, fmode=fmode, ttmode=ttmode, atten=atten, van_norm=van_norm,
                               user_params=startup_object)

    return outwork


def create_group(calruns, startup_objects, ngroupfile, ngroup="bank1,bank2,bank3,bank4"):
    _create_group(calruns=calruns, user_input=startup_objects, ngroup=ngroup, ngroupfile=ngroupfile)


def create_calibration(startup_object, calibration_runs, offset_file_path, grouping_file_path):
    _create_calibration(calruns=calibration_runs, noffsetfile=offset_file_path, groupfile=grouping_file_path,
                        user_params=startup_object)


def create_vanadium(startup_object, vanadium_runs, empty_runs, output_file_name, tt_mode="TT88",
                    num_of_spline_coefficients=60, do_absorp_corrections=True):
    # TODO do we support different extensions?
    _create_van(user_input=startup_object, van=vanadium_runs, empty=empty_runs, nvanfile=output_file_name,
                nspline=num_of_spline_coefficients, absorb=do_absorp_corrections, ttmode=tt_mode)


def setDebug(debug_on=False):
    global g_debug
    g_debug = debug_on


# --- Private Implementation --- #

# Please note these functions can change in any way at any time.
# For this reason please do not call them directly and instead use the Public API provided.

# If this doesn't quite provide what you need please let a developer know so we can create
# another API which will not change without notice

def _attenuate_workspace(work, outwork, attenuation_file_path):
    wc_atten = mantid.PearlMCAbsorption(attenuation_file_path)
    mantid.ConvertToHistogram(InputWorkspace="wc_atten", OutputWorkspace="wc_atten")
    mantid.RebinToWorkspace(WorkspaceToRebin="wc_atten", WorkspaceToMatch=work, OutputWorkspace="wc_atten")
    mantid.Divide(LHSWorkspace=work, RHSWorkspace="wc_atten", OutputWorkspace=outwork)
    _remove_inter_ws(ws_to_remove="wc_atten")
    return


def _create_group(calruns, ngroupfile, user_input=None, ngroup="bank1,bank2,bank3,bank4"):

    if user_input is None:
        raise ValueError("Instrument start object not passed.")

    cycle_information = _get_cycle_info(calruns)

    wcal = "cal_raw"
    _read_pearl_ws(calruns, "raw", wcal, user_input.raw_data_dir, cycle_information["cycle"])
    mantid.ConvertUnits(InputWorkspace=wcal, OutputWorkspace="cal_inD", Target="dSpacing")
    mantid.CreateCalFileByNames(InstrumentWorkspace=wcal, GroupingFileName=ngroupfile, GroupNames=ngroup)
    return


def _generate_cycle_dir(raw_data_dir, run_cycle):
    # Append current cycle to raw data directory
    input_dir = raw_data_dir
    if not input_dir.endswith('\\') or not input_dir.endswith('/'):
        input_dir += ''  # TODO does this work on Windows and Unix
    input_dir += run_cycle + '\\'
    return input_dir


def _generate_out_file_names(number, output_directory):

    if isinstance(number, int):
        outfile = output_directory + "PRL" + str(number) + ".nxs"
        gssfile = output_directory + "PRL" + str(number) + ".gss"
        tof_xye_file = output_directory + "PRL" + str(number) + "_tof_xye.dat"
        d_xye_file = output_directory + "PRL" + str(number) + "_d_xye.dat"
        outwork = "PRL" + str(number)
    else:
        outfile = output_directory + "PRL" + number + ".nxs"
        gssfile = output_directory + "PRL" + number + ".gss"
        tof_xye_file = output_directory + "PRL" + number + "_tof_xye.dat"
        d_xye_file = output_directory + "PRL" + number + "_d_xye.dat"
        outwork = "PRL" + number

    out_file_names = {"nxs_filename": outfile,
                      "gss_filename": gssfile,
                      "tof_xye_filename": tof_xye_file,
                      "dspacing_xye_filename": d_xye_file,
                      "output_name": outwork}

    return out_file_names


def _get_calib_files_full_paths(in_cycle, in_tt_mode, in_pearl_file_dir):

    calibration_file, grouping_file, van_absorb, van_file, instrument_ver = \
        pearl_calib_factory.get_calibration_dir(in_cycle, in_tt_mode, in_pearl_file_dir)

    calibration_details = {"calibration": calibration_file,
                           "grouping": grouping_file,
                           "vanadium_absorption": van_absorb,
                           "vanadium": van_file,
                           "instrument_version": instrument_ver}

    return calibration_details


def _get_cycle_info(number):
    cycle, instrument_version, _unused_data_dir = pearl_cycle_factory.get_cycle_dir(number, "")
    # TODO remove the blank param and wrap then move this into the PEARL specific startup

    cycle_information = {'cycle': cycle,
                         'instrument_version':  instrument_version}

    return cycle_information


def _get_file_name(run_number, ext):

    digit = len(str(run_number))

    if (run_number < 71009):
        number_of_digits = 5
        #
        # filename=data_dir+"PRL"
        #
        filename = "PRL"
    else:
        number_of_digits = 8
        #
        # filename=data_dir+"PEARL"
        #
        filename = "PEARL"

    for i in range(0, number_of_digits - digit):
        filename += "0"

    filename += str(run_number) + "." + ext

    return filename


def _get_instrument_ranges(inst_vers):

    # TODO move this to instrument specific class
    alg_range = None
    save_range = None

    if inst_vers == "new" or inst_vers == "old":  # New and old have identical ranges
        alg_range = 12
        save_range = 3
    elif inst_vers == "new2":
        alg_range = 14
        save_range = 5

    if alg_range is None or save_range is None:
        raise ValueError("Instrument version unknown")

    return alg_range, save_range


def _get_lambda_range(instrument_name):
    if instrument_name != "PEARL":
        raise NotImplementedError("Other instruments lambda not available")

    return 0.03, 6.00


def _get_monitor(run_number, ext, input_dir, spline_terms=20):
    works = "monitor" + str(run_number)
    _load_monitor(run_number, ext, works, input_dir=input_dir)
    mantid.ConvertUnits(InputWorkspace=works, OutputWorkspace=works, Target="Wavelength")
    lmin, lmax = _get_lambda_range("PEARL") # TODO move this to instrument override
    mantid.CropWorkspace(InputWorkspace=works, OutputWorkspace=works, XMin=lmin, XMax=lmax)
    ex_regions = numpy.zeros((2, 4))
    ex_regions[:, 0] = [3.45, 3.7]
    ex_regions[:, 1] = [2.96, 3.2]
    ex_regions[:, 2] = [2.1, 2.26]
    ex_regions[:, 3] = [1.73, 1.98]
    # ConvertToDistribution(works)

    for reg in range(0, 4):
        mantid.MaskBins(InputWorkspace=works, OutputWorkspace=works, XMin=ex_regions[0, reg], XMax=ex_regions[1, reg])

    mantid.SplineBackground(InputWorkspace=works, OutputWorkspace=works, WorkspaceIndex=0, NCoeff=spline_terms)

    return works


def _get_monitor_sepctrum(run_number):
    mode = ""
    if run_number < 71009:
        raise NotImplementedError()  # TODO figure out how to implement this
    #    if mode == "trans":
    #       mspectra = 1081
    #    elif mode == "all":
    #       mspectra = 2721
    #    elif mode == "novan":
    #       mspectra = 2721
    #    else:
    #        raise ValueError("Not supported")
    else:
        mspectra = 1
    return mspectra


def _load_monitor(files, ext, outname, input_dir):
    if isinstance(files, int):
        file_name = _get_file_name(files, ext)
        infile = input_dir + file_name
        mspectra = _get_monitor_sepctrum(files)
        print("loading ", infile, "into ", outname)
        mantid.LoadRaw(Filename=infile, OutputWorkspace=outname, SpectrumMin=mspectra, SpectrumMax=mspectra,
                  LoadLogFiles="0")
    else:
        loop = 0
        num = files.split("_")
        frange = list(range(int(num[0]), int(num[1]) + 1))
        mspectra = _get_monitor_sepctrum(int(num[0]))
        for i in frange:
            file_name = _get_file_name(i, ext)
            infile = input_dir + file_name
            outwork = "mon" + str(i)
            mantid.LoadRaw(Filename=infile, OutputWorkspace=outwork, SpectrumMin=mspectra, SpectrumMax=mspectra,
                    LoadLogFiles="0")
            loop = loop + 1
            if loop == 2:
                firstwk = "mon" + str(i - 1)
                secondwk = "mon" + str(i)
                mantid.Plus(LHSWorkspace=firstwk, RHSWorkspace=secondwk, OutputWorkspace=outname)
                mantid.mtd.remove(firstwk)
                mantid.mtd.remove(secondwk)
            elif loop > 2:
                secondwk = "mon" + str(i)
                mantid.Plus(LHSWorkspace=outname, RHSWorkspace=secondwk, OutputWorkspace=outname)
                mantid.mtd.remove(secondwk)
    return




def _load_raw_files(files, ext, outname, input_dir):
    # TODO low priority tidy this
    if isinstance(files, int):
        file_name = _get_file_name(files, ext)
        directory = input_dir
        if ext[0] == 's':
            # TODO deal with liveData in higher class
            raise NotImplementedError()

        infile = directory + file_name

        print("loading ", infile, "into ", outname)
        mantid.LoadRaw(Filename=infile, OutputWorkspace=outname, LoadLogFiles="0")
    else:
        loop = 0
        num = files.split("_")
        frange = list(range(int(num[0]), int(num[1]) + 1))
        for i in frange:
            file_name = _get_file_name(i, ext)
            file_path = input_dir + file_name
            outwork = "run" + str(i)
            mantid.LoadRaw(Filename=file_path, OutputWorkspace=outwork, LoadLogFiles="0")
            loop = loop + 1
            if loop == 2:
                firstwk = "run" + str(i - 1)
                secondwk = "run" + str(i)
                mantid.Plus(LHSWorkspace=firstwk, RHSWorkspace=secondwk, OutputWorkspace=outname)
                mantid.mtd.remove(firstwk)
                mantid.mtd.remove(secondwk)
            elif loop > 2:
                secondwk = "run" + str(i)
                mantid.Plus(LHSWorkspace=outname, RHSWorkspace=secondwk, OutputWorkspace=outname)
                mantid.mtd.remove(secondwk)
    return


def _read_pearl_ws(number, ext, outname, raw_data_dir, run_cycle):
    input_dir = _generate_cycle_dir(raw_data_dir, run_cycle)
    _load_raw_files(number, ext, outname, input_dir)
    mantid.ConvertUnits(InputWorkspace=outname, OutputWorkspace=outname, Target="Wavelength")
    # lmin,lmax=WISH_getlambdarange()
    # CropWorkspace(output,output,XMin=lmin,XMax=lmax)
    monitor = _get_monitor(number, ext, spline_terms=20, input_dir=input_dir)
    # NormaliseToMonitor(InputWorkspace=outname,OutputWorkspace=outname,MonitorWorkspace=monitor)
    mantid.NormaliseToMonitor(InputWorkspace=outname, OutputWorkspace=outname, MonitorWorkspace=monitor,
                              IntegrationRangeMin=0.6, IntegrationRangeMax=5.0)
    mantid.ConvertUnits(InputWorkspace=outname, OutputWorkspace=outname, Target="TOF")
    mantid.mtd.remove(monitor)
    # ReplaceSpecialValues(output,output,NaNValue=0.0,NaNError=0.0,InfinityValue=0.0,InfinityError=0.0)
    return


def _run_pearl_focus(number, ext="raw", fmode="trans", ttmode="TT70", atten=True, van_norm=True, user_params=None):
    """
     @type user_params: PearlStart
    """
    if user_params is None:
        raise ValueError("Instrument start object not passed.")

    cycle_information = _get_cycle_info(number)

    alg_range, save_range = _get_instrument_ranges(cycle_information["instrument_version"])

    input_file_paths = _get_calib_files_full_paths(in_cycle=cycle_information["cycle"], in_tt_mode=ttmode,
                                                   in_pearl_file_dir=user_params.calibration_dir)

    output_file_names = _generate_out_file_names(number, user_params.output_dir)

    work = "work"
    focus = "focus"

    _read_pearl_ws(number=number, ext=ext, outname=work, raw_data_dir=user_params.raw_data_dir,
                   run_cycle=cycle_information["cycle"])

    mantid.Rebin(InputWorkspace=work, OutputWorkspace=work, Params=user_params.tof_binning)

    mantid.AlignDetectors(InputWorkspace=work, OutputWorkspace=work,
                          CalibrationFile=input_file_paths["calibration"])

    mantid.DiffractionFocussing(InputWorkspace=work, OutputWorkspace=focus,
                                GroupingFileName=input_file_paths["grouping"])

    _remove_inter_ws(ws_to_remove=work)

    for i in range(0, alg_range):

        output = output_file_names["output_name"] + "_mod" + str(i + 1)
        van = "van" + str(i + 1)
        rdata = "rdata" + str(i + 1)

        if (van_norm):
            mantid.LoadNexus(Filename=input_file_paths["vanadium"], OutputWorkspace=van, EntryNumber=i + 1)
            mantid.ExtractSingleSpectrum(InputWorkspace=focus, OutputWorkspace=rdata, WorkspaceIndex=i)
            mantid.Rebin(InputWorkspace=van, OutputWorkspace=van, Params=user_params.tof_binning)
            mantid.ConvertUnits(InputWorkspace=rdata, OutputWorkspace=rdata, Target="TOF")
            mantid.Rebin(InputWorkspace=rdata, OutputWorkspace=rdata, Params=user_params.tof_binning)
            mantid.Divide(LHSWorkspace=rdata, RHSWorkspace=van, OutputWorkspace=output)
            mantid.CropWorkspace(InputWorkspace=output, OutputWorkspace=output, XMin=0.1)
            mantid.Scale(InputWorkspace=output, OutputWorkspace=output, Factor=10)
        else:
            mantid.ExtractSingleSpectrum(InputWorkspace=focus, OutputWorkspace=rdata, WorkspaceIndex=i)
            mantid.ConvertUnits(InputWorkspace=rdata, OutputWorkspace=rdata, Target="TOF")
            mantid.Rebin(InputWorkspace=rdata, OutputWorkspace=output, Params=user_params.tof_binning)
            mantid.CropWorkspace(InputWorkspace=output, OutputWorkspace=output, XMin=0.1)

    _remove_inter_ws(focus)

    if (fmode == "all"):

        name = output_file_names["output_name"] + "_mods1-9"

        input = output_file_names["output_name"] + "_mod1"

        mantid.CloneWorkspace(InputWorkspace=input, OutputWorkspace=name)

        for i in range(1, 9):
            toadd = output_file_names["output_name"] + "_mod" + str(i + 1)
            mantid.Plus(LHSWorkspace=name, RHSWorkspace=toadd, OutputWorkspace=name)

        mantid.Scale(InputWorkspace=name, OutputWorkspace=name, Factor=0.111111111111111)

        mantid.SaveGSS(InputWorkspace=name, Filename=output_file_names["gss_filename"], Append=False, Bank=1)

        mantid.ConvertUnits(InputWorkspace=name, OutputWorkspace=name, Target="dSpacing")

        mantid.SaveNexus(Filename=output_file_names["nxs_filename"], InputWorkspace=name, Append=False)

        for i in range(0, 3):
            tosave = output_file_names["output_name"] + "_mod" + str(i + 10)

            mantid.SaveGSS(InputWorkspace=tosave, Filename=output_file_names["gss_filename"], Append=True, Bank=i + 2)

            mantid.ConvertUnits(InputWorkspace=tosave, OutputWorkspace=tosave, Target="dSpacing")

            mantid.SaveNexus(Filename=output_file_names["nxs_filename"], InputWorkspace=tosave, Append=True)

        for i in range(0, alg_range):
            _remove_inter_ws(ws_to_remove=(output_file_names["output_name"] + "_mod" + str(i + 1)))
            _remove_inter_ws(ws_to_remove=("van" + str(i + 1)))
            _remove_inter_ws(ws_to_remove="rdata" + str(i + 1))
            _remove_inter_ws(ws_to_remove=name)

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
            _remove_inter_ws(ws_to_remove=(output_file_names["output_name"] + "_mod" + str(i + 1)))
            _remove_inter_ws(ws_to_remove=("van" + str(i + 1)))
            _remove_inter_ws(ws_to_remove=("rdata" + str(i + 1)))
            _remove_inter_ws(ws_to_remove=output)
        for i in range(1, 4):
            _remove_inter_ws(ws_to_remove=name[i])

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

            _attenuate_workspace(name, name, user_params.attenuation_full_path)

            mantid.ConvertUnits(InputWorkspace=name, OutputWorkspace=name, Target="TOF")

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
            _remove_inter_ws(ws_to_remove=(output_file_names["output_name"] + "_mod" + str(i + 1)))
            _remove_inter_ws(ws_to_remove=("van" + str(i + 1)))
            _remove_inter_ws(ws_to_remove=("rdata" + str(i + 1)))
            _remove_inter_ws(ws_to_remove=output)
        _remove_inter_ws(ws_to_remove=name)

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

            _remove_inter_ws(ws_to_remove=rdata)
            _remove_inter_ws(ws_to_remove=van)
            _remove_inter_ws(ws_to_remove=output)

    else:
        print("Sorry I don't know that mode", fmode)
        return

    mantid.LoadNexus(Filename=output_file_names["nxs_filename"], OutputWorkspace=output_file_names["output_name"])
    return output_file_names["output_name"]


def _remove_inter_ws(ws_to_remove):
    """
    Removes any intermediate workspaces if debug is set to false
        @param ws_to_remove: The workspace to remove from the ADS
    """
    try:
        if not g_debug:
            mantid.mtd.remove(ws_to_remove)
    except NameError:  # TODO fix this we shouldn't be using exceptions for flow
        mantid.mtd.remove(ws_to_remove)

