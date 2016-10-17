# pylint: disable=anomalous-backslash-in-string, global-variable-undefined, global-variable-not-assigned
# pylint: disable=invalid-name, too-many-arguments, superfluous-parens, too-many-branches, redefined-builtin

import os.path
import sys

import mantid.simpleapi as mantid
import numpy as n

# directories generator
import pearl_calib_factory
import pearl_cycle_factory


def PEARL_startup(usern="matt", thiscycle='11_1'):
    global attenfile
    global currentdatadir
    global tofbinning
    global tt_mode
    global pearl_file_dir
    global userdataprocessed

    # ------ User modifiable ----- #
    # TODO - Do we change this to be set from the outside i.e. params

    # Directories
    calibration_directory = "D:\\PEARL\\"  # TODO remove my hardcoded path and generic it
    raw_data_directory = "D:\\PEARL\\"
    live_data_directory = None  # Is this even used?

    output_directory = "D:\\PEARL\\output\\"

    # File names
    atten_file_name = "PRL112_DC25_10MM_FF.OUT"
    cal_file_name = "pearl_offset_11_2.cal"
    group_file_name = "pearl_group_11_2_TT88.cal"
    van_absorb_file_name = "van_spline_all_cycle_11_1.nxs"
    van_file_name = "van_spline_all_cycle_11_1.nxs"

    # Script settings
    mode = "all"
    tt_mode = "TT88"
    tofbinning = "1500,-0.0006,19900"

    # ---- Script setup ---- #
    # Setup globals
    pearl_file_dir = calibration_directory
    currentdatadir = raw_data_directory
    livedatadir = live_data_directory
    userdataprocessed = output_directory + "Cycle_" + thiscycle + "\\" + usern + "\\"


    # Gen full paths to files
    attenfile = calibration_directory + atten_file_name  # Is this in calibration folder or raw data folder?
    calfile = calibration_directory + cal_file_name
    groupfile = calibration_directory + group_file_name
    vabsorbfile = calibration_directory + van_absorb_file_name
    vanfile = calibration_directory + van_file_name

    # TODO now would be a good time to check they all exist

    PEARL_setdatadir(raw_data_directory)

    print "Output cycle is set to", thiscycle
    print "Processed Data in : ", userdataprocessed
    print "Offset file set to :", calfile
    print "Grouping file set to :", groupfile
    print "Vanadium file is :", vanfile
    print "The default focusing mode is :", mode
    print "Time of flight binning set to :", tofbinning
    print
    return


def PEARL_getlambdarange():
    return 0.03, 6.00


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
            print "Sorry mode not supported"
            sys.exit(0)
    else:
        mspectra = 1

    print "Monitor spectrum is", mspectra

    return mspectra


def PEARL_getcycle(number):
    global cycle
    global instver
    global datadir

    cycle, instver, datadir = pearl_cycle_factory.get_cycle_dir(number, currentdatadir)

    print "ISIS cycle is set to", cycle

    return


# pylint: disable=unused-variable, too-many-locals, too-many-statements, undefined-loop-variable, redefined-outer-name
def PEARL_getcalibfiles():
    global calfile
    global groupfile
    global vabsorbfile
    global vanfile

    print "Setting calibration for cycle", cycle

    calfile, groupfile, vabsorbfile, vanfile, instver = \
        pearl_calib_factory.get_calibration_dir(cycle, tt_mode, pearl_file_dir)
    return


# sets the intial directory for all calibration files
def pearl_initial_dir(directory='P:\Mantid\\'):
    global pearl_file_dir
    pearl_file_dir = directory
    print "Set pearl_file_dir directory to ", directory
    return


# sets the current raw data files directory
def pearl_set_currentdatadir(directory="I:\\"):
    global currentdatadir
    currentdatadir = directory
    print "Set currentdatadir directory to ", directory
    return


