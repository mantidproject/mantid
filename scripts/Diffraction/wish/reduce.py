import os, sys

sys.path.insert(0, "/opt/Mantid/bin")
sys.path.append("/isis/NDXWISH/user/scripts/autoreduction")

# Set of routines to normalise WISH data- new look Mantid with mantidsimple removed
from mantid.simpleapi import *
# import matplotlib.pyplot as p # Had to remove matplotlib as autoreduce running on server without display
import numpy as n
import mantid.simpleapi as mantid
def Wish_Run(input):
    __name__=input
    print("Running")
    wish_dir = ""


    def validate(input_file, output_dir):
        """
        Autoreduction validate Function
        -------------------------------

        Function to ensure that the files we want to use in reduction exist.
        Please add any files/directories to the required_files/dirs lists.
        """
        print("Running validation")
        required_files = [input_file]
        required_dirs = [output_dir]
        for file_path in required_files:
            if not os.path.isfile(file_path):
                raise RuntimeError("Unable to find file: {}".format(file_path))
        for dir in required_dirs:
            if not os.path.isdir(dir):
                raise RuntimeError("Unable to find directory: {}".format(dir))
        print("Validation successful")


    # Get the run number from the input data path
    def get_run_number(data_path):
        data_path = data_path.split('/')[-1]  # Get the file name
        data_path = data_path.split('.')[0]  # Remove the extension
        data_path = data_path[4:]  # Remove the WISH prefix
        return int(data_path)


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


    def WISH_setdatafile(filename):
        global wish_datafile
        wish_datafile = filename


    def WISH_getdatafile():
        return wish_datafile


    def WISH_datadir():
        return wish_datadir


    def WISH_userdir(cycle='cycle_10_1'):
        return wish_userdir


    def WISH_calibration(cycle="11_4"):
        return "/home/sjenkins/Documents/WISH/Cal/"


    #   This is no longer needed unless run manually
    def WISH_startup(usern, cycle='14_3'):
        global userdatadir
        global userdataprocessed
        global mtdplt
        import sys
        sys.path.append('/home/' + usern + '/Scripts/')
        # import Mantid_plotting as mtdplt
        # userdatadir="/home/rawdata/"
        userdatadir = "/archive/ndxwish/Instrument/data/cycle_" + cycle + '/'
        WISH_setdatadir(userdatadir)
        print "Raw Data in :   ", userdatadir
        userdataprocessed =  r"/home/sjenkins/Documents/WISH"
        WISH_setuserdir(directory=userdataprocessed)
        print "Processed Data in :   ", userdataprocessed
        return


    # Returns the calibration filename
    def WISH_cal(panel):
        return WISH_calibration() + "WISH_cycle_10_3_noends_10to10.cal"
        # return "/home/mp43/Desktop/Si_Mar15/test_detOffsets_SiMar15_noends.cal"
        # return "/home/ryb18365/Desktop/WISH_cycle_10_3_noends_10to10_dodgytubesremoved.cal"


    # Returns the grouping filename
    def WISH_group():
        return WISH_calibration() + "WISH_cycle_10_3_noends_10to10.cal"
        # return "/home/mp43/Desktop/Si_Mar15/test_detOffsets_SiMar15_noends.cal"
        # return "/home/ryb18365/Desktop/WISH_cycle_10_3_noends_10to10_dodgytubesremoved.cal"


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
                return WISH_calibration(cycle) + "vana38428-" + str(panel) + "foc-SF-SS.nxs"
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


    # This function no longer works for a list of numbers
    # Reads a wish data file return a workspace with a short name
    def WISH_read(number, panel, ext):
        if type(number) is int:
            filename = WISH_getdatafile()  # Changed as full path is set in main now
            ext = filename.split('.')[-1]  # Get the extension from the inputted filename
            print "Extension is: " + ext
            # if (ext[0:10]=="nxs_event"):
            #    filename=WISH_getfilename(number,"nxs")
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
                    mantid.LoadEventNexus(Filename=filename, OutputWorkspace=output, FilterByTimeStart=tmin, LoadMonitors='1',
                                   MonitorsAsEvents='1', FilterMonByTimeStart=tmin)
                else:
                    mantid.LoadEventNexus(Filename=filename, OutputWorkspace=output, FilterByTimeStart=tmin, FilterByTimeStop=tmax,
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
                mantid.ConvertUnits(InputWorkspace=output, OutputWorkspace=output, Target="Wavelength", Emode="Elastic")
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
        mantid.NormaliseToMonitor(InputWorkspace=output + "norm1", OutputWorkspace=output + "norm2", MonitorWorkspace=monitor,
                           IntegrationRangeMin=0.7, IntegrationRangeMax=10.35)
        mantid.DeleteWorkspace(output)
        mantid.DeleteWorkspace(output + "norm1")
        mantid.RenameWorkspace(InputWorkspace=output + "norm2", OutputWorkspace=output)
        mantid.ConvertUnits(InputWorkspace=output, OutputWorkspace=output, Target="TOF", EMode="Elastic")
        mantid.ReplaceSpecialValues(InputWorkspace=output, OutputWorkspace=output, NaNValue=0.0, NaNError=0.0, InfinityValue=0.0,
                             InfinityError=0.0)
        return output


    # Focus dataset for a given panel and return the workspace
    def WISH_focus_onepanel(work, focus, panel):
        mantid.AlignDetectors(InputWorkspace=work, OutputWorkspace=work, CalibrationFile=WISH_cal(panel))
        mantid.DiffractionFocussing(InputWorkspace=work, OutputWorkspace=focus, GroupingFileName=WISH_group())
        if (panel == 5 or panel == 6):
            mantid.CropWorkspace(InputWorkspace=focus, OutputWorkspace=focus, XMin=0.3)
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


    def WISH_process(number, panel, ext, SEsample="WISHcryo", emptySEcycle="09_4", SEvana="candlestick", cyclevana="09_4",
                     absorb=False, nd=0.0, Xs=0.0, Xa=0.0, h=0.0, r=0.0):
        w = WISH_read(number, panel, ext)
        print "file read and normalized"
        if (absorb):
            mantid.ConvertUnits(InputWorkspace=w, OutputWorkspace=w, Target="Wavelength", EMode="Elastic")
            mantid.CylinderAbsorption(InputWorkspace=w, OutputWorkspace="T",
                               CylinderSampleHeight=h, CylinderSampleRadius=r, AttenuationXSection=Xa,
                               ScatteringXSection=Xs, SampleNumberDensity=nd,
                               NumberOfSlices="10", NumberOfAnnuli="10", NumberOfWavelengthPoints="25", ExpMethod="Normal")
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
                mantid.Minus(LHSWorkspace=wfocname, RHSWorkspace="empty", OutputWorkspace=wfocname)
                mantid.DeleteWorkspace("empty")
                print "will try to load a vanadium with the name:" + WISH_getvana(i, SEvana, cyclevana)
                mantid.LoadNexusProcessed(Filename=WISH_getvana(i, SEvana, cyclevana), OutputWorkspace="vana")
                mantid.RebinToWorkspace(WorkspaceToRebin="vana", WorkspaceToMatch=wfocname, OutputWorkspace="vana")
                mantid.Divide(LHSWorkspace=wfocname, RHSWorkspace="vana", OutputWorkspace=wfocname)
                mantid.DeleteWorkspace("vana")
                mantid.ConvertUnits(InputWorkspace=wfocname, OutputWorkspace=wfocname, Target="TOF", EMode="Elastic")
                mantid.ReplaceSpecialValues(InputWorkspace=wfocname, OutputWorkspace=wfocname, NaNValue=0.0, NaNError=0.0,
                                     InfinityValue=0.0, InfinityError=0.0)
                mantid.SaveGSS(InputWorkspace=wfocname, Filename=WISH_userdir() + str(number) + "-" + str(i) + ext + ".gss",
                        Append=False, Bank=1)
                mantid.SaveFocusedXYE(wfocname, WISH_userdir() + str(number) + "-" + str(i) + ext + ".dat")
                mantid.SaveNexusProcessed(wfocname, WISH_userdir() + str(number) + "-" + str(i) + ext + ".nxs")
        else:
            mantid.LoadNexusProcessed(Filename=WISH_getempty(panel, SEsample, emptySEcycle), OutputWorkspace="empty")
            mantid.RebinToWorkspace(WorkspaceToRebin="empty", WorkspaceToMatch=wfocname, OutputWorkspace="empty")
            mantid.Minus(LHSWorkspace=wfocname, RHSWorkspace="empty", OutputWorkspace=wfocname)
            mantid.DeleteWorkspace("empty")
            print "will try to load a vanadium with the name:" + WISH_getvana(panel, SEvana, cyclevana)
            mantid.LoadNexusProcessed(Filename=WISH_getvana(panel, SEvana, cyclevana), OutputWorkspace="vana")
            mantid.RebinToWorkspace(WorkspaceToRebin="vana", WorkspaceToMatch=wfocname, OutputWorkspace="vana")
            mantid.Divide(LHSWorkspace=wfocname, RHSWorkspace="vana", OutputWorkspace=wfocname)
            mantid.DeleteWorkspace("vana")
            mantid.ConvertUnits(InputWorkspace=wfocname, OutputWorkspace=wfocname, Target="TOF", EMode="Elastic")
            mantid.ReplaceSpecialValues(InputWorkspace=wfocname, OutputWorkspace=wfocname, NaNValue=0.0, NaNError=0.0,
                                 InfinityValue=0.0, InfinityError=0.0)
            mantid.SaveGSS(InputWorkspace=wfocname, Filename=WISH_userdir() + str(number) + "-" + str(panel) + ext + ".gss",
                    Append=False, Bank=1)
            mantid.SaveFocusedXYE(wfocname, WISH_userdir() + str(number) + "-" + str(panel) + ext + ".dat")
            mantid.SaveNexusProcessed(wfocname, WISH_userdir() + str(number) + "-" + str(panel) + ext + ".nxs")
        return wfocname


    # Create a corrected vanadium (normalise,corrected for attenuation and empty, strip peaks) and
    # save a a nexus processed file.
    # It looks like smoothing of 100 works quite well
    def WISH_createvan(van, empty, panel, smoothing, vh, vr, cycle_van="18_2", cycle_empty="17_1"):
        WISH_startup("ffv81422", cycle_van)
        WISH_setdatafile(WISH_getfilename(41870, "nxs"))
        WISH_setdatadir("/archive/ndxwish/Instrument/data/cycle_" + cycle_van + "/")
        wvan = WISH_read(van, panel, "nxs_event")
        WISH_startup("ffv81422", cycle_empty)
        WISH_setdatafile(WISH_getfilename(38581, "nxs"))
        WISH_setdatadir("/archive/ndxwish/Instrument/data/cycle_" + cycle_empty + "/")
        wempty = WISH_read(empty, panel, "nxs_event")
        mantid.Minus(LHSWorkspace=wvan, RHSWorkspace=wempty, OutputWorkspace=wvan)
        print "read van and empty"
        mantid.DeleteWorkspace(wempty)
        mantid.ConvertUnits(InputWorkspace=wvan, OutputWorkspace=wvan, Target="Wavelength", EMode="Elastic")
        mantid.CylinderAbsorption(InputWorkspace=wvan, OutputWorkspace="T",
                           CylinderSampleHeight=str(vh), CylinderSampleRadius=str(vr), AttenuationXSection="4.8756",
                           ScatteringXSection="5.16", SampleNumberDensity="0.07118",
                           NumberOfSlices="10", NumberOfAnnuli="10", NumberOfWavelengthPoints="25", ExpMethod="Normal")
        mantid.Divide(LHSWorkspace=wvan, RHSWorkspace="T", OutputWorkspace=wvan)
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


    # Have made no changes here as not called (may not work in future though)
    def WISH_monitors(rb, ext):
        data_dir = WISH_dir()
        file = WISH_getfilename(rb, ext)
        wout = "w" + str(rb)
        print "reading File..." + file
        mantid.LoadRaw(Filename=file, OutputWorkspace=wout, SpectrumMin=str(1), SpectrumMax=str(5), LoadLogFiles="0")
        mantid.NormaliseByCurrent(InputWorkspace=wout, OutputWorkspace=wout)
        mantid.ConvertToDistribution(wout)
        return wout


    # Have made no changes here as not called (may not work in future though)
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
                               NumberOfSlices="10", NumberOfAnnuli="10", NumberOfWavelengthPoints="25", ExpMethod="Normal")
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
            mantid.Minus(LHSWorkspace=wfocname, RHSWorkspace="empty", OutputWorkspace=wfocname)
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
        mantid.LoadRaw(Filename=file, OutputWorkspace=output, spectrummin=str(min), spectrummax=str(max), LoadLogFiles="0")
        mantid.Integration(InputWorkspace=output, OutputWorkspace=output + "int")
        g = plotTimeBin(output + "int", 0)


    # Smoothing the incident beam monitor using a spline function.  Regions around Bragg edges are masked, before fitting with a  spline function.
    # Returns a smooth monitor spectrum


    def WISH_process_incidentmon(number, ext, spline_terms=20, debug=False):
        if type(number) is int:
            fname = WISH_getdatafile()
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
                mantid.ConvertUnits(InputWorkspace=works, OutputWorkspace=works, Target="Wavelength", Emode="Elastic")
            if (ext[0:9] == "nxs_event"):
                temp = "w" + str(number) + "_monitors"
                works = "w" + str(number) + "_monitor4"
                mantid.Rebin(InputWorkspace=temp, OutputWorkspace=temp, Params='6000,-0.00063,110000', PreserveEvents=False)
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
            mantid.MaskBins(InputWorkspace=works, OutputWorkspace=works, XMin=ex_regions[0, reg], XMax=ex_regions[1, reg])
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
    def Removepeaks_spline_smooth_vana(works, panel, debug=False):
        splineterms = 0
        smoothterms=0
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
            x, y, z = mantid.getnarray(works, 0)
            p.plot(x, y)
        if (splineterms != 0):
            mantid.SplineBackground(InputWorkspace=works, OutputWorkspace=works, WorkspaceIndex=0, NCoeff=splineterms)
        if (debug):
            x, y, z = mantid.getnarray(works, 0)
            p.plot(x, y)
        if not (smoothterms==0):
            mantid.SmoothData(InputWorkspace=works, OutputWorkspace=works, NPoints=smoothterms)
        if (debug):
            x, y, z = mantid.getnarray(works, 0)
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
        panel_list = ['-1foc', '-2foc', '-3foc', '-4foc', '-5foc', '-6foc', '-7foc', '-8foc', '-9foc', '-10foc', '-1_10foc',
                      '-2_9foc', '-3_8foc', '-4_7foc', '-5_6foc']
        for p in panel_list:
            mantid.Minus(LHSWorkspace='w' + str(runno) + p, RHSWorkspace='w' + str(empty) + p,
                  OutputWorkspace='w' + str(runno) + 'minus' + str(empty) + p)
            mantid.ConvertUnits(InputWorkspace='w' + str(runno) + 'minus' + str(empty) + p,
                         OutputWorkspace='w' + str(runno) + 'minus' + str(empty) + p + '-d', Target='dSpacing')
            mantid.SaveGSS("w" + str(runno) + 'minus' + str(empty) + p, WISH_userdir() + str(i) + p + ".gss", Append=False, Bank=1)


    def main(input_file, output_dir):
        # Check files can be found
        validate(input_file, output_dir)

        # test="nxs_event_1_300.00_600.00"
        # print split_string_event(test)

        # #####################################################################
        # #####     			USER SPECIFIC PART STARTS BELOW 									   ##
        # #####     			IN CASE LINES ABOVE HAVE BEEN EDITED AND SCRIPTS NO LONGER WORK   ##
        # #####     			LOG OUT AND BACK IN TO THE MACHINE								   ##
        # #####################################################################
        # ########### SETTING the paths automatically : WISH_startup(username,cycle_name) ##############
        # WISH_startup("ryb18365","15_1")

        WISH_setuserdir(output_dir)
        WISH_setdatafile(input_file)
        print(output_dir)
        print(input_file)
        # #        WISH PROCESS ROUTINES TO EDIT   (penultimate line optional but useful for recovering data later on)  #
        # To add two raw files together, replace runno (integer, eg. 16800) by a string "int1+int2" (eg "16800+16801" note quotes)
        # ##############################################################################################
        # beg=0
        # end=1800
        # nbslices=int(end/180)
        # suffix=[]
        # for k in range(0,nbslices):
        #	suffix.append("nxs_event_slice"+str(k)+"_"+str(int(k*180))+"_"+str((k+1)*180))

        # print len(suffix)
        # print suffix[0], suffix[k]

        # for i in range(24901,24902):
        #	for j in range(2,3):
        #		for k in range(0,len(suffix)):
        #			wout=WISH_process(i,j,suffix[k],"candlestick","11_4","candlestick","11_4",absorb=False,nd=0.0,Xs=0.0,Xa=0.0,h=0.0,r=0.0)
        #			ConvertUnits(wout,wout+"-d","dSpacing")
        # ##############################################################################################
        # for i in range(24895,24896):
        #	for j in range(5,6):
        #		wout=WISH_process(i,j,"nxs_event_slice1_0_300","candlestick","11_4","candlestick","11_4",absorb=False,nd=0.0,Xs=0.0,Xa=0.0,h=0.0,r=0.0)
        #		ConvertUnits(wout,wout+"-d","dSpacing")
        #		wout=WISH_process(i,j,"nxs_event_slice2_300_600","candlestick","11_4","candlestick","11_4",absorb=False,nd=0.0,Xs=0.0,Xa=0.0,h=0.0,r=0.0)
        #		ConvertUnits(wout,wout+"-d","dSpacing")
        #		wout=WISH_process(i,j,"nxs_event_slice3_600_900","candlestick","11_4","candlestick","11_4",absorb=False,nd=0.0,Xs=0.0,Xa=0.0,h=0.0,r=0.0)
        #		ConvertUnits(wout,wout+"-d","dSpacing")
        #		wout=WISH_process(i,j,"nxs_event_slice4_900_1200","WISHcryo","11_3","WISHcryo","11_3",absorb=False,nd=0.0,Xs=0.0,Xa=0.0,h=0.0,r=0.0)
        #		ConvertUnits(wout,wout+"-d","dSpacing")
        #		wout=WISH_process(i,j,"nxs_event_slice5_1200_1500","WISHcryo","11_3","WISHcryo","11_3",absorb=False,nd=0.0,Xs=0.0,Xa=0.0,h=0.0,r=0.0)
        #		ConvertUnits(wout,wout+"-d","dSpacing")
        #		wout=WISH_process(i,j,"nxs_event_slice6_1500_1800","WISHcryo","11_3","WISHcryo","11_3",absorb=False,nd=0.0,Xs=0.0,Xa=0.0,h=0.0,r=0.0)
        #		ConvertUnits(wout,wout+"-d","dSpacing")-4foc
        #		wout=WISH_process(i,j,"nxs_event_slice7_1800_2100","WISHcryo","11_3","WISHcryo","11_3",absorb=False,nd=0.0,Xs=0.0,Xa=0.0,h=0.0,r=0.0)
        #		ConvertUnits(wout,wout+"-d","dSpacing")
        #		wout=WISH_process(i,j,"nxs_event_slice8_2100_2400","WISHcryo","11_3","WISHcryo","11_3",absorb=False,nd=0.0,Xs=0.0,Xa=0.0,h=0.0,r=0.0)
        #		ConvertUnits(wout,wout+"-d","dSpacing")
        #		wout=WISH_process(i,j,"nxs_event_slice9_2400_2700","WISHcryo","11_3","WISHcryo","11_3",absorb=False,nd=0.0,Xs=0.0,Xa=0.0,h=0.0,r=0.0)
        #		ConvertUnits(wout,wout+"-d","dSpacing")
        #		wout=WISH_process(i,j,"nxs_event_slice10_2700_3000","WISHcryo","11_3","WISHcryo","11_3",absorb=False,nd=0.0,Xs=0.0,Xa=0.0,h=0.0,r=0.0)
        #		ConvertUnits(wout,wout+"-d","dSpacing")
        #		wout=WISH_process(i,j,"nxs_event_slice11_3000_3300","WISHcryo","11_3","WISHcryo","11_3",absorb=False,nd=0.0,Xs=0.0,Xa=0.0,h=0.0,r=0.0)
        #		ConvertUnits(wout,wout+"-d","dSpacing")
        #		wout=WISH_process(i,j,"nxs_event_slice12_3300_end","WISHcryo","11_3","WISHcryo","11_3",absorb=False,nd=0.0,Xs=0.0,Xa=0.0,h=0.0,r=0.0)

        i = get_run_number(input_file)
        for j in range(1, 11):
            wout = WISH_process(i, j, "raw", "candlestick", "11_4", "candlestick", "11_4", absorb=False, nd=0.0, Xs=0.0,
                                Xa=0.0, h=4.0, r=0.4)
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

        # minus_emptycans(26977,26969)
        # #############################################################################################
        # for i in range(23840,23841):
        #	for j in range(5,0,-1):
        #		wout=WISH_process(i,j,"raw","candlestick","11_4","candlestick","11_4",absorb=False,nd=0.0,Xs=0.0,Xa=0.0,h=0.0,r=0.0)
        #		SaveNexusProcessed(wout,WISH_userdir()+wout+".nxs")
        #		ConvertUnits(wout,wout+"-d","dSpacing")

        # ###########################################################################################
        # for i in range(18880,18881):
        #	for j in range(1,6):
        #		wout=WISH_process(i,j,"raw","WISHcryo","11_3","WISHcryo","11_3",absorb=True,nd=0.0035,Xs=62.27,Xa=429.95,h=4.0,r=0.15)
        # ###########################################################################################
        # #########               END OF WISH PROCESS ROUTINES                               ##################

        # #####################################################################
        # How to retrieve already processed Data without having to reprocess it  (in case Mantid session has been closed #
        # list_overlap=['5_6','4_7','3_8','2_9','1_10']
        # for i in range(27739,27740):
        # for j in range(1,11):
        #    wr=str(i)+"-"+str(j)+"raw"
        #    LoadNexusProcessed(Filename=WISH_userdir()+wr+".nxs",OutputWorkspace=wr)
        #    ConvertUnits(InputWorkspace=wr,OutputWorkspace=wr+"-d",Target="dSpacing")
        # for k in list_overlap:
        #   wr=str(i)+"-"+k+"raw"
        #   LoadNexusProcessed(Filename=WISH_userdir()+wr+".nxs",OutputWorkspace=wr)
        #  ConvertUnits(InputWorkspace=wr,OutputWorkspace=wr+"-d",Target="dSpacing")
        #####################################################################
        # for runno in range(22678,22686):
        # for j in range(1,6):
        # wr="w"+str(runno)+"-"+str(j)+"foc"

        # for j in range(1,6):
        #	Plus("w22663"+"-"+str(j)+"foc","w22664"+"-"+str(j)+"foc","vancan"+"-"+str(j)+"foc")
        #	SaveGSS(InputWorkspace="vancan"+"-"+str(j)+"foc",Filename=WISH_userdir()+"vancan"+"-"+str(j)+"foc.gss",Append=False,Bank=1)

        # ####################################################################
        # If you don't have a correct empty, either use the most suitable one or use the lines below ####
        # for i in range(1197,1410):
        #	for j in range(1,6):
        #		wout=WISH_read(i,j,"raw")
        #		wfoc=WISH_focus(wout,j)
        # ####################################################################
        # use the lines below to manually set the paths if needed

        # ########  use the lines below to create a processed empty (instrument, cryostat, can..) run     ########
        # for i in range(4,5):
        #	WISH_createempty(20620,i)
        # SaveNexusProcessed("w16748-"+str(i)+"foc",WISH_userdir()+"emptyinst16748-"+str(i)+"foc.nx5")

    def create_vanadium():

        # ######### use the lines below to process a LoadRawvanadium run                               #################
        for j in range(1, 11):
            WISH_createvan(41865, 38581, j, 100, 4.0, 0.15, cycle_van="18_2", cycle_empty="17_1")
            mantid.CropWorkspace(InputWorkspace="w41865-" + str(j) + "foc", OutputWorkspace="w41865-" + str(j) + "foc",
                      XMin='0.35',
                      XMax='5.0')
            Removepeaks_spline_smooth_vana("w41865-" + str(j) + "foc", j, debug=False)
            mantid.SaveNexusProcessed("w41865-" + str(j) + "foc", WISH_userdir() + "vana41865-" + str(j) + "foc.nxs")


    if __name__ == "__main__":
        create_vanadium()


        #main(WISH_getdatafile(), WISH_userdir())


