# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=invalid-name,no-init,too-many-lines
from __future__ import (absolute_import, division, print_function)
from mantid.kernel import Direction, FloatArrayProperty, IntArrayBoundedValidator, \
    IntArrayProperty, StringListValidator
from mantid.api import AlgorithmFactory, DataProcessorAlgorithm, FileAction, FileProperty, \
    MultipleFileProperty, Progress, PropertyMode, WorkspaceProperty
from mantid.simpleapi import AlignAndFocusPowder, CloneWorkspace, \
    ConvertUnits, CreateGroupingWorkspace, DeleteWorkspace, Divide, EditInstrumentGeometry, \
    GetIPTS, Load, LoadDiffCal, LoadEventNexus, LoadMask, LoadIsawDetCal, LoadNexusProcessed, \
    NormaliseByCurrent, PreprocessDetectorsToMD, Rebin, ReplaceSpecialValues, SaveAscii, \
    SaveFocusedXYE, SaveGSS, SaveNexusProcessed, mtd
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
                                 ['Convert Units', 'Calibration File', 'DetCal File']),
                             direction=Direction.Input,
                             doc="The type of conversion to d_spacing to be used.")

        self.declareProperty(FileProperty(name="CalibrationFilename", defaultValue="",
                                          extensions=['.h5', '.cal'],
                                          direction=Direction.Input,
                                          action=FileAction.OptionalLoad),
                             doc="The calibration file to convert to d_spacing.")

        self.declareProperty(MultipleFileProperty(name='DetCalFilename',
                                                  extensions=['.detcal'], action=FileAction.OptionalLoad),
                             'ISAW DetCal file')

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
            raise ValueError("Masking value \"%s\" not supported" % masking)

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
            raise ValueError("Normalization value \"%s\" not supported" % normalization)

        # cross check method of converting to d-spacing
        calibration = self.getProperty('Calibration').value
        if calibration == 'Convert Units':
            pass
        elif calibration == 'Calibration File':
            filename = self.getProperty('CalibrationFilename').value
            if len(filename) <= 0:
                issues['CalibrationFilename'] \
                    = "Calibration=\"%s\" requires a filename" % calibration
        elif calibration == 'DetCal File':
            filenames = self.getProperty('DetCalFilename').value
            if len(filenames) <= 0:
                issues['DetCalFilename'] \
                    = "Calibration=\"%s\" requires a filename" % calibration
            if len(filenames) > 2:
                issues['DetCalFilename'] \
                    = "Calibration=\"%s\" requires one or two filenames" % calibration
        else:
            raise ValueError("Calibration value \"%s\" not supported" % calibration)

        return issues

    def _getMaskWSname(self):
        masking = self.getProperty("Masking").value
        maskWSname = None
        maskFile = None

        # none and workspace are special
        if masking == 'None':
            pass
        elif masking == "Masking Workspace":
            maskWSname = str(self.getProperty("MaskingWorkspace").value)

        # deal with files
        elif masking == 'Custom - xml masking file':
            maskWSname = 'CustomMask'
            maskFile = self.getProperty('MaskingFilename').value
        # TODO not reading the correct mask file geometry
        elif masking == 'Horizontal' or masking == 'Vertical':
            maskWSname = masking + 'Mask' # append the work 'Mask' for the wksp name
            if not mtd.doesExist(maskWSname): # only load if it isn't already loaded
                maskFile = '/SNS/SNAP/shared/libs/%s_Mask.xml' % masking

        if maskFile is not None:
            LoadMask(InputFile=maskFile, Instrument='SNAP', OutputWorkspace=maskWSname)

        if maskWSname is None:
            maskWSname = ''
        return maskWSname

    def _generateGrouping(self, runnumber, metaWS, progress):
        group_to_real = {'Banks': 'Group', 'Modules': 'bank', '2_4 Grouping': '2_4Grouping'}
        group = self.getProperty('GroupDetectorsBy').value
        real_name = group_to_real.get(group, group)

        if not mtd.doesExist(group):
            if group == '2_4 Grouping':
                group = '2_4_Grouping'

            if metaWS is None:
                metaWS = self._loadMetaWS(runnumber)
            CreateGroupingWorkspace(InputWorkspace=metaWS, GroupDetectorsBy=real_name,
                                    OutputWorkspace=group)
            progress.report('create grouping')
        else:
            progress.report()

        return group

    def _generateNormalization(self, WS, normType, normWS):
        if normType == 'None':
            return None
        elif normType == "Extracted from Data":
            window = self.getProperty("PeakClippingWindowSize").value

            smooth_range = self.getProperty("SmoothingRange").value

            peak_clip_WS = str(WS).replace('_red', '_normalizer')
            peak_clip_WS = CloneWorkspace(InputWorkspace=WS, OutputWorkspace=peak_clip_WS)
            n_histo = peak_clip_WS.getNumberHistograms()

            for h in range(n_histo):
                peak_clip_WS.setY(h, self.peak_clip(peak_clip_WS.readY(h), win=window, decrese=True,
                                                    LLS=True, smooth_window=smooth_range))
            return str(peak_clip_WS)
        else: # other values are already held in normWS
            return normWS

    def _save(self, saveDir, basename, outputWksp):
        if not self.getProperty("SaveData").value:
            return

        self.log().notice('Writing to \'' + saveDir + '\'')

        SaveNexusProcessed(InputWorkspace=outputWksp,
                           Filename=os.path.join(saveDir, 'nexus', basename + '.nxs'))
        SaveAscii(InputWorkspace=outputWksp,
                  Filename=os.path.join(saveDir, 'd_spacing', basename + '.dat'))
        ConvertUnits(InputWorkspace=outputWksp, OutputWorkspace='WS_tof',
                     Target="TOF", AlignBins=False)

        # GSAS and FullProf require data in time-of-flight
        SaveGSS(InputWorkspace='WS_tof',
                Filename=os.path.join(saveDir, 'gsas', basename + '.gsa'),
                Format='SLOG', SplitFiles=False, Append=False, ExtendedHeader=True)
        SaveFocusedXYE(InputWorkspace='WS_tof',
                       Filename=os.path.join(
                           saveDir, 'fullprof', basename + '.dat'),
                       SplitFiles=True, Append=False)
        DeleteWorkspace(Workspace='WS_tof')

    def _loadMetaWS(self, runnumber):
        # currently only event nexus files are supported
        wsname = '__meta_SNAP_{}'.format(runnumber)
        LoadEventNexus(Filename='SNAP' + str(runnumber), OutputWorkspace=wsname,
                       MetaDataOnly=True, LoadLogs=False)
        return wsname

    def PyExec(self):
        # Retrieve all relevant notice

        in_Runs = self.getProperty("RunNumbers").value

        maskWSname = self._getMaskWSname()

        progress = Progress(self, 0., .25, 3)

        # default arguments for AlignAndFocusPowder
        alignAndFocusArgs={'TMax':50000,
                           'RemovePromptPulseWidth':1600,
                           'PreserveEvents':False,
                           'Dspacing':True,  # binning parameters in d-space
                           'Params':self.getProperty("Binning").value}

        # workspace for loading metadata only to be used in LoadDiffCal and
        # CreateGroupingWorkspace
        metaWS = None

        # either type of file-based calibration is stored in the same variable
        calib = self.getProperty("Calibration").value
        detcalFile = None
        if calib == "Calibration File":
            metaWS = self._loadMetaWS(in_Runs[0])
            LoadDiffCal(Filename=self.getPropertyValue("CalibrationFilename"),
                        WorkspaceName='SNAP',
                        InputWorkspace=metaWS,
                        MakeGroupingWorkspace=False, MakeMaskWorkspace=False)
            alignAndFocusArgs['CalibrationWorkspace'] = 'SNAP_cal'
        elif calib == 'DetCal File':
            detcalFile = ','.join(self.getProperty('DetCalFilename').value)
        progress.report('loaded calibration')

        norm = self.getProperty("Normalization").value

        if norm == "From Processed Nexus":
            norm_File = self.getProperty("NormalizationFilename").value
            normalizationWS = 'normWS'
            LoadNexusProcessed(Filename=norm_File, OutputWorkspace=normalizationWS)
            progress.report('loaded normalization')
        elif norm == "From Workspace":
            normalizationWS = str(self.getProperty("NormalizationWorkspace").value)
            progress.report('')
        else:
            normalizationWS = None
            progress.report('')

        group = self._generateGrouping(in_Runs[0], metaWS, progress)

        if metaWS is not None:
            DeleteWorkspace(Workspace=metaWS)

        Process_Mode = self.getProperty("ProcessingMode").value

        prefix = self.getProperty("OptionalPrefix").value

        # --------------------------- REDUCE DATA -----------------------------

        Tag = 'SNAP'
        if self.getProperty("LiveData").value:
            Tag = 'Live'

        progStart = .25
        progDelta = (1.-progStart)/len(in_Runs)
        for runnumber in in_Runs:
            self.log().notice("processing run %s" % runnumber)
            self.log().information(str(self.get_IPTS_Local(runnumber)))

            # put together output names
            new_Tag = Tag
            if len(prefix) > 0:
                new_Tag += '_' + prefix
            basename = '%s_%s_%s' % (new_Tag, runnumber, group)

            if self.getProperty("LiveData").value:
                raise RuntimeError('Live data is not currently supported')
            else:
                Load(Filename='SNAP' + str(runnumber), OutputWorkspace=basename + '_red', startProgress=progStart,
                     endProgress=progStart + .25 * progDelta)
                progStart += .25 * progDelta
            redWS = basename + '_red'

            # overwrite geometry with detcal files
            if calib == 'DetCal File':
                LoadIsawDetCal(InputWorkspace=redWS, Filename=detcalFile)

            # create unfocussed data if in set-up mode
            if Process_Mode == "Set-Up":
                unfocussedWksp = '{}_{}_d'.format(new_Tag, runnumber)
            else:
                unfocussedWksp = ''

            AlignAndFocusPowder(InputWorkspace=redWS, OutputWorkspace=redWS,
                                MaskWorkspace=maskWSname,  # can be empty string
                                GroupingWorkspace=group,
                                UnfocussedWorkspace=unfocussedWksp,  # can be empty string
                                startProgress = progStart,
                                endProgress = progStart + .5 * progDelta,
                                **alignAndFocusArgs)
            progStart += .5 * progDelta

            # the rest takes up .25 percent of the run processing
            progress = Progress(self, progStart, progStart+.25*progDelta, 2)

            # AlignAndFocusPowder leaves the data in time-of-flight
            ConvertUnits(InputWorkspace=redWS, OutputWorkspace=redWS, Target='dSpacing', EMode='Elastic')

            # Edit instrument geometry to make final workspace smaller on disk
            det_table = PreprocessDetectorsToMD(Inputworkspace=redWS,
                                                OutputWorkspace='__SNAP_det_table')
            polar = np.degrees(det_table.column('TwoTheta'))
            azi = np.degrees(det_table.column('Azimuthal'))
            EditInstrumentGeometry(Workspace=redWS, L2=det_table.column('L2'),
                                   Polar=polar, Azimuthal=azi)
            mtd.remove('__SNAP_det_table')
            progress.report('simplify geometry')

            # AlignAndFocus doesn't necessarily rebin the data correctly
            if Process_Mode == "Set-Up":
                Rebin(InputWorkspace=unfocussedWksp, Params=alignAndFocusArgs['Params'],
                      Outputworkspace=unfocussedWksp)

            NormaliseByCurrent(InputWorkspace=redWS, OutputWorkspace=redWS)

            # normalize the data as requested
            normalizationWS = self._generateNormalization(redWS, norm, normalizationWS)
            normalizedWS = None
            if normalizationWS is not None:
                normalizedWS = basename + '_nor'
                Divide(LHSWorkspace=redWS, RHSWorkspace=normalizationWS,
                       OutputWorkspace=normalizedWS)
                ReplaceSpecialValues(Inputworkspace=normalizedWS,
                                     OutputWorkspace=normalizedWS,
                                     NaNValue='0', NaNError='0',
                                     InfinityValue='0', InfinityError='0')
                progress.report('normalized')
            else:
                progress.report()

            # rename everything as appropriate and determine output workspace name
            if normalizedWS is None:
                outputWksp = redWS
            else:
                outputWksp = normalizedWS

                if norm == "Extracted from Data" and Process_Mode == "Production":
                        DeleteWorkspace(Workspace=redWS)
                        DeleteWorkspace(Workspace=normalizationWS)

            # Save requested formats
            saveDir = self.getPropertyValue("OutputDirectory").strip()
            if len(saveDir) <= 0:
                self.log().notice('Using default save location')
                saveDir = os.path.join(self.get_IPTS_Local(runnumber), 'shared', 'data')
            self._save(saveDir, basename, outputWksp)

            # set workspace as an output so it gets history
            propertyName = 'OutputWorkspace_'+str(outputWksp)
            self.declareProperty(WorkspaceProperty(
                propertyName, outputWksp, Direction.Output))
            self.setProperty(propertyName, outputWksp)

            # declare some things as extra outputs in set-up
            if Process_Mode != "Production":  # TODO set them as workspace properties
                propNames = ['OuputWorkspace_'+str(it) for it in ['d', 'norm', 'normalizer']]
                wkspNames = ['%s_%s_d' % (new_Tag, runnumber), basename + '_red', '%s_%s_normalizer' % (new_Tag, runnumber)]
                for (propName, wkspName) in zip(propNames, wkspNames):
                    if mtd.doesExist(wkspName):
                        self.declareProperty(WorkspaceProperty(propName, wkspName, Direction.Output))
                        self.setProperty(propName, wkspName)


AlgorithmFactory.subscribe(SNAPReduce)
