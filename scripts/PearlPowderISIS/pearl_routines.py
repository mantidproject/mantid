# pylint: disable=anomalous-backslash-in-string, global-variable-undefined, global-variable-not-assigned
# pylint: disable=invalid-name, too-many-arguments, superfluous-parens, too-many-branches, redefined-builtin

from __future__ import (absolute_import, division, print_function)
import os.path
import numpy as numpy
import sys

import mantid.simpleapi as mantid

import pearl_calib_factory
import pearl_cycle_factory

# ---- New Public API ---- #


def PEARL_gettofrange():
    return 150, 19900


def PEARL_getmonitorspectrum(runno):
    if (runno < 71009):
        if (mode == "trans"):
            mspectra = 1081
        elif (mode == "all"):
            mspectra = 2721
        elif (mode == "novan"):
            mspectra = 2721
        else:
            print("Sorry mode not supported")
            sys.exit(0)
    else:
        mspectra = 1

    print("Monitor spectrum is", mspectra)

    return mspectra


def PearlLoadMon(files, ext, outname, input_dir):
    if isinstance(files, int):
        file_name = _get_file_name(files, ext)
        infile = input_dir + file_name
        mspectra = PEARL_getmonitorspectrum(files)
        print("loading ", infile, "into ", outname)
        mantid.LoadRaw(Filename=infile, OutputWorkspace=outname, SpectrumMin=mspectra, SpectrumMax=mspectra,
                  LoadLogFiles="0")
    else:
        loop = 0
        num = files.split("_")
        frange = list(range(int(num[0]), int(num[1]) + 1))
        mspectra = PEARL_getmonitorspectrum(int(num[0]))
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


def PEARL_getmonitor(number, ext, input_dir, spline_terms=20):
    raise NotImplementedError()




def PEARL_createcal_Si(calruns, noffsetfile="C:\PEARL\\pearl_offset_11_2.cal"):
    cycle_information = _calculate_current_cycle(calruns)

    wcal = "cal_raw"
    PEARL_read(calruns, "raw", wcal)

    if cycle_information["instrument_version"] == "new" or cycle_information["instrument_version"] == "new2":
        mantid.Rebin(InputWorkspace=wcal, OutputWorkspace=wcal, Params="100,-0.0006,19950")

    mantid.ConvertUnits(InputWorkspace=wcal, OutputWorkspace="cal_inD", Target="dSpacing")

    if cycle_information["instrument_version"] == "new2":
        mantid.Rebin(InputWorkspace="cal_inD", OutputWorkspace="cal_Drebin", Params="1.71,0.002,2.1")
        mantid.CrossCorrelate(InputWorkspace="cal_Drebin", OutputWorkspace="crosscor", ReferenceSpectra=20,
                       WorkspaceIndexMin=9, WorkspaceIndexMax=1063, XMin=1.71, XMax=2.1)
    elif cycle_information["instrument_version"] == "new":
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
    # TODO fix this
    #mantid.DiffractionFocussing(InputWorkspace="cal_aligned", OutputWorkspace="cal_grouped",
    # GroupingFileName=groupfile)

    return


def PEARL_sumspec(number, ext, mintof=500, maxtof=1000, minspec=0, maxspec=943):
    cycle_information = _calculate_current_cycle(number)
    if cycle_information["instrument_version"] == "old":
        maxspec = 2720
    elif cycle_information["instrument_version"] == "new":
        maxspec = 943
    else:
        maxspec = 1063

    _load_raw_files(number, ext, "work")
    mantid.NormaliseByCurrent(InputWorkspace="work", OutputWorkspace="work")
    mantid.Integration(InputWorkspace="work", OutputWorkspace="integral", RangeLower=mintof, RangeUpper=maxtof,
                StartWorkspaceIndex=minspec, EndWorkspaceIndex=maxspec)
    mantid.mtd.remove("work")
    # sumplot=plotBin("integral",0)
    return


def PEARL_sumspec_lam(number, ext, minlam=0.1, maxlam=4, minspec=8, maxspec=943):
    cycle_information = _calculate_current_cycle(number)
    if cycle_information["instrument_version"] == "old":
        maxspec = 2720
    elif cycle_information["instrument_version"] == "new":
        maxspec = 943
    else:
        maxspec = 1063

    _load_raw_files(number, ext, "work")
    mantid.NormaliseByCurrent(InputWorkspace="work", OutputWorkspace="work")
    # TODO fix this
    #mantid.AlignDetectors(InputWorkspace="work", OutputWorkspace="work", CalibrationFile=calfile)
    mantid.ConvertUnits(InputWorkspace="worl", OutputWorkspace="work", Target="Wavelength")
    mantid.Integration(InputWorkspace="work", OutputWorkspace="integral", RangeLower=minlam, RangeUpper=maxlam,
                StartWorkspaceIndex=minspec, EndWorkspaceIndex=maxspec)
    mantid.mtd.remove("work")
    # sumplot=plotBin("integral",0)
    return


