# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=too-many-public-methods,invalid-name
import unittest
from mantid.api import WorkspaceGroup
from mantid.simpleapi import CompareWorkspaces, CreateSimulationWorkspace, CreateWorkspace, DeleteWorkspace, ISISIndirectEnergyTransfer

import numpy as np


def _generate_calibration_workspace(instrument, ws_name="__fake_calib"):
    inst = CreateSimulationWorkspace(Instrument=instrument, BinParams="0,1,2")
    n_hist = inst.getNumberHistograms()

    fake_calib = CreateWorkspace(OutputWorkspace=ws_name, DataX=np.ones(1), DataY=np.ones(n_hist), NSpec=n_hist, ParentWorkspace=inst)

    DeleteWorkspace(inst)

    return fake_calib


class ISISIndirectEnergyTransferTest(unittest.TestCase):
    def test_basic_reduction(self):
        """
        Sanity test to ensure the most basic reduction actually completes.
        """

        wks = ISISIndirectEnergyTransfer(
            InputFiles=["IRS26176.RAW"], Instrument="IRIS", Analyser="graphite", Reflection="002", SpectraRange=[3, 53]
        )

        self.assertTrue(isinstance(wks, WorkspaceGroup), "Result workspace should be a workspace group.")
        self.assertEqual(wks.getNames()[0], "iris26176_graphite002_red")

        red_ws = wks.getItem(0)
        self.assertEqual(red_ws.getNumberHistograms(), 51)
        self.assertEqual(red_ws.getAxis(0).getUnit().unitID(), "DeltaE")

    def test_reduction_with_range(self):
        """
        Sanity test to ensure a reduction with a spectra range completes.
        """

        wks = ISISIndirectEnergyTransfer(
            InputFiles=["IRS26176.RAW"], Instrument="IRIS", Analyser="graphite", Reflection="002", SpectraRange=[35, 40]
        )

        self.assertTrue(isinstance(wks, WorkspaceGroup), "Result workspace should be a workspace group.")
        self.assertEqual(wks.getNames()[0], "iris26176_graphite002_red")

        red_ws = wks.getItem(0)
        self.assertEqual(red_ws.getNumberHistograms(), 6)

    def test_grouping_all(self):
        """
        Tests setting the grouping policy to all.
        """

        wks = ISISIndirectEnergyTransfer(
            InputFiles=["IRS26176.RAW"],
            Instrument="IRIS",
            Analyser="graphite",
            Reflection="002",
            SpectraRange=[3, 53],
            GroupingMethod="All",
        )

        self.assertTrue(isinstance(wks, WorkspaceGroup), "Result workspace should be a workspace group.")
        self.assertEqual(wks.getNames()[0], "iris26176_graphite002_red")

        red_ws = wks.getItem(0)
        self.assertEqual(red_ws.getNumberHistograms(), 1)

    def test_grouping_individual(self):
        """
        Tests setting the grouping policy to individual.
        """

        wks = ISISIndirectEnergyTransfer(
            InputFiles=["IRS26176.RAW"],
            Instrument="IRIS",
            Analyser="graphite",
            Reflection="002",
            SpectraRange=[3, 53],
            GroupingMethod="Individual",
        )

        self.assertTrue(isinstance(wks, WorkspaceGroup), "Result workspace should be a workspace group.")
        self.assertEqual(wks.getNames()[0], "iris26176_graphite002_red")

        red_ws = wks.getItem(0)
        self.assertEqual(red_ws.getNumberHistograms(), 51)

    def test_ISISIndirectEnergyTransfer_with_different_custom_groupings_creates_a_workspace_with_the_correct_size(self):
        custom_grouping_strings = {"3:53": 51, "3:25,27:53": 50, "3-53": 1, "3-25,26:53": 29, "3+5+7,8-40,41:53": 15}

        for custom_string, expected_size in custom_grouping_strings.items():
            reduced_workspace = ISISIndirectEnergyTransfer(
                InputFiles=["IRS26176.RAW"],
                Instrument="IRIS",
                Analyser="graphite",
                Reflection="002",
                SpectraRange=[3, 53],
                GroupingMethod="Custom",
                GroupingString=custom_string,
            )

            self.assertEqual(reduced_workspace.getItem(0).getNumberHistograms(), expected_size)

    def test_ISISIndirectEnergyTransfer_with_different_number_of_groups(self):
        # Non-divisible numbers will have an extra group with the remaining detectors
        results = {5: 5, 10: 10, 4: 5, 12: 13}
        spectra_min, spectra_max = 3, 52

        for number_of_groups, expected_size in results.items():
            reduced_workspace = ISISIndirectEnergyTransfer(
                InputFiles=["IRS26176.RAW"],
                Instrument="IRIS",
                Analyser="graphite",
                Reflection="002",
                SpectraRange=[spectra_min, spectra_max],
                GroupingMethod="Groups",
                NGroups=number_of_groups,
            )

            workspace = reduced_workspace.getItem(0)
            self.assertEqual(expected_size, workspace.getNumberHistograms())

            n_detectors_per_group = (spectra_max - spectra_min + 1) // number_of_groups
            for spec_i in range(number_of_groups):
                self.assertEqual(n_detectors_per_group, len(workspace.getSpectrum(spec_i).getDetectorIDs()))

    def test_reduction_with_background_subtraction(self):
        """
        Tests running a reduction with a background subtraction.
        """

        wks = ISISIndirectEnergyTransfer(
            InputFiles=["IRS26176.RAW"],
            Instrument="IRIS",
            Analyser="graphite",
            Reflection="002",
            SpectraRange=[3, 53],
            BackgroundRange=[70000, 75000],
        )

        self.assertTrue(isinstance(wks, WorkspaceGroup), "Result workspace should be a workspace group.")
        self.assertEqual(wks.getNames()[0], "iris26176_graphite002_red")

    def test_reduction_with_output_unit(self):
        """
        Tests creating reduction in different X units.
        """

        wks = ISISIndirectEnergyTransfer(
            InputFiles=["IRS26176.RAW"],
            Instrument="IRIS",
            Analyser="graphite",
            Reflection="002",
            SpectraRange=[3, 53],
            UnitX="DeltaE_inWavenumber",
        )

        self.assertTrue(isinstance(wks, WorkspaceGroup), "Result workspace should be a workspace group.")
        self.assertEqual(wks.getNames()[0], "iris26176_graphite002_red")

        red_ws = wks.getItem(0)
        self.assertEqual(red_ws.getNumberHistograms(), 51)
        self.assertEqual(red_ws.getAxis(0).getUnit().unitID(), "DeltaE_inWavenumber")

    def test_reduction_with_detailed_balance(self):
        """
        Sanity test to ensure a reduction using detailed balance option
        completes.
        """

        wks = ISISIndirectEnergyTransfer(
            InputFiles=["IRS26176.RAW"],
            Instrument="IRIS",
            Analyser="graphite",
            Reflection="002",
            SpectraRange=[3, 53],
            DetailedBalance="300",
        )

        self.assertTrue(isinstance(wks, WorkspaceGroup), "Result workspace should be a workspace group.")
        self.assertEqual(wks.getNames()[0], "iris26176_graphite002_red")

        red_ws = wks.getItem(0)
        self.assertEqual(red_ws.getNumberHistograms(), 51)

    def test_reduction_with_map_file(self):
        """
        Sanity test to ensure a reduction using a mapping/grouping file
        completes.
        """

        wks = ISISIndirectEnergyTransfer(
            InputFiles=["OSI97919.raw"],
            Instrument="OSIRIS",
            Analyser="graphite",
            Reflection="002",
            SpectraRange=[963, 1004],
            GroupingMethod="File",
            GroupingFile="osi_002_14Groups.map",
        )

        self.assertTrue(isinstance(wks, WorkspaceGroup), "Result workspace should be a workspace group.")
        self.assertEqual(wks.getNames()[0], "osiris97919_graphite002_red")

        red_ws = wks.getItem(0)
        self.assertEqual(red_ws.getNumberHistograms(), 14)

    def test_reduction_with_calibration(self):
        """
        Sanity test to ensure a reduction using a calibration workspace
        completes.
        """

        wks = ISISIndirectEnergyTransfer(
            InputFiles=["IRS26176.RAW"],
            Instrument="IRIS",
            Analyser="graphite",
            Reflection="002",
            SpectraRange=[3, 53],
            CalibrationWorkspace=_generate_calibration_workspace("IRIS"),
        )

        self.assertTrue(isinstance(wks, WorkspaceGroup), "Result workspace should be a workspace group.")
        self.assertEqual(wks.getNames()[0], "iris26176_graphite002_red")

        red_ws = wks.getItem(0)
        self.assertEqual(red_ws.getNumberHistograms(), 51)

    def test_reduction_with_calibration_and_range(self):
        """
        Sanity test to ensure a reduction using a calibration workspace
        completes.
        """

        wks = ISISIndirectEnergyTransfer(
            InputFiles=["IRS26176.RAW"],
            Instrument="IRIS",
            Analyser="graphite",
            Reflection="002",
            SpectraRange=[35, 40],
            CalibrationWorkspace=_generate_calibration_workspace("IRIS"),
        )

        self.assertTrue(isinstance(wks, WorkspaceGroup), "Result workspace should be a workspace group.")
        self.assertEqual(wks.getNames()[0], "iris26176_graphite002_red")

        red_ws = wks.getItem(0)
        self.assertEqual(red_ws.getNumberHistograms(), 6)

    def test_multi_files(self):
        """
        Test reducing multiple files.
        """

        wks = ISISIndirectEnergyTransfer(
            InputFiles=["IRS26176.RAW", "IRS26173.RAW"], Instrument="IRIS", Analyser="graphite", Reflection="002", SpectraRange=[3, 53]
        )

        self.assertTrue(isinstance(wks, WorkspaceGroup), "Result workspace should be a workspace group.")
        self.assertEqual(len(wks), 2)
        self.assertEqual(wks.getNames()[0], "iris26176_graphite002_red")
        self.assertEqual(wks.getNames()[1], "iris26173_graphite002_red")

    def test_sum_files(self):
        """
        Test summing multiple runs.
        """

        wks = ISISIndirectEnergyTransfer(
            InputFiles=["IRS26176.RAW", "IRS26173.RAW"],
            SumFIles=True,
            Instrument="IRIS",
            Analyser="graphite",
            Reflection="002",
            SpectraRange=[3, 53],
        )

        self.assertTrue(isinstance(wks, WorkspaceGroup), "Result workspace should be a workspace group.")
        self.assertEqual(len(wks), 1)
        self.assertEqual(wks.getNames()[0], "iris26173-26176_multi_graphite002_red")

        red_ws = wks[0]
        self.assertTrue("multi_run_reduction" in red_ws.getRun())
        self.assertEqual(red_ws.getRun().get("run_number").value, "26176,26173")

    def test_instrument_validation_failure(self):
        """
        Tests that an invalid instrument configuration causes the validation to
        fail.
        """

        self.assertRaisesRegex(
            RuntimeError,
            "Invalid instrument configuration",
            ISISIndirectEnergyTransfer,
            OutputWorkspace="__ISISIndirectEnergyTransferTest_ws",
            InputFiles=["IRS26176.RAW"],
            Instrument="IRIS",
            Analyser="graphite",
            Reflection="006",
            SpectraRange=[3, 53],
        )

    def test_group_workspace_validation_failure(self):
        """
        Tests that validation fails when Workspace is selected as the
        GroupingMethod but no workspace is provided.
        """

        self.assertRaisesRegex(
            RuntimeError,
            "Must select a grouping workspace for current GroupingWorkspace",
            ISISIndirectEnergyTransfer,
            OutputWorkspace="__ISISIndirectEnergyTransferTest_ws",
            InputFiles=["IRS26176.RAW"],
            Instrument="IRIS",
            Analyser="graphite",
            Reflection="002",
            SpectraRange=[3, 53],
            GroupingMethod="Workspace",
        )

    def test_reduction_with_manual_efixed(self):
        """
        Sanity test to ensure a reduction with a manual Efixed value completes.
        """

        wks = ISISIndirectEnergyTransfer(
            InputFiles=["IRS26176.RAW"], Instrument="IRIS", Analyser="graphite", Reflection="002", SpectraRange=[3, 53], Efixed=1.9
        )

        self.assertTrue(isinstance(wks, WorkspaceGroup), "Result workspace should be a workspace group.")
        self.assertEqual(wks.getNames()[0], "iris26176_graphite002_red")

        red_ws = wks.getItem(0)
        self.assertEqual(red_ws.getNumberHistograms(), 51)

    def test_reduction_with_manual_efixed_same_as_default(self):
        """
        Sanity test to ensure that manually setting the default Efixed value
        gives the same results as not providing it.
        """

        ref = ISISIndirectEnergyTransfer(
            InputFiles=["IRS26176.RAW"], Instrument="IRIS", Analyser="graphite", Reflection="002", SpectraRange=[3, 53]
        )

        wks = ISISIndirectEnergyTransfer(
            InputFiles=["IRS26176.RAW"], Instrument="IRIS", Analyser="graphite", Reflection="002", SpectraRange=[3, 53], Efixed=1.845
        )

        self.assertTrue(CompareWorkspaces(ref, wks)[0])

    def test_reduction_with_can_scale(self):
        """
        Sanity check tio ensure a reduction with can scale value completes.
        """

        wks = ISISIndirectEnergyTransfer(
            InputFiles=["IRS26176.RAW"], Instrument="IRIS", Analyser="graphite", Reflection="002", SpectraRange=[3, 53], ScaleFactor=0.5
        )

        red_ws = wks.getItem(0)
        self.assertEqual(red_ws.getNumberHistograms(), 51)

    def test_change_spectra_for_groups(self):
        """Check that the spectra are changed for a custom grouping"""

        custom_grouping_strings = {"3:53": 51, "3:25,27:53": 50, "3-53": 1, "3-25,26:53": 29, "3+5+7,8-40,41:53": 15}

        for custom_string, expected_size in custom_grouping_strings.items():
            reduced_workspace = ISISIndirectEnergyTransfer(
                InputFiles=["IRS26176.RAW"],
                Instrument="IRIS",
                Analyser="graphite",
                Reflection="002",
                SpectraRange=[3, 53],
                GroupingMethod="Custom",
                GroupingString=custom_string,
            )
        red_ws = reduced_workspace.getItem(0)
        self.assertEqual(red_ws.getSpectrum(0).getSpectrumNo(), 0)

    def test_change_spectra_for_individual(self):
        """Check that the spectra are changed for a custom grouping"""

        wks = ISISIndirectEnergyTransfer(
            InputFiles=["IRS26176.RAW"],
            Instrument="IRIS",
            Analyser="graphite",
            Reflection="002",
            SpectraRange=[3, 53],
            GroupingMethod="Individual",
        )

        self.assertTrue(isinstance(wks, WorkspaceGroup), "Result workspace should be a workspace group.")
        self.assertEqual(wks.getNames()[0], "iris26176_graphite002_red")

        red_ws = wks.getItem(0)
        self.assertEqual(red_ws.getSpectrum(0).getSpectrumNo(), 0)

    def test_reduction_with_suffix(self):
        """Check that the output suffix is appended at the end of output workspace names"""

        wks = ISISIndirectEnergyTransfer(
            InputFiles=["IRS26176.RAW"],
            Instrument="IRIS",
            Analyser="graphite",
            Reflection="002",
            SpectraRange=[3, 53],
            OutputWorkspace="OutputGroup",
            OutputSuffix="test",
        )

        self.assertTrue(isinstance(wks, WorkspaceGroup), "Result workspace should be a workspace group.")
        self.assertEqual(wks.name(), "OutputGroup")
        self.assertEqual(wks.getNames()[0], "iris26176_graphite002_test_red")


if __name__ == "__main__":
    unittest.main()