# sets the user data output directory
def pearl_set_userdataoutput_dir(directory="P:\\users\\MantidOutput\\"):
    global userdataprocessed
    userdataprocessed = directory
    print "Set userdataprocessed directory to ", directory
    return


def PEARL_setdatadir(directory="C:\PEARL\RAW\\"):
    global pearl_datadir
    pearl_datadir = directory
    print "Set pearl_datadir directory to ", directory
    return


# sets the atten file's directory
def PEARL_setattenfile(new_atten="P:\Mantid\\Attentuation\\PRL985_WC_HOYBIDE_NK_10MM_FF.OUT"):
    global attenfile
    attenfile = new_atten
    print "Set attenuation file to ", attenfile
    return


def PEARL_datadir():
    return pearl_datadir


def PEARL_getfilename(run_number, ext):
    global livedatadir

    if (ext[0] != 's'):
        data_dir = PEARL_datadir()
    else:
        data_dir = livedatadir
    digit = len(str(run_number))

    if (run_number < 71009):
        numdigits = 5
        #
        # filename=data_dir+"PRL"
        #
        filename = "PRL"
    else:
        numdigits = 8
        #
        # filename=data_dir+"PEARL"
        #
        filename = "PEARL"

    for i in range(0, numdigits - digit):
        filename = filename + "0"

    filename += str(run_number) + "." + ext

    full_filename = data_dir + filename
    #
    # Check if file exists in default data folder & if not use alternate folder stored in "datadir"...
    #
    if not os.path.exists(full_filename):
        print "No such file as ", full_filename, "; trying X-drive folder..."

        full_filename = datadir + filename

    return full_filename


def PearlLoad(files, ext, outname):
    if isinstance(files, int):
        infile = PEARL_getfilename(files, ext)
        print "loading ", infile, "into ", outname
        print "--g_debugGING: ", mantid.LoadRaw.func_code.co_filename
        mantid.LoadRaw(Filename=infile, OutputWorkspace=outname, LoadLogFiles="0")
    else:
        loop = 0
        num = files.split("_")
        frange = range(int(num[0]), int(num[1]) + 1)
        for i in frange:
            infile = PEARL_getfilename(i, ext)
            print "loading ", infile
            outwork = "run" + str(i)
            mantid.LoadRaw(Filename=infile, OutputWorkspace=outwork, LoadLogFiles="0")
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


def PearlLoadMon(files, ext, outname):
    if isinstance(files, int):
        infile = PEARL_getfilename(files, ext)
        mspectra = PEARL_getmonitorspectrum(files)
        print "loading ", infile, "into ", outname
        mantid.LoadRaw(Filename=infile, OutputWorkspace=outname, SpectrumMin=mspectra, SpectrumMax=mspectra,
                  LoadLogFiles="0")
    else:
        loop = 0
        num = files.split("_")
        frange = range(int(num[0]), int(num[1]) + 1)
        mspectra = PEARL_getmonitorspectrum(int(num[0]))
        for i in frange:
            infile = PEARL_getfilename(i, ext)
            print "loading ", infile
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


def PEARL_getmonitor(number, ext, spline_terms=20):
    works = "monitor" + str(number)
    PearlLoadMon(number, ext, works)
    mantid.ConvertUnits(InputWorkspace=works, OutputWorkspace=works, Target="Wavelength")
    lmin, lmax = PEARL_getlambdarange()
    mantid.CropWorkspace(InputWorkspace=works, OutputWorkspace=works, XMin=lmin, XMax=lmax)
    ex_regions = n.zeros((2, 4))
    ex_regions[:, 0] = [3.45, 3.7]
    ex_regions[:, 1] = [2.96, 3.2]
    ex_regions[:, 2] = [2.1, 2.26]
    ex_regions[:, 3] = [1.73, 1.98]
    # ConvertToDistribution(works)

    for reg in range(0, 4):
        mantid.MaskBins(InputWorkspace=works, OutputWorkspace=works, XMin=ex_regions[0, reg], XMax=ex_regions[1, reg])

    mantid.SplineBackground(InputWorkspace=works, OutputWorkspace=works, WorkspaceIndex=0, NCoeff=spline_terms)

    return works