def PEARL_atten(work, outwork, attenuation_file_path):
    # attenfile="P:\Mantid\\Attentuation\\PRL985_WC_HOYBIDE_NK_10MM_FF.OUT"
    wc_atten = mantid.PearlMCAbsorption(attenuation_file_path)
    mantid.ConvertToHistogram(InputWorkspace="wc_atten", OutputWorkspace="wc_atten")
    mantid.RebinToWorkspace(WorkspaceToRebin="wc_atten", WorkspaceToMatch=work, OutputWorkspace="wc_atten")
    mantid.Divide(LHSWorkspace=work, RHSWorkspace="wc_atten", OutputWorkspace=outwork)
    _remove_inter_ws(ws_to_remove="wc_atten")
    return


def PEARL_add(a_name, a_spectra, a_outname, atten=True):
    w_add_out = a_outname
    gssfile = output_directory + a_outname + ".gss"
    nxsfile = output_directory + a_outname + ".nxs"

    loop = 0
    for i in a_spectra[:]:
        loop = loop + 1
        if loop == 1:
            w_add1 = "PRL" + a_name + "_mod" + str(i)
        elif loop == 2:
            w_add2 = "PRL" + a_name + "_mod" + str(i)
            mantid.Plus(LHSWorkspace=w_add1, RHSWorkspace=w_add2, OutputWorkspace=w_add_out)
        else:
            w_add2 = "PRL" + a_name + "_mod" + str(i)
            mantid.Plus(LHSWorkspace=w_add_out, RHSWorkspace=w_add2, OutputWorkspace=w_add_out)
    if (atten):
        PEARL_atten(w_add_out, w_add_out)

    mantid.SaveNexus(Filename=nxsfile, InputWorkspace=w_add_out, Append=False)
    mantid.ConvertUnits(InputWorkspace=w_add_out, OutputWorkspace=w_add_out, Target="TOF")
    mantid.SaveGSS(InputWorkspace=w_add_out, Filename=gssfile, Append=False, Bank=i + 1)
    mantid.ConvertUnits(InputWorkspace=w_add_out, OutputWorkspace=w_add_out, Target="dSpacing")

    return







def _align_workspace(work, focus):
    # TODO  - Do we need this or can we just push people to calling align detectors bare
    _get_calib_files_full_paths()
    #mantid.AlignDetectors(InputWorkspace=work, OutputWorkspace=work, CalibrationFile=calfile)
    return focus



# ------- Legacy interface -------- #

# These are here to preserve compatibility with any existing scripts and should
# be gradually deprecated and removed

# ----    Setters/Getters   -------#

# sets the intial directory for all calibration files
def pearl_initial_dir(directory='P:\Mantid\\'):
    """
    Sets the directory for the calibration files
        @param directory: The directory where calibration files are located
    """
    global calibration_directory
    calibration_directory = directory
    print("Set pearl_file_dir directory to ", directory)
    return


# sets the current raw data files directory
def pearl_set_currentdatadir(directory="I:\\"):
    """
    Sets the location of the raw data
        @param directory: The directory where the raw data to process is located
    """
    global currentdatadir
    currentdatadir = directory
    print("Set currentdatadir directory to ", directory)
    return


# sets the user data output directory
def pearl_set_userdataoutput_dir(directory="P:\\users\\MantidOutput\\"):
    """
    Sets the output directory for the script
        directory: The location to output data to. A folder with the current cycle will be created in this folder
    """
    global g_output_directory
    g_output_directory = directory
    print("Set userdataprocessed directory to ", directory)
    return


def PEARL_setdatadir(directory="C:\PEARL\RAW\\"):
    global g_raw_data_directory
    g_raw_data_directory = directory
    print("Set pearl_datadir directory to ", directory)
    return


# sets the atten file's directory
def PEARL_setattenfile(new_atten="P:\Mantid\\Attentuation\\PRL985_WC_HOYBIDE_NK_10MM_FF.OUT"):
    global g_attenfile
    g_attenfile = new_atten
    print("Set attenuation file to ", g_attenfile)
    return


def PEARL_datadir():
    raise NotImplementedError()

# ----   Old APIs   ----#


def PEARL_focus(number, ext="raw", fmode="trans", ttmode="TT70", atten=True, van_norm=True, debug=False):
    return focus(number=number, ext=ext, fmode=fmode, ttmode=ttmode, atten=atten, van_norm=van_norm, debug=debug)


def PEARL_align(work, focus):
    # TODO focus doesn't seem to do anything (just returns) is this API used?
    _align_workspace(work, focus)


def PEARL_read(number, ext, outname):
    raise NotImplementedError()
    #  _read_pearl_ws(number=number, ext=ext, outname=outname)


def PEARL_creategroup(calruns, ngroupfile="C:\PEARL\\test_cal_group_11_1.cal", ngroup="bank1,bank2,bank3,bank4"):
    raise NotImplementedError()
    #_create_group(calruns=calruns)


def PEARL_getfilename(run_number, ext):
    raise NotImplementedError()

def PEARL_createcal(calruns, noffsetfile="C:\PEARL\\pearl_offset_11_2.cal",
                    groupfile="P:\Mantid\\Calibration\\pearl_group_11_2_TT88.cal"):
    raise NotImplementedError()

def PEARL_createvan(van, empty, ext="raw", fmode="all", ttmode="TT88",
                    nvanfile="P:\Mantid\\Calibration\\van_spline_all_cycle_11_1.nxs", nspline=60, absorb=True,
                    debug=False):
    raise NotImplementedError()

def PEARL_startup(usern, thiscycle):
    # TODO turn this into a compatible wrapper for the old API
    raise NotImplementedError()