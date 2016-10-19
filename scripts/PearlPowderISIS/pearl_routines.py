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


def _create_van(user_input, van, empty, nvanfile, ext="raw", fmode="all", ttmode="TT88", nspline=60, absorb=True):
    global mode  # TODO used in PEARL_getmonitorspectrum
    global tt_mode
    mode = fmode
    tt_mode = ttmode

    generate_absorption = False # TODO implement this

    # tt_mode set here will not be used within the function but instead when the PEARL_calibfiles()
    # is called it will return the correct tt_mode files.

    cycle_information = _calculate_current_cycle(van)

    full_file_paths = _get_calib_files_full_paths(in_cycle=cycle_information["cycle"], in_tt_mode=ttmode,
                                                  in_pearl_file_dir=user_input.calibration_dir)
    wvan = "wvan"
    wempty = "wempty"
    print("Creating ", nvanfile)
    _read_pearl_ws(number=van, ext=ext, outname=wvan, raw_data_dir=user_input.raw_data_dir,
                   run_cycle=cycle_information["cycle"])
    _read_pearl_ws(number=empty, ext=ext, outname=wempty, raw_data_dir=user_input.raw_data_dir,
                   run_cycle=cycle_information["cycle"])
    mantid.Minus(LHSWorkspace=wvan, RHSWorkspace=wempty, OutputWorkspace=wvan)
    print("read van and empty")

    _remove_inter_ws(ws_to_remove=wempty)

    if absorb:
        mantid.ConvertUnits(InputWorkspace=wvan, OutputWorkspace=wvan, Target="Wavelength")

        # TODO Change out name from T to something meaningful
        mantid.LoadNexus(Filename=full_file_paths["vanadium_absorption"], OutputWorkspace="T")
        mantid.RebinToWorkspace(WorkspaceToRebin=wvan, WorkspaceToMatch="T", OutputWorkspace=wvan)
        mantid.Divide(LHSWorkspace=wvan, RHSWorkspace="T", OutputWorkspace=wvan)
        _remove_inter_ws(ws_to_remove="T")

    if generate_absorption:
        # Comment out 3 lines below if absorbtion file exists and uncomment the load line
        mantid.CreateSampleShape(wvan,'<sphere id="sphere_1"> <centre x="0" y="0" z= "0" />\
                          <radius val="0.005" /> </sphere>')

        mantid.AbsorptionCorrection(InputWorkspace=wvan,OutputWorkspace="T",AttenuationXSection="5.08",
        ScatteringXSection="5.1",SampleNumberDensity="0.072",NumberOfWavelengthPoints="25",ElementSize="0.05")

        mantid.SaveNexus(Filename=full_file_paths["vanadium_absorption"],InputWorkspace="T",Append=False)

    mantid.ConvertUnits(InputWorkspace=wvan, OutputWorkspace=wvan, Target="TOF")
    trange = "100,-0.0006,19990"
    print("Cropping TOF range to ", trange)
    mantid.Rebin(InputWorkspace=wvan, OutputWorkspace=wvan, Params=trange)

    # tmin,tmax=PEARL_gettofrange()
    # print "Cropping TOF range to ",tmin,tmax
    # CropWorkspace(wvan,wvan,XMin=tmin,XMax=tmax)

    vanfoc = "vanfoc_" + cycle_information["cycle"]
    mantid.AlignDetectors(InputWorkspace=wvan, OutputWorkspace=wvan, CalibrationFile=full_file_paths["calibration"])
    mantid.DiffractionFocussing(InputWorkspace=wvan, OutputWorkspace=vanfoc,
                                GroupingFileName=full_file_paths["grouping"])
    mantid.ConvertUnits(InputWorkspace=vanfoc, OutputWorkspace=vanfoc, Target="TOF")
    trange = "150,-0.0006,19900"
    print("Cropping TOF range to ", trange)
    mantid.Rebin(InputWorkspace=vanfoc, OutputWorkspace=vanfoc, Params=trange)
    mantid.ConvertUnits(InputWorkspace=vanfoc, OutputWorkspace=vanfoc, Target="dSpacing")

    _remove_inter_ws(ws_to_remove=wvan)

    if cycle_information["instrument_version"] == "new2":
        mantid.ConvertUnits(InputWorkspace=vanfoc, OutputWorkspace="vanmask", Target="dSpacing")
        _remove_inter_ws(ws_to_remove=vanfoc)

        # remove bragg peaks before spline

        print("About to strip Work=0")
        mantid.StripPeaks(InputWorkspace="vanmask", OutputWorkspace="vanstrip", FWHM=15, Tolerance=8, WorkspaceIndex=0)

        for i in range(1, 12):
            print("About to strip Work=" + str(i))
            mantid.StripPeaks(InputWorkspace="vanstrip", OutputWorkspace="vanstrip", FWHM=15, Tolerance=8,
                           WorkspaceIndex=i)

        # run twice on low angle as peaks are very broad
        print("About to strip work=12 and work=13 twice")
        for i in range(0, 2):
            print("About to strip Work=12")
            mantid.StripPeaks(InputWorkspace="vanstrip", OutputWorkspace="vanstrip", FWHM=100, Tolerance=10,
                        WorkspaceIndex=12)
            print("About to strip Work=13")
            mantid.StripPeaks(InputWorkspace="vanstrip", OutputWorkspace="vanstrip", FWHM=60, Tolerance=10,
                         WorkspaceIndex=13)

        print("Finished striping-out peaks...")

        _remove_inter_ws(ws_to_remove="vanmask")
        mantid.ConvertUnits(InputWorkspace="vanstrip", OutputWorkspace="vanstrip", Target="TOF")

        print("Starting splines...")

        for i in range(0, 14):
            mantid.SplineBackground(InputWorkspace="vanstrip", OutputWorkspace="spline" + str(i + 1), WorkspaceIndex=i,
                             NCoeff=nspline)

        # ConvertUnits("spline1","spline1","TOF")
        # ConvertUnits("spline2","spline2","TOF")
        # ConvertUnits("spline3","spline3","TOF")
        # ConvertUnits("spline4","spline4","TOF")
        # ConvertUnits("spline5","spline5","TOF")
        # ConvertUnits("spline6","spline6","TOF")
        # ConvertUnits("spline7","spline7","TOF")
        # ConvertUnits("spline8","spline8","TOF")
        # ConvertUnits("spline9","spline9","TOF")
        # ConvertUnits("spline10","spline10","TOF")
        # ConvertUnits("spline11","spline11","TOF")
        # ConvertUnits("spline12","spline12","TOF")

        mantid.SaveNexus(Filename=nvanfile, InputWorkspace="spline1", Append=False)

        for i in range(2, 15):
            mantid.SaveNexus(Filename=nvanfile, InputWorkspace="spline" + str(i), Append=True)

        _remove_inter_ws(ws_to_remove="vanstrip")
        for i in range(1, 15):
            _remove_inter_ws(ws_to_remove=("spline" + str(i)))

    elif cycle_information["instrument_version"] == "new":
        mantid.ConvertUnits(InputWorkspace=vanfoc, OutputWorkspace="vanmask", Target="dSpacing")
        _remove_inter_ws(ws_to_remove=vanfoc)

        # remove bragg peaks before spline
        mantid.StripPeaks(InputWorkspace="vanmask", OutputWorkspace="vanstrip", FWHM=15, Tolerance=8, WorkspaceIndex=0)

        for i in range(1, 12):
            mantid.StripPeaks(InputWorkspace="vanstrip", OutputWorkspace="vanstrip", FWHM=15, Tolerance=8,
                           WorkspaceIndex=i)

        _remove_inter_ws(ws_to_remove="vanmask")
        mantid.ConvertUnits(InputWorkspace="vanstrip", OutputWorkspace="vanstrip", Target="TOF")

        for i in range(0, 12):
            mantid.SplineBackground(InputWorkspace="vanstrip", OutputWorkspace="spline" + str(i + 1), WorkspaceIndex=i,
                             NCoeff=nspline)

        # ConvertUnits("spline1","spline1","TOF")
        # ConvertUnits("spline2","spline2","TOF")
        # ConvertUnits("spline3","spline3","TOF")
        # ConvertUnits("spline4","spline4","TOF")
        # ConvertUnits("spline5","spline5","TOF")
        # ConvertUnits("spline6","spline6","TOF")
        # ConvertUnits("spline7","spline7","TOF")
        # ConvertUnits("spline8","spline8","TOF")
        # ConvertUnits("spline9","spline9","TOF")
        # ConvertUnits("spline10","spline10","TOF")
        # ConvertUnits("spline11","spline11","TOF")
        # ConvertUnits("spline12","spline12","TOF")

        mantid.SaveNexus(Filename=nvanfile, InputWorkspace="spline1", Append=False)

        for i in range(2, 13):
            mantid.SaveNexus(Filename=nvanfile, InputWorkspace="spline" + str(i), Append=True)

        _remove_inter_ws(ws_to_remove="vanstrip")
        for i in range(1, 13):
            _remove_inter_ws(ws_to_remove=("spline" + str(i)))

    elif cycle_information["instrument_version"] == "old":
        mantid.ConvertUnits(InputWorkspace=vanfoc, OutputWorkspace="vanmask", Target="dSpacing")
        _remove_inter_ws(ws_to_remove=vanfoc)

        # remove bragg peaks before spline
        mantid.StripPeaks(InputWorkspace="vanmask", OutputWorkspace="vanstrip", FWHM=15, Tolerance=6, WorkspaceIndex=0)
        mantid.StripPeaks(InputWorkspace="vanstrip", OutputWorkspace="vanstrip", FWHM=15, Tolerance=6, WorkspaceIndex=2)
        mantid.StripPeaks(InputWorkspace="vanstrip", OutputWorkspace="vanstrip", FWHM=15, Tolerance=6, WorkspaceIndex=3)
        mantid.StripPeaks(InputWorkspace="vanstrip", OutputWorkspace="vanstrip", FWHM=40, Tolerance=12, WorkspaceIndex=1)
        mantid.StripPeaks(InputWorkspace="vanstrip", OutputWorkspace="vanstrip", FWHM=60, Tolerance=12, WorkspaceIndex=1)

        _remove_inter_ws(ws_to_remove="vanmask")

        # Mask low d region that is zero before spline
        for reg in range(0, 4):
            if (reg == 1):
                mantid.MaskBins(InputWorkspace="vanstrip", OutputWorkspace="vanstrip", XMin=0, XMax=0.14, SpectraList=reg)
            else:
                mantid.MaskBins(InputWorkspace="vanstrip", OutputWorkspace="vanstrip", XMin=0, XMax=0.06, SpectraList=reg)

        mantid.ConvertUnits(InputWorkspace="vanstrip", OutputWorkspace="vanstrip", Target="TOF")

        for i in range(0, 4):
            coeff = 100
            if (i == 1):
                coeff = 80
            mantid.SplineBackground(InputWorkspace="vanstrip", OutputWorkspace="spline" + str(i + 1), WorkspaceIndex=i,
                             NCoeff=coeff)

        # ConvertUnits("spline1","spline1","TOF")
        # ConvertUnits("spline2","spline2","TOF")
        # ConvertUnits("spline3","spline3","TOF")
        # ConvertUnits("spline4","spline4","TOF")

        mantid.SaveNexus(Filename=nvanfile, InputWorkspace="spline1", Append=False)
        for i in range(1, 4):
            mantid.SaveNexus(Filename=nvanfile, InputWorkspace="spline" + str(i), Append=True)

        _remove_inter_ws(ws_to_remove="vanstrip")
        for i in range(1, 5):
            _remove_inter_ws(ws_to_remove="spline" + str(i))

    else:
        print("Sorry I don't know that mode")
        return

    mantid.LoadNexus(Filename=nvanfile, OutputWorkspace="Van_data")

    return