def PEARL_read(number, ext, outname):
    PearlLoad(number, ext, outname)
    mantid.ConvertUnits(InputWorkspace=outname, OutputWorkspace=outname, Target="Wavelength")
    # lmin,lmax=WISH_getlambdarange()
    # CropWorkspace(output,output,XMin=lmin,XMax=lmax)
    monitor = PEARL_getmonitor(number, ext, spline_terms=20)
    # NormaliseToMonitor(InputWorkspace=outname,OutputWorkspace=outname,MonitorWorkspace=monitor)
    mantid.NormaliseToMonitor(InputWorkspace=outname, OutputWorkspace=outname, MonitorWorkspace=monitor,
                       IntegrationRangeMin=0.6, IntegrationRangeMax=5.0)
    mantid.ConvertUnits(InputWorkspace=outname, OutputWorkspace=outname, Target="TOF")
    mantid.mtd.remove(monitor)
    # ReplaceSpecialValues(output,output,NaNValue=0.0,NaNError=0.0,InfinityValue=0.0,InfinityError=0.0)
    return


def PEARL_align(work, focus):
    PEARL_getcalibfiles()
    mantid.AlignDetectors(InputWorkspace=work, OutputWorkspace=work, CalibrationFile=calfile)
    # mtd.remove(work)
    return focus


def PEARL_focus(number, ext="raw", fmode="trans", ttmode="TT70", atten=True, van_norm=True, debug=False):
    global instver
    global g_debug
    g_debug = debug
    PEARL_getcycle(number)


    print "Instrument version is:", instver
    if (instver == "new2"):
        outwork = pearl_run_focus(number, ext=ext, fmode=fmode, ttmode=ttmode, atten=atten, van_norm=van_norm, version=2)
    else:
        outwork = pearl_run_focus(number, ext=ext, fmode=fmode, ttmode=ttmode, atten=atten, van_norm=van_norm, version=1)

    return outwork


