# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantid.api import (
    AlgorithmFactory,
    FileAction,
    FileProperty,
    IMDHistoWorkspace,
    IMDHistoWorkspaceProperty,
    PythonAlgorithm,
    Progress,
    PropertyMode,
    MultipleFileProperty,
    WorkspaceFactory,
    WorkspaceProperty,
)
from mantid.kernel import (
    Direction,
    EnabledWhenProperty,
    LogicOperator,
    PropertyCriterion,
    FloatArrayProperty,
    FloatArrayLengthValidator,
    FloatPropertyWithValue,
    StringListValidator,
)
from mantid.simpleapi import (
    ConvertHFIRSCDtoMDE,
    ConvertWANDSCDtoQ,
    CloneMDWorkspace,
    DeleteWorkspace,
    DeleteWorkspaces,
    DivideMD,
    LoadMD,
    MergeMD,
    ReplicateMD,
    SetGoniometer,
    mtd,
    GroupWorkspaces,
    RenameWorkspace,
    ConvertToMD,
    GroupDetectors,
    LoadInstrument,
    CreateMDHistoWorkspace,
    CreateSimulationWorkspace,
    MoveInstrumentComponent,
    AddSampleLog,
    CopySample,
    Rebin,
)
import os
import numpy as np


class HB3AAdjustSampleNorm(PythonAlgorithm):
    def category(self):
        return "Crystal\\Corrections"

    def seeAlso(self):
        return ["ConvertWANDSCDtoQ", "ConvertHFIRSCDtoMDE", "HB3AFindPeaks", "HB3APredictPeaks", "HB3AIntegratePeaks"]

    def name(self):
        return "HB3AAdjustSampleNorm"

    def summary(self):
        return (
            "Adjusts the detector position based on a detector height and distance offset and normalizes with "
            "detector efficiency from a vanadium file or workspace, and converts the input to Q-space."
        )

    def PyInit(self):
        # Input params
        self.declareProperty(
            MultipleFileProperty(name="Filename", extensions=[".nxs.h5", ".nxs"], action=FileAction.OptionalLoad),
            doc="Input autoreduced detector scan data files to convert to Q-space.",
        )
        self.declareProperty(
            FileProperty(
                name="VanadiumFile", defaultValue="", extensions=[".nxs"], direction=Direction.Input, action=FileAction.OptionalLoad
            ),
            doc="File with Vanadium normalization scan data",
        )

        self.declareProperty("NormaliseBy", "Time", StringListValidator(["None", "Time", "Monitor"]), "Normalise to monitor, time or None.")

        self.declareProperty("NormalizeData", True, "When False, skip normalization of the output data even if vanadium data is provided.")

        # Alternative WS inputs
        self.declareProperty(
            "InputWorkspaces",
            defaultValue="",
            direction=Direction.Input,
            doc="Workspace or comma-separated workspace list containing input MDHisto scan data.",
        )
        self.declareProperty(
            IMDHistoWorkspaceProperty("VanadiumWorkspace", defaultValue="", direction=Direction.Input, optional=PropertyMode.Optional),
            doc="MDHisto workspace containing vanadium normalization data",
        )

        # Detector adjustment options
        self.declareProperty(
            "DetectorHeightOffset",
            defaultValue=0.0,
            direction=Direction.Input,
            doc="Optional distance (in meters) to move detector height (relative to current position)",
        )
        self.declareProperty(
            "DetectorDistanceOffset",
            defaultValue=0.0,
            direction=Direction.Input,
            doc="Optional distance (in meters) to move detector distance (relative to current position)",
        )

        self.declareProperty(
            FloatPropertyWithValue("Wavelength", FloatPropertyWithValue.EMPTY_DBL),  # EMPTY_DBL so it shows as blank in GUI
            doc="Optional wavelength value to use as backup if one was not found in the sample log",
        )

        # Which conversion algorithm to use
        self.declareProperty(
            "OutputType",
            "Q-sample events",
            StringListValidator(["Q-sample events", "Q-sample histogram", "Detector"]),
            direction=Direction.Input,
            doc="Whether to use ConvertHFIRSCDtoQ for an MDEvent, or ConvertWANDSCDtoQ for an MDHisto",
        )

        self.declareProperty(
            "ScaleByMotorStep",
            False,
            "If True then the intensity of the Q-sample events output will be scaled by the motor step size. "
            "This will allow directly comparing the intensity of data measured with different motor step sizes.",
        )

        # MDEvent WS Specific options for ConvertHFIRSCDtoQ
        self.declareProperty(
            FloatArrayProperty("MinValues", [-10, -10, -10], FloatArrayLengthValidator(3), direction=Direction.Input),
            doc="3 comma separated values, one for each q_sample dimension",
        )
        self.declareProperty(
            FloatArrayProperty("MaxValues", [10, 10, 10], FloatArrayLengthValidator(3), direction=Direction.Input),
            doc="3 comma separated values, one for each q_sample dimension; must be larger than those specified in MinValues",
        )
        self.declareProperty(
            "MergeInputs",
            defaultValue=False,
            direction=Direction.Input,
            doc="If all inputs should be merged into one MDEvent output workspace",
        )

        # MDHisto WS Specific options for ConvertWANDSCDtoQ
        self.declareProperty(
            FloatArrayProperty("BinningDim0", [-8.02, 8.02, 401], FloatArrayLengthValidator(3), direction=Direction.Input),
            "Binning parameters for the 0th dimension. Enter it as a"
            "comma-separated list of values with the"
            "format: 'minimum,maximum,number_of_bins'.",
        )
        self.declareProperty(
            FloatArrayProperty("BinningDim1", [-2.52, 2.52, 126], FloatArrayLengthValidator(3), direction=Direction.Input),
            "Binning parameters for the 1st dimension. Enter it as a"
            "comma-separated list of values with the"
            "format: 'minimum,maximum,number_of_bins'.",
        )
        self.declareProperty(
            FloatArrayProperty("BinningDim2", [-8.02, 8.02, 401], FloatArrayLengthValidator(3), direction=Direction.Input),
            "Binning parameters for the 2nd dimension. Enter it as a"
            "comma-separated list of values with the"
            "format: 'minimum,maximum,number_of_bins'.",
        )

        self.setPropertySettings("Filename", EnabledWhenProperty("InputWorkspaces", PropertyCriterion.IsDefault))
        self.setPropertySettings("VanadiumFile", EnabledWhenProperty("VanadiumWorkspace", PropertyCriterion.IsDefault))
        self.setPropertySettings("InputWorkspaces", EnabledWhenProperty("Filename", PropertyCriterion.IsDefault))
        self.setPropertySettings("VanadiumWorkspace", EnabledWhenProperty("VanadiumFile", PropertyCriterion.IsDefault))

        self.setPropertySettings("ScaleByMotorStep", EnabledWhenProperty("OutputType", PropertyCriterion.IsEqualTo, "Q-sample events"))

        event_settings = EnabledWhenProperty("OutputType", PropertyCriterion.IsEqualTo, "Q-sample events")
        self.setPropertyGroup("MinValues", "MDEvent Settings")
        self.setPropertyGroup("MaxValues", "MDEvent Settings")
        self.setPropertyGroup("MergeInputs", "MDEvent Settings")
        self.setPropertySettings("MinValues", event_settings)
        self.setPropertySettings("MaxValues", event_settings)
        self.setPropertySettings("MergeInputs", event_settings)

        histo_settings = EnabledWhenProperty("OutputType", PropertyCriterion.IsEqualTo, "Q-sample histogram")
        self.setPropertyGroup("BinningDim0", "MDHisto Settings")
        self.setPropertyGroup("BinningDim1", "MDHisto Settings")
        self.setPropertyGroup("BinningDim2", "MDHisto Settings")
        self.setPropertySettings("BinningDim0", histo_settings)
        self.setPropertySettings("BinningDim1", histo_settings)
        self.setPropertySettings("BinningDim2", histo_settings)

        # Grouping info
        self.declareProperty(
            "Grouping", "None", StringListValidator(["None", "2x2", "4x4"]), "Group pixels (shared by input and normalization)"
        )

        self.declareProperty(
            WorkspaceProperty("OutputWorkspace", "", optional=PropertyMode.Mandatory, direction=Direction.Output),
            doc="Output MDWorkspace in Q-space, name is prefix if multiple input files were provided.",
        )

        self.declareProperty(
            WorkspaceProperty("OutputNormalizationWorkspace", "", optional=PropertyMode.Optional, direction=Direction.Output),
            doc="Optional MDEvent output workspace containing the per-file vanadium normalization scaled by the "
            "monitor flux ratio. Requires NormalizeData=False, OutputType='Q-sample events', and vanadium data. "
            "When multiple inputs are provided, MergeInputs must also be True.",
        )
        # Enabled when: OutputType=='Q-sample events' AND NormalizeData==False AND (VanadiumFile OR VanadiumWorkspace)
        self.setPropertySettings(
            "OutputNormalizationWorkspace",
            EnabledWhenProperty(
                EnabledWhenProperty(
                    EnabledWhenProperty("OutputType", PropertyCriterion.IsEqualTo, "Q-sample events"),
                    EnabledWhenProperty("NormalizeData", PropertyCriterion.IsEqualTo, "0"),
                    LogicOperator.And,
                ),
                EnabledWhenProperty(
                    EnabledWhenProperty("VanadiumFile", PropertyCriterion.IsNotDefault),
                    EnabledWhenProperty("VanadiumWorkspace", PropertyCriterion.IsNotDefault),
                    LogicOperator.Or,
                ),
                LogicOperator.And,
            ),
        )

        self.declareProperty(
            WorkspaceProperty("OutputGroupingWorkspace", "", optional=PropertyMode.Optional, direction=Direction.Output),
            doc="Optional: output GroupingWorkspace mapping every detector's workspace index to its group ID. "
            "Only produced when Grouping is '2x2' or '4x4'.",
        )
        self.setPropertySettings(
            "OutputGroupingWorkspace",
            EnabledWhenProperty("Grouping", PropertyCriterion.IsNotEqualTo, "None"),
        )

    def validateInputs(self):
        issues = dict()

        filelist = self.getProperty("Filename").value
        vanfile = self.getProperty("VanadiumFile").value
        input_ws = self.getProperty("InputWorkspaces")
        van_ws = self.getProperty("VanadiumWorkspace")
        wavelength = self.getProperty("Wavelength")

        # Make sure files and workspaces aren't both set
        if len(filelist) >= 1:
            if not input_ws.isDefault:
                issues["InputWorkspaces"] = "Cannot specify both a filename and input workspace"
        else:
            if input_ws.isDefault:
                issues["Filename"] = "Either a file or input workspace must be specified"

        if len(vanfile) > 0 and not van_ws.isDefault:
            issues["VanadiumWorkspace"] = "Cannot specify both a vanadium file and workspace"

        # Verify given workspaces exist
        if not input_ws.isDefault:
            input_ws_list = list(map(str.strip, input_ws.value.split(",")))
            for ws in input_ws_list:
                if not mtd.doesExist(ws):
                    issues["InputWorkspaces"] = "Could not find input workspace '{}'".format(ws)
                else:
                    # If it does exist, make sure the workspace is an MDHisto with 3 dimensions
                    if not isinstance(mtd[ws], IMDHistoWorkspace):
                        issues["InputWorkspaces"] = "Workspace '{}' must be a MDHistoWorkspace".format(ws)
                    elif mtd[ws].getNumDims() != 3:
                        issues["InputWorkspaces"] = "Workspace '{}' expected to have 3 dimensions".format(ws)

        if not wavelength.isDefault:
            if wavelength.value <= 0.0:
                issues["Wavelength"] = "Wavelength should be greater than zero"

        # OutputGroupingWorkspace requires grouping to be active
        if self.getProperty("Grouping").value == "None" and self.getPropertyValue("OutputGroupingWorkspace") != "":
            grp_ws_name = self.getPropertyValue("OutputGroupingWorkspace")
            issues["OutputGroupingWorkspace"] = (
                f"OutputGroupingWorkspace '{grp_ws_name}' can only be produced when Grouping is '2x2' or '4x4'"
            )

        if self.getProperty("MergeInputs").value and not self.__hasMultipleInputs():
            issues["MergeInputs"] = "MergeInputs requires more than one input file or workspace."

        if self.getPropertyValue("OutputNormalizationWorkspace") != "" and not self.__saveNormalizationWorkspace():
            norm_ws_name = self.getPropertyValue("OutputNormalizationWorkspace")
            issues["OutputNormalizationWorkspace"] = (
                f"OutputNormalizationWorkspace '{norm_ws_name}' can only be produced when NormalizeData is false, "
                "OutputType is 'Q-sample events', vanadium data is provided, and MergeInputs is true when "
                "multiple inputs are given."
            )

        return issues

    def PyExec(self):
        load_van = not self.getProperty("VanadiumFile").isDefault
        load_files = not self.getProperty("Filename").isDefault
        output = self.getProperty("OutputType").value
        datafiles = self.__input_datafiles(load_files)

        prog = Progress(self, 0.0, 1.0, len(datafiles) + 1)

        height = self.getProperty("DetectorHeightOffset").value
        distance = self.getProperty("DetectorDistanceOffset").value
        wslist = []
        normalizing_wslist = []
        grouping = self.__grouping_size()
        out_ws_rootname = self.getPropertyValue("OutputWorkspace")

        vanws = self.__load_vanadium(load_van, grouping, height, distance)
        has_multiple = self.__hasMultipleInputs()
        grouping_ws = None

        for index, in_file in enumerate(datafiles):
            scan = self.__load_scan(in_file, load_files)
            self.log().information("Detector adjustments '({},{})m'".format(height, distance))
            scan, grouping_ws = self.__prepare_scan(scan, in_file, index == 0, grouping, height, distance, grouping_ws)
            prog.report()
            self.log().information("Processing '{}'".format(in_file))

            SetGoniometer(Workspace=scan, Axis0="omega,0,1,0,-1", Axis1="chi,0,0,1,-1", Axis2="phi,0,1,0,-1", Average=False)
            out_ws_name = self.__output_workspace_name(out_ws_rootname, in_file, has_multiple, load_files)
            wslist.append(out_ws_name)

            # Get the wavelength from experiment info if it exists, or fallback on property value
            exp_info = scan.getExperimentInfo(0)
            wavelength = self.__get_wavelength(exp_info)
            self.__ensure_run_number(exp_info)

            if output == "Q-sample histogram":
                self.__process_q_sample_histogram(scan, vanws, wavelength, out_ws_name)
            elif output == "Detector":
                self.__process_detector(scan, vanws, out_ws_name)
            elif output == "Q-sample events":
                normalizing_ws = self.__process_q_sample_events(scan, vanws, wavelength, out_ws_name)
                if normalizing_ws is not None:
                    normalizing_wslist.append(normalizing_ws)
            else:
                raise RuntimeError("Invalid output type '{}'".format(output))

        out_ws_name = self.__save_output_workspace(has_multiple, output, wslist, out_ws_rootname)
        self.setProperty("OutputWorkspace", out_ws_name)
        if grouping_ws is not None:
            self.setProperty("OutputGroupingWorkspace", grouping_ws)

        self.__save_output_normalization_workspace(has_multiple, normalizing_wslist)

        # Clean up temporary workspaces
        if load_van:
            DeleteWorkspace(vanws)

    def __input_datafiles(self, load_files):
        if load_files:
            return self.getProperty("Filename").value
        return list(map(str.strip, self.getProperty("InputWorkspaces").value.split(",")))

    def __grouping_size(self):
        grouping = self.getProperty("Grouping").value
        return {"None": 1, "2x2": 2, "4x4": 4}[grouping]

    def __load_vanadium(self, load_van, grouping, height, distance):
        if load_van:
            vanws = LoadMD(self.getProperty("VanadiumFile").value, StoreInADS=True)
            vanws, _ = self.__regroup_and_move(vanws, grouping, height, distance)
            return vanws
        return self.getProperty("VanadiumWorkspace").value

    def __load_scan(self, in_file, load_files):
        if load_files:
            scan = LoadMD(in_file, LoadHistory=False, OutputWorkspace="__scan")
        else:
            scan = CloneMDWorkspace(in_file)
        return scan

    def __prepare_scan(self, scan, in_file, first_iteration, grouping, height, distance, grouping_ws):
        # Make sure the workspace has experiment info, otherwise SetGoniometer will add some, causing issues.
        if scan.getNumExperimentInfo() == 0:
            raise RuntimeError("No experiment info was found in '{}'".format(in_file))

        create_grouping_ws = first_iteration and grouping > 1 and not self.getProperty("OutputGroupingWorkspace").isDefault
        scan, new_grouping_ws = self.__regroup_and_move(scan, grouping, height, distance, create_grouping_ws=create_grouping_ws)
        return scan, new_grouping_ws or grouping_ws

    def __output_workspace_name(self, out_ws_rootname, in_file, has_multiple, load_files):
        if not has_multiple:
            return out_ws_rootname
        if load_files:
            return out_ws_rootname + "_" + os.path.basename(in_file).strip(",.nxs")
        return out_ws_rootname + "_" + in_file

    def __ensure_run_number(self, exp_info):
        # Set the run number to be the same as scan number, this will be used for peaks.
        if not exp_info.run().hasProperty("run_number") and exp_info.run().hasProperty("scan"):
            try:
                exp_info.mutableRun().addProperty("run_number", int(exp_info.run().getProperty("scan").value), True)
            except ValueError:
                # scan must be an int
                pass

    def __process_q_sample_histogram(self, scan, vanws, wavelength, out_ws_name):
        normalize_data = self.getProperty("NormalizeData").value
        ConvertWANDSCDtoQ(
            InputWorkspace=scan,
            NormalisationWorkspace=vanws if normalize_data else None,
            Frame="Q_sample",
            Wavelength=wavelength,
            NormaliseBy=self.getProperty("NormaliseBy").value if normalize_data else "None",
            BinningDim0=self.getProperty("BinningDim0").value,
            BinningDim1=self.getProperty("BinningDim1").value,
            BinningDim2=self.getProperty("BinningDim2").value,
            OutputWorkspace=out_ws_name,
        )
        DeleteWorkspace(scan)

    def __process_detector(self, scan, vanws, out_ws_name):
        if self.getProperty("NormalizeData").value:
            scan = self.__normalize_and_divide(scan, vanws)
        RenameWorkspace(scan, OutputWorkspace=out_ws_name)

    def __process_q_sample_events(self, scan, vanws, wavelength, out_ws_name):
        flux_ratio = self.__normaliseByScaling(scan, vanws) / self.__scaleByMotorStep(scan)
        normalizing_ws = self.__create_normalizing_workspace(scan, vanws, flux_ratio)
        norm_data = self.__normalized_event_data(scan, vanws, normalizing_ws, flux_ratio)
        minvals = self.getProperty("MinValues").value
        maxvals = self.getProperty("MaxValues").value
        ConvertHFIRSCDtoMDE(
            InputWorkspace=norm_data, Wavelength=wavelength, MinValues=minvals, MaxValues=maxvals, OutputWorkspace=out_ws_name
        )
        DeleteWorkspace(norm_data)
        return self.__converted_normalization_workspace(normalizing_ws, wavelength, minvals, maxvals)

    def __create_normalizing_workspace(self, scan, vanws, flux_ratio):
        if not vanws:
            return None
        if scan.getDimension(2).getNBins() > 1:  # more than one scan
            normalizing_ws = ReplicateMD(
                ShapeWorkspace=scan,
                DataWorkspace=vanws,
                OutputWorkspace=mtd.unique_hidden_name(),
            )
        else:  # we clone scan rather than vanws to keep in line with what ReplicateMD does.
            normalizing_ws = CloneMDWorkspace(
                InputWorkspace=scan,
                OutputWorkspace=mtd.unique_hidden_name(),
            )
            normalizing_ws.setSignalArray(vanws.getSignalArray().copy())
            normalizing_ws.setErrorSquaredArray(vanws.getErrorSquaredArray().copy())
        return self.__scaleBy(normalizing_ws, flux_ratio)

    def __normalized_event_data(self, scan, vanws, normalizing_ws, flux_ratio):
        if not self.getProperty("NormalizeData").value:
            return scan
        if not vanws:
            return self.__scaleBy(scan, 1.0 / flux_ratio)
        norm_data = DivideMD(LHSWorkspace=scan, RHSWorkspace=normalizing_ws)
        DeleteWorkspace(scan)
        return norm_data

    def __converted_normalization_workspace(self, normalizing_ws, wavelength, minvals, maxvals):
        if self.__saveNormalizationWorkspace():
            return ConvertHFIRSCDtoMDE(
                InputWorkspace=normalizing_ws,
                Wavelength=wavelength,
                MinValues=minvals,
                MaxValues=maxvals,
                OutputWorkspace=normalizing_ws.name(),  # overwrite the input MDHistoWorkspace
            )
        if normalizing_ws:
            DeleteWorkspace(normalizing_ws)
        return None

    def __save_output_workspace(self, has_multiple, output, wslist, out_ws_rootname):
        if not has_multiple:
            return out_ws_rootname
        if output == "Q-sample events" and self.getProperty("MergeInputs").value:
            MergeMD(InputWorkspaces=wslist, OutputWorkspace=out_ws_rootname)
            DeleteWorkspaces(wslist)
        else:
            GroupWorkspaces(InputWorkspaces=wslist, OutputWorkspace=out_ws_rootname)
        return out_ws_rootname

    def __save_output_normalization_workspace(self, has_multiple, normalizing_wslist):
        if not self.__saveNormalizationWorkspace():
            return
        normalizing_ws_name = self.getPropertyValue("OutputNormalizationWorkspace")
        if has_multiple:
            MergeMD(InputWorkspaces=normalizing_wslist, OutputWorkspace=normalizing_ws_name)
            DeleteWorkspaces(normalizing_wslist)
        else:
            RenameWorkspace(InputWorkspace=normalizing_wslist[0], OutputWorkspace=normalizing_ws_name)
        self.setProperty("OutputNormalizationWorkspace", normalizing_ws_name)

    def __regroup_and_move(self, scan, grouping, height, distance, create_grouping_ws=False):
        output_workspace_name = scan.name()  # the input workspace will be modified or replaced

        array = mtd[output_workspace_name].getSignalArray().copy()
        groups = None  # stores detector groupings
        if grouping > 1:
            # Layout of the detector IDs
            #     1,      2,..,   512
            #   513,    514,..,  1024
            # ..
            # 785921, 785922,..,786432
            idmap = np.arange(1, 512 * 512 * 3 + 1).reshape((512 * 3, 512))  # reproduces the layout of detector IDs
            if grouping == 2:
                array = array[0::2, 0::2] + array[1::2, 0::2] + array[0::2, 1::2] + array[1::2, 1::2]
                # groups[0, 0] = [1, 2, 513, 514]  the very first group of detectors
                groups = np.stack([idmap[0::2, 0::2], idmap[0::2, 1::2], idmap[1::2, 0::2], idmap[1::2, 1::2]], axis=2)
            elif grouping == 4:
                array = (
                    array[0::4, 0::4]
                    + array[1::4, 0::4]
                    + array[2::4, 0::4]
                    + array[3::4, 0::4]
                    + array[0::4, 1::4]
                    + array[1::4, 1::4]
                    + array[2::4, 1::4]
                    + array[3::4, 1::4]
                    + array[0::4, 2::4]
                    + array[1::4, 2::4]
                    + array[2::4, 2::4]
                    + array[3::4, 2::4]
                    + array[0::4, 3::4]
                    + array[1::4, 3::4]
                    + array[2::4, 3::4]
                    + array[3::4, 3::4]
                )
                # groups[0, 0] = [1, 2, 3, 4, 513, 514, 515, 516, 1025, 1026, 1027, 1028, 1537, 1538, 1539, 1540]
                groups = np.stack(
                    [
                        idmap[0::4, 0::4],
                        idmap[1::4, 0::4],
                        idmap[2::4, 0::4],
                        idmap[3::4, 0::4],
                        idmap[0::4, 1::4],
                        idmap[1::4, 1::4],
                        idmap[2::4, 1::4],
                        idmap[3::4, 1::4],
                        idmap[0::4, 2::4],
                        idmap[1::4, 2::4],
                        idmap[2::4, 2::4],
                        idmap[3::4, 2::4],
                        idmap[0::4, 3::4],
                        idmap[1::4, 3::4],
                        idmap[2::4, 3::4],
                        idmap[3::4, 3::4],
                    ],
                    axis=2,
                )

        y_dim, x_dim, number_of_runs = array.shape  # y_dim, x_dim effective dimensions after grouping
        array = array.T  # shape == (number_of_runs, x_dim, y_dim)

        # Create a temporary Workspace2D with the HB3A instrument geometry, using the detector
        # translation and two-theta values from the scan to correctly position the instrument components.
        # This workspace serves as a carrier for computing per-detector angles and IDs via ConvertToMD.
        _tmp_ws = CreateSimulationWorkspace(Instrument="HB3A", BinParams="0,1,2", UnitX="TOF", SetErrors=True)
        run = mtd[output_workspace_name].getExperimentInfo(0).run()
        det_trans = run.getProperty("det_trans").timeAverageValue()
        two_theta = run.getProperty("2theta").timeAverageValue()
        AddSampleLog(Workspace=_tmp_ws, LogName="det_trans", LogText=str(det_trans), LogType="Number Series", NumberType="Double")
        AddSampleLog(Workspace=_tmp_ws, LogName="2theta", LogText=str(two_theta), LogType="Number Series", NumberType="Double")
        LoadInstrument(Workspace=_tmp_ws, RewriteSpectraMap=True, InstrumentName="HB3A")
        self.__move_components(_tmp_ws, height, distance)

        grouping_workspace = None
        if grouping > 1:
            CreateMDHistoWorkspace(
                SignalInput=array,
                ErrorInput=np.sqrt(array),
                Dimensionality=3,
                Extents="0.5,{},0.5,{},0.5,{}".format(y_dim + 0.5, x_dim + 0.5, number_of_runs + 0.5),
                NumberOfBins="{},{},{}".format(y_dim, x_dim, number_of_runs),
                Names="y,x,scanIndex",
                Units="bin,bin,number",
                OutputWorkspace="__scan_grouped",
            )
            # Create the grouping workspace, if so required.
            if create_grouping_ws:
                detector_ids_per_group = groups.reshape(-1, groups.shape[2])
                # WorkspaceFactory.create is used directly rather than CreateGroupingWorkspace to avoid
                # building the detector-ID-to-workspace-index map (detID_to_WI), which is prohibitively
                # slow for HB3A's ~786k detectors. Since HB3A detector IDs start at 1,
                # workspace_index = det_id - 1, so dataY can be written directly.
                grouping_workspace = WorkspaceFactory.create("GroupingWorkspace", _tmp_ws.getNumberHistograms(), 1, 1)
                for i, det_ids in enumerate(detector_ids_per_group):
                    group_id = float(i + 1)
                    for det_id in det_ids:
                        grouping_workspace.dataY(int(det_id) - 1)[0] = group_id
                        grouping_workspace.getSpectrum(int(det_id) - 1).setDetectorID(int(det_id))

            # Group spectra
            workspace_indexes = groups - 1  # for the HB3A instrument, detector IDs start at 1, not 0
            workspace_indexes = workspace_indexes.reshape(-1, workspace_indexes.shape[2])
            # grouping_pattern = '0+1+512+513, 2+3+514+515,..'
            grouping_pattern = ",".join("+".join(str(idx) for idx in row) for row in workspace_indexes)
            _tmp_ws = GroupDetectors(InputWorkspace=_tmp_ws, GroupingPattern=grouping_pattern, OutputWorkspace=_tmp_ws, EnableLogging=False)

        # Convert Workspace2D to IMDEventWorkspace
        _tmp_ws = Rebin(InputWorkspace=_tmp_ws, Params="0,1,2", EnableLogging=False)
        _tmp_ws = ConvertToMD(
            InputWorkspace=_tmp_ws, dEAnalysisMode="Elastic", EnableLogging=False, PreprocDetectorsWS="_PreprocessedDetectorsWS"
        )

        # Copy all original scan logs into _tmp_ws, overwriting the scalar "det_trans" and "2theta" logs
        # added earlier with the full time-series logs from the input scan workspace
        run = _tmp_ws.getExperimentInfo(0).run()
        scan_logs = mtd[output_workspace_name].getExperimentInfo(0).run().getLogData()
        for log in scan_logs:
            run[log.name] = log

        # Collect twotheta, azimuthal, and detector ID's from the _PreprocessedDetectorsWS table
        run["twotheta"] = np.array(mtd["_PreprocessedDetectorsWS"].column(2)).reshape(y_dim, x_dim).T.flatten().tolist()
        run["azimuthal"] = np.array(mtd["_PreprocessedDetectorsWS"].column(3)).reshape(y_dim, x_dim).T.flatten().tolist()
        # Store detector ID's. If grouping, follow Mantid convention by storing the first ID in each group
        # The sequence of detector ID's follows an ascending order along the X-dimension first
        first_detid = np.array(mtd["_PreprocessedDetectorsWS"].column(4))  # 1, 3, 5,..,1025, 1027,.. for 2x2 grouping
        # sample-log "detectorID" stores ID's in ordering of ascending along the Y-dimension first,
        # replicating the ordering in which "twotheta" and "azimuthal" are stored
        run["detectorID"] = first_detid.reshape(y_dim, x_dim).T.flatten().tolist()  # 1, 1025, 2049,..,3,.. for 2x2

        # transfer the UB matrix from the original scan data into _tmp_ws
        CopySample(scan, _tmp_ws, CopyName=False, CopyMaterial=False, CopyEnvironment=False, CopyShape=False)

        # transfer the experiment info from _tmp_ws to the output workspace
        if grouping > 1:
            mtd["__scan_grouped"].copyExperimentInfos(_tmp_ws)
            # replace the input scan workspace with the grouped workspace
            DeleteWorkspace(scan)
            RenameWorkspace("__scan_grouped", OutputWorkspace=output_workspace_name)
        else:
            mtd[output_workspace_name].copyExperimentInfos(_tmp_ws)

        # clean up temporary workspaces
        DeleteWorkspace(_tmp_ws, EnableLogging=False)
        DeleteWorkspace("_PreprocessedDetectorsWS", EnableLogging=False)

        return mtd[output_workspace_name], grouping_workspace

    def __hasMultipleInputs(self):
        """Return True when the user has provided more than one input file or workspace.
        :return: bool
        """
        if not self.getProperty("Filename").isDefault:
            return len(self.getProperty("Filename").value) > 1
        ws_names = [w.strip() for w in self.getProperty("InputWorkspaces").value.split(",") if w.strip()]
        return len(ws_names) > 1

    def __saveNormalizationWorkspace(self):
        """Return True when all conditions to output the normalization workspace are met:
        NormalizeData is False, OutputType is 'Q-sample events', vanadium data is provided
        (via VanadiumFile or VanadiumWorkspace), and OutputNormalizationWorkspace has been
        given a name (non-default). When multiple inputs are provided, MergeInputs must also
        be True so that the per-file normalization workspaces can be merged into one.
        :return: bool
        """
        vanadium_provided = not self.getProperty("VanadiumFile").isDefault or not self.getProperty("VanadiumWorkspace").isDefault
        merge_ok = not self.__hasMultipleInputs() or self.getProperty("MergeInputs").value
        return (
            not self.getProperty("NormalizeData").value
            and merge_ok
            and self.getProperty("OutputType").value == "Q-sample events"
            and vanadium_provided
            and not self.getProperty("OutputNormalizationWorkspace").isDefault
        )

    def __scaleByMotorStep(self, ws):
        """Return the motor step size derived from ws, so that data measured
        with different step sizes can be directly compared.
        :param ws: MDHistoWorkspace whose experiment info supplies the motor axis log.
        :return: Scalar step size of the scan motor.
        """
        step_size = 1.0
        if self.getProperty("ScaleByMotorStep").value:
            run_info = ws.getExperimentInfo(0).run()
            scan_log = "omega" if np.isclose(run_info.getTimeAveragedStd("phi"), 0.0) else "phi"
            scan_axis = run_info[scan_log].value
            step_size = (scan_axis[-1] - scan_axis[0]) / (scan_axis.size - 1)  # assumes all steps same as this average
        return step_size

    def __normaliseByScaling(self, data, vanadium):
        """Return the per-step NormaliseBy scale factor array, or 1.0 when NormaliseBy is 'None'.
        :param data: MDHistoWorkspace whose run logs supply the per-step monitor or time values.
        :param vanadium: Vanadium MDHistoWorkspace used to divide out the vanadium monitor/time value, or None.
        :return: 1-D numpy array of scale factors, or 1.0.
        """
        normaliseBy = self.getProperty("NormaliseBy").value.lower()
        if normaliseBy not in ("monitor", "time"):
            return 1.0
        scale = np.asarray(data.getExperimentInfo(0).run().getProperty(normaliseBy).value)
        if vanadium:
            scale /= vanadium.getExperimentInfo(0).run().getProperty(normaliseBy).value[0]
        return scale

    def __scaleBy(self, ws, scalings):
        ws.setSignalArray(ws.getSignalArray() * scalings)
        ws.setErrorSquaredArray(ws.getErrorSquaredArray() * scalings**2)
        return ws

    def __normalize_and_divide(self, data, vanadium):
        if vanadium:
            norm_data = ReplicateMD(ShapeWorkspace=data, DataWorkspace=vanadium)  # `data` to be deleted later
            norm_data = DivideMD(LHSWorkspace=data, RHSWorkspace=norm_data)
        else:
            norm_data = data
        flux_ratio = self.__normaliseByScaling(norm_data, vanadium)
        self.__scaleBy(norm_data, 1.0 / flux_ratio)

        if vanadium:
            DeleteWorkspace(data)  # clean up
        return norm_data

    def __move_components(self, ws, height, distance):
        """
        Moves all instrument banks by a given height (y) and distance (x-z) in meters,
        relative to the current instrument position.
        :param ws: Workspace2d with instrument definition
        :param height: Distance to move the instrument along y axis
        :param distance: Distance to move the instrument in the x-z plane
        """
        # Adjust detector height and distance with new offsets
        instrument = ws.getInstrument()
        for bank in range(1, 4):
            # Set height offset (y) first on bank
            panel_name = "bank{}/panel".format(bank)
            if height != 0.0:
                MoveInstrumentComponent(Workspace=ws, ComponentName=panel_name, Y=height)

            # Set distance offset to detector (x,z) on bank/panel
            if distance != 0.0:
                component = instrument.getComponentByName(panel_name)
                panel_pos = component.getPos()
                # need to move detector in direction in x-z plane
                panel_pos[1] = 0
                panel_offset = panel_pos * (distance / panel_pos.norm())
                MoveInstrumentComponent(Workspace=ws, ComponentName=panel_name, X=panel_offset[0], Z=panel_offset[2])

    def __get_wavelength(self, exp_info):
        """
        Gets the wavelength from experiment info if provided, otherwise it will try to get the value
        from the algorithm property. Throws a RuntimeError if a wavelength cannot be found from either.
        :param exp_info: The experiment info of the run to lookup set wavelength value
        :return: wavelength value from experiment info if set, or from wavelength property
        """
        if exp_info.run().hasProperty("wavelength"):
            return exp_info.run().getProperty("wavelength").value
        else:
            # Set wavelength value from the backup property, if provided
            wl_prop = self.getProperty("Wavelength")
            if not wl_prop.isDefault:
                return wl_prop.value
            else:
                # If wavelength value not set, throw an error
                raise RuntimeError("Wavelength not found in sample log and was not provided as input to the algorithm")


AlgorithmFactory.subscribe(HB3AAdjustSampleNorm)