def _create_calibration(calruns, noffsetfile, groupfile, user_params):
    cycle_information = _calculate_current_cycle(calruns)


    wcal = "cal_raw"
    _read_pearl_ws(number=calruns, ext="raw", outname=wcal, raw_data_dir=user_params.raw_data_dir,
                   run_cycle=cycle_information["cycle"])

    if cycle_information["instrument_version"] == "new" or cycle_information["instrument_version"] == "new2":
        mantid.Rebin(InputWorkspace=wcal, OutputWorkspace=wcal, Params="100,-0.0006,19950")

    mantid.ConvertUnits(InputWorkspace=wcal, OutputWorkspace="cal_inD", Target="dSpacing")
    mantid.Rebin(InputWorkspace="cal_inD", OutputWorkspace="cal_Drebin", Params="1.8,0.002,2.1")

    if cycle_information["instrument_version"] == "new2":
        mantid.CrossCorrelate(InputWorkspace="cal_Drebin", OutputWorkspace="crosscor", ReferenceSpectra=20,
                       WorkspaceIndexMin=9, WorkspaceIndexMax=1063, XMin=1.8, XMax=2.1)
    elif cycle_information["instrument_version"] == "new":
        mantid.CrossCorrelate(InputWorkspace="cal_Drebin", OutputWorkspace="crosscor", ReferenceSpectra=20,
                       WorkspaceIndexMin=9, WorkspaceIndexMax=943, XMin=1.8, XMax=2.1)
    else:
        mantid.CrossCorrelate(InputWorkspace="cal_Drebin", OutputWorkspace="crosscor", ReferenceSpectra=500,
                       WorkspaceIndexMin=1, WorkspaceIndexMax=1440, XMin=1.8, XMax=2.1)

    # Ceo Cell refeined to 5.4102(3) so 220 is 1.912795
    mantid.GetDetectorOffsets(InputWorkspace="crosscor", OutputWorkspace="OutputOffsets", Step=0.002, DReference=1.912795,
                       XMin=-200, XMax=200, GroupingFileName=noffsetfile)
    mantid.AlignDetectors(InputWorkspace=wcal, OutputWorkspace="cal_aligned", CalibrationFile=noffsetfile)
    mantid.DiffractionFocussing(InputWorkspace="cal_aligned", OutputWorkspace="cal_grouped", GroupingFileName=groupfile)

    return


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