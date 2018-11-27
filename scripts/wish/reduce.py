# Set of routines to normalise WISH data- new look Mantid with mantidsimple removed
import mantid.simpleapi as mantid
import matplotlib.pyplot as p
import numpy as n


def Wish_Run(input_mode, calibration_folder, input_dir, output_dir, deleteWorkspace):
    wish_dir = ""

    # Get the valid wavelength range, i.e. excluding regions where choppers cut
    def WISH_getlambdarange():
        return 0.7, 10.35

    def WISH_setuser(usern):
        global username
        username = usern

    def WISH_setdatadir(directory="/archive/ndxwish/Instrument/data/cycle_09_5/"):
        global wish_datadir
        wish_datadir = directory

    def WISH_setuserdir(directory):
        global wish_userdir
        wish_userdir = directory

    def WISH_datadir():
        return wish_datadir

    def WISH_userdir(cycle='cycle_10_1'):
        return wish_userdir

    def WISH_calibration(cycle="11_4"):
        return "/home/sjenkins/Work/Build-1/ExternalData/Testing/Data/SystemTest/WISH/input/Cal/"

    def WISH_startup(usern, cycle='14_3'):
        global userdatadir
        global userdataprocessed
        global mtdplt
        import sys
        sys.path.append('/data/' + usern + '/Scripts/')
        #	import Mantid_plotting as mtdplt
        #	userdatadir="/data/rawdata/"
        userdatadir = "/archive/ndxwish/Instrument/data/cycle_" + cycle + '/'
        WISH_setdatadir(userdatadir)
        print "Raw Data in :   ", userdatadir
        userdataprocessed = "/home/sjenkins/Work/Build-1/ExternalData/Testing/Data/SystemTest/WISH/output/"
        WISH_setuserdir(directory=userdataprocessed)
        print "Processed Data in :   ", userdataprocessed
        return

    # Returns the calibration filename
    def WISH_cal(panel):
        return WISH_calibration() + "WISH_cycle_10_3_noends_10to10.cal"

    # return "/data/mp43/Desktop/Si_Mar15/test_detOffsets_SiMar15_noends.cal"
    # return "/data/ryb18365/Desktop/WISH_cycle_10_3_noends_10to10_dodgytubesremoved.cal"
    # return "/data/mp43/Calibration/Cycle_11_4/test3_VX4.cal"
    # return "/data/mp43/Desktop/sputnik_NiSbF6/sapphire_masked_sputnik.cal"

    # Returns the grouping filename
    def WISH_group():
        return WISH_calibration() + "WISH_cycle_10_3_noends_10to10.cal"

    # return "/data/mp43/Desktop/Si_Mar15/test_detOffsets_SiMar15_noends.cal"
    # return "/data/ryb18365/Desktop/WISH_cycle_10_3_noends_10to10_dodgytubesremoved.cal"
    # return "/data/mp43/Calibration/Cycle_11_4/test3_VX4.cal"
    # return "/data/mp43/Desktop/sputnik_NiSbF6/sapphire_masked_sputnik.cal"

    def WISH_getvana(panel, SE="candlestick", cycle="09_4"):
        if (SE == "candlestick"):
            if (cycle == "09_2"):
                return WISH_calibration() + "vana318-" + str(panel) + "foc-rmbins-smooth50.nx5"
            if (cycle == "09_3"):
                return WISH_calibration(cycle) + "vana935-" + str(panel) + "foc-SS.nx5"
            if (cycle == "09_4"):
                return WISH_calibration(cycle) + "vana3123-" + str(panel) + "foc-SS.nx5"
            if (cycle == "09_5"):
                return WISH_calibration(cycle) + "vana3123-" + str(panel) + "foc-SS.nx5"
            if (cycle == "11_4"):
                return WISH_calibration(cycle) + "vana19612-" + str(panel) + "foc-SF-SS.nxs"
        if (SE == "WISHcryo"):
            if (cycle == "09_2"):
                return WISH_calibration() + "vana318-" + str(panel) + "foc-rmbins-smooth50.nx5"
            if (cycle == "09_3"):
                return WISH_calibration(cycle) + "vana935-" + str(panel) + "foc-SS.nx5"
            if (cycle == "09_4"):
                return WISH_calibration(cycle) + "vana3123-" + str(panel) + "foc-SS.nx5"
            if (cycle == "11_1"):
                return WISH_calibration(cycle) + "vana17718-" + str(panel) + "foc-SS.nxs"
            if (cycle == "11_2"):
                return WISH_calibration(cycle) + "vana16812-" + str(panel) + "foc-SS.nx5"
            if (cycle == "11_3"):
                return WISH_calibration(cycle) + "vana18590-" + str(panel) + "foc-SS-new.nxs"

    def split_string(t):
        indxp = 0
        for i in range(0, len(t)):
            if (t[i] == "+"):
                indxp = i
        if (indxp != 0):
            return int(t[0:indxp]), int(t[indxp + 1:len(t)])

    def WISH_getemptyinstrument(panel, cycle="09_4"):
        if (cycle == "09_4"):
            return WISH_calibration(cycle) + "emptyinst3120-" + str(panel) + "foc.nx5"

    def WISH_getempty(panel, SE="WISHcryo", cycle="09_4"):
        if (SE == "WISHcryo"):
            if (cycle == "09_2"):
                return WISH_calibration(cycle) + "emptycryo322-" + str(panel) + "-smooth50.nx5"
            if (cycle == "09_3"):
                return WISH_calibration(cycle) + "emptycryo1725-" + str(panel) + "foc.nx5"
            if (cycle == "09_4"):
                return WISH_calibration(cycle) + "emptycryo3307-" + str(panel) + "foc.nx5"
            if (cycle == "09_5"):
                return WISH_calibration(cycle) + "emptycryo16759-" + str(panel) + "foc.nx5"
            if (cycle == "11_1"):
                return WISH_calibration(cycle) + "emptycryo17712-" + str(panel) + "foc-SS.nxs"
            if (cycle == "11_2"):
                return WISH_calibration(cycle) + "emptycryo16759-" + str(panel) + "foc-SS.nx5"
            if (cycle == "11_3"):
                return WISH_calibration(cycle) + "emptycryo17712-" + str(panel) + "foc-SS-new.nxs"
            if (cycle == "11_4"):
                return WISH_calibration(cycle) + "empty_mag20620-" + str(panel) + "foc-HR-SF.nxs"
        if (SE == "candlestick"):
            if (cycle == "09_4"):
                return WISH_calibration(cycle) + "emptyinst3120-" + str(panel) + "foc.nxs"
            if (cycle == "09_3"):
                return WISH_calibration(cycle) + "emptyinst1726-" + str(panel) + "foc-monitor.nxs"
            if (cycle == "11_4"):
                return WISH_calibration(cycle) + "emptyinst19618-" + str(panel) + "foc-SF-S.nxs"

    def WISH_getfilename(run_number, ext):
        if (ext[0] != 's'):
            data_dir = WISH_datadir()
        else:
            # data_dir="/datad/ndxwish/"
            data_dir = WISH_datadir()
        digit = len(str(run_number))
        filename = data_dir + "WISH"
        for i in range(0, 8 - digit):
            filename = filename + "0"
        filename += str(run_number) + "." + ext
        return filename

    def WISH_returnpanel(panel):
        if (panel == 1):
            min = 6

    def WISH_getfilename(run_number, ext):
        if (ext[0] != 's'):
            data_dir = WISH_datadir()
        else:
            # data_dir="/datad/ndxwish/"
            data_dir = WISH_datadir()
        digit = len(str(run_number))
        filename = data_dir + "WISH"
        for i in range(0, 8 - digit):
            filename = filename + "0"
        filename += str(run_number) + "." + ext
        return filename

    def WISH_returnpanel(panel):
        if (panel == 1):
            min = 6
            max = 19461
        elif (panel == 2):
            min = 19462
            max = 38917
        elif (panel == 3):
            min = 38918
            max = 58373
        elif (panel == 4):
            min = 58374
            max = 77829
        elif (panel == 5):
            min = 77830
            max = 97285
        elif (panel == 6):
            min = 97286
            max = 116741
        elif (panel == 7):
            min = 116742
            max = 136197
        elif (panel == 8):
            min = 136198
            max = 155653
        elif (panel == 9):
            min = 155654
            max = 175109
        elif (panel == 10):
            min = 175110
            max = 194565
        elif (panel == 0):
            min = 6
            max = 194565
        return min, max

    # Reads a wish data file return a workspace with a short name
    def WISH_read(number, panel, ext):
        if type(number) is int:
            filename = WISH_getfilename(number, ext)
            if (ext[0:10] == "nxs_event"):
                filename = WISH_getfilename(number, "nxs")
            print "will be reading filename..." + filename
            min, max = WISH_returnpanel(panel)
            if (panel != 0):
                output = "w" + str(number) + "-" + str(panel)
            else:
                output = "w" + str(number)
            if (ext == "raw"):
                mantid.LoadRaw(Filename=filename, OutputWorkspace=output, SpectrumMin=str(min), SpectrumMax=str(max),
                               LoadLogFiles="0")
                mantid.MaskBins(InputWorkspace=output, OutputWorkspace=output, XMin=99900, XMax=106000)
                print "standard raw file loaded"
            if (ext[0] == "s"):
                mantid.LoadRaw(Filename=filename, OutputWorkspace=output, SpectrumMin=str(min), SpectrumMax=str(max),
                               LoadLogFiles="0")
                mantid.MaskBins(InputWorkspace=output, OutputWorkspace=output, XMin=99900, XMax=106000)
                print "sav file loaded"
            if (ext == "nxs_event"):
                mantid.LoadEventNexus(Filename=filename, OutputWorkspace=output, LoadMonitors='1')
                mantid.RenameWorkspace(output + "_monitors", "w" + str(number) + "_monitors")
                mantid.Rebin(InputWorkspace=output, OutputWorkspace=output, Params='6000,-0.00063,110000')
                mantid.ConvertToMatrixWorkspace(output, output)
                min, max = WISH_returnpanel(panel)
                mantid.CropWorkspace(InputWorkspace=output, OutputWorkspace=output, StartWorkspaceIndex=min - 6,
                                     EndWorkspaceIndex=max - 6)
                mantid.MaskBins(InputWorkspace=output, OutputWorkspace=output, XMin=99900, XMax=106000)
                print "full nexus eventfile loaded"
            if (ext[0:10] == "nxs_event_"):
                label, tmin, tmax = split_string_event(ext)
                output = output + "_" + label
                if (tmax == "end"):
                    mantid.LoadEventNexus(Filename=filename, OutputWorkspace=output, FilterByTimeStart=tmin,
                                          LoadMonitors='1',
                                          MonitorsAsEvents='1', FilterMonByTimeStart=tmin)
                else:
                    mantid.LoadEventNexus(Filename=filename, OutputWorkspace=output, FilterByTimeStart=tmin,
                                          FilterByTimeStop=tmax,
                                          LoadMonitors='1', MonitorsAsEvents='1', FilterMonByTimeStart=tmin,
                                          FilterMonByTimeStop=tmax)
                    mantid.RenameWorkspace(output + "_monitors", "w" + str(number) + "_monitors")
                print "renaming monitors done!"
                mantid.Rebin(InputWorkspace=output, OutputWorkspace=output, Params='6000,-0.00063,110000')
                mantid.ConvertToMatrixWorkspace(output, output)
                min, max = WISH_returnpanel(panel)
                mantid.CropWorkspace(InputWorkspace=output, OutputWorkspace=output, StartWorkspaceIndex=min - 6,
                                     EndWorkspaceIndex=max - 6)
                mantid.MaskBins(output, output, XMin=99900, XMax=106000)
                print "nexus event file chopped"
            if (ext == "nxs"):
                mantid.LoadNexus(Filename=filename, OutputWorkspace=output, SpectrumMin=str(min), SpectrumMax=str(max))
                mantid.MaskBins(InputWorkspace=output, OutputWorkspace=output, XMin=99900, XMax=106000)
                print "standard histo nxs file loaded"
        else:
            n1, n2 = split_string(number)
            output = "w" + str(n1) + "_" + str(n2) + "-" + str(panel)
            filename = WISH_getfilename(n1, ext)
            print "reading filename..." + filename
            min, max = WISH_returnpanel(panel)
            output1 = "w" + str(n1) + "-" + str(panel)
            mantid.LoadRaw(Filename=filename, OutputWorkspace=output1, SpectrumMin=str(min), SpectrumMax=str(max),
                           LoadLogFiles="0")
            filename = WISH_getfilename(n2, ext)
            print "reading filename..." + filename
            min, max = WISH_returnpanel(panel)
            output2 = "w" + str(n2) + "-" + str(panel)
            mantid.LoadRaw(Filename=filename, OutputWorkspace=output2, SpectrumMin=str(min), SpectrumMax=str(max),
                           LoadLogFiles="0")
            mantid.MergeRuns(output1 + "," + output2, output)
            mantid.DeleteWorkspace(output1)
            mantid.DeleteWorkspace(output2)
        mantid.ConvertUnits(InputWorkspace=output, OutputWorkspace=output, Target="Wavelength", Emode="Elastic")
        lmin, lmax = WISH_getlambdarange()
        mantid.CropWorkspace(InputWorkspace=output, OutputWorkspace=output, XMin=lmin, XMax=lmax)
        monitor = WISH_process_incidentmon(number, ext, spline_terms=70, debug=False)
        print "first norm to be done"
        mantid.NormaliseToMonitor(InputWorkspace=output, OutputWorkspace=output + "norm1", MonitorWorkspace=monitor)
        print "second norm to be done"
        mantid.NormaliseToMonitor(InputWorkspace=output + "norm1", OutputWorkspace=output + "norm2",
                                  MonitorWorkspace=monitor,
                                  IntegrationRangeMin=0.7, IntegrationRangeMax=10.35)
        mantid.DeleteWorkspace(output)
        mantid.DeleteWorkspace(output + "norm1")
        mantid.RenameWorkspace(InputWorkspace=output + "norm2", OutputWorkspace=output)
        mantid.ConvertUnits(InputWorkspace=output, OutputWorkspace=output, Target="TOF", EMode="Elastic")
        mantid.ReplaceSpecialValues(InputWorkspace=output, OutputWorkspace=output, NaNValue=0.0, NaNError=0.0,
                                    InfinityValue=0.0,
                                    InfinityError=0.0)
        return output

    # Focus dataset for a given panel and return the workspace
    def WISH_focus_onepanel(work, focus, panel):
        mantid.AlignDetectors(InputWorkspace=work, OutputWorkspace=work, CalibrationFile=WISH_cal(panel))
        mantid.DiffractionFocussing(InputWorkspace=work, OutputWorkspace=focus, GroupingFileName=WISH_group())
        if (panel == 5 or panel == 6):
            mantid.CropWorkspace(InputWorkspace=focus, OutputWorkspace=focus, XMin=0.3)
            mantid.DeleteWorkspace(work)
        return focus

    def WISH_split(focus):
        for i in range(0, 11):
            out = focus[0:len(focus) - 3] + "-" + str(i + 1) + "foc"
            mantid.ExtractSingleSpectrum(InputWorkspace=focus, OutputWorkspace=out, WorkspaceIndex=i)
            mantid.DeleteWorkspace(focus)
        return

    def WISH_focus(work, panel):
        focus = work + "foc"
        if (panel != 0):
            WISH_focus_onepanel(work, focus, panel)
        else:
            WISH_focus_onepanel(work, focus, panel)
            WISH_split(focus)

    def WISH_process(number, panel, ext, SEsample="WISHcryo", emptySEcycle="09_4", SEvana="candlestick",
                     cyclevana="09_4",
                     absorb=False, nd=0.0, Xs=0.0, Xa=0.0, h=0.0, r=0.0):
        w = WISH_read(number, panel, ext)
        print "file read and normalized"
        if (absorb):
            mantid.ConvertUnits(InputWorkspace=w, OutputWorkspace=w, Target="Wavelength", EMode="Elastic")
            mantid.CylinderAbsorption(InputWorkspace=w, OutputWorkspace="T",
                                      CylinderSampleHeight=h, CylinderSampleRadius=r, AttenuationXSection=Xa,
                                      ScatteringXSection=Xs, SampleNumberDensity=nd,
                                      NumberOfSlices="10", NumberOfAnnuli="10", NumberOfWavelengthPoints="25",
                                      ExpMethod="Normal")
            mantid.Divide(LHSWorkspace=w, RHSWorkspace="T", OutputWorkspace=w)
            mantid.DeleteWorkspace("T")
            mantid.ConvertUnits(InputWorkspace=w, OutputWorkspace=w, Target="TOF", EMode="Elastic")
        wfoc = WISH_focus(w, panel)
        print "focussing done!"
        if type(number) is int:
            wfocname = "w" + str(number) + "-" + str(panel) + "foc"
            if (len(ext) > 9):
                label, tmin, tmax = split_string_event(ext)
                wfocname = "w" + str(number) + "-" + str(panel) + "_" + label + "foc"
        else:
            n1, n2 = split_string(number)
            wfocname = "w" + str(n1) + "_" + str(n2) + "-" + str(panel) + "foc"
        if (panel == 1):
            mantid.CropWorkspace(InputWorkspace=wfocname, OutputWorkspace=wfocname, XMin=0.80, XMax=53.3)
        elif (panel == 2):
            mantid.CropWorkspace(InputWorkspace=wfocname, OutputWorkspace=wfocname, XMin=0.50, XMax=13.1)
        elif (panel == 3):
            mantid.CropWorkspace(InputWorkspace=wfocname, OutputWorkspace=wfocname, XMin=0.50, XMax=7.77)
        elif (panel == 4):
            mantid.CropWorkspace(InputWorkspace=wfocname, OutputWorkspace=wfocname, XMin=0.40, XMax=5.86)
        elif (panel == 5):
            mantid.CropWorkspace(InputWorkspace=wfocname, OutputWorkspace=wfocname, XMin=0.35, XMax=4.99)
        elif (panel == 6):
            mantid.CropWorkspace(InputWorkspace=wfocname, OutputWorkspace=wfocname, XMin=0.35, XMax=4.99)
        elif (panel == 7):
            mantid.CropWorkspace(InputWorkspace=wfocname, OutputWorkspace=wfocname, XMin=0.40, XMax=5.86)
        elif (panel == 8):
            mantid.CropWorkspace(InputWorkspace=wfocname, OutputWorkspace=wfocname, XMin=0.50, XMax=7.77)
        elif (panel == 9):
            mantid.CropWorkspace(InputWorkspace=wfocname, OutputWorkspace=wfocname, XMin=0.50, XMax=13.1)
        elif (panel == 10):
            mantid.CropWorkspace(InputWorkspace=wfocname, OutputWorkspace=wfocname, XMin=0.80, XMax=53.3)

        if (panel == 0):
            for i in range(1, 11):
                wfocname = "w" + str(number) + "-" + str(i) + "foc"
                mantid.CropWorkspace(InputWorkspace=wfocname, OutputWorkspace=wfocname, XMin=0.80, XMax=53.3)
                mantid.CropWorkspace(InputWorkspace=wfocname, OutputWorkspace=wfocname, XMin=0.50, XMax=13.1)
                mantid.CropWorkspace(InputWorkspace=wfocname, OutputWorkspace=wfocname, XMin=0.50, XMax=7.77)
                mantid.CropWorkspace(InputWorkspace=wfocname, OutputWorkspace=wfocname, XMin=0.40, XMax=5.86)
                mantid.CropWorkspace(InputWorkspace=wfocname, OutputWorkspace=wfocname, XMin=0.35, XMax=4.99)
                mantid.CropWorkspace(InputWorkspace=wfocname, OutputWorkspace=wfocname, XMin=0.35, XMax=4.99)
                mantid.CropWorkspace(InputWorkspace=wfocname, OutputWorkspace=wfocname, XMin=0.40, XMax=5.86)
                mantid.CropWorkspace(InputWorkspace=wfocname, OutputWorkspace=wfocname, XMin=0.50, XMax=7.77)
                mantid.CropWorkspace(InputWorkspace=wfocname, OutputWorkspace=wfocname, XMin=0.50, XMax=13.1)
                mantid.CropWorkspace(InputWorkspace=wfocname, OutputWorkspace=wfocname, XMin=0.80, XMax=53.3)
        # print "will try to load an empty with the name:"
        print WISH_getempty(panel, SEsample, emptySEcycle)
        if (panel == 0):
            for i in range(1, 11):
                wfocname = "w" + str(number) + "-" + str(i) + "foc"
                mantid.LoadNexusProcessed(Filename=WISH_getempty(i, SEsample, emptySEcycle), OutputWorkspace="empty")
                mantid.RebinToWorkspace(WorkspaceToRebin="empty", WorkspaceToMatch=wfocname, OutputWorkspace="empty")
                # Minus(LHSWorkspace=wfocname,RHSWorkspace="empty",OutputWorkspace=wfocname)
                mantid.DeleteWorkspace("empty")
                print "will try to load a vanadium with the name:" + WISH_getvana(i, SEvana, cyclevana)
                mantid.LoadNexusProcessed(Filename=WISH_getvana(i, SEvana, cyclevana), OutputWorkspace="vana")
                mantid.RebinToWorkspace(WorkspaceToRebin="vana", WorkspaceToMatch=wfocname, OutputWorkspace="vana")
                mantid.Divide(LHSWorkspace=wfocname, RHSWorkspace="vana", OutputWorkspace=wfocname)
                mantid.DeleteWorkspace("vana")
                mantid.ConvertUnits(InputWorkspace=wfocname, OutputWorkspace=wfocname, Target="TOF", EMode="Elastic")
                mantid.ReplaceSpecialValues(InputWorkspace=wfocname, OutputWorkspace=wfocname, NaNValue=0.0,
                                            NaNError=0.0,
                                            InfinityValue=0.0, InfinityError=0.0)
                mantid.SaveGSS(InputWorkspace=wfocname,
                               Filename=WISH_userdir() + str(number) + "-" + str(i) + ext + ".gss",
                               Append=False, Bank=1)
                mantid.SaveFocusedXYE(wfocname, WISH_userdir() + str(number) + "-" + str(i) + ext + ".dat")
                mantid.SaveNexusProcessed(wfocname, WISH_userdir() + str(number) + "-" + str(i) + ext + ".nxs")
        else:
            mantid.LoadNexusProcessed(Filename=WISH_getempty(panel, SEsample, emptySEcycle), OutputWorkspace="empty")
            mantid.RebinToWorkspace(WorkspaceToRebin="empty", WorkspaceToMatch=wfocname, OutputWorkspace="empty")
            # Minus(LHSWorkspace=wfocname,RHSWorkspace="empty",OutputWorkspace=wfocname)
            mantid.DeleteWorkspace("empty")
            print "will try to load a vanadium with the name:" + WISH_getvana(panel, SEvana, cyclevana)
            mantid.LoadNexusProcessed(Filename=WISH_getvana(panel, SEvana, cyclevana), OutputWorkspace="vana")
            mantid.RebinToWorkspace(WorkspaceToRebin="vana", WorkspaceToMatch=wfocname, OutputWorkspace="vana")
            mantid.Divide(LHSWorkspace=wfocname, RHSWorkspace="vana", OutputWorkspace=wfocname)
            mantid.DeleteWorkspace("vana")
            mantid.ConvertUnits(InputWorkspace=wfocname, OutputWorkspace=wfocname, Target="TOF", EMode="Elastic")
            mantid.ReplaceSpecialValues(InputWorkspace=wfocname, OutputWorkspace=wfocname, NaNValue=0.0, NaNError=0.0,
                                        InfinityValue=0.0, InfinityError=0.0)
            mantid.SaveGSS(InputWorkspace=wfocname,
                           Filename=WISH_userdir() + str(number) + "-" + str(panel) + ext + ".gss",
                           Append=False, Bank=1)
            mantid.SaveFocusedXYE(wfocname, WISH_userdir() + str(number) + "-" + str(panel) + ext + ".dat")
            mantid.SaveNexusProcessed(wfocname, WISH_userdir() + str(number) + "-" + str(panel) + ext + ".nxs")
        return wfocname

    # Create a corrected vanadium (normalise,corrected for attenuation and empty, strip peaks) and
    # save a a nexus processed file.
    # It looks like smoothing of 100 works quite well
    def WISH_createvan(van, empty, panel, smoothing, vh, vr, cycle_van="09_3", cycle_empty="09_3"):
        WISH_setdatadir("/archive/ndxwish/Instrument/data/cycle_" + cycle_van + "/")
        wvan = WISH_read(van, panel, "nxs_event")
        WISH_setdatadir("/archive/ndxwish/Instrument/data/cycle_" + cycle_empty + "/")
        wempty = WISH_read(empty, panel, "nxs_event")
        mantid.Minus(LHSWorkspace=wvan, RHSWorkspace=wempty, OutputWorkspace=wvan)
        print "read van and empty"
        mantid.DeleteWorkspace(wempty)
        mantid.ConvertUnits(InputWorkspace=wvan, OutputWorkspace=wvan, Target="Wavelength", EMode="Elastic")
        mantid.CylinderAbsorption(InputWorkspace=wvan, OutputWorkspace="T",
                                  CylinderSampleHeight=str(vh), CylinderSampleRadius=str(vr),
                                  AttenuationXSection="4.8756",
                                  ScatteringXSection="5.16", SampleNumberDensity="0.07118",
                                  NumberOfSlices="10", NumberOfAnnuli="10", NumberOfWavelengthPoints="25",
                                  ExpMethod="Normal")
        mantid.Divide(LHSWorkspace=wvan, RHSWorkspace="T", OutputWorkspave=wvan)
        mantid.DeleteWorkspace("T")
        mantid.ConvertUnits(InputWorkspace=wvan, OutputWorkspace=wvan, Target="TOF", EMode="Elastic")
        vanfoc = WISH_focus(wvan, panel)
        mantid.DeleteWorkspace(wvan)
        # StripPeaks(vanfoc,vanfoc)
        # SmoothData(vanfoc,vanfoc,str(smoothing))
        return

    def WISH_createempty(empty, panel):
        wempty = WISH_read(empty, panel, "raw")
        emptyfoc = WISH_focus(wempty, panel)
        return emptyfoc

    def WISH_monitors(rb, ext):
        data_dir = WISH_dir()
        file = WISH_getfilename(rb, ext)
        wout = "w" + str(rb)
        print "reading File..." + file
        mantid.LoadRaw(Filename=file, OutputWorkspace=wout, SpectrumMin=str(1), SpectrumMax=str(5), LoadLogFiles="0")
        mantid.NormaliseByCurrent(InputWorkspace=wout, OutputWorkspace=wout)
        mantid.ConvertToDistribution(wout)
        return wout

    def WISH_PH_TOF(runnumber, tubemin):
        min = 6 + (tubenumber - 1) * 128
        max = min + 128
        file = WISH_getfilename(runnumber, tubemin)
        output = "Run" + str(runnumber) + "tube" + str(tubenumber)
        w = WISH_read(number, panel, ext)
        print "file read and normalized"
        if (absorb):
            mantid.ConvertUnits(w, w, "Wavelength")
            mantid.CylinderAbsorption(InputWorkspace=w, OutputWorkspace="T",
                                      CylinderSampleHeight=h, CylinderSampleRadius=r, AttenuationXSection=Xa,
                                      ScatteringXSection=Xs, SampleNumberDensity=nd,
                                      NumberOfSlices="10", NumberOfAnnuli="10", NumberOfWavelengthPoints="25",
                                      ExpMethod="Normal")
            mantid.Divide(w, "T", w)
            mantid.DeleteWorkspace("T")
            mantid.ConvertUnits(InputWorkspace=w, OutputWorkspace=w, Target="TOF", EMode="Elastic")
        wfoc = WISH_focus(w, panel)
        print "focussing done!"
        if type(number) is int:
            wfocname = "w" + str(number) + "-" + str(panel) + "foc"
            if (len(ext) > 9):
                label, tmin, tmax = split_string_event(ext)
                wfocname = "w" + str(number) + "-" + str(panel) + "_" + label + "foc"
        else:
            n1, n2 = split_string(number)
            wfocname = "w" + str(n1) + "_" + str(n2) + "-" + str(panel) + "foc"
        if (panel == 1):
            mantid.CropWorkspace(InputWorkspace=wfocname, OutputWorkspace=wfocname, XMin=0.80, XMax=53.3)
        elif (panel == 2):
            mantid.CropWorkspace(InputWorkspace=wfocname, OutputWorkspace=wfocname, XMin=0.50, XMax=13.1)
        elif (panel == 3):
            mantid.CropWorkspace(InputWorkspace=wfocname, OutputWorkspace=wfocname, XMin=0.50, XMax=7.77)
        elif (panel == 4):
            mantid.CropWorkspace(InputWorkspace=wfocname, OutputWorkspace=wfocname, XMin=0.40, XMax=5.86)
        elif (panel == 5):
            mantid.CropWorkspace(InputWorkspace=wfocname, OutputWorkspace=wfocname, XMin=0.35, XMax=4.99)
        if (panel == 10):
            mantid.CropWorkspace(InputWorkspace=wfocname, OutputWorkspace=wfocname, XMin=0.80, XMax=53.3)
        elif (panel == 9):
            mantid.CropWorkspace(InputWorkspace=wfocname, OutputWorkspace=wfocname, XMin=0.50, XMax=13.1)
        elif (panel == 8):
            mantid.CropWorkspace(InputWorkspace=wfocname, OutputWorkspace=wfocname, XMin=0.50, XMax=7.77)
        elif (panel == 7):
            mantid.CropWorkspace(InputWorkspace=wfocname, OutputWorkspace=wfocname, XMin=0.40, XMax=5.86)
        elif (panel == 6):
            mantid.CropWorkspace(InputWorkspace=wfocname, OutputWorkspace=wfocname, XMin=0.35, XMax=4.99)
        # print "will try to load an empty with the name:"
        print WISH_getempty(panel, SEsample, emptySEcycle)
        if (panel == 0):
            for i in range(1, 11):
                mantid.LoadNexusProcessed(Filename=WISH_getempty(i, SEsample, emptySEcycle), OutputWorkspace="empty")
                mantid.RebinToWorkspace(WorkspaceToRebin="empty", WorkspaceToMatch=wfocname, OutputWorkspace="empty")
                mantid.Minus(LHSWorkspace=wfocname, RHSWorkspace="empty", OutputWorkspace=wfocname)
                mantid.DeleteWorkspace("empty")
                print "will try to load a vanadium with the name:" + WISH_getvana(i, SEvana, cyclevana)
                mantid.LoadNexusProcessed(Filename=WISH_getvana(i, SEvana, cyclevana), OutputWorkspace="vana")
                mantid.RebinToWorkspace(WorkspaceToRebin="vana", WorkspaceToMatch=wfocname, OutputWorkspace="vana")
                mantid.Divide(LHSWorkspace=wfocname, RHSWorkspace="vana", OutputWorkspace=wfocname)
                mantid.DeleteWorkspace("vana")
                mantid.ConvertUnits(InputWorkspace=wfocname, OutputWorkspace=wfocname, Target="TOF", EMode="Elastic")
        #			SaveGSS(InputWorkspace=wfocname,Filename=WISH_userdir()+str(number)+"-"+str(i)+ext+".gss",Append=False,Bank=1)
        #			SaveFocusedXYE(wfocname,WISH_userdir()+str(number)+"-"+str(i)+ext+".dat")
        else:
            mantid.LoadNexusProcessed(Filename=WISH_getempty(panel, SEsample, emptySEcycle), OutputWorkspace="empty")
            mantid.RebinToWorkspace(WorkspaceToRebin="empty", WorkspaceToMatch=wfocname, OutputWorkspace="empty")
            # Minus(LHSWorkspace=wfocname,RHSWorkspace="empty",OutputWorkspace=wfocname)
            mantid.DeleteWorkspace("empty")
            print "will try to load a vanadium with the name:" + WISH_getvana(panel, SEvana, cyclevana)
            mantid.LoadNexusProcessed(Filename=WISH_getvana(panel, SEvana, cyclevana), OutputWorkspace="vana")
            mantid.RebinToWorkspace(WorkspaceToRebin="vana", WorkspaceToMatch=wfocname, OutputWorkspace="vana")
            mantid.Divide(LHSWorkspace=wfocname, RHSWorkspace="vana", OutputWorkspace=wfocname)
            mantid.DeleteWorkspace("vana")
            mantid.ConvertUnits(InputWorkspace=wfocname, OutputWorkspace=wfocname, Target="TOF", EMode="Elastic")
        #		SaveGSS(InputWorkspace=wfocname,Filename=WISH_userdir()+str(number)+"-"+str(panel)+ext+".gss",Append=False,Bank=1)
        #		SaveFocusedXYE(wfocname,WISH_userdir()+str(number)+"-"+str(panel)+ext+".dat")
        return wfocname
        LoadRaw(Filename=file, OutputWorkspace=output, spectrummin=str(min), spectrummax=str(max), LoadLogFiles="0")
        Integration(InputWorkspace=output, OutputWorkspace=output + "int")
        g = plotTimeBin(output + "int", 0)

    # Smoothing the incident beam monitor using a spline function.  Regions around Bragg edges are masked, before fitting with a  spline function.
    # Returns a smooth monitor spectrum

    def WISH_process_incidentmon(number, ext, spline_terms=20, debug=False):
        if type(number) is int:
            fname = WISH_getfilename(number, ext)
            works = "monitor" + str(number)
            if (ext == "raw"):
                works = "monitor" + str(number)
                mantid.LoadRaw(Filename=fname, OutputWorkspace=works, SpectrumMin=4, SpectrumMax=4, LoadLogFiles="0")
            if (ext[0] == "s"):
                works = "monitor" + str(number)
                mantid.LoadRaw(Filename=fname, OutputWorkspace=works, SpectrumMin=4, SpectrumMax=4, LoadLogFiles="0")
            if (ext == "nxs"):
                works = "monitor" + str(number)
                mantid.LoadNexus(Filename=fname, OutputWorkspace=works, SpectrumMin=4, SpectrumMax=4)
            if (ext[0:9] == "nxs_event"):
                temp = "w" + str(number) + "_monitors"
                works = "w" + str(number) + "_monitor4"
                mantid.Rebin(InputWorkspace=temp, OutputWorkspace=temp, Params='6000,-0.00063,110000',
                             PreserveEvents=False)
                mantid.ExtractSingleSpectrum(InputWorkspace=temp, OutputWorkspace=works, WorkspaceIndex=3)
        else:
            n1, n2 = split_string(number)
            works = "monitor" + str(n1) + "_" + str(n2)
            fname = WISH_getfilename(n1, ext)
            works1 = "monitor" + str(n1)
            mantid.LoadRaw(Filename=fname, OutputWorkspace=works1, SpectrumMin=4, SpectrumMax=4, LoadLogFiles="0")
            fname = WISH_getfilename(n2, ext)
            works2 = "monitor" + str(n2)
            mantid.LoadRaw(Filename=fname, OutputWorkspace=works2, SpectrumMin=4, SpectrumMax=4, LoadLogFiles="0")
            mantid.MergeRuns(InputWorkspaces=works1 + "," + works2, OutputWorkspace=works)
            mantid.DeleteWorkspace(works1)
            mantid.DeleteWorkspace(works2)
        mantid.ConvertUnits(InputWorkspace=works, OutputWorkspace=works, Target="Wavelength", Emode="Elastic")
        lmin, lmax = WISH_getlambdarange()
        mantid.CropWorkspace(InputWorkspace=works, OutputWorkspace=works, XMin=lmin, XMax=lmax)
        ex_regions = n.zeros((2, 4))
        ex_regions[:, 0] = [4.57, 4.76]
        ex_regions[:, 1] = [3.87, 4.12]
        ex_regions[:, 2] = [2.75, 2.91]
        ex_regions[:, 3] = [2.24, 2.50]
        mantid.ConvertToDistribution(works)
        if (debug):
            x, y, z = mtdplt.getnarray(works, 0)
            p.plot(x, y)
        for reg in range(0, 4):
            mantid.MaskBins(InputWorkspace=works, OutputWorkspace=works, XMin=ex_regions[0, reg],
                            XMax=ex_regions[1, reg])
        if (debug):
            x, y, z = mtdplt.getnarray(works, LoadRaw0)
            p.plot(x, y)
            
        mantid.SplineBackground(InputWorkspace=works, OutputWorkspace=works, WorkspaceIndex=0, NCoeff=spline_terms)
        if (debug):
            x, y, z = mtdplt.getnarray(works, 0)
            p.plot(x, y)
        p.show()
        mantid.SmoothData(InputWorkspace=works, OutputWorkspace=works, NPoints=40)
        mantid.ConvertFromDistribution(works)
        return works

    # removes the peaks in a vanadium  run, then performs a spline and a smooth
    def Removepeaks_spline_smooth_empty(works, panel, debug=False):
        if (panel == 1):
            splineterms = 0
            smoothterms = 30
        if (panel == 2):
            splineterms = 0
            smoothterms = 10
        if (panel == 3):
            splineterms = 0
            smoothterms = 15
        if (panel == 4):
            splineterms = 0
            smoothterms = 15
        if (panel == 5):
            splineterms = 0
            smoothterms = 10
        if (debug):
            x, y, z = getnarray(works, 0)
            p.plot(x, y)
        if (splineterms != 0):
            mantid.SplineBackground(InputWorkspace=works, OutputWorkspace=works, WorkspaceIndex=0, NCoeff=splineterms)
        if (debug):
            x, y, z = getnarray(works, 0)
            p.plot(x, y)
            mantid.SmoothData(InputWorkspace=works, OutputWorkspace=works, NPoints=smoothterms)
        if (debug):
            x, y, z = getnarray(works, 0)
            p.plot(x, y)
        p.show()
        return works

    def split_string_event(t):
        # this assumes the form nxs_event_label_tmin_tmax
        indx_ = []
        for i in range(10, len(t)):
            if (t[i] == "_"):
                indx_.append(i)
        label = t[10:indx_[0]]
        tmin = t[indx_[0] + 1:indx_[1]]
        tmax = t[indx_[1] + 1:len(t)]
        return label, tmin, tmax

    def minus_emptycans(runno, empty):
        panel_list = ['-1foc', '-2foc', '-3foc', '-4foc', '-5foc', '-6foc', '-7foc', '-8foc', '-9foc', '-10foc',
                      '-1_10foc',
                      '-2_9foc', '-3_8foc', '-4_7foc', '-5_6foc']
        for p in panel_list:
            mantid.Minus(LHSWorkspace='w' + str(runno) + p, RHSWorkspace='w' + str(empty) + p,
                         OutputWorkspace='w' + str(runno) + 'minus' + str(empty) + p)
            mantid.ConvertUnits(InputWorkspace='w' + str(runno) + 'minus' + str(empty) + p,
                                OutputWorkspace='w' + str(runno) + 'minus' + str(empty) + p + '-d', Target='dSpacing')
            mantid.SaveGSS("w" + str(runno) + 'minus' + str(empty) + p, WISH_userdir() + str(i) + p + ".gss",
                           Append=False, Bank=1)

    WISH_startup("mp43", "18_1")

    for i in range(40503, 40504):
        for j in range(1, 11):
            wout = WISH_process(i, j, "raw", "candlestick", "11_4", "candlestick", "11_4", absorb=True, nd=0.025,
                                Xs=5.463,
                                Xa=2.595, h=4.0, r=0.55)
            mantid.ConvertUnits(InputWorkspace=wout, OutputWorkspace=wout + "-d", Target="dSpacing", EMode="Elastic")
        #	SaveGSS("w"+str(i)+"-1foc",WISH_userdir()+str(i)+"-1foc"+".gss",Append=False,Bank=1)
        #	SaveFocusedXYE("w"+str(i)+"-1foc",WISH_userdir()+str(i)+"-1foc"+".dat")
        #	SaveGSS("w"+str(i)+"-2foc",WISH_userdir()+str(i)+"-2foc"+".gss",Append=False,Bank=1)
        #	SaveFocusedXYE("w"+str(i)+"-2foc",WISH_userdir()+str(i)+"-2foc"+".dat")
        mantid.RebinToWorkspace(WorkspaceToRebin="w" + str(i) + "-6foc", WorkspaceToMatch="w" + str(i) + "-5foc",
                                OutputWorkspace="w" + str(i) + "-6foc", PreserveEvents='0')
        mantid.Plus(LHSWorkspace="w" + str(i) + "-5foc", RHSWorkspace="w" + str(i) + "-6foc",
                    OutputWorkspace="w" + str(i) + "-5_6foc")
        mantid.ConvertUnits(InputWorkspace="w" + str(i) + "-5_6foc", OutputWorkspace="w" + str(i) + "-5_6foc" + "-d",
                            Target="dSpacing", EMode="Elastic")
        mantid.SaveGSS("w" + str(i) + "-5_6foc", WISH_userdir() + str(i) + "-5_6raw" + ".gss", Append=False, Bank=1)
        mantid.SaveFocusedXYE("w" + str(i) + "-5_6foc", WISH_userdir() + str(i) + "-5_6raw" + ".dat")
        mantid.SaveNexusProcessed("w" + str(i) + "-5_6foc", WISH_userdir() + str(i) + "-5_6raw" + ".nxs")
        mantid.RebinToWorkspace(WorkspaceToRebin="w" + str(i) + "-7foc", WorkspaceToMatch="w" + str(i) + "-4foc",
                                OutputWorkspace="w" + str(i) + "-7foc", PreserveEvents='0')
        mantid.Plus(LHSWorkspace="w" + str(i) + "-4foc", RHSWorkspace="w" + str(i) + "-7foc",
                    OutputWorkspace="w" + str(i) + "-4_7foc")
        mantid.ConvertUnits(InputWorkspace="w" + str(i) + "-4_7foc", OutputWorkspace="w" + str(i) + "-4_7foc" + "-d",
                            Target="dSpacing", EMode="Elastic")
        mantid.SaveGSS("w" + str(i) + "-4_7foc", WISH_userdir() + str(i) + "-4_7raw" + ".gss", Append=False, Bank=1)
        mantid.SaveFocusedXYE("w" + str(i) + "-4_7foc", WISH_userdir() + str(i) + "-4_7raw" + ".dat")
        mantid.SaveNexusProcessed("w" + str(i) + "-4_7foc", WISH_userdir() + str(i) + "-4_7raw" + ".nxs")
        mantid.RebinToWorkspace(WorkspaceToRebin="w" + str(i) + "-8foc", WorkspaceToMatch="w" + str(i) + "-3foc",
                                OutputWorkspace="w" + str(i) + "-8foc", PreserveEvents='0')
        mantid.Plus(LHSWorkspace="w" + str(i) + "-3foc", RHSWorkspace="w" + str(i) + "-8foc",
                    OutputWorkspace="w" + str(i) + "-3_8foc")
        mantid.ConvertUnits(InputWorkspace="w" + str(i) + "-3_8foc", OutputWorkspace="w" + str(i) + "-3_8foc" + "-d",
                            Target="dSpacing", EMode="Elastic")
        mantid.SaveGSS("w" + str(i) + "-3_8foc", WISH_userdir() + str(i) + "-3_8raw" + ".gss", Append=False, Bank=1)
        mantid.SaveFocusedXYE("w" + str(i) + "-3_8foc", WISH_userdir() + str(i) + "-3_8raw" + ".dat")
        mantid.SaveNexusProcessed("w" + str(i) + "-3_8foc", WISH_userdir() + str(i) + "-3_8raw" + ".nxs")
        mantid.RebinToWorkspace(WorkspaceToRebin="w" + str(i) + "-9foc", WorkspaceToMatch="w" + str(i) + "-2foc",
                                OutputWorkspace="w" + str(i) + "-9foc", PreserveEvents='0')
        mantid.Plus(LHSWorkspace="w" + str(i) + "-2foc", RHSWorkspace="w" + str(i) + "-9foc",
                    OutputWorkspace="w" + str(i) + "-2_9foc")
        mantid.ConvertUnits(InputWorkspace="w" + str(i) + "-2_9foc", OutputWorkspace="w" + str(i) + "-2_9foc" + "-d",
                            Target="dSpacing", EMode="Elastic")
        mantid.SaveGSS("w" + str(i) + "-2_9foc", WISH_userdir() + str(i) + "-2_9raw" + ".gss", Append=False, Bank=1)
        mantid.SaveFocusedXYE("w" + str(i) + "-2_9foc", WISH_userdir() + str(i) + "-2_9raw" + ".dat")
        mantid.SaveNexusProcessed("w" + str(i) + "-2_9foc", WISH_userdir() + str(i) + "-2_9raw" + ".nxs")
        mantid.RebinToWorkspace(WorkspaceToRebin="w" + str(i) + "-10foc", WorkspaceToMatch="w" + str(i) + "-1foc",
                                OutputWorkspace="w" + str(i) + "-10foc", PreserveEvents='0')
        mantid.Plus(LHSWorkspace="w" + str(i) + "-1foc", RHSWorkspace="w" + str(i) + "-10foc",
                    OutputWorkspace="w" + str(i) + "-1_10foc")
        mantid.ConvertUnits(InputWorkspace="w" + str(i) + "-1_10foc", OutputWorkspace="w" + str(i) + "-1_10foc" + "-d",
                            Target="dSpacing", EMode="Elastic")
        mantid.SaveGSS("w" + str(i) + "-1_10foc", WISH_userdir() + str(i) + "-1_10raw" + ".gss", Append=False, Bank=1)
        mantid.SaveFocusedXYE("w" + str(i) + "-1_10foc", WISH_userdir() + str(i) + "-1_10raw" + ".dat")
        mantid.SaveNexusProcessed("w" + str(i) + "-1_10foc", WISH_userdir() + str(i) + "-1_10raw" + ".nxs")