def pearl_run_focus(number, ext="raw", fmode="trans", ttmode="TT70", atten=True, van_norm=True, version=1):
    if version == 1:
        alg_range = 12
        save_range = 3
    elif version == 2:
        alg_range = 14
        save_range = 5

    work = "work"
    focus = "focus"

    PEARL_getcycle(number)
    PEARL_getcalibfiles()

    print "Focussing mode is:", fmode
    print "Two theta mode is:", tt_mode
    print "Group file is", groupfile
    print "Calibration file is", calfile
    print "Tof binning", tofbinning

    if isinstance(number, int):
        outfile = userdataprocessed + "PRL" + str(number) + ".nxs"
        gssfile = userdataprocessed + "PRL" + str(number) + ".gss"
        tof_xye_file = userdataprocessed + "PRL" + str(number) + "_tof_xye.dat"
        d_xye_file = userdataprocessed + "PRL" + str(number) + "_d_xye.dat"
        outwork = "PRL" + str(number)
    else:
        outfile = userdataprocessed + "PRL" + number + ".nxs"
        gssfile = userdataprocessed + "PRL" + number + ".gss"
        tof_xye_file = userdataprocessed + "PRL" + number + "_tof_xye.dat"
        d_xye_file = userdataprocessed + "PRL" + number + "_d_xye.dat"

        outwork = "PRL" + number
    PEARL_read(number, ext, work)
    mantid.Rebin(InputWorkspace=work, OutputWorkspace=work, Params=tofbinning)
    mantid.AlignDetectors(InputWorkspace=work, OutputWorkspace=work, CalibrationFile=calfile)
    mantid.DiffractionFocussing(InputWorkspace=work, OutputWorkspace=focus, GroupingFileName=groupfile)

    _remove_inter_ws(ws_to_remove=work)

    for i in range(0, alg_range):

        output = outwork + "_mod" + str(i + 1)
        van = "van" + str(i + 1)
        rdata = "rdata" + str(i + 1)

        if (van_norm):
            print "Using vanadium file", vanfile
            mantid.LoadNexus(Filename=vanfile, OutputWorkspace=van, EntryNumber=i + 1)
            mantid.ExtractSingleSpectrum(InputWorkspace=focus, OutputWorkspace=rdata, WorkspaceIndex=i)
            # ConvertUnits(van,van,"TOF")
            mantid.Rebin(InputWorkspace=van, OutputWorkspace=van, Params=tofbinning)
            mantid.ConvertUnits(InputWorkspace=rdata, OutputWorkspace=rdata, Target="TOF")
            mantid.Rebin(InputWorkspace=rdata, OutputWorkspace=rdata, Params=tofbinning)
            mantid.Divide(LHSWorkspace=rdata, RHSWorkspace=van, OutputWorkspace=output)
            mantid.CropWorkspace(InputWorkspace=output, OutputWorkspace=output, XMin=0.1)
            mantid.Scale(InputWorkspace=output, OutputWorkspace=output, Factor=10)
        else:
            print "Not Using vanadium file"
            # LoadNexus(Filename=vanfile,OutputWorkspace=van,EntryNumber=i+1)
            mantid.ExtractSingleSpectrum(InputWorkspace=focus, OutputWorkspace=rdata, WorkspaceIndex=i)
            # ConvertUnits(van,van,"TOF")
            # Rebin(van,van,tofbinning)
            mantid.ConvertUnits(InputWorkspace=rdata, OutputWorkspace=rdata, Target="TOF")
            mantid.Rebin(InputWorkspace=rdata, OutputWorkspace=output, Params=tofbinning)
            # Divide(rdata,van,output)
            mantid.CropWorkspace(InputWorkspace=output, OutputWorkspace=output, XMin=0.1)

    _remove_inter_ws(focus)

    if (fmode == "all"):

        name = outwork + "_mods1-9"

        input = outwork + "_mod1"

        mantid.CloneWorkspace(InputWorkspace=input, OutputWorkspace=name)

        for i in range(1, 9):
            toadd = outwork + "_mod" + str(i + 1)
            mantid.Plus(LHSWorkspace=name, RHSWorkspace=toadd, OutputWorkspace=name)

        mantid.Scale(InputWorkspace=name, OutputWorkspace=name, Factor=0.111111111111111)

        mantid.SaveGSS(InputWorkspace=name, Filename=gssfile, Append=False, Bank=1)

        mantid.ConvertUnits(InputWorkspace=name, OutputWorkspace=name, Target="dSpacing")

        mantid.SaveNexus(Filename=outfile, InputWorkspace=name, Append=False)

        for i in range(0, 3):
            tosave = outwork + "_mod" + str(i + 10)

            mantid.SaveGSS(InputWorkspace=tosave, Filename=gssfile, Append=True, Bank=i + 2)

            mantid.ConvertUnits(InputWorkspace=tosave, OutputWorkspace=tosave, Target="dSpacing")

            mantid.SaveNexus(Filename=outfile, InputWorkspace=tosave, Append=True)

        for i in range(0, alg_range):
            _remove_inter_ws(ws_to_remove=(outwork + "_mod" + str(i + 1)))
            _remove_inter_ws(ws_to_remove=("van" + str(i + 1)))
            _remove_inter_ws(ws_to_remove="rdata" + str(i + 1))
            _remove_inter_ws(ws_to_remove=name)

    elif (fmode == "groups"):

        name = []
        name.extend((outwork + "_mods1-3", outwork + "_mods4-6",
                     outwork + "_mods7-9", outwork + "_mods4-9"))

        input = []
        input.extend((outwork + "_mod1", outwork + "_mod4", outwork + "_mod7"))

        for i in range(0, 3):
            mantid.CloneWorkspace(InputWorkspace=input[i], OutputWorkspace=name[i])

        for i in range(1, 3):
            toadd = outwork + "_mod" + str(i + 1)
            mantid.Plus(LHSWorkspace=name[0], RHSWorkspace=toadd, OutputWorkspace=name[0])

        mantid.Scale(InputWorkspace=name[0], OutputWorkspace=name[0], Factor=0.333333333333)

        for i in range(1, 3):
            toadd = outwork + "_mod" + str(i + 4)
            mantid.Plus(LHSWorkspace=name[1], RHSWorkspace=toadd, OutputWorkspace=name[1])

        mantid.Scale(InputWorkspace=name[1], OutputWorkspace=name[1], Factor=0.333333333333)

        for i in range(1, 3):
            toadd = outwork + "_mod" + str(i + 7)
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

            if version == 1:
                mantid.SaveGSS(InputWorkspace=name[i], Filename=gssfile, Append=append, Bank=i + 1)
            elif version == 2:
                mantid.SaveGSS(InputWorkspace=name[i], Filename=gssfile, Append=False, Bank=i + 1)

            mantid.ConvertUnits(InputWorkspace=name[i], OutputWorkspace=name[i], Target="dSpacing")
            mantid.SaveNexus(Filename=outfile, InputWorkspace=name[i], Append=append)

        for i in range(0, save_range):
            tosave = outwork + "_mod" + str(i + 10)

            mantid.SaveGSS(InputWorkspace=tosave, Filename=gssfile, Append=True, Bank=i + 5)

            mantid.ConvertUnits(InputWorkspace=tosave, OutputWorkspace=tosave, Target="dSpacing")

            mantid.SaveNexus(Filename=outfile, InputWorkspace=tosave, Append=True)

        for i in range(0, alg_range):
            _remove_inter_ws(ws_to_remove=(outwork + "_mod" + str(i + 1)))
            _remove_inter_ws(ws_to_remove=("van" + str(i + 1)))
            _remove_inter_ws(ws_to_remove=("rdata" + str(i + 1)))
            _remove_inter_ws(ws_to_remove=output)
        for i in range(1, 4):
            _remove_inter_ws(ws_to_remove=name[i])

    elif (fmode == "trans"):

        name = outwork + "_mods1-9"

        input = outwork + "_mod1"

        mantid.CloneWorkspace(InputWorkspace=input, OutputWorkspace=name)

        for i in range(1, 9):
            toadd = outwork + "_mod" + str(i + 1)
            mantid.Plus(LHSWorkspace=name, RHSWorkspace=toadd, OutputWorkspace=name)

        mantid.Scale(InputWorkspace=name, OutputWorkspace=name, Factor=0.111111111111111)

        if (atten):
            no_att = outwork + "_noatten"

            mantid.ConvertUnits(InputWorkspace=name, OutputWorkspace=name, Target="dSpacing")
            mantid.CloneWorkspace(InputWorkspace=name, OutputWorkspace=no_att)

            PEARL_atten(name, name)

            mantid.ConvertUnits(InputWorkspace=name, OutputWorkspace=name, Target="TOF")

        mantid.SaveGSS(InputWorkspace=name, Filename=gssfile, Append=False, Bank=1)
        mantid.SaveFocusedXYE(InputWorkspace=name, Filename=tof_xye_file, Append=False, IncludeHeader=False)

        mantid.ConvertUnits(InputWorkspace=name, OutputWorkspace=name, Target="dSpacing")

        mantid.SaveFocusedXYE(InputWorkspace=name, Filename=d_xye_file, Append=False, IncludeHeader=False)
        mantid.SaveNexus(Filename=outfile, InputWorkspace=name, Append=False)

        for i in range(0, 9):
            tosave = outwork + "_mod" + str(i + 1)
            # SaveGSS(tosave,Filename=gssfile,Append=True,Bank=i+2)
            mantid.ConvertUnits(InputWorkspace=tosave, OutputWorkspace=tosave, Target="dSpacing")
            mantid.SaveNexus(Filename=outfile, InputWorkspace=tosave, Append=True)

        for i in range(0, alg_range):
            _remove_inter_ws(ws_to_remove=(outwork + "_mod" + str(i + 1)))
            _remove_inter_ws(ws_to_remove=("van" + str(i + 1)))
            _remove_inter_ws(ws_to_remove=("rdata" + str(i + 1)))
            _remove_inter_ws(ws_to_remove=output)
        _remove_inter_ws(ws_to_remove=name)

    elif (fmode == "mods"):

        for i in range(0, alg_range):

            output = outwork + "_mod" + str(i + 1)

            van = "van" + str(i + 1)

            rdata = "rdata" + str(i + 1)

            status = True

            if (i == 0):
                status = False

            mantid.SaveGSS(InputWorkspace=output, Filename=gssfile, Append=status, Bank=i + 1)

            mantid.ConvertUnits(InputWorkspace=output, OutputWorkspace=output, Target="dSpacing")

            mantid.SaveNexus(Filename=outfile, InputWorkspace=output, Append=status)

            _remove_inter_ws(ws_to_remove=rdata)
            _remove_inter_ws(ws_to_remove=van)
            _remove_inter_ws(ws_to_remove=output)

    else:
        print "Sorry I don't know that mode", fmode
        return

    mantid.LoadNexus(Filename=outfile, OutputWorkspace=outwork)
    return outwork


