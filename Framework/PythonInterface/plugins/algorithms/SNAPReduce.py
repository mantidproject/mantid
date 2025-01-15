# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=invalid-name,no-init,too-many-lines

# local
from mantid.simpleapi import (
    AlignAndFocusPowder,
    AlignAndFocusPowderFromFiles,
    CloneWorkspace,
    ConvertUnits,
    CreateGroupingWorkspace,
    DeleteWorkspace,
    Divide,
    EditInstrumentGeometry,
    GetIPTS,
    Load,
    LoadDiffCal,
    LoadEventNexus,
    LoadMask,
    LoadIsawDetCal,
    LoadNexusProcessed,
    Minus,
    NormaliseByCurrent,
    PreprocessDetectorsToMD,
    Rebin,
    ReplaceSpecialValues,
    SaveAscii,
    SaveFocusedXYE,
    SaveGSS,
    SaveNexusProcessed,
    mtd,
)

# 3rd party
from mantid.api import (
    AlgorithmFactory,
    DataProcessorAlgorithm,
    FileAction,
    FileProperty,
    MultipleFileProperty,
    Progress,
    PropertyMode,
    WorkspaceProperty,
)
from mantid.kernel import (
    Direction,
    EnabledWhenProperty,
    FloatArrayProperty,
    IntArrayBoundedValidator,
    IntArrayProperty,
    IntBoundedValidator,
    Property,
    PropertyCriterion,
    StringListValidator,
)
from mantid.kernel import logger

# standard
from datetime import datetime
import json
from mantid.utils.path import run_file
import numpy as np
import os
from pathlib import Path


