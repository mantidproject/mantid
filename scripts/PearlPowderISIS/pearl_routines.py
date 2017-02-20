from __future__ import (absolute_import, division, print_function)

import warnings

# directories generator
from isis_powder.pearl import Pearl

# --------------------------------------------------------------------------------
# This script has been refactored into the isis_powder module and is provided for
# supporting existing scripts. It removed as the new are adopted and less code
# relies on the behaviour of this API

# The old params can be set anywhere so store them in a single global dictionary
global g_oldParams
g_oldParams = {}

# As the global can be set anywhere create a singleton to a PEARL object
global g_pearl_obj
g_pearl_obj = None


def _pearl_obj_singleton():
    global g_pearl_obj
    if g_pearl_obj is None:
        g_pearl_obj = Pearl(user_name="NotSet", calibration_dir="NotSet", output_dir="NotSet",
                            input_file_ext=".raw", tt_mode="NotSet")
    return g_pearl_obj


def _get_global_dict():
    global g_oldParams
    return g_oldParams


def _merge_dict_into_global(d):
    copy = d.copy()
    global g_oldParams
    g_oldParams.update(copy)
    return g_oldParams


def PEARL_startup(usern="matt", thiscycle='11_1'):
    # ---- !!! This is deprecated and should not be used - it is only here for compatibility with old scripts !!!--- #
    warnings.warn("This method of performing Pearl Powder Diffraction is deprecated.", DeprecationWarning)
    pearl_file_dir = "P:\\Mantid\\Calibration\\"
    currentdatadir = "I:\\"
    tt_mode = "TT88"
    # userdataprocessed is the data output directory
    userdataprocessed = "P:\\users\\" + "Cycle_" + thiscycle + "\\" + usern + "\\"

    # LiveDataDir is no longer read. Please set the raw_data_dir to your LiveDataDir
    livedatadir = "I:\\"
    # These should now only be the filename and not the path. The calibration dir (pearl_file_dir)
    # is where the files will be searched for
    calfile = "pearl_offset_11_2.cal"
    groupfile =  "pearl_group_11_2_TT88.cal"
    vabsorbfile = "pearl_absorp_sphere_10mm_newinst_long.nxs"
    vanfile = "van_spline_all_cycle_11_1.nxs"
    attenfile = "P:\\Mantid\\Attentuation\\PRL985_WC_HOYBIDE_NK_10MM_FF.OUT"
    mode = "all"
    tofbinning = "1500,-0.0006,19900"
    PEARL_setdatadir(currentdatadir)

    unused_params = {
                     "pearl_file_dir" : pearl_file_dir,
                     "currentdatadir" : currentdatadir,
                     "livedatadir" : livedatadir,
                     "calfile"     : calfile,
                     "groupfile"   : groupfile,
                     "vabsorbfile" : vabsorbfile,
                     "vanfile"     : vanfile,
                     "attenfile"   : attenfile,
                     "mode"        : mode,
                     "tofbinning"  : tofbinning}

    _merge_dict_into_global(unused_params)

    _pearl_obj_singleton()._old_api_constructor_set(user_name=usern, calibration_dir=pearl_file_dir,
                                                    raw_data_dir=currentdatadir, output_dir=userdataprocessed,
                                                    tt_mode=tt_mode)

    return


def PEARL_getlambdarange():
    return _pearl_obj_singleton()._get_lambda_range()


def PEARL_gettofrange():
    return _pearl_obj_singleton()._get_focus_tof_binning()


def PEARL_getmonitorspectrum(runno):
    pearl_obj = _pearl_obj_singleton()
    # Ensure mode is set to latest value
    pearl_obj._old_api_constructor_set(tt_mode=g_oldParams["mode"])
    return pearl_obj._get_monitor_spectra(runno)


def PEARL_getcycle(number):
    pearl_obj = _pearl_obj_singleton()
    cycle_information = pearl_obj._get_label_information(number)
    datadir = pearl_obj.output_dir
    updated_vals = {"cycle"   : cycle_information["cycle"],
                    "instver" : cycle_information["instrument_version"],
                    "datadir" : datadir}

    _merge_dict_into_global(updated_vals)
    print ("ISIS cycle is set to", cycle_information["cycle"])
    return


def PEARL_getcalibfiles():
    global calfile
    global groupfile
    global vabsorbfile
    global vanfile

    cycle = g_oldParams["cycle"]
    pearl_obj = _pearl_obj_singleton()

    # Need to update tt_mode and calibration dir before
    pearl_obj._old_api_set_tt_mode(g_oldParams["tt_mode"])
    pearl_obj._old_api_set_calib_dir(g_oldParams["pearl_file_dir"])

    print ("Setting calibration for cycle", cycle)

    return


# sets the intial directory for all calibration files
def pearl_initial_dir(directory='P:\Mantid\\'):
    new_val = {"pearl_file_dir": directory}
    _merge_dict_into_global(new_val)
    _pearl_obj_singleton()._old_api_set_calib_dir(directory)
    print ("Set pearl_file_dir directory to ", directory)
    return


# sets the current raw data files directory
def pearl_set_currentdatadir(directory="I:\\"):
    new_val = {"currentdatadir" : directory}
    _merge_dict_into_global(new_val)
    _pearl_obj_singleton()._old_api_set_raw_data_dir(directory)
    print ("Set currentdatadir directory to ", directory)
    return


# sets the user data output directory
def pearl_set_userdataoutput_dir(directory="P:\\users\\MantidOutput\\"):
    new_val = {"userdataprocessed" : directory}
    _merge_dict_into_global(new_val)
    _pearl_obj_singleton()._old_api_set_output_dir(directory)
    print ("Set userdataprocessed directory to ", directory)
    return


