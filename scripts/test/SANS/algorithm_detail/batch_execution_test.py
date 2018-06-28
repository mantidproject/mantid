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
    def test_returns_merged_name_if_present(self):
        state = mock.MagicMock()
        workspaces = ['Sample', 'Transmission', 'Direct']
        monitors = ['monitor1']
        reduction_package = ReductionPackage(state, workspaces, monitors)
        merged_workspace = CreateSampleWorkspace(Function='Flat background', NumBanks=1, BankPixelWidth=1, NumEvents=1,
                                   XMin=1, XMax=14, BinWidth=2)
        lab_workspace = CreateSampleWorkspace(Function='Flat background', NumBanks=1, BankPixelWidth=1, NumEvents=1,
                                   XMin=1, XMax=14, BinWidth=2)
        hab_workspace = CreateSampleWorkspace(Function='Flat background', NumBanks=1, BankPixelWidth=1, NumEvents=1,
                                   XMin=1, XMax=14, BinWidth=2)
        reduction_package.reduced_merged = merged_workspace
        reduction_package.reduced_lab = lab_workspace
        reduction_package.reduced_hab = hab_workspace
        reduction_packages = [reduction_package]

        names_to_save = get_all_names_to_save(reduction_packages)

        self.assertEqual(names_to_save, set(['merged_workspace']))

    def test_hab_and_lab_workspaces_returned_if_merged_workspace_not_present(self):
        state = mock.MagicMock()
        workspaces = ['Sample', 'Transmission', 'Direct']
        monitors = ['monitor1']
        reduction_package = ReductionPackage(state, workspaces, monitors)
        lab_workspace = CreateSampleWorkspace(Function='Flat background', NumBanks=1, BankPixelWidth=1, NumEvents=1,
                                              XMin=1, XMax=14, BinWidth=2)
        hab_workspace = CreateSampleWorkspace(Function='Flat background', NumBanks=1, BankPixelWidth=1, NumEvents=1,
                                              XMin=1, XMax=14, BinWidth=2)
        reduction_package.reduced_lab = lab_workspace
        reduction_package.reduced_hab = hab_workspace
        reduction_packages = [reduction_package]

        names_to_save = get_all_names_to_save(reduction_packages)

        self.assertEqual(names_to_save, set(['lab_workspace', 'hab_workspace']))

if __name__ == '__main__':
    unittest.main()
