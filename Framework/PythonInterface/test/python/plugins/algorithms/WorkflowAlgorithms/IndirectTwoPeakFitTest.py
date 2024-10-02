# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantid.api import AnalysisDataService, MatrixWorkspace, WorkspaceGroup
from mantid.simpleapi import EnergyWindowScan, IndirectTwoPeakFit, LoadNexus
from mantid.testing import assert_almost_equal as assert_wksp_almost_equal

import unittest


def exists_in_ads(workspace_name):
    return AnalysisDataService.doesExist(workspace_name)


def get_ads_workspace(workspace_name):
    return AnalysisDataService.retrieve(workspace_name) if exists_in_ads(workspace_name) else None


class IndirectTwoPeakFitTest(unittest.TestCase):
    def setUp(self):
        EnergyWindowScan(
            InputFiles="OSIRIS92762",
            Instrument="OSIRIS",
            Analyser="graphite",
            Reflection="002",
            SpectraRange="963,980",
            ElasticRange="-0.02,0.02",
            InelasticRange="0.4,0.5",
            TotalRange="-0.5,0.5",
            ReducedWorkspace="__reduced_group",
            ScanWorkspace="__scan_workspace",
        )

        self._red_name = "osiris92762_graphite002_red"
        self._output_name = "osiris92762_graphite002_two_peak_fit"

        self._x_min = -0.5
        self._x_max = 0.5

        self._execute_IndirectTwoPeakFit()

    def tearDown(self):
        AnalysisDataService.clear()

    def test_that_IndirectTwoPeakFit_produces_output_workspaces_with_the_correct_names(self):
        self.assertTrue(exists_in_ads("osiris92762_graphite002_two_peak_fit_1L_Workspaces"))
        self.assertTrue(exists_in_ads("osiris92762_graphite002_two_peak_fit_2L_Workspaces"))
        self.assertTrue(exists_in_ads("osiris92762_graphite002_two_peak_fit_Result"))
        self.assertTrue(exists_in_ads("osiris92762_graphite002_two_peak_fit_ChiSq"))

    def test_that_IndirectTwoPeakFit_produces_the_correct_number_of_fit_workspaces(self):
        one_lorentzian_group = get_ads_workspace("osiris92762_graphite002_two_peak_fit_1L_Workspaces")
        two_lorentzians_group = get_ads_workspace("osiris92762_graphite002_two_peak_fit_2L_Workspaces")

        self.assertTrue(isinstance(one_lorentzian_group, WorkspaceGroup))
        self.assertEqual(one_lorentzian_group.getNumberOfEntries(), 17)
        self.assertTrue(isinstance(two_lorentzians_group, WorkspaceGroup))
        self.assertEqual(two_lorentzians_group.getNumberOfEntries(), 17)

    def test_that_IndirectTwoPeakFit_produces_a_result_workspace_with_the_correct_y_labels(self):
        result_workspace = get_ads_workspace("osiris92762_graphite002_two_peak_fit_Result")

        self.assertTrue(isinstance(result_workspace, MatrixWorkspace))
        y_axis = result_workspace.getAxis(1)
        self.assertEqual(y_axis.label(0), "fwhm.1")
        self.assertEqual(y_axis.label(1), "fwhm.2.1")
        self.assertEqual(y_axis.label(2), "fwhm.2.2")

    def test_that_IndirectTwoPeakFit_produces_a_chi_squared_workspace_with_the_correct_y_labels(self):
        chi_squared_workspace = get_ads_workspace("osiris92762_graphite002_two_peak_fit_ChiSq")

        self.assertTrue(isinstance(chi_squared_workspace, MatrixWorkspace))
        y_axis = chi_squared_workspace.getAxis(1)
        self.assertEqual(y_axis.label(0), "1 peak")
        self.assertEqual(y_axis.label(1), "2 peaks")

    def test_that_IndirectTwoPeakFit_produces_a_result_and_chi_squared_workspace_with_correct_values(self):
        self._assert_equal_to_reference_file("osiris92762_graphite002_two_peak_fit_Result")
        self._assert_equal_to_reference_file("osiris92762_graphite002_two_peak_fit_ChiSq")

    def _execute_IndirectTwoPeakFit(self):
        IndirectTwoPeakFit(SampleWorkspace=self._red_name, EnergyMin=self._x_min, EnergyMax=self._x_max, OutputName=self._output_name)

    def _assert_equal_to_reference_file(self, output_name):
        expected_workspace = LoadNexus(Filename="IndirectTwoPeakFit_" + output_name + ".nxs")
        assert_wksp_almost_equal(
            Workspace1=get_ads_workspace(output_name),
            Workspace2=expected_workspace,
            rtol=5.0,
            CheckUncertainty=False,
        )


if __name__ == "__main__":
    unittest.main()