def PEARL_createvan(van, empty, ext="raw", fmode="all", ttmode="TT88",
                    nvanfile="P:\Mantid\\Calibration\\van_spline_all_cycle_11_1.nxs", nspline=60, absorb=True):
    global mode
    global tt_mode
    mode = fmode
    tt_mode = ttmode

    # tt_mode set here will not be used within the function but instead when the PEARL_calibfiles()
    # is called it will return the correct tt_mode files.

    PEARL_getcycle(van)
    PEARL_getcalibfiles()
    wvan = "wvan"
    wempty = "wempty"
    print "Creating ", nvanfile
    PEARL_read(van, ext, wvan)
    PEARL_read(empty, ext, wempty)
    mantid.Minus(LHSWorkspace=wvan, RHSWorkspace=wempty, OutputWorkspace=wvan)
    print "read van and empty"

    _remove_inter_ws(ws_to_remove=wempty)

    if (absorb):
        print "Correcting Vanadium for absorbtion"
        mantid.ConvertUnits(InputWorkspace=wvan, OutputWorkspace=wvan, Target="Wavelength")
        print "This will create", vabsorbfile
        # Comment out 3 lines below if absorbtion file exists and uncomment the load line
        # CreateSampleShape(wvan,'<sphere id="sphere_1"> <centre x="0" y="0" z= "0" />
        # <radius val="0.005" /> </sphere>')
        # AbsorptionCorrection(InputWorkspace=wvan,OutputWorkspace="T",AttenuationXSection="5.08",
        # ScatteringXSection="5.1",SampleNumberDensity="0.072",NumberOfWavelengthPoints="25",ElementSize="0.05")
        # SaveNexus(Filename=vabsorbfile,InputWorkspace="T",Append=False)
        # TODO Change out name from T to something meaningful
        mantid.LoadNexus(Filename=vabsorbfile, OutputWorkspace="T")
        mantid.RebinToWorkspace(WorkspaceToRebin=wvan, WorkspaceToMatch="T", OutputWorkspace=wvan)
        mantid.Divide(LHSWorkspace=wvan, RHSWorkspace="T", OutputWorkspace=wvan)
        _remove_inter_ws(ws_to_remove="T")



    mantid.ConvertUnits(InputWorkspace=wvan, OutputWorkspace=wvan, Target="TOF")
    trange = "100,-0.0006,19990"
    print "Cropping TOF range to ", trange
    mantid.Rebin(InputWorkspace=wvan, OutputWorkspace=wvan, Params=trange)
    # tmin,tmax=PEARL_gettofrange()
    # print "Cropping TOF range to ",tmin,tmax
    # CropWorkspace(wvan,wvan,XMin=tmin,XMax=tmax)

    vanfoc = "vanfoc_" + cycle
    mantid.AlignDetectors(InputWorkspace=wvan, OutputWorkspace=wvan, CalibrationFile=calfile)
    mantid.DiffractionFocussing(InputWorkspace=wvan, OutputWorkspace=vanfoc, GroupingFileName=groupfile)
    mantid.ConvertUnits(InputWorkspace=vanfoc, OutputWorkspace=vanfoc, Target="TOF")
    trange = "150,-0.0006,19900"
    print "Cropping TOF range to ", trange
    mantid.Rebin(InputWorkspace=vanfoc, OutputWorkspace=vanfoc, Params=trange)
    mantid.ConvertUnits(InputWorkspace=vanfoc, OutputWorkspace=vanfoc, Target="dSpacing")

    _remove_inter_ws(ws_to_remove=wvan)

    if (instver == "new2"):
        mantid.ConvertUnits(InputWorkspace=vanfoc, OutputWorkspace="vanmask", Target="dSpacing")
        _remove_inter_ws(ws_to_remove=vanfoc)

        # remove bragg peaks before spline

        print "About to strip Work=0"
        mantid.StripPeaks(InputWorkspace="vanmask", OutputWorkspace="vanstrip", FWHM=15, Tolerance=8, WorkspaceIndex=0)

        for i in range(1, 12):
            print "About to strip Work=" + str(i)
            mantid.StripPeaks(InputWorkspace="vanstrip", OutputWorkspace="vanstrip", FWHM=15, Tolerance=8,
                           WorkspaceIndex=i)

        # run twice on low angle as peaks are very broad
        print("About to strip work=12 and work=13 twice")
        for i in range(0, 2):
            print "About to strip Work=12"
            mantid.StripPeaks(InputWorkspace="vanstrip", OutputWorkspace="vanstrip", FWHM=100, Tolerance=10,
                        WorkspaceIndex=12)
            print "About to strip Work=13"
            mantid.StripPeaks(InputWorkspace="vanstrip", OutputWorkspace="vanstrip", FWHM=60, Tolerance=10,
                         WorkspaceIndex=13)

        print "Finished striping-out peaks..."

        _remove_inter_ws(ws_to_remove="vanmask")
        mantid.ConvertUnits(InputWorkspace="vanstrip", OutputWorkspace="vanstrip", Target="TOF")

        print "Starting splines..."

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

    elif (instver == "new"):
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

    elif (instver == "old"):
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
        print "Sorry I don't know that mode"
        return

    mantid.LoadNexus(Filename=nvanfile, OutputWorkspace="Van_data")

    return


