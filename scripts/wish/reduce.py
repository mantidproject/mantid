from __future__ import (absolute_import, division, print_function)
import mantid.simpleapi as mantid
import os
import numpy as n


class Wish:
    NUM_PANELS = 11
    NUM_MONITORS = 6
    LAMBDA_RANGE = (0.7, 10.35)

    def __init__(self, input_mode, cal_directory, output_folder, delete_workspace,
                 user_directory="/archive/ndxwish/Instrument/data/cycle_"):
        self.name = input_mode
        self.cal_dir = cal_directory
        self.use_folder = user_directory
        self.out_folder = output_folder
        self.deleteWorkspace = delete_workspace
        self.username = None
        self.data_directory = None
        self.user_directory = None
        self.datafile = None
        self.userdatadir = None
        self.userdataprocessed = None
        self.return_panel = {
            1: (6, 19461),
            2: (19462, 38917),
            3: (38918, 58373),
            4: (58374, 77829),
            5: (77830, 97285),
            6: (97286, 116741),
            7: (116742, 136197),
            8: (136198, 155653),
            9: (155654, 175109),
            10: (175110, 194565),
            0: (6, 194565)
        }

    def set_user_name(self, username):
        self.username = username

    def set_data_directory(self, directory="/archive/ndxwish/Instrument/data/cycle_09_5/"):
        self.data_directory = directory

    def set_user_directory(self, directory):
        self.user_directory = directory

    def startup(self, cycle='14_3'):
        user_data_directory = self.use_folder + cycle + '/'
        self.set_data_directory(user_data_directory)
        print ("Raw Data in :   ", user_data_directory)
        user_data_processed = self.out_folder
        self.set_user_directory(directory=user_data_processed)
        print ("Processed Data in :   ", user_data_processed)

    # Returns the calibration filename
    def get_cal(self):
        return self.cal_dir + "cycle_10_3_noends_10to10.cal"

    # Returns the grouping filename
    def get_group_file(self):
        return self.cal_dir + "cycle_10_3_noends_10to10.cal"

    def get_vanadium(self, panel, cycle="09_4"):
        vanadium_string = {
            "09_2": "vana318-{}foc-rmbins-smooth50.nx5",
            "09_3": "vana935-{}foc-SS.nx5",
            "09_4": "vana3123-{}foc-SS.nx5",
            "09_5": "vana3123-{}foc-SS.nx5",
            "11_1": "vana17718-{}foc-SS.nxs",
            "11_2": "vana16812-{}foc-SS.nx5",
            "11_3": "vana18590-{}foc-SS-new.nxs",
            "11_4": "vana19612-{}foc-SF-SS.nxs",
        }
        return self.cal_dir + vanadium_string.get(cycle).format(panel)

    def get_file_name(self, run_number, extension):
        if extension[0] != 's':
            data_dir = self.data_directory
        else:
            data_dir = self.data_directory
        digit = len(str(run_number))
        filename = os.path.join(data_dir, "WISH")
        for i in range(8 - digit):
            filename = filename + "0"
        filename += str(run_number) + "." + extension
        return filename

    # Reads a wish data file return a workspace with a short name
    def read(self, number, panel, ext):
        if type(number) is int:
            filename = self.datafile
            print ("will be reading filename..." + filename)
            spectra_min, spectra_max = self.return_panel.get(panel)
            if panel != 0:
                output = "w{0}-{1}".format(number, panel)
            else:
                output = "w{}".format(number)
            shared_load_files(ext, filename,output,spectra_max,spectra_min,False)
            if ext == "nxs_event":
                mantid.LoadEventNexus(Filename=filename, OutputWorkspace=output, LoadMonitors='1')
                mantid.RenameWorkspace(output + "_monitors", "w" + str(number) + "_monitors")
                mantid.Rebin(InputWorkspace=output, OutputWorkspace=output, Params='6000,-0.00063,110000')
                mantid.ConvertToMatrixWorkspace(output, output)
                spectra_min, spectra_max = self.return_panel.get(panel)
                mantid.CropWorkspace(InputWorkspace=output, OutputWorkspace=output, StartWorkspaceIndex=spectra_min - 6,
                                     EndWorkspaceIndex=spectra_max - 6)
                mantid.MaskBins(InputWorkspace=output, OutputWorkspace=output, XMin=99900, XMax=106000)
                print ("full nexus eventfile loaded")
            if ext[0:10] == "nxs_event_":
                label, tmin, tmax = split_string_event(ext)
                output = output + "_" + label
                if tmax == "end":
                    mantid.LoadEventNexus(Filename=filename, OutputWorkspace=output, FilterByTimeStart=tmin,
                                          LoadMonitors='1',
                                          MonitorsAsEvents='1', FilterMonByTimeStart=tmin)
                else:
                    mantid.LoadEventNexus(Filename=filename, OutputWorkspace=output, FilterByTimeStart=tmin,
                                          FilterByTimeStop=tmax,
                                          LoadMonitors='1', MonitorsAsEvents='1', FilterMonByTimeStart=tmin,
                                          FilterMonByTimeStop=tmax)
                    mantid.RenameWorkspace(output + "_monitors", "w" + str(number) + "_monitors")
                print ("renaming monitors done!")
                mantid.Rebin(InputWorkspace=output, OutputWorkspace=output, Params='6000,-0.00063,110000')
                mantid.ConvertToMatrixWorkspace(output, output)
                spectra_min, spectra_max = self.return_panel.get(panel)
                mantid.CropWorkspace(InputWorkspace=output, OutputWorkspace=output, StartWorkspaceIndex=spectra_min - 6,
                                     EndWorkspaceIndex=spectra_max - 6)
                mantid.MaskBins(output, output, XMin=99900, XMax=106000)
                print ("nexus event file chopped")
        else:
            n1, n2 = split_string(number)
            output = "w" + str(n1) + "_" + str(n2) + "-" + str(panel)
            filename = self.get_file_name(n1, ext)
            print ("reading filename..." + filename)
            spectra_min, spectra_max = self.return_panel.get(panel)
            output1 = "w" + str(n1) + "-" + str(panel)
            mantid.LoadRaw(Filename=filename, OutputWorkspace=output1, SpectrumMin=str(spectra_min), SpectrumMax=str(spectra_max),
                           LoadLogFiles="0")
            filename = self.get_file_name(n2, ext)
            print ("reading filename..." + filename)
            spectra_min, spectra_max = self.return_panel.get(panel)
            output2 = "w" + str(n2) + "-" + str(panel)
            mantid.LoadRaw(Filename=filename, OutputWorkspace=output2, SpectrumMin=str(spectra_min), SpectrumMax=str(spectra_max),
                           LoadLogFiles="0")
            mantid.MergeRuns(output1 + "," + output2, output)
            mantid.DeleteWorkspace(output1)
            mantid.DeleteWorkspace(output2)
        mantid.ConvertUnits(InputWorkspace=output, OutputWorkspace=output, Target="Wavelength", Emode="Elastic")
        lmin, lmax = Wish.LAMBDA_RANGE
        mantid.CropWorkspace(InputWorkspace=output, OutputWorkspace=output, XMin=lmin, XMax=lmax)
        monitor = self.process_incidentmon(number, ext, spline_terms=70)
        print ("first norm to be done")
        mantid.NormaliseToMonitor(InputWorkspace=output, OutputWorkspace=output + "norm1", MonitorWorkspace=monitor)
        print ("second norm to be done")
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
    def focus_onepanel(self, work, focus, panel):
        mantid.AlignDetectors(InputWorkspace=work, OutputWorkspace=work, CalibrationFile=self.get_cal())
        mantid.DiffractionFocussing(InputWorkspace=work, OutputWorkspace=focus, GroupingFileName=self.get_group_file())
        if self.deleteWorkspace:
            mantid.DeleteWorkspace(work)
        if panel == 5 or panel == 6:
            mantid.CropWorkspace(InputWorkspace=focus, OutputWorkspace=focus, XMin=0.3)
        return focus

    def split(self, focus):
        for i in range(0, 11):
            out = focus[0:len(focus) - 3] + "-" + str(i + 1) + "foc"
            mantid.ExtractSingleSpectrum(InputWorkspace=focus, OutputWorkspace=out, WorkspaceIndex=i)
            mantid.DeleteWorkspace(focus)
        return

    def focus(self, work, panel):
        focus = work + "foc"
        if panel != 0:
            return self.focus_onepanel(work, focus, panel)
        else:
            self.focus_onepanel(work, focus, panel)
            self.split(focus)

    def process(self, number, panel, ext, cyclevana="09_4", absorb=False, nd=0.0, Xs=0.0, Xa=0.0, h=0.0, r=0.0):
        w = self.read(number, panel, ext)
        print ("file read and normalized")
        if absorb:
            mantid.ConvertUnits(InputWorkspace=w, OutputWorkspace=w, Target="Wavelength", EMode="Elastic")
            mantid.CylinderAbsorption(InputWorkspace=w, OutputWorkspace="T",
                                      CylinderSampleHeight=h, CylinderSampleRadius=r, AttenuationXSection=Xa,
                                      ScatteringXSection=Xs, SampleNumberDensity=nd,
                                      NumberOfSlices="10", NumberOfAnnuli="10", NumberOfWavelengthPoints="25",
                                      ExpMethod="Normal")
            mantid.Divide(LHSWorkspace=w, RHSWorkspace="T", OutputWorkspace=w)
            mantid.DeleteWorkspace("T")
            mantid.ConvertUnits(InputWorkspace=w, OutputWorkspace=w, Target="TOF", EMode="Elastic")
        wfocname = self.focus(w, panel)
        print ("focussing done!")

        panel_crop = {
            1: (0.8, 53.3),
            2: (0.5, 13.1),
            3: (0.5, 7.77),
            4: (0.4, 5.86),
            5: (0.35, 4.99),
            6: (0.35, 4.99),
            7: (0.4, 5.86),
            8: (0.5, 7.77),
            9: (0.5, 13.1),
            10: (0.8, 53.3)
        }
        d_min, d_max = panel_crop.get(panel)
        mantid.CropWorkspace(InputWorkspace=wfocname, OutputWorkspace=wfocname, XMin=d_min, XMax=d_max)

        if panel == 0:
            for i in range(Wish.NUM_PANELS):
                wfocname = "w" + str(number) + "-" + str(i) + "foc"
                mantid.CropWorkspace(InputWorkspace=wfocname, OutputWorkspace=wfocname, XMin=d_min, XMax=d_max)
            for i in range(Wish.NUM_PANELS):
                wfocname = "w" + str(number) + "-" + str(i) + "foc"
                print ("will try to load a vanadium with the name:" + self.get_vanadium(i,  cyclevana))
                mantid.LoadNexusProcessed(Filename=self.get_vanadium(i,  cyclevana), OutputWorkspace="vana")
                mantid.RebinToWorkspace(WorkspaceToRebin="vana", WorkspaceToMatch=wfocname, OutputWorkspace="vana")
                mantid.Divide(LHSWorkspace=wfocname, RHSWorkspace="vana", OutputWorkspace=wfocname)
                mantid.DeleteWorkspace("vana")
                mantid.ConvertUnits(InputWorkspace=wfocname, OutputWorkspace=wfocname, Target="TOF", EMode="Elastic")
                mantid.ReplaceSpecialValues(InputWorkspace=wfocname, OutputWorkspace=wfocname, NaNValue=0.0,
                                            NaNError=0.0,
                                            InfinityValue=0.0, InfinityError=0.0)
                mantid.SaveGSS(InputWorkspace=wfocname,
                               Filename=self.user_directory + str(number) + "-" + str(i) + ext + ".gss",
                               Append=False, Bank=1)
                mantid.SaveFocusedXYE(wfocname, self.user_directory + str(number) + "-" + str(i) + ext + ".dat")
                mantid.SaveNexusProcessed(wfocname, self.user_directory + str(number) + "-" + str(i) + ext + ".nxs")
        else:
            print ("will try to load a vanadium with the name:" + self.get_vanadium(panel,  cyclevana))
            mantid.LoadNexusProcessed(Filename=self.get_vanadium(panel, cyclevana), OutputWorkspace="vana")
            mantid.RebinToWorkspace(WorkspaceToRebin="vana", WorkspaceToMatch=wfocname, OutputWorkspace="vana")
            mantid.Divide(LHSWorkspace=wfocname, RHSWorkspace="vana", OutputWorkspace=wfocname)
            mantid.DeleteWorkspace("vana")
            mantid.ConvertUnits(InputWorkspace=wfocname, OutputWorkspace=wfocname, Target="TOF", EMode="Elastic")
            mantid.ReplaceSpecialValues(InputWorkspace=wfocname, OutputWorkspace=wfocname, NaNValue=0.0, NaNError=0.0,
                                        InfinityValue=0.0, InfinityError=0.0)
            mantid.SaveGSS(InputWorkspace=wfocname,
                           Filename=self.user_directory + str(number) + "-" + str(panel) + ext + ".gss",
                           Append=False, Bank=1)
            mantid.SaveFocusedXYE(wfocname, self.user_directory + str(number) + "-" + str(panel) + ext + ".dat")
            mantid.SaveNexusProcessed(wfocname, self.user_directory + str(number) + "-" + str(panel) + ext + ".nxs")
        return wfocname

    # Create a corrected vanadium (normalise,corrected for attenuation and empty, strip peaks) and
    # save a a nexus processed file.
    # It looks like smoothing of 100 works quite well
    # def createvan(self, van, empty, panel, smoothing, vh, vr, cycle_van="09_3", cycle_empty="09_3"):
    #    setdatadir("/archive/ndxwish/Instrument/data/cycle_" + cycle_van + "/")
    #    wvan = read(van, panel, "nxs_event")
    #    setdatadir("/archive/ndxwish/Instrument/data/cycle_" + cycle_empty + "/")
    #    mantid.ConvertUnits(InputWorkspace=wvan, OutputWorkspace=wvan, Target="Wavelength", EMode="Elastic")
    #    mantid.CylinderAbsorption(InputWorkspace=wvan, OutputWorkspace="T",
    #                              CylinderSampleHeight=str(vh), CylinderSampleRadius=str(vr),
    #                              AttenuationXSection="4.8756",
    #                              ScatteringXSection="5.16", SampleNumberDensity="0.07118",
    #                              NumberOfSlices="10", NumberOfAnnuli="10", NumberOfWavelengthPoints="25",
    #                              ExpMethod="Normal")
    #    mantid.Divide(LHSWorkspace=wvan, RHSWorkspace="T", OutputWorkspave=wvan)
    #    mantid.DeleteWorkspace("T")
    #    mantid.ConvertUnits(InputWorkspace=wvan, OutputWorkspace=wvan, Target="TOF", EMode="Elastic")
    #    vanfoc = focus(wvan, panel)
    #    mantid.DeleteWorkspace(wvan)
    #    # StripPeaks(vanfoc,vanfoc)
    #    # SmoothData(vanfoc,vanfoc,str(smoothing))
    #    return

    def monitors(self, rb, ext):
        monitor_file = self.get_file_name(rb, ext)
        wout = "w" + str(rb)
        print ("reading File..." + monitor_file)
        mantid.LoadRaw(Filename=monitor_file, OutputWorkspace=wout, SpectrumMin=str(1), SpectrumMax=str(5),
                       LoadLogFiles="0")
        mantid.NormaliseByCurrent(InputWorkspace=wout, OutputWorkspace=wout)
        mantid.ConvertToDistribution(wout)
        return wout

    def process_incidentmon(self, number, ext, spline_terms=20):
        if type(number) is int:
            fname = self.get_file_name(number, ext)
            works = "monitor" + str(number)
            shared_load_files(ext, fname, works, 4, 4, True)
            if ext[:9] == "nxs_event":
                temp = "w" + str(number) + "_monitors"
                works = "w" + str(number) + "_monitor4"
                mantid.Rebin(InputWorkspace=temp, OutputWorkspace=temp, Params='6000,-0.00063,110000',
                             PreserveEvents=False)
                mantid.ExtractSingleSpectrum(InputWorkspace=temp, OutputWorkspace=works, WorkspaceIndex=3)
        else:
            n1, n2 = split_string(number)
            works = "monitor" + str(n1) + "_" + str(n2)
            fname = self.get_file_name(n1, ext)
            works1 = "monitor" + str(n1)
            mantid.LoadRaw(Filename=fname, OutputWorkspace=works1, SpectrumMin=4, SpectrumMax=4, LoadLogFiles="0")
            fname = self.get_file_name(n2, ext)
            works2 = "monitor" + str(n2)
            mantid.LoadRaw(Filename=fname, OutputWorkspace=works2, SpectrumMin=4, SpectrumMax=4, LoadLogFiles="0")
            mantid.MergeRuns(InputWorkspaces=works1 + "," + works2, OutputWorkspace=works)
            mantid.DeleteWorkspace(works1)
            mantid.DeleteWorkspace(works2)
        mantid.ConvertUnits(InputWorkspace=works, OutputWorkspace=works, Target="Wavelength", Emode="Elastic")
        lmin, lmax = Wish.LAMBDA_RANGE
        mantid.CropWorkspace(InputWorkspace=works, OutputWorkspace=works, XMin=lmin, XMax=lmax)
        ex_regions = n.zeros((2, 4))
        ex_regions[:, 0] = [4.57, 4.76]
        ex_regions[:, 1] = [3.87, 4.12]
        ex_regions[:, 2] = [2.75, 2.91]
        ex_regions[:, 3] = [2.24, 2.50]
        mantid.ConvertToDistribution(works)

        for reg in range(0, 4):
            mantid.MaskBins(InputWorkspace=works, OutputWorkspace=works, XMin=ex_regions[0, reg],
                            XMax=ex_regions[1, reg])

        mantid.SplineBackground(InputWorkspace=works, OutputWorkspace=works, WorkspaceIndex=0, NCoeff=spline_terms)

        mantid.SmoothData(InputWorkspace=works, OutputWorkspace=works, NPoints=40)
        mantid.ConvertFromDistribution(works)
        return works
    
    def reduce(self):

        self.startup("18_1")
        for i in range(40503, 40504):
            self.datafile = self.get_file_name(i, "raw")
            for j in range(5, 7):
                wout = self.process(i, j, "raw", "11_4", absorb=True, nd=0.025, Xs=5.463, Xa=2.595, h=4.0, r=0.55)
                mantid.ConvertUnits(InputWorkspace=wout, OutputWorkspace=wout + "-d", Target="dSpacing",
                                    EMode="Elastic")

            for panel in range(5, 6):
                self.save_combined_panel(i, panel)

    def save_combined_panel(self, run, panel):
        panel_combination = {
            5: "6",
            4: "7",
            3: "8",
            2: "9",
            1: "10"
        }
        input_workspace1 = "w{0}-{1}foc".format(run, panel)
        input_workspace2 = "w{0}-{1}foc".format(run, panel_combination.get(panel))
        combined = "{0}{1}-{2}_{3}foc{4}".format("{0}", run, panel, panel_combination.get(panel), "{1}")
        combined_save = combined.format("", "{}")
        combined_ws = combined.format("w", "")

        mantid.RebinToWorkspace(WorkspaceToRebin=input_workspace2, WorkspaceToMatch=input_workspace1,
                                OutputWorkspace=input_workspace2, PreserveEvents='0')
        mantid.Plus(LHSWorkspace=input_workspace1, RHSWorkspace=input_workspace2,
                    OutputWorkspace=combined_ws)
        mantid.ConvertUnits(InputWorkspace=combined_ws, OutputWorkspace=combined_ws + "-d",
                            Target="dSpacing",
                            EMode="Elastic")

        mantid.SaveGSS(combined_ws, os.path.join(self.user_directory, combined_save.format("raw.gss")),
                       Append=False,
                       Bank=1)
        mantid.SaveFocusedXYE(combined_ws,
                              os.path.join(self.user_directory, combined_save.format("raw.dat")))
        mantid.SaveNexusProcessed(combined_ws, os.path.join(self.user_directory, combined_save.format("raw.nxs")))
        

