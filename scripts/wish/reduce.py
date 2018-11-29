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
        print("Raw Data in :   ", user_data_directory)
        user_data_processed = self.out_folder
        self.set_user_directory(directory=user_data_processed)
        print("Processed Data in :   ", user_data_processed)

    # Returns the calibration filename
    def get_cal(self):
        return self.cal_dir + "WISH_cycle_10_3_noends_10to10.cal"

    # Returns the grouping filename
    def get_group_file(self):
        return self.cal_dir + "WISH_cycle_10_3_noends_10to10.cal"

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
            print("will be reading filename...", filename)
            spectra_min, spectra_max = self.return_panel.get(panel)
            if panel != 0:
                output = "w{0}-{1}".format(number, panel)
            else:
                output = "w{}".format(number)
            shared_load_files(ext, filename, output, spectra_max, spectra_min, False)
            if ext == "nxs_event":
                mantid.LoadEventNexus(Filename=filename, OutputWorkspace=output, LoadMonitors='1')
                self.read_event_nexus(number, output, panel)
                print("full nexus eventfile loaded")
            if ext[:10] == "nxs_event_":
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
                self.read_event_nexus(number, output, panel)
                print("nexus event file chopped")
        else:
            num_1, num_2 = split_string(number)
            output = "w{0}_{1}-{2}".format(num_1, num_2, panel)
            output1 = self.load_multi_run_part(ext, num_1, panel)
            output2 = self.load_multi_run_part(ext, num_2, panel)
            mantid.MergeRuns(output1 + "," + output2, output)
            mantid.DeleteWorkspace(output1)
            mantid.DeleteWorkspace(output2)
        mantid.ConvertUnits(InputWorkspace=output, OutputWorkspace=output, Target="Wavelength", Emode="Elastic")
        lmin, lmax = Wish.LAMBDA_RANGE
        mantid.CropWorkspace(InputWorkspace=output, OutputWorkspace=output, XMin=lmin, XMax=lmax)
        monitor = self.process_incidentmon(number, ext, spline_terms=70)
        print("first norm to be done")
        mantid.NormaliseToMonitor(InputWorkspace=output, OutputWorkspace=output + "norm1", MonitorWorkspace=monitor)
        print("second norm to be done")
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

    def load_multi_run_part(self, ext, run, panel):
        filename = self.get_file_name(run, ext)
        print("reading filename...", filename)
        spectra_min, spectra_max = self.return_panel.get(panel)
        output1 = "w{0}-{1}".format(run, panel)
        mantid.LoadRaw(Filename=filename, OutputWorkspace=output1, SpectrumMin=str(spectra_min),
                       SpectrumMax=str(spectra_max), LoadLogFiles="0")
        return output1

    def read_event_nexus(self, number, output, panel):
        mantid.RenameWorkspace("{}_monitors".format(output), "w{}_monitors".format(number))
        mantid.Rebin(InputWorkspace=output, OutputWorkspace=output, Params='6000,-0.00063,110000')
        mantid.ConvertToMatrixWorkspace(output, output)
        spectra_min, spectra_max = self.return_panel.get(panel)
        mantid.CropWorkspace(InputWorkspace=output, OutputWorkspace=output, StartWorkspaceIndex=spectra_min - 6,
                             EndWorkspaceIndex=spectra_max - 6)
        mantid.MaskBins(InputWorkspace=output, OutputWorkspace=output, XMin=99900, XMax=106000)

    # Focus dataset for a given panel and return the workspace
    def focus_onepanel(self, work, focus, panel):
        mantid.AlignDetectors(InputWorkspace=work, OutputWorkspace=work, CalibrationFile=self.get_cal())
        mantid.DiffractionFocussing(InputWorkspace=work, OutputWorkspace=focus, GroupingFileName=self.get_group_file())
        if self.deleteWorkspace:
            mantid.DeleteWorkspace(work)
        if panel == 5 or panel == 6:
            mantid.CropWorkspace(InputWorkspace=focus, OutputWorkspace=focus, XMin=0.3)
        return focus

    def focus(self, work, panel):
        focus = "{}foc".format(work)
        if panel != 0:
            return self.focus_onepanel(work, focus, panel)
        else:
            self.focus_onepanel(work, focus, panel)
            split(focus)

    def process_run(self, number, panel, extension, cyclevana="09_4", absorb=False, number_density=0.0, scattering=0.0,
                    attenuation=0.0, height=0.0, radius=0.0):
        workspace_to_focus = self.read(number, panel, extension)
        print("file read and normalized")
        if absorb:
            absorption_corrections(attenuation, height, number_density, radius, scattering, workspace_to_focus)
        wfocname = self.focus(workspace_to_focus, panel)
        print("focussing done!")

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
                wfocname = "w{0}-{1}foc".format(number, i)
                mantid.CropWorkspace(InputWorkspace=wfocname, OutputWorkspace=wfocname, XMin=d_min, XMax=d_max)
                print("will try to load a vanadium with the name:", self.get_vanadium(i,  cyclevana))
                self.apply_vanadium_corrections(cyclevana, i, wfocname)
                mantid.SaveGSS(InputWorkspace=wfocname,
                               Filename="{0}{1}-{2}{3}.gss".format(self.user_directory, number, i, extension),
                               Append=False, Bank=1)
                mantid.SaveFocusedXYE(wfocname, "{0}{1}-{2}{3}.dat".format(self.user_directory, number, i, extension))
                mantid.SaveNexusProcessed(wfocname, "{0}{1}-{2}{3}.nxs".format(self.user_directory, number, i,
                                                                               extension))
        else:
            print("will try to load a vanadium with the name:", self.get_vanadium(panel,  cyclevana))
            self.apply_vanadium_corrections(cyclevana, panel, wfocname)
            mantid.SaveGSS(InputWorkspace=wfocname,
                           Filename="{0}{1}-{2}{3}.gss".format(self.user_directory, number, panel, extension),
                           Append=False, Bank=1)
            mantid.SaveFocusedXYE(wfocname, "{0}{1}-{2}{3}.dat".format(self.user_directory, number, panel, extension))
            mantid.SaveNexusProcessed(wfocname, "{0}{1}-{2}{3}.nxs".format(self.user_directory, number, panel,
                                                                           extension))
        return wfocname

    def apply_vanadium_corrections(self, cyclevana, i, wfocname):
        mantid.LoadNexusProcessed(Filename=self.get_vanadium(i, cyclevana), OutputWorkspace="vana")
        mantid.RebinToWorkspace(WorkspaceToRebin="vana", WorkspaceToMatch=wfocname, OutputWorkspace="vana")
        mantid.Divide(LHSWorkspace=wfocname, RHSWorkspace="vana", OutputWorkspace=wfocname)
        mantid.DeleteWorkspace("vana")
        mantid.ConvertUnits(InputWorkspace=wfocname, OutputWorkspace=wfocname, Target="TOF", EMode="Elastic")
        mantid.ReplaceSpecialValues(InputWorkspace=wfocname, OutputWorkspace=wfocname, NaNValue=0.0,
                                    NaNError=0.0,
                                    InfinityValue=0.0, InfinityError=0.0)

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
        wout = "w{}".format(rb)
        print("reading File...", monitor_file)
        mantid.LoadRaw(Filename=monitor_file, OutputWorkspace=wout, SpectrumMin=1, SpectrumMax=5, LoadLogFiles="0")
        mantid.NormaliseByCurrent(InputWorkspace=wout, OutputWorkspace=wout)
        mantid.ConvertToDistribution(wout)
        return wout

    def process_incidentmon(self, number, extension, spline_terms=20):
        if type(number) is int:
            filename = self.get_file_name(number, extension)
            works = "monitor{}".format(number)
            shared_load_files(extension, filename, works, 4, 4, True)
            if extension[:9] == "nxs_event":
                temp = "w{}_monitors".format(number)
                works = "w{}_monitor4".format(number)
                mantid.Rebin(InputWorkspace=temp, OutputWorkspace=temp, Params='6000,-0.00063,110000',
                             PreserveEvents=False)
                mantid.ExtractSingleSpectrum(InputWorkspace=temp, OutputWorkspace=works, WorkspaceIndex=3)
        else:
            num_1, num_2 = split_string(number)
            works = "monitor{0}_{1}".format(num_1, num_2)
            filename = self.get_file_name(num_1, extension)
            works1 = "monitor{}".format(num_1)
            mantid.LoadRaw(Filename=filename, OutputWorkspace=works1, SpectrumMin=4, SpectrumMax=4, LoadLogFiles="0")
            filename = self.get_file_name(num_2, extension)
            works2 = "monitor{}".format(num_2)
            mantid.LoadRaw(Filename=filename, OutputWorkspace=works2, SpectrumMin=4, SpectrumMax=4, LoadLogFiles="0")
            mantid.MergeRuns(InputWorkspaces=works1 + "," + works2, OutputWorkspace=works)
            mantid.DeleteWorkspace(works1)
            mantid.DeleteWorkspace(works2)
        mantid.ConvertUnits(InputWorkspace=works, OutputWorkspace=works, Target="Wavelength", Emode="Elastic")
        lmin, lmax = Wish.LAMBDA_RANGE
        mantid.CropWorkspace(InputWorkspace=works, OutputWorkspace=works, XMin=lmin, XMax=lmax)
        ex_regions = n.array([[4.57, 4.76],
                              [3.87, 4.12],
                              [2.75, 2.91],
                              [2.24, 2.50]])
        mantid.ConvertToDistribution(works)

        for reg in range(0, 4):
            mantid.MaskBins(InputWorkspace=works, OutputWorkspace=works, XMin=ex_regions[reg, 0],
                            XMax=ex_regions[reg, 1])

        mantid.SplineBackground(InputWorkspace=works, OutputWorkspace=works, WorkspaceIndex=0, NCoeff=spline_terms)

        mantid.SmoothData(InputWorkspace=works, OutputWorkspace=works, NPoints=40)
        mantid.ConvertFromDistribution(works)
        return works
    
    def reduce(self, run_numbers, panels):

        self.startup("18_1")
        for run in run_numbers:
            self.datafile = self.get_file_name(run, "raw")
            for panel in panels:
                wout = self.process_run(run, panel, "raw", "11_4", absorb=True, number_density=0.025, scattering=5.463,
                                        attenuation=2.595, height=4.0, radius=0.55)
                mantid.ConvertUnits(InputWorkspace=wout, OutputWorkspace="{}-d".format(wout), Target="dSpacing",
                                    EMode="Elastic")
            if 0 in panels:
                panels = [1, 2, 3, 4, 5]
            else:
                panels = [x for x in panels if x < 6]
            for panel in panels:
                self.save_combined_panel(run, panel)

    def save_combined_panel(self, run, panel):
        panel_combination = {
            5: 6,
            4: 7,
            3: 8,
            2: 9,
            1: 10
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


def absorption_corrections(attenuation, height, number_density, radius, scattering, w):
    mantid.ConvertUnits(InputWorkspace=w, OutputWorkspace=w, Target="Wavelength", EMode="Elastic")
    mantid.CylinderAbsorption(InputWorkspace=w, OutputWorkspace="absorptionWS",
                              CylinderSampleHeight=height, CylinderSampleRadius=radius,
                              AttenuationXSection=attenuation, ScatteringXSection=scattering,
                              SampleNumberDensity=number_density, NumberOfSlices="10", NumberOfAnnuli="10",
                              NumberOfWavelengthPoints="25", ExpMethod="Normal")
    mantid.Divide(LHSWorkspace=w, RHSWorkspace="absorptionWS", OutputWorkspace=w)
    mantid.DeleteWorkspace("absorptionWS")
    mantid.ConvertUnits(InputWorkspace=w, OutputWorkspace=w, Target="TOF", EMode="Elastic")


def split(focus):
    for i in range(Wish.NUM_PANELS):
        out = "{0}-{1}foc".format(focus[0:len(focus) - 3], i+1)
        mantid.ExtractSingleSpectrum(InputWorkspace=focus, OutputWorkspace=out, WorkspaceIndex=i)
        mantid.DeleteWorkspace(focus)


def split_string(t):
    indxp = 0
    for i in range(0, len(t)):
        if t[i] == "+":
            indxp = i
    if indxp != 0:
        return int(t[0:indxp]), int(t[indxp + 1:len(t)])


def split_string_event(input_string):
    # this assumes the form nxs_event_label_tmin_tmax
    section = input_string.split('_')
    label = section[2]
    t_min = section[3]
    t_max = section[4]
    return label, t_min, t_max


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