def PEARL_createcal(calruns, noffsetfile="C:\PEARL\\pearl_offset_11_2.cal",
                    groupfile="P:\Mantid\\Calibration\\pearl_group_11_2_TT88.cal"):
    PEARL_getcycle(calruns)

    print "Instrument version is ", instver

    wcal = "cal_raw"
    PEARL_read(calruns, "raw", wcal)

    if instver == "new" or instver == "new2":
        mantid.Rebin(InputWorkspace=wcal, OutputWorkspace=wcal, Params="100,-0.0006,19950")

    mantid.ConvertUnits(InputWorkspace=wcal, OutputWorkspace="cal_inD", Target="dSpacing")
    mantid.Rebin(InputWorkspace="cal_inD", OutputWorkspace="cal_Drebin", Params="1.8,0.002,2.1")

    if instver == "new2":
        mantid.CrossCorrelate(InputWorkspace="cal_Drebin", OutputWorkspace="crosscor", ReferenceSpectra=20,
                       WorkspaceIndexMin=9, WorkspaceIndexMax=1063, XMin=1.8, XMax=2.1)
    elif instver == "new":
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
    PEARL_getcycle(calruns)

    wcal = "cal_raw"
    PEARL_read(calruns, "raw", wcal)

    if instver == "new" or instver == "new2":
        mantid.Rebin(InputWorkspace=wcal, OutputWorkspace=wcal, Params="100,-0.0006,19950")

    mantid.ConvertUnits(InputWorkspace=wcal, OutputWorkspace="cal_inD", Target="dSpacing")

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

    return


