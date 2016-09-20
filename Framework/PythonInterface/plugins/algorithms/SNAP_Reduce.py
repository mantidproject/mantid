"""*WIKI*

The purpose of this algorithm is to do a full reduction of SNAP
data. This alloWS several runs, and with all the typical options that
are usually used at the beamline, including calibrate from a cal file
and from Convert Units, mask from file workspace and default masks,
several groupings and save in GSAS or Fullprof format

*WIKI*

"""

from mantid.kernel import *
from mantid.api import *
from mantid.simpleapi import *
import os
import numpy as np


class SNAP_Reduce(PythonAlgorithm):
    IPTS_dir = None

    def get_IPTS_Local(self, run):
        if self.IPTS_dir is None:
            self.IPTS_dir = GetIPTS(Instrument='SNAP',
                                    RunNumber=str(run))
        return self.IPTS_dir

    def get_Run_Log_Local(self, run):

        runs = range(run-5, run+1)
        file_Str = ''

        while len(runs) > 0:
            try:
                r = runs.pop()
                self.log().notice('\n run is : %s' % r)
                file_Str = FileFinder.findRuns(str(r))[0]
                break
            except RuntimeError:
                pass

        if file_Str == '':
            self.log().error('The SNAP run %s is not found in the server - '
                             + 'cannot find current IPTS, Choose another '
                             + 'run' % run)
            return False
        else:
            self.log().notice('File is: %s' % file_Str)
            LogRun = int(file_Str.split('_')[1])
            self.log().notice('Log Run is: %s' % LogRun)
            return LogRun

    def smooth(self, data, order):
        # This smooths data based on linear weigthed average around
        # point i for example for an order of 7 the i point is
        # weighted 4, i=/- 1 weighted 3, i+/-2 weighted 2 and i+/-3
        # weighted 1 this input is only the y values
        sm = np.zeros(len(data))
        factor = order/2+1

        for i in xrange(len(data)):
            temp = 0
            ave = 0
            for r in range(max(0, i-order/2), min(i+order/2, len(data)-1)+1):
                temp = temp+(factor-abs(r-i))*data[r]
                ave = ave + factor - abs(r-i)
            sm[i] = temp/ave

        return sm

    def LLS_transformation(self, input):
        # this transforms data to be more sensitive to weak peaks. The
        # function is reversed by the Inv_LLS function below
        out = np.log(np.log((input+1)**0.5+1)+1)

        return out

    def Inv_LLS_transformation(self, input):
        # See Function LLS function above
        out = (np.exp(np.exp(input)-1)-1)**2-1

        return out

    def peak_clip(self, data, win=30, decrese=True, LLS=True, smooth_window=0):

        start_data = np.copy(data)

        window = win
        print smooth_window

        if smooth_window > 0:
            data = self.smooth(data, smooth_window)

        if LLS:
            data = self.LLS_transformation(data)

        temp = data.copy()

        if decrese:
            scan = range(window+1, 0, -1)
        else:
            scan = range(1, window+1)

        for w in scan:
            for i in range(len(temp)):
                if i < w or i > (len(temp)-w-1):
                    continue
                else:
                    win_array = temp[i-w:i+w+1].copy()
                    win_array_reversed = win_array[::-1]
                    average = (win_array+win_array_reversed) / 2
                    temp[i] = np.min(average[:len(average) / 2])

        if LLS:
            temp = self.Inv_LLS_transformation(temp)

        print min(start_data-temp)

        index = np.where((start_data - temp) == min(start_data - temp))[0][0]

        output = temp * (start_data[index] / temp[index])

        return output

    def PyInit(self):

        validator = IntArrayBoundedValidator()
        validator.setLower(0)
        self.declareProperty(IntArrayProperty("RunNumbers", values=[0], direction=Direction.Input,
                                              validator=validator),
                             "Run numbers to process, comma separated")

        self.declareProperty("Live Data", False,
                             "Read live data - requires a saved run in the current IPTS "
                             + "with the same Instrument configuration as the live run")

        mask = ["None", "Horizontal", "Vertical", "Masking Workspace", "Custom - xml masking file"]
        self.declareProperty("Masking", "None", StringListValidator(mask),
                             "Mask to be applied to the data")

        self.declareProperty(WorkspaceProperty("Masking Workspace", "",
                                               Direction.Input, PropertyMode.Optional),
                             "The workspace containing the mask.")

        self.declareProperty(FileProperty(name="Masking Filename", defaultValue="",
                                          direction=Direction.Input,
                                          action=FileAction.OptionalLoad),
                             doc="The file containing the xml mask.")

        self.declareProperty(name="Calibration", defaultValue="Convert Units",
                             validator=StringListValidator(["Convert Units", "Calibration File"]),
                             direction=Direction.Input,
                             doc="The type of conversion to d_spacing to be used.")

        self.declareProperty(FileProperty(name="Calibration Filename", defaultValue="",
                                          direction=Direction.Input,
                                          action=FileAction.OptionalLoad),
                             doc="The calibration file to convert to d_spacing.")

        self.declareProperty(FloatArrayProperty("Binning", [0.5, -0.004, 7.0]),
                             "Min, Step, and Max of d-space bins.  Logarithmic binning is used if Step is negative.")

        nor_corr = ["None", "From Workspace", "From Processed Nexus", "Extracted from Data"]
        self.declareProperty("Normalization", "None", StringListValidator(nor_corr),
                             "If needed what type of input to use as normalization, Extracted from "
                             + "Data uses a background determination that is peak independent.This "
                             + "implemantation can be tested in algorithm SNAP Peak Clipping Background")

        self.declareProperty(FileProperty(name="Normalization Filename", defaultValue="",
                                          direction=Direction.Input,
                                          action=FileAction.OptionalLoad),
                             doc="The file containing the processed nexus for normalization.")

        self.declareProperty(WorkspaceProperty("Normalization Workspace", "",
                                               Direction.Input, PropertyMode.Optional),
                             "The workspace containing the normalization data.")

        self.declareProperty("Peak Clipping Window Size", 10,
                             "Read live data - requires a saved run in the current "
                             + "IPTS with the same Instrumnet configuration")

        self.declareProperty("Smoothing Range", 10,
                             "Read live data - requires a saved run in the "
                             + "current IPTS with the same Instrumnet configuration")

        grouping = ["All", "Column", "Banks", "Modules", "2_4 Grouping"]
        self.declareProperty("GroupDetectorsBy", "All", StringListValidator(grouping),
                             "Detector groups to use for future focussing: "
                             + "All detectors as one group, Groups (East,West for "
                             + "SNAP), Columns for SNAP, detector banks")

        mode = ["Set-Up", "Production"]
        self.declareProperty("Processing Mode", "Production", StringListValidator(mode),
                             "Set-Up Mode is used for establishing correct parameters. Production "
                             + "Mode only Normalized workspace is kept for each run.")

        self.declareProperty(name="Optional Prefix", defaultValue="",
                             direction=Direction.Input,
                             doc="Optional Prefix to be added to workspaces and output filenames")

        self.declareProperty("Save Data", False,
                             "Save data in the following formats: Ascii- d-spacing ,Nexus Processed,GSAS and Fullprof")

    def validateInputs(self):
        issues = dict()

        # cross check masking
        masking = self.getProperty("Masking").value
        if masking in ("None", "Horizontal", "Vertical"):
            pass
        elif masking in ("Custom - xml masking file"):
            filename = self.getProperty("Masking Filename").value
            if len(filename) <= 0:
                issues["Masking Filename"] = "Masking=\"%s\" requires a filename" % masking
        elif masking == "Masking Workspace":
            mask_workspace = self.getPropertyValue("Masking Workspace")
            if mask_workspace is None or len(mask_workspace) <= 0:
                issues["Masking Workspace"] = "Must supply masking workspace"
        else:
            raise RuntimeError("Masking value \"%s\" not supported" % masking)

        # cross check normalization
        normalization = self.getProperty("Normalization").value
        if normalization in ("None", "Extracted from Data"):
            pass
        elif normalization == "From Workspace":
            norm_workspace = self.getPropertyValue("Normalization Workspace")
        elif normalization == "From Processed Nexus":
            filename = self.getProperty("Normalization Filename").value
            if len(filename) <= 0:
                issues["Normalization Filename"] = "Normalization=\"%s\" requires a filename" \
                                                   % normalization
        else:
            raise RuntimeError("Normalization value \"%s\" not supported" % normalization)

        return issues

    def PyExec(self):
        # Retrieve all relevant notice

        in_Runs = self.getProperty("RunNumbers").value

        live = self.getProperty("Live Data").value

        masking = self.getProperty("Masking").value

        if masking == "Custom - xml masking file":
            LoadMask(InputFile=self.getProperty("Masking Filename").value,
                     Instrument='SNAP', OutputWorkspace='CustomMask')
        elif masking == "Horizontal":
            if not mtd.doesExist('HorizontalMask'):
                LoadMask(InputFile='/SNS/SNAP/shared/libs/Horizontal_Mask.xml',
                         Instrument='SNAP', OutputWorkspace='HorizontalMask')
        elif masking == "Vertical":
            if not mtd.doesExist('VerticalMask'):
                LoadMask(InputFile='/SNS/SNAP/shared/libs/Vertical_Mask.xml',
                         Instrument='SNAP', OutputWorkspace='VerticalMask')
        elif masking == "Masking Workspace":
            mask_workspace = self.getProperty("Masking Workspace").value

        calib = self.getProperty("Calibration").value
        if calib == "Calibration File":
            cal_File = self.getProperty("Calibration Filename").value

        params = self.getProperty("Binning").value
        norm = self.getProperty("Normalization").value

        if norm == "From Processed Nexus":
            norm_File = self.getProperty("Normalization filename").value
            Normalization = LoadNexusProcessed(Filename=norm_File)
        if norm == "From Workspace":
            normWS = self.getProperty("Normalization Workspace").value

        group = self.getProperty("GroupDetectorsBy").value

        if group == 'All':
            real_name = 'All'
        elif group == 'Column':
            real_name = 'Column'
        elif group == 'Banks':
            real_name = 'Group'
        elif group == 'Modules':
            real_name = 'bank'
        elif group == '2_4 Grouping':
            real_name = '2_4 Grouping'

        if not mtd.doesExist(group):
            if group == "2_4 Grouping":
                LoadDetectorsGroupingFile(InputFile=r'/SNS/SNAP/shared/libs/SNAP_group_2_4.xml',
                                          OutputWorkspace='2_4 Grouping')
            else:
                CreateGroupingWorkspace(InstrumentName='SNAP', GroupDetectorsBy=real_name,
                                        OutputWorkspace=group)

        Process_Mode = self.getProperty("Processing Mode").value

        prefix = self.getProperty("Optional Prefix").value

        save_Data = self.getProperty("Save Data").value

        # --------------------------- REDUCE DATA ------------------------------------------------

        Tag = 'SNAP'
        for r in in_Runs:
            self.log().notice("processing run %s" % r)
            print self.get_IPTS_Local(r)
            if live:
                Tag = 'Live'
                # TODO should remove branch and always use LoadPreNexusLive
                if AlgorithmFactory.exists('LoadPreNexusLive'):
                    WS = LoadPreNexusLive(Instrument='SNAP')
                else:
                    WS = LoadEventPreNexus(EventFilename=r'/SNS/SNAP/shared/live/SNAP_%s_live_neutron_event.dat' % r,
                                           PulseidFilename=r'/SNS/SNAP/shared/live/SNAP_%s_live_pulseid.dat' % r)
                    print 'going normalize'
                    WS = NormaliseByCurrent(InputWorkspace='WS', Outputworkspace='WS')
                    print 'going loadlogs'
                    LoadNexusLogs(Workspace=WS,
                                  Filename='%s/data/SNAP_%s_event.nxs' % (self.get_IPTS_Local(r),
                                                                          self.get_Run_Log_Local(r)))
                    LoadInstrument(Workspace=WS, InstrumentName='SNAP')
                    WS = FilterByXValue(InputWorkspace=WS, XMin=1)
            else:
                WS = Load(Filename='SNAP'+str(r), Outputworkspace='WS')
                WS = NormaliseByCurrent(InputWorkspace=WS, Outputworkspace='WS')

            WS = CompressEvents(InputWorkspace=WS, Outputworkspace='WS')
            WS = CropWorkspace(InputWorkspace='WS', OutputWorkspace='WS', XMax=50000)
            WS = RemovePromptPulse(InputWorkspace=WS, OutputWorkspace='WS',
                                   Width='1600', Frequency='60.4')

            if masking == "Custom - xml masking file":
                WS = MaskDetectors(Workspace=WS, MaskedWorkspace='CustomMask')
            elif masking == "Masking Workspace":
                WS = MaskDetectors(Workspace=WS, MaskedWorkspace=mask_workspace)
            elif masking == "Vertical":
                WS = MaskDetectors(Workspace=WS, MaskedWorkspace='VerticalMask')
            elif masking == "Horizontal":
                WS = MaskDetectors(Workspace=WS, MaskedWorkspace='HorizontalMask')

            if calib == "Convert Units":
                WS_d = ConvertUnits(InputWorkspace='WS', Target='dSpacing', Outputworkspace='WS_d')

            else:
                self.log().notice("\n calibration file : %s" % cal_File)
                WS_d = AlignDetectors(InputWorkspace='WS', CalibrationFile=cal_File, Outputworkspace='WS_d')

            WS_d = Rebin(InputWorkspace=WS_d, Params=params, Outputworkspace='WS_d')

            if group == "Low Res 5:6-high Res 1:4":
                WS_red = GroupDetectors(InputWorkspace=WS_d,
                                        MapFile=r'/SNS/SNAP/shared/libs/14_56_grouping.xml',
                                        PreserveEvents=False)
            else:
                WS_red = DiffractionFocussing(InputWorkspace=WS_d, GroupingWorkspace=group,
                                              PreserveEvents=False)

            if norm == "From Processed Nexus":
                WS_nor = Divide(LHSWorkspace=WS_red, RHSWorkspace=Normalization)
            elif norm == "From Workspace":
                WS_nor = Divide(LHSWorkspace=WS_red, RHSWorkspace=normWS)
            elif norm == "Extracted from Data":

                window = self.getProperty("Peak Clipping Window Size").value

                smooth_range = self.getProperty("Smoothing Range").value

                peak_clip_WS = CloneWorkspace(WS_red)
                n_histo = peak_clip_WS.getNumberHistograms()

                x = peak_clip_WS.extractX()
                y = peak_clip_WS.extractY()
                e = peak_clip_WS.extractE()

                for h in range(n_histo):
                    peak_clip_WS.setX(h, x[h])
                    peak_clip_WS.setY(h, self.peak_clip(y[h], win=window, decrese=True,
                                                        LLS=True, smooth_window=smooth_range))
                    peak_clip_WS.setE(h, e[h])

                WS_nor = Divide(LHSWorkspace=WS_red, RHSWorkspace=peak_clip_WS)

            if norm != 'None':
                WS_nor = ReplaceSpecialValues(Inputworkspace=WS_nor,
                                              NaNValue='0', NaNError='0',
                                              InfinityValue='0', InfinityError='0')

            new_Tag = Tag + '_' + prefix

            if save_Data:
                if norm == 'None':
                    WS_tof = ConvertUnits(InputWorkspace="WS_red", Target="TOF", AlignBins=False)
                else:
                    WS_tof = ConvertUnits(InputWorkspace="WS_nor", Target="TOF", AlignBins=False)

                if norm == 'None':
                    SaveNexusProcessed(InputWorkspace=WS_red,
                                       Filename='%s/shared/data/nexus/%s_%s_%s.nxs' % (self.get_IPTS_Local(r), new_Tag, r, group))
                    SaveAscii(InputWorkspace=WS_red,
                              Filename='%s/shared/data/d_spacing/%s_%s_%s.dat' % (self.get_IPTS_Local(r), new_Tag, r, group))
                else:
                    SaveNexusProcessed(InputWorkspace=WS_nor,
                                       Filename='%s/shared/data/nexus/%s_%s_%s.nxs' % (self.get_IPTS_Local(r), new_Tag,  r, group))
                    SaveAscii(InputWorkspace=WS_nor,
                              Filename='%s/shared/data/d_spacing/%s_%s_%s.dat' % (self.get_IPTS_Local(r), new_Tag, r, group))

                SaveGSS(InputWorkspace=WS_tof,
                        Filename='%s/shared/data/gsas/%s_%s_%s.gsa' % (self.get_IPTS_Local(r), new_Tag, r, group),
                        Format='SLOG', SplitFiles=False, Append=False, ExtendedHeader=True)
                SaveFocusedXYE(InputWorkspace=WS_tof,
                               Filename='%s/shared/data/fullprof/%s_%s_%s.dat' % (self.get_IPTS_Local(r), new_Tag, r, group),
                               SplitFiles=True, Append=False)
                DeleteWorkspace(Workspace='WS_tof')

            DeleteWorkspace(Workspace='WS')

            if Process_Mode == "Production":
                DeleteWorkspace(Workspace='WS_d')
                if norm != "None":
                    DeleteWorkspace(Workspace='WS_red')
                    RenameWorkspace(InputWorkspace=WS_nor,
                                    OutputWorkspace='%s_%s_%s_nor' % (new_Tag, r, group))
                else:
                    RenameWorkspace(InputWorkspace=WS_red,
                                    OutputWorkspace='%s_%s_%s_red' % (new_Tag, r, group))
                if norm == "Extracted from Data":
                    DeleteWorkspace(Workspace='peak_clip_WS')

            else:

                RenameWorkspace(Inputworkspace='WS_d',
                                OutputWorkspace='%s_%s_d' % (new_Tag, r))
                RenameWorkspace(Inputworkspace='WS_red',
                                OutputWorkspace='%s_%s_%s_red' % (new_Tag, r, group))
                if norm != "None":
                    RenameWorkspace(Inputworkspace='WS_nor',
                                    OutputWorkspace='%s_%s_%s_nor' % (new_Tag, r, group))
                if norm == "Extracted from Data":
                    RenameWorkspace(Inputworkspace='peak_clip_WS',
                                    OutputWorkspace='%s_%s_normalizer' % (new_Tag, r))


AlgorithmFactory.subscribe(SNAP_Reduce)