class SNAPReduce(DataProcessorAlgorithm):
    IPTS_dir = None

    def get_IPTS_Local(self, run):
        if self.IPTS_dir is None:
            self.IPTS_dir = GetIPTS(Instrument="SNAP", RunNumber=str(run))

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
            for r in range(max(0, i - int(order / 2)), min(i + int(order / 2), len(data) - 1) + 1):
                temp = temp + (factor - abs(r - i)) * data[r]
                ave = ave + factor - abs(r - i)
            sm[i] = temp / ave

        return sm

    def LLS_transformation(self, input):
        # this transforms data to be more sensitive to weak peaks. The
        # function is reversed by the Inv_LLS function below
        out = np.log(np.log((input + 1) ** 0.5 + 1) + 1)

        return out

    def Inv_LLS_transformation(self, input):
        # See Function LLS function above
        out = (np.exp(np.exp(input) - 1) - 1) ** 2 - 1

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
                    win_array = temp[i - w : i + w + 1].copy()
                    win_array_reversed = win_array[::-1]
                    average = (win_array + win_array_reversed) / 2
                    temp[i] = np.min(average[: int(len(average) / 2)])

        if LLS:
            temp = self.Inv_LLS_transformation(temp)

        self.log().information(str(min(start_data - temp)))

        index = np.where((start_data - temp) == min(start_data - temp))[0][0]

        output = temp * (start_data[index] / temp[index])

        return output

    def _exportWorkspace(self, propName, wkspName):
        if wkspName and mtd.doesExist(wkspName):
            if not self.existsProperty(propName):
                self.declareProperty(WorkspaceProperty(propName, wkspName, Direction.Output))
            self.log().debug('Exporting workspace through property "{}"={}'.format(propName, wkspName))
            self.setProperty(propName, wkspName)

    def category(self):
        return "Diffraction\\Reduction"

    def PyInit(self):
        validator = IntArrayBoundedValidator(lower=0)
        self.declareProperty(
            IntArrayProperty("RunNumbers", values=[0], direction=Direction.Input, validator=validator),
            "Run numbers to process, comma separated",
        )
        self.declareProperty("Background", Property.EMPTY_INT, doc="Background to subtract from each individual run")

        mask = ["None", "Horizontal", "Vertical", "Masking Workspace", "Custom - xml masking file"]
        self.declareProperty("Masking", "None", StringListValidator(mask), "Mask to be applied to the data")

        self.declareProperty(
            WorkspaceProperty("MaskingWorkspace", "", Direction.Input, PropertyMode.Optional), "The workspace containing the mask."
        )

        self.declareProperty(
            FileProperty(name="MaskingFilename", defaultValue="", direction=Direction.Input, action=FileAction.OptionalLoad),
            doc="The file containing the xml mask.",
        )

        self.declareProperty(
            name="Calibration",
            defaultValue="Convert Units",
            validator=StringListValidator(["Convert Units", "Calibration File", "DetCal File"]),
            direction=Direction.Input,
            doc="The type of conversion to d_spacing to be used.",
        )

        self.declareProperty(
            FileProperty(
                name="CalibrationFilename",
                defaultValue="",
                extensions=[".h5", ".cal"],
                direction=Direction.Input,
                action=FileAction.OptionalLoad,
            ),
            doc="The calibration file to convert to d_spacing.",
        )

        self.declareProperty(
            MultipleFileProperty(name="DetCalFilename", extensions=[".detcal"], action=FileAction.OptionalLoad), "ISAW DetCal file"
        )

        self.declareProperty(
            FloatArrayProperty("Binning", [0.5, -0.004, 7.0]),
            "Min, Step, and Max of d-space bins.  Logarithmic binning is used if Step is negative.",
        )

        nor_corr = ["None", "From Workspace", "From Processed Nexus", "Extracted from Data"]
        self.declareProperty(
            "Normalization",
            nor_corr[0],
            StringListValidator(nor_corr),
            "If needed what type of input to use as normalization, Extracted from "
            + "Data uses a background determination that is peak independent.This "
            + "implemantation can be tested in algorithm SNAP Peak Clipping Background",
        )

        self.declareProperty(
            FileProperty(name="NormalizationFilename", defaultValue="", direction=Direction.Input, action=FileAction.OptionalLoad),
            doc="The file containing the processed nexus for normalization.",
        )

        self.declareProperty(
            WorkspaceProperty("NormalizationWorkspace", "", Direction.Input, PropertyMode.Optional),
            "The workspace containing the normalization data.",
        )

        validator_peak_clipping = IntBoundedValidator(lower=4, upper=15)
        self.declareProperty(
            name="PeakClippingWindowSize",
            defaultValue=10,
            validator=validator_peak_clipping,
            doc="Read live data - requires a saved run in the current IPTS with the same instrument configuration",
        )

        validator_smoothing_range = IntBoundedValidator(lower=1, upper=20)
        self.declareProperty(
            name="SmoothingRange",
            defaultValue=10,
            validator=validator_smoothing_range,
            doc="Read live data - requires a saved run in the current IPTS with the same instrument configuration",
        )

        grouping = ["All", "Column", "Banks", "Modules", "2_4 Grouping"]
        self.declareProperty(
            "GroupDetectorsBy",
            grouping[0],
            StringListValidator(grouping),
            "Detector groups to use for future focussing: "
            + "All detectors as one group, Groups (East,West for "
            + "SNAP), Columns for SNAP, detector banks",
        )

        self.declareProperty(
            "MaxChunkSize", 16.0, "Specify maximum Gbytes of file to read in one chunk. Zero reads the whole file at once."
        )

        mode = ["Set-Up", "Production"]
        self.declareProperty(
            "ProcessingMode",
            mode[1],
            StringListValidator(mode),
            "Set-Up Mode is used for establishing correct parameters. Production " + "Mode only Normalized workspace is kept for each run.",
        )

        final_units = ["dSpacing", "MomentumTransfer", "Wavelength"]
        self.declareProperty(
            "FinalUnits", final_units[0], StringListValidator(final_units), "Units to convert the data to at the end of processing"
        )

        self.declareProperty(
            name="OptionalPrefix",
            defaultValue="",
            direction=Direction.Input,
            doc="Optional Prefix to be added to workspaces and output filenames",
        )

        self.declareProperty(
            "SaveData", False, "Save data in the following formats: Ascii- " + "d-spacing ,Nexus Processed,GSAS and Fullprof"
        )

        self.declareProperty(
            FileProperty(name="OutputDirectory", defaultValue="", action=FileAction.OptionalDirectory),
            doc="Default value is proposal shared directory",
        )
        #
        # Section for the Autoreduction Configurator
        #
        self.declareProperty(
            name="EnableConfigurator",
            defaultValue=False,
            direction=Direction.Input,
            doc="Do not reduce, just save the configuration file for autoreduction",
        )
        config_enabled = EnabledWhenProperty("EnableConfigurator", PropertyCriterion.IsNotDefault)
        self.declareProperty(
            FileProperty(name="ConfigSaveDir", defaultValue="", action=FileAction.OptionalDirectory),
            doc="Default directory is /SNS/IPTS-XXXX/shared/config where XXXX is the IPTS number of the first input run number",
        )
        self.setPropertySettings("ConfigSaveDir", config_enabled)
        property_names = ["EnableConfigurator", "ConfigSaveDir"]
        [self.setPropertyGroup(name, "Autoreduction Configurator") for name in property_names]

    def validateInputs(self):  # noqa: C901  ignore "too complex" warning
        issues = dict()

        def _check_file(property_name: str) -> None:
            r"""
            Checks the extension and existence of or or more files
            @param property_name : property whose value is the file(s)
            """
            file_names = self.getProperty(property_name).value  # could be one file path or a list of file paths
            if isinstance(file_names, str):  # it's only one file
                file_names = [
                    file_names,
                ]
            for file_name in file_names:
                if len(file_name) <= 0:
                    issues[property_name] = f"{property_name} requires a file"
                elif not Path(file_name).is_file():
                    issues[property_name] = f"{property_name} {file_name} not found"

        # Check files for RunNumbers exist
        for run_number in self.getProperty("RunNumbers").value:
            if run_file(run_number, instrument="SNAP") is None:
                issues["RunNumbers"] = f"Events file not found for run {run_number}"
                break

        # Check file for background run number exists, if background is passed on
        background_property = self.getProperty("Background")
        if not background_property.isDefault:
            run_number = background_property.value
            if run_file(run_number, instrument="SNAP") is None:
                issues["Background"] = f"Events file not found for run {run_number}"

        # cross check masking
        masking = self.getProperty("Masking").value
        if masking in ("None", "Horizontal", "Vertical"):
            pass
        elif masking in ("Custom - xml masking file"):
            _check_file("MaskingFilename")
        elif masking == "Masking Workspace":
            mask_workspace = self.getPropertyValue("MaskingWorkspace")
            if mask_workspace is None or len(mask_workspace) <= 0:
                issues["MaskingWorkspace"] = "Must supply masking workspace"
        else:
            raise ValueError('Masking value "%s" not supported' % masking)

        # Check calibration file exists if passed on
        cal_type_to_file = {"Calibration File": "CalibrationFilename", "DetCal File": "DetCalFilename"}
        calibration_type = self.getProperty("Calibration").value
        if calibration_type in cal_type_to_file:
            _check_file(cal_type_to_file.get(calibration_type))

        # Check binning low < x < high
        low, step, high = self.getProperty("Binning").value
        if low >= high:
            issues["Binning"] = "Binning triad must be Low, Step, High with Low < High"

        # cross check normalization
        normalization = self.getProperty("Normalization").value
        if normalization in ("None", "Extracted from Data"):
            pass
        elif normalization == "From Workspace":
            norm_workspace = self.getPropertyValue("NormalizationWorkspace")
            if norm_workspace is None:
                issues["NormalizationWorkspace"] = "Cannot be unset"
        elif normalization == "From Processed Nexus":
            _check_file("NormalizationFilename")
        else:
            raise ValueError('Normalization value "%s" not supported' % normalization)

        # cross check method of converting to d-spacing
        calibration = self.getProperty("Calibration").value
        if calibration == "Convert Units":
            pass
        elif calibration == "Calibration File":
            filename = self.getProperty("CalibrationFilename").value
            if len(filename) <= 0:
                issues["CalibrationFilename"] = 'Calibration="%s" requires a filename' % calibration
        elif calibration == "DetCal File":
            filenames = self.getProperty("DetCalFilename").value
            if len(filenames) <= 0:
                issues["DetCalFilename"] = 'Calibration="%s" requires a filename' % calibration
            if len(filenames) > 2:
                issues["DetCalFilename"] = 'Calibration="%s" requires one or two filenames' % calibration
        else:
            raise ValueError('Calibration value "%s" not supported' % calibration)

        # Check ConfigSaveDir directory
        dir_name = self.getProperty("ConfigSaveDir").value
        if len(dir_name) > 0 and not Path(dir_name).is_dir():
            issues["ConfigSaveDir"] = f"Directory {dir_name} not found"

        return issues

    def _getMaskWSname(self, runnumber, metaWS):
        masking = self.getProperty("Masking").value
        maskWSname = None
        maskFile = None

        # none and workspace are special
        if masking == "None":
            pass
        elif masking == "Masking Workspace":
            maskWSname = str(self.getProperty("MaskingWorkspace").value)

        # deal with files
        elif masking == "Custom - xml masking file":
            maskWSname = "CustomMask"
            maskFile = self.getProperty("MaskingFilename").value
        elif masking == "Horizontal" or masking == "Vertical":
            maskWSname = masking + "Mask"  # append the work 'Mask' for the wksp name
            if not mtd.doesExist(maskWSname):  # only load if it isn't already loaded
                maskFile = "/SNS/SNAP/shared/libs/%s_Mask.xml" % masking

        if maskFile is not None:
            if not metaWS:
                metaWS = self._loadMetaWS(runnumber)
            LoadMask(InputFile=maskFile, RefWorkspace=metaWS, Instrument="SNAP", OutputWorkspace=maskWSname)

        if maskWSname is None:
            maskWSname = ""
        return maskWSname

    def _generateGrouping(self, runnumber, metaWS, progress):
        group_to_real = {"Banks": "Group", "Modules": "bank", "2_4 Grouping": "2_4Grouping"}
        group = self.getProperty("GroupDetectorsBy").value
        real_name = group_to_real.get(group, group)

        if not mtd.doesExist(group):
            if group == "2_4 Grouping":
                group = "2_4_Grouping"

            if not metaWS:
                metaWS = self._loadMetaWS(runnumber)
            CreateGroupingWorkspace(InputWorkspace=metaWS, GroupDetectorsBy=real_name, OutputWorkspace=group)
            progress.report("create grouping")
        else:
            progress.report()

        return group

    def _generateNormalization(self, WS, normType, normWS):
        if normType == "None":
            return None
        elif normType == "Extracted from Data":
            window = self.getProperty("PeakClippingWindowSize").value

            smooth_range = self.getProperty("SmoothingRange").value

            peak_clip_WS = str(WS).replace("_red", "_normalizer")
            peak_clip_WS = CloneWorkspace(InputWorkspace=WS, OutputWorkspace=peak_clip_WS)
            n_histo = peak_clip_WS.getNumberHistograms()

            for h in range(n_histo):
                peak_clip_WS.setY(h, self.peak_clip(peak_clip_WS.readY(h), win=window, decrese=True, LLS=True, smooth_window=smooth_range))
            return str(peak_clip_WS)
        else:  # other values are already held in normWS
            return normWS

    def _save(self, runnumber, basename, outputWksp):
        if not self.getProperty("SaveData").value:
            return

        # determine where to save the data
        saveDir = self.getPropertyValue("OutputDirectory").strip()
        if len(saveDir) <= 0:
            self.log().notice("Using default save location")
            saveDir = os.path.join(self.get_IPTS_Local(runnumber), "shared", "data")

        self.log().notice("Writing to '" + saveDir + "'")

        SaveNexusProcessed(InputWorkspace=outputWksp, Filename=os.path.join(saveDir, "nexus", basename + ".nxs"))
        SaveAscii(InputWorkspace=outputWksp, Filename=os.path.join(saveDir, "d_spacing", basename + ".dat"))
        ConvertUnits(InputWorkspace=outputWksp, OutputWorkspace="WS_tof", Target="TOF", AlignBins=False)

        # GSAS and FullProf require data in time-of-flight
        SaveGSS(
            InputWorkspace="WS_tof",
            Filename=os.path.join(saveDir, "gsas", basename + ".gsa"),
            Format="SLOG",
            SplitFiles=False,
            Append=False,
            ExtendedHeader=True,
        )
        SaveFocusedXYE(
            InputWorkspace="WS_tof", Filename=os.path.join(saveDir, "fullprof", basename + ".dat"), SplitFiles=True, Append=False
        )
        DeleteWorkspace(Workspace="WS_tof")

    def _loadMetaWS(self, runnumber):
        # currently only event nexus files are supported
        wsname = "__meta_SNAP_{}".format(runnumber)
        LoadEventNexus(
            Filename="SNAP" + str(runnumber),
            OutputWorkspace=wsname,
            MetaDataOnly=True,
            NumberOfBins=1,
            AllowList="det_arc1,det_arc2,det_lin1,det_lin2",
        )
        return wsname

    def _alignAndFocus(self, filename, wkspname, detCalFilename, withUnfocussed, progStart, progDelta):
        # create the unfocussed name
        if withUnfocussed:
            unfocussed = wkspname.replace("_red", "")
            unfocussed = unfocussed + "_d"
        else:
            unfocussed = ""

        # process the data
        if detCalFilename:
            progEnd = progStart + 0.45 * progDelta
            # have to load and override the instrument here
            Load(Filename=filename, OutputWorkspace=wkspname, startProgress=progStart, endProgress=progEnd)
            progStart = progEnd
            progEnd += 0.45 * progDelta

            LoadIsawDetCal(InputWorkspace=wkspname, Filename=detCalFilename)

            AlignAndFocusPowder(
                InputWorkspace=wkspname,
                OutputWorkspace=wkspname,
                UnfocussedWorkspace=unfocussed,  # can be empty string
                startProgress=progStart,
                endProgress=progEnd,
                **self.alignAndFocusArgs,
            )
            progStart = progEnd
        else:
            progEnd = progStart + 0.9 * progDelta
            # pass all of the work to the child algorithm
            AlignAndFocusPowderFromFiles(
                Filename=filename,
                OutputWorkspace=wkspname,
                MaxChunkSize=self.chunkSize,
                UnfocussedWorkspace=unfocussed,  # can be empty string
                startProgress=progStart,
                endProgress=progEnd,
                **self.alignAndFocusArgs,
            )
            progStart = progEnd

        progEnd = progStart + 0.1 * progDelta
        NormaliseByCurrent(InputWorkspace=wkspname, OutputWorkspace=wkspname, startProgress=progStart, endProgress=progEnd)

        return wkspname, unfocussed

    def PyExec(self):
        if self.getProperty("EnableConfigurator").value:
            self._create_and_save_configuration()
            return  # do not carry out the reduction

        in_Runs = self.getProperty("RunNumbers").value
        progress = Progress(self, 0.0, 0.25, 3)
        finalUnits = self.getPropertyValue("FinalUnits")
        self.chunkSize = self.getProperty("MaxChunkSize").value

        # default arguments for AlignAndFocusPowder
        self.alignAndFocusArgs = {
            "Tmin": 0,
            "TMax": 50000,
            "RemovePromptPulseWidth": 1600,
            "PreserveEvents": False,
            "Dspacing": True,  # binning parameters in d-space
            "Params": self.getProperty("Binning").value,
        }

        # workspace for loading metadata only to be used in LoadDiffCal and
        # CreateGroupingWorkspace
        metaWS = None

        # either type of file-based calibration is stored in the same variable
        calib = self.getProperty("Calibration").value
        detcalFile = None
        if calib == "Calibration File":
            metaWS = self._loadMetaWS(in_Runs[0])
            LoadDiffCal(
                Filename=self.getPropertyValue("CalibrationFilename"),
                WorkspaceName="SNAP",
                InputWorkspace=metaWS,
                MakeGroupingWorkspace=False,
                MakeMaskWorkspace=False,
            )
            self.alignAndFocusArgs["CalibrationWorkspace"] = "SNAP_cal"
        elif calib == "DetCal File":
            detcalFile = ",".join(self.getProperty("DetCalFilename").value)
        progress.report("loaded calibration")

        norm = self.getProperty("Normalization").value

        if norm == "From Processed Nexus":
            norm_File = self.getProperty("NormalizationFilename").value
            normalizationWS = "normWS"
            LoadNexusProcessed(Filename=norm_File, OutputWorkspace=normalizationWS)
            progress.report("loaded normalization")
        elif norm == "From Workspace":
            normalizationWS = str(self.getProperty("NormalizationWorkspace").value)
            progress.report("")
        else:
            normalizationWS = None
            progress.report("")

        self.alignAndFocusArgs["GroupingWorkspace"] = self._generateGrouping(in_Runs[0], metaWS, progress)
        self.alignAndFocusArgs["MaskWorkspace"] = self._getMaskWSname(in_Runs[0], metaWS)  # can be empty string

        if metaWS is not None:
            DeleteWorkspace(Workspace=metaWS)

        Process_Mode = self.getProperty("ProcessingMode").value

        prefix = self.getProperty("OptionalPrefix").value

        Tag = "SNAP"
        progStart = 0.25
        progDelta = (1.0 - progStart) / len(in_Runs)

        # --------------------------- PROCESS BACKGROUND ----------------------
        if not self.getProperty("Background").isDefault:
            progDelta = (1.0 - progStart) / (len(in_Runs) + 1)  # redefine to account for background

            background = "SNAP_{}".format(self.getProperty("Background").value)
            self.log().notice("processing run background {}".format(background))
            background, unfocussedBkgd = self._alignAndFocus(
                background,
                background + "_bkgd_red",
                detCalFilename=detcalFile,
                withUnfocussed=(Process_Mode == "Set-Up"),
                progStart=progStart,
                progDelta=progDelta,
            )
        else:
            background = None
            unfocussedBkgd = ""

        # --------------------------- REDUCE DATA -----------------------------

        for i, runnumber in enumerate(in_Runs):
            self.log().notice("processing run %s" % runnumber)

            # put together output names
            new_Tag = Tag
            if len(prefix) > 0:
                new_Tag = prefix + "_" + new_Tag
            basename = "%s_%s_%s" % (new_Tag, runnumber, self.alignAndFocusArgs["GroupingWorkspace"])
            self.log().warning("{}:{}:{}".format(i, new_Tag, basename))
            redWS, unfocussedWksp = self._alignAndFocus(
                "SNAP_{}".format(runnumber),
                basename + "_red",
                detCalFilename=detcalFile,
                withUnfocussed=(Process_Mode == "Set-Up"),
                progStart=progStart,
                progDelta=progDelta * 0.5,
            )
            progStart += 0.5 * progDelta

            # subtract the background if it was supplied
            if background:
                self.log().information("subtracting {} from {}".format(background, redWS))
                Minus(LHSWorkspace=redWS, RHSWorkspace=background, OutputWorkspace=redWS)
                # intentionally don't subtract the unfocussed workspace since it hasn't been normalized by counting time

            # the rest takes up .25 percent of the run processing
            progress = Progress(self, progStart, progStart + 0.25 * progDelta, 2)

            # AlignAndFocusPowder leaves the data in time-of-flight
            ConvertUnits(InputWorkspace=redWS, OutputWorkspace=redWS, Target="dSpacing", EMode="Elastic")

            # Edit instrument geometry to make final workspace smaller on disk
            det_table = PreprocessDetectorsToMD(Inputworkspace=redWS, OutputWorkspace="__SNAP_det_table")
            polar = np.degrees(det_table.column("TwoTheta"))
            azi = np.degrees(det_table.column("Azimuthal"))
            EditInstrumentGeometry(Workspace=redWS, L2=det_table.column("L2"), Polar=polar, Azimuthal=azi)
            mtd.remove("__SNAP_det_table")
            progress.report("simplify geometry")

            # AlignAndFocus doesn't necessarily rebin the data correctly
            if Process_Mode == "Set-Up":
                Rebin(InputWorkspace=unfocussedWksp, Params=self.alignAndFocusArgs["Params"], Outputworkspace=unfocussedWksp)
                if background:
                    Rebin(InputWorkspace=unfocussedBkgd, Params=self.alignAndFocusArgs["Params"], Outputworkspace=unfocussedBkgd)
            # normalize the data as requested
            normalizationWS = self._generateNormalization(redWS, norm, normalizationWS)
            normalizedWS = None
            if normalizationWS is not None:
                normalizedWS = basename + "_nor"
                Divide(LHSWorkspace=redWS, RHSWorkspace=normalizationWS, OutputWorkspace=normalizedWS)
                ReplaceSpecialValues(
                    Inputworkspace=normalizedWS,
                    OutputWorkspace=normalizedWS,
                    NaNValue="0",
                    NaNError="0",
                    InfinityValue="0",
                    InfinityError="0",
                )
                progress.report("normalized")
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

            # Save requested formats - function checks that saving is requested
            self._save(runnumber, basename, outputWksp)

            # set workspace as an output so it gets history
            ConvertUnits(InputWorkspace=str(outputWksp), OutputWorkspace=str(outputWksp), Target=finalUnits, EMode="Elastic")
            self._exportWorkspace("OutputWorkspace_" + str(outputWksp), outputWksp)

            # declare some things as extra outputs in set-up
            if Process_Mode != "Production":
                propprefix = "OutputWorkspace_{:d}_".format(i)
                propNames = [propprefix + it for it in ["d", "norm", "normalizer"]]
                wkspNames = ["%s_%s_d" % (new_Tag, runnumber), basename + "_red", "%s_%s_normalizer" % (new_Tag, runnumber)]
                for propName, wkspName in zip(propNames, wkspNames):
                    self._exportWorkspace(propName, wkspName)

        if background:
            ConvertUnits(InputWorkspace=str(background), OutputWorkspace=str(background), Target=finalUnits, EMode="Elastic")
            prefix = "OutputWorkspace_{}".format(len(in_Runs))
            propNames = [prefix + it for it in ["", "_d"]]
            wkspNames = [background, unfocussedBkgd]
            for propName, wkspName in zip(propNames, wkspNames):
                self._exportWorkspace(propName, wkspName)

    def _create_and_save_configuration(self):
        logger.notice("Reduction will not be carried out")
        #
        # configuration file name and save location
        #
        basename = datetime.now().strftime("%Y-%m-%d_%H-%M-%S")
        dir_name = self.getProperty("ConfigSaveDir").value
        if len(dir_name) <= 0:  # default directory
            run_number = self.getProperty("RunNumbers").value[0]  # first run number
            dir_name = Path(self.get_IPTS_Local(run_number)) / "shared" / "autoreduce" / "configurations"
            dir_name.mkdir(parents=True, exist_ok=True)  # in case it has not yet been created
        filename = str(Path(dir_name) / f"{basename}.json")
        #
        # Selected algorithm properties as a dictionary
        #
        dict_repr = json.loads(str(self)).get("properties")  # representation of the algorithm's properties in a dict
        # Remove not wanted properties
        for not_wanted in ("RunNumbers", "OutputDirectory", "EnableConfigurator", "ConfigSaveDir"):
            if not_wanted in dict_repr:
                del dict_repr[not_wanted]
        """
        hack to fix the entry for the default JSON represenation of property DetCalFilename, which is saved as a list of lists
        Example: "DetCalFilename": [ ["/SNS/SNAP/IPTS-26217/shared/E76p2_W65p3.detcal"],
                                     ["/SNS/SNAP/IPTS-26217/shared/E76p2_W65p5.detcal"]]
                 must become:
                 "DetCalFilename": "/SNS/SNAP/IPTS-26217/shared/E76p2_W65p3.detcal,/SNS/SNAP/IPTS-26217/shared/E76p2_W65p5.detcal"
        """
        if "DetCalFilename" in dict_repr:
            dict_repr["DetCalFilename"] = ",".join([entry[0] for entry in dict_repr.get("DetCalFilename")])
        #
        # Save to file in JSON format
        #
        formatted_pretty = json.dumps(dict_repr, sort_keys=True, indent=4)
        with open(filename, "w") as f:
            f.write(formatted_pretty)
            logger.information(f"Saving configuration to {filename}")
            logger.debug(f"Configuration contents:\n{formatted_pretty}")


AlgorithmFactory.subscribe(SNAPReduce)