def PEARL_creategroup(calruns, ngroupfile="C:\PEARL\\test_cal_group_11_1.cal", ngroup="bank1,bank2,bank3,bank4"):
    PEARL_getcycle(calruns)

    wcal = "cal_raw"
    PEARL_read(calruns, "raw", wcal)
    mantid.ConvertUnits(InputWorkspace=wcal, OutputWorkspace="cal_inD", Target="dSpacing")
    mantid.CreateCalFileByNames(InstrumentWorkspace=wcal, GroupingFileName=ngroupfile, GroupNames=ngroup)
    return


def PEARL_sumspec(number, ext, mintof=500, maxtof=1000, minspec=0, maxspec=943):
    PEARL_getcycle(number)
    if (instver == "old"):
        maxspec = 2720
    elif (instver == "new"):
        maxspec = 943
    else:
        maxspec = 1063

    PearlLoad(number, ext, "work")
    mantid.NormaliseByCurrent(InputWorkspace="work", OutputWorkspace="work")
    mantid.Integration(InputWorkspace="work", OutputWorkspace="integral", RangeLower=mintof, RangeUpper=maxtof,
                StartWorkspaceIndex=minspec, EndWorkspaceIndex=maxspec)
    mantid.mtd.remove("work")
    # sumplot=plotBin("integral",0)
    return


