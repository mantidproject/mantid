# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=no-init

"""
System test for MDNorm background
"""

from mantid.simpleapi import (
    CloneMDWorkspace,
    CompareMDWorkspaces,
    config,
    ConvertToMD,
    CropWorkspaceForMDNorm,
    DeleteWorkspace,
    DgsReduction,
    FilterEvents,
    GenerateEventsFilter,
    Load,
    MDNorm,
    MergeMD,
    mtd,
    RenameWorkspace,
    SetGoniometer,
    SetUB,
)
import systemtesting

# Define symmetry operation
SYMMETRY_OPERATION = "x,y,z;x,-y,z;x,y,-z;x,-y,-z"
# SYMMETRY_OPERATION = 'x,y,z'
NUM_SO = SYMMETRY_OPERATION.count(";") + 1
LOG_VALUE_STEP = 1


class MDNormBackgroundHYSPECTest(systemtesting.MantidSystemTest):
    @staticmethod
    def prepare_single_exp_info_background(input_md_name, output_md_name, target_qframe="Q_lab"):
        if target_qframe not in ["Q_sample", "Q_lab"]:
            raise ValueError(f"QFrame with name {target_qframe} is not recognized.")

        # Create background workspace in Q lab
        ConvertToMD(
            InputWorkspace=input_md_name,
            QDimensions="Q3D",
            Q3DFrames=target_qframe,
            OutputWorkspace=output_md_name,
            MinValues="-11,-11,-11,-25",
            MaxValues="11,11,11,49",
        )

    @staticmethod
    def prepare_md(input_ws_name, merged_md_name, min_log_value, max_log_value, log_step, prefix) -> str:
        """Load raw event Nexus file and reduce to MDEventWorkspace"""
        # Filter
        gef_kw_dict = dict()
        if log_step <= max_log_value - min_log_value:
            LogValueInterval = log_step
            gef_kw_dict["LogValueInterval"] = LogValueInterval
        GenerateEventsFilter(
            InputWorkspace=input_ws_name,
            OutputWorkspace="splboth",
            InformationWorkspace="info",
            UnitOfTime="Nanoseconds",
            LogName="s1",
            MinimumLogValue=min_log_value,
            MaximumLogValue=max_log_value,
            **gef_kw_dict,
        )
        FilterEvents(
            InputWorkspace=input_ws_name,
            SplitterWorkspace="splboth",
            InformationWorkspace="info",
            FilterByPulseTime=True,
            GroupWorkspaces=True,
            OutputWorkspaceIndexedFrom1=True,
            OutputWorkspaceBaseName="split",
        )
        # Clean memory
        DeleteWorkspace("splboth")
        DeleteWorkspace("info")

        reduced_ws_group = f"reduced_{prefix}"
        DgsReduction(
            SampleInputWorkspace="split",
            SampleInputMonitorWorkspace="split_1",
            IncidentEnergyGuess=50,
            SofPhiEIsDistribution=False,
            TimeIndepBackgroundSub=True,
            TibTofRangeStart=10400,
            TibTofRangeEnd=12400,
            OutputWorkspace=reduced_ws_group,
        )
        # Clean memory
        DeleteWorkspace("split")

        SetUB(Workspace=reduced_ws_group, a=5.823, b=6.475, c=3.186, u="0,1,0", v="0,0,1")
        CropWorkspaceForMDNorm(InputWorkspace=reduced_ws_group, XMin=-25, XMax=49, OutputWorkspace=reduced_ws_group)
        ConvertToMD(
            InputWorkspace=reduced_ws_group,
            QDimensions="Q3D",
            Q3DFrames="Q_sample",
            OutputWorkspace="md",
            MinValues="-11,-11,-11,-25",
            MaxValues="11,11,11,49",
        )

        if mtd["md"].getNumberOfEntries() == 1:
            RenameWorkspace(mtd["md"][0], merged_md_name)
        else:
            MergeMD(InputWorkspaces="md", OutputWorkspace=merged_md_name)

        return reduced_ws_group

    def requiredMemoryMB(self):
        return 5000

    def requiredFiles(self):
        return ["HYS_13656_event.nxs"]

    def runTest(self):
        # Set facility that load data
        config.setFacility("SNS")
        Load(Filename="HYS_13656", OutputWorkspace="sum")
        SetGoniometer(Workspace="sum", Axis0="s1,0,1,0,1")

        # prepare sample MD: in test must between 10 and 12 to match the gold data
        # **************** HERE WE USE FILTER EVENTS !!!!!!!!!!!!!
        dgs_red_group_name1 = self.prepare_md(
            input_ws_name="sum", merged_md_name="merged1", min_log_value=10, max_log_value=12, log_step=LOG_VALUE_STEP, prefix="step1"
        )

        # Prepare background workspace
        self.prepare_single_exp_info_background(input_md_name=f"{dgs_red_group_name1}_1", output_md_name="bkgd_md1", target_qframe="Q_lab")
        common_options = dict(
            InputWorkspace="merged1",
            BackgroundWorkspace="bkgd_md1",
            Dimension0Name="QDimension1",
            Dimension0Binning="-5,0.05,5",
            Dimension1Name="QDimension2",
            Dimension1Binning="-5,0.05,5",
            Dimension2Name="DeltaE",
            Dimension2Binning="-2,2",
            Dimension3Name="QDimension0",
            Dimension3Binning="-0.5,0.5",
            SymmetryOperations=SYMMETRY_OPERATION,
        )

        MDNorm(
            OutputWorkspace="clean_data",
            OutputDataWorkspace="dataMD",
            OutputNormalizationWorkspace="normMD",
            OutputBackgroundDataWorkspace="background_dataMD",
            OutputBackgroundNormalizationWorkspace="background_normMD",
            **common_options,
        )
        CloneMDWorkspace(InputWorkspace="background_normMD", OutputWorkspace="background_normMD_original")

        # Accumulation mode
        MDNorm(
            TemporaryDataWorkspace="dataMD",
            TemporaryNormalizationWorkspace="normMD",
            TemporaryBackgroundDataWorkspace="background_dataMD",
            TemporaryBackgroundNormalizationWorkspace="background_normMD",
            OutputWorkspace="clean_data_2",
            OutputDataWorkspace="dataMD2",
            OutputNormalizationWorkspace="normMD2",
            OutputBackgroundDataWorkspace="background_dataMD2",
            OutputBackgroundNormalizationWorkspace="background_normMD2",
            **common_options,
        )

        # Compare background normalization  between modes
        rescaled = 0.5 * mtd["background_normMD2"]
        r_sd = CompareMDWorkspaces(Workspace1="background_normMD_original", Workspace2=rescaled, Tolerance=1e-7)
        assert r_sd.Equals, f"Sample binned data MD not equal: {r_sd.Result}"

    def validate(self):
        self.tolerance = 1e-5
        test_ws_name = "background_normMD_original"
        return test_ws_name, "MDNormBackgroundHYSPEC.nxs"