def PEARL_setdatadir(directory="C:\PEARL\RAW\\"):
    new_val = {"pearl_datadir" : directory}
    _merge_dict_into_global(new_val)
    _pearl_obj_singleton()._old_api_set_raw_data_dir(directory)
    print ("Set pearl_datadir directory to ", directory)
    return


# sets the atten file's directory
def PEARL_setattenfile(new_atten="P:\Mantid\\Attentuation\\PRL985_WC_HOYBIDE_NK_10MM_FF.OUT"):
    new_val = {"attenfile" : new_atten}
    _merge_dict_into_global(new_val)
    _pearl_obj_singleton()._old_api_set_atten(new_atten)
    print ("Set attenuation file to ", new_atten)
    return


def PEARL_datadir():
    return g_oldParams["pearl_datadir"]


def PEARL_getfilename(run_number, ext):
    pearl_obj = _pearl_obj_singleton()
    pearl_obj._old_api_set_ext(ext)
    return pearl_obj._generate_inst_file_name(run_number)


def PearlLoad(files, ext, outname):
    raise NotImplementedError("PearlLoad not implemented as it should be part of private API")


def PearlLoadMon(files, ext, outname):
    raise NotImplementedError("PearlLoadMon not implemented as it should be part of private API")


def PEARL_getmonitor(number, ext, spline_terms=20, debug=False):
    raise NotImplementedError("PEARL_getmonitor not implemented as it should be part of private API")


def PEARL_read(number, ext, outname):
    raise NotImplementedError("PEARL_read not implemented as it should be part of private API")


def PEARL_align(work, focus):
    raise NotImplementedError("PEARL_align not implemented as it should be of private API")


def PEARL_focus(number, ext="raw", fmode="trans", ttmode="TT70", atten=True, van_norm=True, debug=False):

    PEARL_getcycle(number)

    pearl_obj = _pearl_obj_singleton()
    pearl_obj._old_api_set_tt_mode(ttmode)
    pearl_obj._old_api_set_ext(ext)
    pearl_obj.set_debug_mode(debug)
    return pearl_obj.focus(run_number=number, focus_mode=fmode, do_attenuation=atten,
                           do_van_normalisation=van_norm)


def pearl_run_focus(number, ext="raw", fmode="trans", ttmode="TT70", atten=True, van_norm=True, debug=False,
                    focus_mode=1):
    PEARL_getcycle(number)

    pearl_obj = _pearl_obj_singleton()
    pearl_obj._old_api_set_tt_mode(ttmode)
    pearl_obj._old_api_set_ext(ext)
    pearl_obj.set_debug_mode(debug)
    return pearl_obj.focus(run_number=number, focus_mode=fmode, do_attenuation=atten,
                           do_van_normalisation=van_norm)


def PEARL_createvan(van, empty, ext="raw", fmode="all", ttmode="TT88",
                    nvanfile="P:\Mantid\\Calibration\\van_spline_all_cycle_11_1.nxs", nspline=60, absorb=True,
                    debug=False):

    new_vals = {"mode"    : fmode,
                "tt_mode" : ttmode}
    _merge_dict_into_global(new_vals)

    pearl_obj = _pearl_obj_singleton()
    pearl_obj._old_api_set_tt_mode(ttmode)
    pearl_obj._old_api_set_ext(ext)
    pearl_obj.set_debug_mode(debug)
    pearl_obj._old_api_uses_full_paths = True
    return_val = pearl_obj.create_calibration_vanadium(vanadium_runs=van, empty_runs=empty,
                                                       output_file_name=nvanfile, num_of_splines=nspline,
                                                       do_absorb_corrections=absorb)
    pearl_obj._old_api_uses_full_paths = False
    return return_val


def PEARL_createcal(calruns, noffsetfile="C:\PEARL\\pearl_offset_11_2.cal",
                    groupfile="P:\Mantid\\Calibration\\pearl_group_11_2_TT88.cal"):
    PEARL_getcycle(calruns)

    # Split the full path used in the old API call
    pearl_obj = _pearl_obj_singleton()
    pearl_obj._old_api_uses_full_paths = True
    pearl_obj.create_calibration(calibration_runs=calruns, offset_file_name=noffsetfile,
                                 grouping_file_name=groupfile)
    pearl_obj._old_api_uses_full_paths = False
    return


def PEARL_createcal_Si(calruns, noffsetfile="C:\PEARL\\pearl_offset_11_2.cal"):
    PEARL_getcycle(calruns)
    grouping_file_name = g_oldParams["groupfile"]
    _pearl_obj_singleton().create_calibration_si(calibration_runs=calruns,
                                                 cal_file_name=noffsetfile, grouping_file_name=grouping_file_name)
    return


def PEARL_creategroup(calruns, ngroupfile="C:\PEARL\\test_cal_group_11_1.cal", ngroup="bank1,bank2,bank3,bank4"):
    PEARL_getcycle(calruns)

    _pearl_obj_singleton().create_empty_calibration_by_names(calibration_numbers=calruns,
                                                             output_file_name=ngroupfile, group_names=ngroupfile)
    return


def PEARL_sumspec(number, ext, mintof=500, maxtof=1000, minspec=0, maxspec=943):
    raise NotImplementedError("PEARL_sumspec not implemented as it should be part of private API")


def PEARL_sumspec_lam(number, ext, minlam=0.1, maxlam=4, minspec=8, maxspec=943):
    raise NotImplementedError("PEARL_sumspec_lam not implemented")


def PEARL_atten(work, outwork, debug=False):
    raise NotImplementedError("PEARL_atten not implemented as it should be part of private API")


def PEARL_add(a_name, a_spectra, a_outname, atten=True):
    raise NotImplementedError("PEARL_add not implemented")