# removes the peaks in a vanadium  run, then performs a spline and a smooth
def removepeaks_spline_smooth_empty(works, panel):
    smoothterms = {
        1: 30,
        2: 10,
        3: 15,
        4: 15,
        5: 10,
        6: 10,
        7: 15,
        8: 15,
        9: 10,
        10: 30
    }
    smooth_term = smoothterms.get(panel)

    mantid.SmoothData(InputWorkspace=works, OutputWorkspace=works, NPoints=smooth_term)
    return works


def split_string(t):
    indxp = 0
    for i in range(0, len(t)):
        if t[i] == "+":
            indxp = i
    if indxp != 0:
        return int(t[0:indxp]), int(t[indxp + 1:len(t)])


def split_string_event(t):
    # this assumes the form nxs_event_label_tmin_tmax
    indx_ = []
    for i in range(10, len(t)):
        if t[i] == "_":
            indx_.append(i)
    label = t[10:indx_[0]]
    tmin = t[indx_[0] + 1:indx_[1]]
    tmax = t[indx_[1] + 1:len(t)]
    return label, tmin, tmax


def shared_load_files(extension, filename, output, spectrum_max, spectrum_min, is_monitor):
    if not (extension == "nxs" or extension == "raw" or extension[0] == "s"):
        return False
    if extension == "nxs":
        mantid.Load(Filename=filename, OutputWorkspace=output, SpectrumMin=spectrum_min,
                    SpectrumMax=spectrum_min)
    else:
        mantid.LoadRaw(Filename=filename, OutputWorkspace=output, SpectrumMin=spectrum_min, SpectrumMax=spectrum_max,
                       LoadLogFiles="0")
    if not is_monitor:
        mantid.MaskBins(InputWorkspace=output,  OutputWorkspace=output, XMin=99900, XMax=106000)
    return True
