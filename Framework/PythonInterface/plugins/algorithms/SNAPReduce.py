# pylint: disable=invalid-name,no-init,too-many-lines
from __future__ import (absolute_import, division, print_function)
from mantid.kernel import Direction, FloatArrayProperty, IntArrayBoundedValidator, \
    IntArrayProperty, StringListValidator
from mantid.api import AlgorithmFactory, DataProcessorAlgorithm, FileAction, FileProperty, \
    PropertyMode, WorkspaceProperty
from mantid.simpleapi import AlignDetectors, CloneWorkspace, CompressEvents, \
    ConvertUnits, CreateGroupingWorkspace, CropWorkspace, DeleteWorkspace, DiffractionFocussing, \
    Divide, EditInstrumentGeometry, GetIPTS, Load, LoadDetectorsGroupingFile, LoadMask, \
    LoadNexusProcessed, LoadPreNexusLive, MaskDetectors, NormaliseByCurrent, \
    PreprocessDetectorsToMD, Rebin, RenameWorkspace, ReplaceSpecialValues, RemovePromptPulse, \
    SaveAscii, SaveFocusedXYE, SaveGSS, SaveNexusProcessed, mtd
import os
import numpy as np


class SNAPReduce(DataProcessorAlgorithm):
    IPTS_dir = None

    def get_IPTS_Local(self, run):
        if self.IPTS_dir is None:
            self.IPTS_dir = GetIPTS(Instrument='SNAP',
                                    RunNumber=str(run))
        return self.IPTS_dir

    def smooth(self, data, order):
        # This smooths data based on linear weigthed average around
        # point i for example for an order of 7 the i point is
        # weighted 4, i=/- 1 weighted 3, i+/-2 weighted 2 and i+/-3
        # weighted 1 this input is only the y values
        sm = np.zeros(len(data))
        factor = order / 2 + 1

        for i in range(len(data)):
            temp = 0
            ave = 0
            for r in range(max(0, i - int(order / 2)),
                           min(i + int(order / 2), len(data) - 1) + 1):
                temp = temp + (factor - abs(r - i)) * data[r]
                ave = ave + factor - abs(r - i)
            sm[i] = temp / ave

        return sm

    def LLS_transformation(self, input):
        # this transforms data to be more sensitive to weak peaks. The
        # function is reversed by the Inv_LLS function below
        out = np.log(np.log((input + 1)**0.5 + 1) + 1)

        return out

    def Inv_LLS_transformation(self, input):
        # See Function LLS function above
        out = (np.exp(np.exp(input) - 1) - 1)**2 - 1

        return out

    def peak_clip(self, data, win=30, decrese=True, LLS=True, smooth_window=0):
        start_data = np.copy(data)

        window = win
        self.log().information(str(smooth_window))

        if smooth_window > 0:
            data = self.smooth(data, smooth_window)

        if LLS:
            data = self.LLS_transformation(data)

        temp = data.copy()

        if decrese:
            scan = list(range(window + 1, 0, -1))
        else:
            scan = list(range(1, window + 1))

        for w in scan:
            for i in range(len(temp)):
                if i < w or i > (len(temp) - w - 1):
                    continue
                else:
                    win_array = temp[i - w:i + w + 1].copy()
                    win_array_reversed = win_array[::-1]
                    average = (win_array + win_array_reversed) / 2
                    temp[i] = np.min(average[:len(average) / 2])

        if LLS:
            temp = self.Inv_LLS_transformation(temp)

        self.log().information(str(min(start_data - temp)))

        index = np.where((start_data - temp) == min(start_data - temp))[0][0]

        output = temp * (start_data[index] / temp[index])

        return output

    def category(self):
        return "Diffraction\\Reduction"

    def PyInit(self):

        validator = IntArrayBoundedValidator()
        validator.setLower(0)
        self.declareProperty(IntArrayProperty("RunNumbers", values=[0], direction=Direction.Input,
                                              validator=validator),
                             "Run numbers to process, comma separated")

        self.declareProperty("LiveData", False,
                             "Read live data - requires a saved run in the current IPTS "
                             + "with the same Instrument configuration as the live run")

        mask = ["None", "Horizontal", "Vertical",
                "Masking Workspace", "Custom - xml masking file"]
        self.declareProperty("Masking", "None", StringListValidator(mask),
                             "Mask to be applied to the data")

        self.declareProperty(WorkspaceProperty("MaskingWorkspace", "",
                                               Direction.Input, PropertyMode.Optional),
                             "The workspace containing the mask.")

        self.declareProperty(FileProperty(name="MaskingFilename", defaultValue="",
                                          direction=Direction.Input,
                                          action=FileAction.OptionalLoad),
                             doc="The file containing the xml mask.")

        self.declareProperty(name="Calibration", defaultValue="Convert Units",
                             validator=StringListValidator(
                                 ["Convert Units", "Calibration File"]),
                             direction=Direction.Input,
                             doc="The type of conversion to d_spacing to be used.")

        self.declareProperty(FileProperty(name="CalibrationFilename", defaultValue="",
                                          direction=Direction.Input,
                                          action=FileAction.OptionalLoad),
                             doc="The calibration file to convert to d_spacing.")

        self.declareProperty(FloatArrayProperty("Binning", [0.5, -0.004, 7.0]),
                             "Min, Step, and Max of d-space bins.  Logarithmic binning is used if Step is negative.")

        nor_corr = ["None", "From Workspace",
                    "From Processed Nexus", "Extracted from Data"]
        self.declareProperty("Normalization", "None", StringListValidator(nor_corr),
                             "If needed what type of input to use as normalization, Extracted from "
                             + "Data uses a background determination that is peak independent.This "
                             + "implemantation can be tested in algorithm SNAP Peak Clipping Background")

        self.declareProperty(FileProperty(name="NormalizationFilename", defaultValue="",
                                          direction=Direction.Input,
                                          action=FileAction.OptionalLoad),
                             doc="The file containing the processed nexus for normalization.")

        self.declareProperty(WorkspaceProperty("NormalizationWorkspace", "",
                                               Direction.Input, PropertyMode.Optional),
                             "The workspace containing the normalization data.")

        self.declareProperty("PeakClippingWindowSize", 10,
                             "Read live data - requires a saved run in the current "
                             + "IPTS with the same Instrumnet configuration")

        self.declareProperty("SmoothingRange", 10,
                             "Read live data - requires a saved run in the "
                             + "current IPTS with the same Instrumnet configuration")

        grouping = ["All", "Column", "Banks", "Modules", "2_4 Grouping"]
        self.declareProperty("GroupDetectorsBy", "All", StringListValidator(grouping),
                             "Detector groups to use for future focussing: "
                             + "All detectors as one group, Groups (East,West for "
                             + "SNAP), Columns for SNAP, detector banks")

        mode = ["Set-Up", "Production"]
        self.declareProperty("ProcessingMode", "Production", StringListValidator(mode),
                             "Set-Up Mode is used for establishing correct parameters. Production "
                             + "Mode only Normalized workspace is kept for each run.")

        self.declareProperty(name="OptionalPrefix", defaultValue="",
                             direction=Direction.Input,
                             doc="Optional Prefix to be added to workspaces and output filenames")

        self.declareProperty("SaveData", False,
                             "Save data in the following formats: Ascii- "
                             + "d-spacing ,Nexus Processed,GSAS and Fullprof")

        self.declareProperty(FileProperty(name="OutputDirectory", defaultValue="",
                                          action=FileAction.OptionalDirectory),
                             doc='Default value is proposal shared directory')

    def validateInputs(self):
        issues = dict()

        # cross check masking
        masking = self.getProperty("Masking").value
        if masking in ("None", "Horizontal", "Vertical"):
            pass
        elif masking in ("Custom - xml masking file"):
            filename = self.getProperty("MaskingFilename").value
            if len(filename) <= 0:
                issues[
                    "MaskingFilename"] = "Masking=\"%s\" requires a filename" % masking
        elif masking == "Masking Workspace":
            mask_workspace = self.getPropertyValue("MaskingWorkspace")
            if mask_workspace is None or len(mask_workspace) <= 0:
                issues["MaskingWorkspace"] = "Must supply masking workspace"
        else:
            raise RuntimeError("Masking value \"%s\" not supported" % masking)

        # cross check normalization
        normalization = self.getProperty("Normalization").value
        if normalization in ("None", "Extracted from Data"):
            pass
        elif normalization == "From Workspace":
            norm_workspace = self.getPropertyValue("NormalizationWorkspace")
            if norm_workspace is None:
                issues['NormalizationWorkspace'] = 'Cannot be unset'
        elif normalization == "From Processed Nexus":
            filename = self.getProperty("NormalizationFilename").value
            if len(filename) <= 0:
                issues["NormalizationFilename"] = "Normalization=\"%s\" requires a filename" \
                                                  % normalization
        else:
            raise RuntimeError(
                "Normalization value \"%s\" not supported" % normalization)

        return issues

    def _getMaskWSname(self):
        masking = self.getProperty("Masking").value
        maskWSname = None
        if masking == 'Custom - xml masking file':
            maskWSname = 'CustomMask'
            LoadMask(InputFile=self.getProperty('MaskingFilename').value,
                     Instrument='SNAP', OutputWorkspace=maskWSname)
        elif masking == 'Horizontal':
            maskWSname = 'HorizontalMask'
            if not mtd.doesExist('HorizontalMask'):
                LoadMask(InputFile='/SNS/SNAP/shared/libs/Horizontal_Mask.xml',
                         Instrument='SNAP', OutputWorkspace=maskWSname)
        elif masking == 'Vertical':
            maskWSname = 'VerticalMask'
            if not mtd.doesExist('VerticalMask'):
                LoadMask(InputFile='/SNS/SNAP/shared/libs/Vertical_Mask.xml',
                         Instrument='SNAP', OutputWorkspace=maskWSname)
        elif masking == "Masking Workspace":
            maskWSname = str(self.getProperty("MaskingWorkspace").value)

        return maskWSname

    def _generateNormalization(self, WS, normType, normWS):
        if normType == 'None':
            return None
        elif normType == "Extracted from Data":
            window = self.getProperty("PeakClippingWindowSize").value

            smooth_range = self.getProperty("SmoothingRange").value

            peak_clip_WS = CloneWorkspace(WS)
            n_histo = peak_clip_WS.getNumberHistograms()

            x = peak_clip_WS.extractX()
            y = peak_clip_WS.extractY()
            e = peak_clip_WS.extractE()

            for h in range(n_histo):
                peak_clip_WS.setX(h, x[h])
                peak_clip_WS.setY(h, self.peak_clip(y[h], win=window, decrese=True,
                                                    LLS=True, smooth_window=smooth_range))
                peak_clip_WS.setE(h, e[h])
            return peak_clip_WS
        else: # other values are already held in normWS
            return normWS

    def _save(self, runnumber, basename, norm):
        if not self.getProperty("SaveData").value:
            return

        saveDir = self.getProperty("OutputDirectory").value.strip()
        if len(saveDir) <= 0:
            self.log().notice('Using default save location')
            saveDir = os.path.join(
                self.get_IPTS_Local(runnumber), 'shared', 'data')
        self.log().notice('Writing to \'' + saveDir + '\'')

        if norm == 'None':
            SaveNexusProcessed(InputWorkspace='WS_red',
                               Filename=os.path.join(saveDir, 'nexus', basename + '.nxs'))
            SaveAscii(InputWorkspace='WS_red',
                      Filename=os.path.join(saveDir, 'd_spacing', basename + '.dat'))
            ConvertUnits(InputWorkspace='WS_red', OutputWorkspace='WS_tof',
                         Target="TOF", AlignBins=False)
        else:
            SaveNexusProcessed(InputWorkspace='WS_nor',
                               Filename=os.path.join(saveDir, 'nexus', basename + '.nxs'))
            SaveAscii(InputWorkspace='WS_nor',
                      Filename=os.path.join(saveDir, 'd_spacing', basename + '.dat'))
            ConvertUnits(InputWorkspace='WS_nor', OutputWorkspace='WS_tof',
                         Target="TOF", AlignBins=False)

        SaveGSS(InputWorkspace='WS_tof',
                Filename=os.path.join(saveDir, 'gsas', basename + '.gsa'),
                Format='SLOG', SplitFiles=False, Append=False, ExtendedHeader=True)
        SaveFocusedXYE(InputWorkspace='WS_tof',
                       Filename=os.path.join(
                           saveDir, 'fullprof', basename + '.dat'),
                       SplitFiles=True, Append=False)
        DeleteWorkspace(Workspace='WS_tof')

    def PyExec(self):
        # Retrieve all relevant notice

        in_Runs = self.getProperty("RunNumbers").value

        maskWSname = self._getMaskWSname()

        calib = self.getProperty("Calibration").value
        if calib == "Calibration File":
            cal_File = self.getProperty("CalibrationFilename").value

        params = self.getProperty("Binning").value
        norm = self.getProperty("Normalization").value

        if norm == "From Processed Nexus":
            norm_File = self.getProperty("Normalization filename").value
            normWS = LoadNexusProcessed(Filename=norm_File)
        elif norm == "From Workspace":
            normWS = self.getProperty("NormalizationWorkspace").value
        else:
            normWS = None

        group_to_real = {'Banks':'Group', 'Modules':'bank', '2_4 Grouping':'2_4_Grouping'}
        group = self.getProperty("GroupDetectorsBy").value
        real_name = group_to_real.get(group, group)

        if not mtd.doesExist(group):
            if group == "2_4 Grouping":
                group = real_name
                LoadDetectorsGroupingFile(InputFile=r'/SNS/SNAP/shared/libs/SNAP_group_2_4.xml',
                                          OutputWorkspace=group)
            else:
                CreateGroupingWorkspace(InstrumentName='SNAP', GroupDetectorsBy=real_name,
                                        OutputWorkspace=group)

        Process_Mode = self.getProperty("ProcessingMode").value

        prefix = self.getProperty("OptionalPrefix").value

        # --------------------------- REDUCE DATA -----------------------------

        Tag = 'SNAP'
        for r in in_Runs:
            self.log().notice("processing run %s" % r)
            self.log().information(str(self.get_IPTS_Local(r)))
            if self.getProperty("LiveData").value:
                Tag = 'Live'
                WS = LoadPreNexusLive(Instrument='SNAP')
            else:
                WS = Load(Filename='SNAP' + str(r), Outputworkspace='WS')
                WS = NormaliseByCurrent(InputWorkspace=WS,
                                        Outputworkspace='WS')

            WS = CompressEvents(InputWorkspace=WS, Outputworkspace='WS')
            WS = CropWorkspace(InputWorkspace='WS',
                               OutputWorkspace='WS', XMax=50000)
            WS = RemovePromptPulse(InputWorkspace=WS, OutputWorkspace='WS',
                                   Width='1600', Frequency='60.4')

            if maskWSname is not None:
                WS = MaskDetectors(Workspace=WS, MaskedWorkspace=maskWSname)

            if calib == "Convert Units":
                WS_d = ConvertUnits(InputWorkspace='WS',
                                    Target='dSpacing', Outputworkspace='WS_d')
            else:
                self.log().notice("\n calibration file : %s" % cal_File)
                WS_d = AlignDetectors(
                    InputWorkspace='WS', CalibrationFile=cal_File, Outputworkspace='WS_d')

            WS_d = Rebin(InputWorkspace=WS_d, Params=params,
                         Outputworkspace='WS_d')

            WS_red = DiffractionFocussing(InputWorkspace=WS_d, GroupingWorkspace=group,
                                          PreserveEvents=False)

            normWS = self._generateNormalization(WS_red, norm, normWS)
            WS_nor = None
            if normWS is not None:
                WS_nor = Divide(LHSWorkspace=WS_red, RHSWorkspace=normWS)
                WS_nor = ReplaceSpecialValues(Inputworkspace=WS_nor,
                                              NaNValue='0', NaNError='0',
                                              InfinityValue='0', InfinityError='0')

            new_Tag = Tag
            if len(prefix) > 0:
                new_Tag += '_' + prefix

            # Edit instrument geomety to make final workspace smaller on disk
            det_table = PreprocessDetectorsToMD(Inputworkspace='WS_red',
                                                OutputWorkspace='__SNAP_det_table')
            polar = np.degrees(det_table.column('TwoTheta'))
            azi = np.degrees(det_table.column('Azimuthal'))
            EditInstrumentGeometry(Workspace="WS_red", L2=det_table.column('L2'),
                                   Polar=polar, Azimuthal=azi)
            if WS_nor is not None:
                EditInstrumentGeometry(Workspace="WS_nor", L2=det_table.column('L2'),
                                       Polar=polar, Azimuthal=azi)
            mtd.remove('__SNAP_det_table')

            # Save requested formats
            basename = '%s_%s_%s' % (new_Tag, r, group)
            self._save(r, basename, norm)

            # temporary workspace no longer needed
            DeleteWorkspace(Workspace='WS')

            # rename everything as appropriate and determine output workspace name
            RenameWorkspace(Inputworkspace='WS_d',
                            OutputWorkspace='%s_%s_d' % (new_Tag, r))
            RenameWorkspace(Inputworkspace='WS_red',
                            OutputWorkspace=basename + '_red')
            if norm == 'None':
                outputWksp = basename + '_red'
            else:
                outputWksp = basename + '_nor'
                RenameWorkspace(Inputworkspace='WS_nor',
                                OutputWorkspace=basename + '_nor')
            if norm == "Extracted from Data":
                RenameWorkspace(Inputworkspace='peak_clip_WS',
                                OutputWorkspace='%s_%s_normalizer' % (new_Tag, r))

            # delte some things in production
            if Process_Mode == "Production":
                DeleteWorkspace(Workspace='%s_%s_d' % (new_Tag, r)) # was 'WS_d'

                if norm != "None":
                    DeleteWorkspace(Workspace=basename + '_red') # was 'WS_red'

                if norm == "Extracted from Data":
                    DeleteWorkspace(Workspace='%s_%s_normalizer' % (new_Tag, r)) # was 'peak_clip_WS'

            propertyName = 'OutputWorkspace_'+str(outputWksp)
            self.declareProperty(WorkspaceProperty(
                propertyName, outputWksp, Direction.Output))
            self.setProperty(propertyName, outputWksp)

AlgorithmFactory.subscribe(SNAPReduce)