def PEARL_sumspec_lam(number, ext, minlam=0.1, maxlam=4, minspec=8, maxspec=943):
    PEARL_getcycle(number)
    if (instver == "old"):
        maxspec = 2720
    elif (instver == "new"):
        maxspec = 943
    else:
        maxspec = 1063

    PearlLoad(number, ext, "work")
    mantid.NormaliseByCurrent(InputWorkspace="work", OutputWorkspace="work")
    mantid.AlignDetectors(InputWorkspace="work", OutputWorkspace="work", CalibrationFile=calfile)
    mantid.ConvertUnits(InputWorkspace="worl", OutputWorkspace="work", Target="Wavelength")
    mantid.Integration(InputWorkspace="work", OutputWorkspace="integral", RangeLower=minlam, RangeUpper=maxlam,
                StartWorkspaceIndex=minspec, EndWorkspaceIndex=maxspec)
    mantid.mtd.remove("work")
    # sumplot=plotBin("integral",0)
    return


def PEARL_atten(work, outwork):
    # attenfile="P:\Mantid\\Attentuation\\PRL985_WC_HOYBIDE_NK_10MM_FF.OUT"
    print "Correct for attenuation using", attenfile
    wc_atten = mantid.PearlMCAbsorption(attenfile)
    mantid.ConvertToHistogram(InputWorkspace="wc_atten", OutputWorkspace="wc_atten")
    mantid.RebinToWorkspace(WorkspaceToRebin="wc_atten", WorkspaceToMatch=work, OutputWorkspace="wc_atten")
    mantid.Divide(LHSWorkspace=work, RHSWorkspace="wc_atten", OutputWorkspace=outwork)
    _remove_inter_ws(ws_to_remove="wc_atten")
    return


def PEARL_add(a_name, a_spectra, a_outname, atten=True):
    w_add_out = a_outname
    gssfile = userdataprocessed + a_outname + ".gss"
    nxsfile = userdataprocessed + a_outname + ".nxs"

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


def _remove_inter_ws(ws_to_remove):
    """
    Removes any intermediate workspaces if debug is set to false
        @param ws_to_remove: The workspace to remove from the ADS
    """
    if not g_debug:
        mantid.mtd.remove(ws_to_remove)

