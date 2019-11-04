# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)

from mantid.api import (AnalysisDataService, WorkspaceGroup)
from mantid.simpleapi import (CompareWorkspaces, IndirectQuickRun, LoadNexus)
import systemtesting
import unittest


def exists_in_ads(workspace_name):
    return AnalysisDataService.doesExist(workspace_name)


def get_ads_workspace(workspace_name):
    return AnalysisDataService.retrieve(workspace_name) if exists_in_ads(workspace_name) else None


class IndirectQuickRunTest(unittest.TestCase):

    def setUp(self):
        self._run_numbers = '92762-92766'
        self._instrument = 'OSIRIS'
        self._analyser = 'graphite'
        self._reflection = '002'
        self._spectra_range = '963,980'
        self._elastic_range = '-0.02,0.02'
        self._inelastic_range = '0.4,0.5'
        self._total_range = '-0.5,0.5'

    def tearDown(self):
        AnalysisDataService.clear()

    def test_that_IndirectQuickRun_produces_output_workspaces_with_the_correct_names(self):
        self._execute_IndirectQuickRun()

        self.assertTrue(exists_in_ads('osiris92762_to_osiris92766_scan_eisf'))
        self.assertTrue(exists_in_ads('osiris92762_to_osiris92766_scan_q'))

    def test_that_IndirectQuickRun_performs_an_energy_window_scan_and_produces_a_workspace_with_the_correct_size(self):
        self._execute_IndirectQuickRun()

        scan_group = get_ads_workspace('osiris92762_to_osiris92766_scan_q')

        self.assertTrue(isinstance(scan_group, WorkspaceGroup))
        self.assertEqual(scan_group.getNumberOfEntries(), 12)

    def test_that_IndirectQuickRun_produces_the_correct_eisf_workspace(self):
        self._execute_IndirectQuickRun()
        self._assert_equal_to_reference_file('osiris92762_to_osiris92766_scan_eisf')

    def test_that_IndirectQuickRun_produces_the_correct_workspaces_when_doing_an_MSDFit(self):
        self._execute_IndirectQuickRun(msd_fit=True)

        self.assertTrue(exists_in_ads('osiris92762_to_osiris92766_scan_msd'))
        self.assertTrue(exists_in_ads('osiris92762_to_osiris92766_scan_msd_Parameters'))
        self.assertTrue(exists_in_ads('osiris92762_to_osiris92766_scan_msd_fit'))

        msd_fit_group = get_ads_workspace('osiris92762_to_osiris92766_scan_msd_fit')
        self.assertTrue(isinstance(msd_fit_group, WorkspaceGroup))
        self.assertEqual(msd_fit_group.getNumberOfEntries(), 5)

        self._assert_equal_to_reference_file('osiris92762_to_osiris92766_scan_msd')

    def test_that_IndirectQuickRun_produces_the_correct_workspaces_when_doing_a_WidthFit(self):
        self._execute_IndirectQuickRun(width_fit=True)

        self.assertTrue(exists_in_ads('osiris92762_to_osiris92766_scan_red_Width1'))
        self.assertTrue(exists_in_ads('osiris92762_to_osiris92766_scan_red_Diffusion'))
        self.assertTrue(exists_in_ads('osiris92762_to_osiris92766_scan_red_Width_Fit'))

        self._assert_equal_to_reference_file('osiris92762_to_osiris92766_scan_red_Width1',
                                             'osiris92762_to_osiris92766_scan_red_Width')
        self._assert_equal_to_reference_file('osiris92762_to_osiris92766_scan_red_Diffusion')

        width_fit_group = get_ads_workspace('osiris92762_to_osiris92766_scan_red_Width_Fit')
        self.assertTrue(isinstance(width_fit_group, WorkspaceGroup))
        self.assertEqual(width_fit_group.getNumberOfEntries(), 12)

    def _execute_IndirectQuickRun(self, msd_fit=False, width_fit=False):
        IndirectQuickRun(InputFiles=self._run_numbers, Instrument=self._instrument, Analyser=self._analyser,
                         Reflection=self._reflection, SpectraRange=self._spectra_range,
                         ElasticRange=self._elastic_range, InelasticRange=self._inelastic_range,
                         TotalRange=self._total_range, MSDFit=msd_fit, WidthFit=width_fit)

    def _assert_equal_to_reference_file(self, output_name, reference_name=None):
        reference_name = output_name if reference_name is None else reference_name
        expected_workspace = LoadNexus(Filename='IndirectQuickRun_' + reference_name + '.nxs')
        self.assertTrue(CompareWorkspaces(Workspace1=get_ads_workspace(output_name), Workspace2=expected_workspace,
                                          Tolerance=5.0, ToleranceRelErr=True)[0])


class IndirectQuickRunTestRunner(systemtesting.MantidSystemTest):
    def __init__(self):
        systemtesting.MantidSystemTest.__init__(self)
        self._success = False

    def runTest(self):
        suite = unittest.TestSuite()
        suite.addTest(unittest.makeSuite(IndirectQuickRunTest, 'test'))
        runner = unittest.TextTestRunner()
        res = runner.run(suite)
        if res.wasSuccessful():
            self._success = True

    def requiredMemoryMB(self):
        return 2000

    def validate(self):
        return self._success
