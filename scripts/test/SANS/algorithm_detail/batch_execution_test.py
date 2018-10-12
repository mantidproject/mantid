# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)
import unittest
import sys
from sans.algorithm_detail.batch_execution import get_all_names_to_save, ReductionPackage
from mantid.simpleapi import CreateSampleWorkspace
if sys.version_info.major > 2:
    from unittest import mock
else:
    import mock


class GetAllNamesToSaveTest(unittest.TestCase):
    def setUp(self):
        state = mock.MagicMock()
        workspaces = ['Sample', 'Transmission', 'Direct']
        monitors = ['monitor1']
        self.reduction_package_merged = ReductionPackage(state, workspaces, monitors)
        self.reduction_package = ReductionPackage(state, workspaces, monitors)
        merged_workspace = CreateSampleWorkspace(Function='Flat background', NumBanks=1, BankPixelWidth=1, NumEvents=1,
                                                 XMin=1, XMax=14, BinWidth=2)
        lab_workspace = CreateSampleWorkspace(Function='Flat background', NumBanks=1, BankPixelWidth=1, NumEvents=1,
                                              XMin=1, XMax=14, BinWidth=2)
        hab_workspace = CreateSampleWorkspace(Function='Flat background', NumBanks=1, BankPixelWidth=1, NumEvents=1,
                                              XMin=1, XMax=14, BinWidth=2)
        reduced_lab_can = CreateSampleWorkspace(Function='Flat background', NumBanks=1, BankPixelWidth=1, NumEvents=1,
                                                XMin=1, XMax=14, BinWidth=2)
        reduced_hab_can = CreateSampleWorkspace(Function='Flat background', NumBanks=1, BankPixelWidth=1, NumEvents=1,
                                                XMin=1, XMax=14, BinWidth=2)
        self.reduction_package_merged.reduced_merged = merged_workspace
        self.reduction_package_merged.reduced_lab = lab_workspace
        self.reduction_package_merged.reduced_hab = hab_workspace
        self.reduction_package_merged.reduced_lab_can = reduced_lab_can
        self.reduction_package_merged.reduced_hab_can = reduced_hab_can

        self.reduction_package.reduced_lab = lab_workspace
        self.reduction_package.reduced_hab = hab_workspace
        self.reduction_package.reduced_lab_can = reduced_lab_can
        self.reduction_package.reduced_hab_can = reduced_hab_can

    def test_returns_merged_name_if_present(self):
        reduction_packages = [self.reduction_package_merged]
        names_to_save = get_all_names_to_save(reduction_packages)

        self.assertEqual(names_to_save, {'merged_workspace'})

    def test_hab_and_lab_workspaces_returned_if_merged_workspace_not_present(self):
        reduction_packages = [self.reduction_package]
        names_to_save = get_all_names_to_save(reduction_packages)

        self.assertEqual(names_to_save, {'lab_workspace', 'hab_workspace'})

    def test_can_workspaces_returned_if_save_can_selected(self):
        reduction_packages = [self.reduction_package_merged]
        names_to_save = get_all_names_to_save(reduction_packages, True)

        self.assertEqual(names_to_save, {'merged_workspace', 'lab_workspace', 'hab_workspace',
                                         'reduced_lab_can', 'reduced_hab_can'})

    def test_can_workspaces_returned_if_save_can_selected_no_merged(self):
        reduction_packages = [self.reduction_package]
        names_to_save = get_all_names_to_save(reduction_packages, True)

        self.assertEqual(names_to_save, {'lab_workspace', 'hab_workspace',
                                         'reduced_lab_can', 'reduced_hab_can'})


if __name__ == '__main__':
    unittest.main()
