# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)

import unittest

from mantid.simpleapi import CreateSampleWorkspace
from mantid.py3compat import mock
from sans.algorithm_detail.batch_execution import (get_all_names_to_save, get_transmission_names_to_save,
                                                   ReductionPackage, select_reduction_alg)


class ADSMock(object):
    """
    An object to mock out the ADS
    """
    class GroupWS(object):
        def contains(self, _):
            return True

    def __init__(self, inADS):
        self.inADS = inADS

    def doesExist(self, _):
        if self.inADS:
            return True
        else:
            return False

    def retrieve(self, _):
        return ADSMock.GroupWS()


class GetAllNamesToSaveTest(unittest.TestCase):
    def setUp(self):
        state = mock.MagicMock()
        workspaces = ['Sample', 'Transmission', 'Direct']
        monitors = ['monitor1']
        self.reduction_package_merged = ReductionPackage(state, workspaces, monitors)
        self.reduction_package = ReductionPackage(state, workspaces, monitors)
        self.reduction_package_transmissions = ReductionPackage(state, workspaces, monitors)
        self.reduction_package_transmissions.unfitted_transmission_base_name = 'Base'
        self.reduction_package_transmissions.unfitted_transmission_name = 'transmission'
        self.reduction_package_transmissions.unfitted_transmission_can_base_name = 'Base'
        self.reduction_package_transmissions.unfitted_transmission_can_name = 'transmission_can'

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
        reduced_lab_sample = CreateSampleWorkspace(Function='Flat background', NumBanks=1, BankPixelWidth=1, NumEvents=1,
                                                XMin=1, XMax=14, BinWidth=2)
        reduced_hab_sample = CreateSampleWorkspace(Function='Flat background', NumBanks=1, BankPixelWidth=1, NumEvents=1,
                                                XMin=1, XMax=14, BinWidth=2)
        transmission = CreateSampleWorkspace(Function='Flat background', NumBanks=1, BankPixelWidth=1,
                                                   NumEvents=1,
                                                   XMin=1, XMax=14, BinWidth=2)
        transmission_can = CreateSampleWorkspace(Function='Flat background', NumBanks=1, BankPixelWidth=1,
                                                   NumEvents=1,
                                                   XMin=1, XMax=14, BinWidth=2)

        self.reduction_package_merged.reduced_merged = merged_workspace
        self.reduction_package_merged.reduced_lab = lab_workspace
        self.reduction_package_merged.reduced_hab = hab_workspace
        self.reduction_package_merged.reduced_lab_can = reduced_lab_can
        self.reduction_package_merged.reduced_hab_can = reduced_hab_can
        self.reduction_package_merged.reduced_lab_sample = reduced_lab_sample
        self.reduction_package_merged.reduced_hab_sample = reduced_hab_sample
        self.reduction_package_merged.unfitted_transmission = ''
        self.reduction_package_merged.unfitted_transmission_can = ''

        self.reduction_package.reduced_lab = lab_workspace
        self.reduction_package.reduced_hab = hab_workspace
        self.reduction_package.reduced_lab_can = reduced_lab_can
        self.reduction_package.reduced_hab_can = reduced_hab_can
        self.reduction_package.reduced_lab_sample = reduced_lab_sample
        self.reduction_package.reduced_hab_sample = reduced_hab_sample
        self.reduction_package.unfitted_transmission = ''
        self.reduction_package.unfitted_transmission_can = ''

        self.reduction_package_transmissions.reduced_lab = lab_workspace
        self.reduction_package_transmissions.reduced_hab = hab_workspace
        self.reduction_package_transmissions.reduced_lab_can = reduced_lab_can
        self.reduction_package_transmissions.reduced_hab_can = reduced_hab_can
        self.reduction_package_transmissions.reduced_lab_sample = reduced_lab_sample
        self.reduction_package_transmissions.reduced_hab_sample = reduced_hab_sample
        self.reduction_package_transmissions.unfitted_transmission = transmission
        self.reduction_package_transmissions.unfitted_transmission_can = transmission_can

    def test_returns_merged_name_if_present(self):
        reduction_packages = [self.reduction_package_merged]
        names_to_save = get_all_names_to_save(reduction_packages, save_can=False)

        self.assertEqual(names_to_save, {('merged_workspace', '', '')})

    def test_hab_and_lab_workspaces_returned_if_merged_workspace_not_present(self):
        reduction_packages = [self.reduction_package]
        names_to_save = get_all_names_to_save(reduction_packages, save_can=False)

        self.assertEqual(names_to_save, {('lab_workspace', '', ''), ('hab_workspace', '', '')})

    def test_can_workspaces_returned_if_save_can_selected(self):
        reduction_packages = [self.reduction_package_merged]
        names_to_save = get_all_names_to_save(reduction_packages, True)
        names = {'merged_workspace', 'lab_workspace', 'hab_workspace',
                 'reduced_lab_can', 'reduced_hab_can',
                'reduced_lab_sample', 'reduced_hab_sample'}
        names_expected = set([(name, '', '') for name in names])
        self.assertEqual(names_to_save, names_expected)

    def test_can_workspaces_returned_if_save_can_selected_no_merged(self):
        reduction_packages = [self.reduction_package]
        names_to_save = get_all_names_to_save(reduction_packages, True)

        names = {'lab_workspace', 'hab_workspace',
                 'reduced_lab_can', 'reduced_hab_can',
                 'reduced_lab_sample', 'reduced_hab_sample'}
        names_expected = set([(name, '', '') for name in names])
        self.assertEqual(names_to_save, names_expected)

    def test_that_empty_string_returned_if_transmission_base_name_is_none(self):
        # Test when base name is None
        reduction_package = self.reduction_package
        reduction_package.unfitted_transmission_base_name = None
        returned_name = get_transmission_names_to_save(reduction_package, False)
        self.assertEqual(returned_name, '', "Should have returned an empty string because "
                                            "transmission sample base name was None. "
                                            "Returned {} instead".format(returned_name))

        # Test when base name is not None but name is None
        reduction_package.unfitted_transmission_base_name = 'Base'
        reduction_package.unfitted_transmission_name = None
        returned_name = get_transmission_names_to_save(reduction_package, False)
        self.assertEqual(returned_name, '', "Should have returned an empty string because "
                                            "transmission sample name was None. "
                                            "Returned {} instead".format(returned_name))

    @mock.patch("sans.algorithm_detail.batch_execution.AnalysisDataService", new=ADSMock(False))
    def test_no_transmission_workspace_names_returned_if_not_in_ADS(self):
        # Check transmission sample
        returned_name = get_transmission_names_to_save(self.reduction_package_transmissions, False)
        self.assertEqual(returned_name, '', "Should have returned an empty string because "
                                            "transmission sample was not in the ADS. "
                                            "Returned {} instead.".format(returned_name))

        # Check transmission_can
        returned_name = get_transmission_names_to_save(self.reduction_package_transmissions, True)
        self.assertEqual(returned_name, '', "Should have returned an empty string because "
                                            "transmission can was not in the ADS. "
                                            "Returned {} instead".format(returned_name))

    @mock.patch("sans.algorithm_detail.batch_execution.AnalysisDataService", new=ADSMock(True))
    def test_transmission_workspace_names_from_reduction_package_are_returned_if_in_ADS(self):
        reduction_package = self.reduction_package_transmissions
        reduction_package.unfitted_transmission_base_name = 'Base'
        reduction_package.unfitted_transmission_name = 'transmission'
        reduction_package.unfitted_transmission_can_base_name = 'Base'
        reduction_package.unfitted_transmission_can_name = 'transmission_can'

        # Check transmission sample
        returned_name = get_transmission_names_to_save(reduction_package, False)
        self.assertEqual(returned_name, 'transmission', "Should have transmission as name because "
                                                        "transmission sample was in the ADS. "
                                                        "Returned {} instead.".format(returned_name))

        # Check transmission_can
        returned_name = get_transmission_names_to_save(reduction_package, True)
        self.assertEqual(returned_name, 'transmission_can', "Should have returned transmission can as name because "
                                                            "transmission can was not in the ADS. "
                                                            "Returned {} instead.".format(returned_name))

    @mock.patch("sans.algorithm_detail.batch_execution.AnalysisDataService", new=ADSMock(True))
    def test_transmission_names_added_to_correct_workspaces_when_not_saving_can(self):
        reduction_packages = [self.reduction_package_transmissions]
        names_to_save = get_all_names_to_save(reduction_packages, False)

        names_expected = {('lab_workspace', 'transmission', 'transmission_can'),
                          ('hab_workspace', 'transmission', 'transmission_can')}

        self.assertEqual(names_to_save, names_expected)

    @mock.patch("sans.algorithm_detail.batch_execution.AnalysisDataService", new=ADSMock(True))
    def test_transmission_names_added_to_unsubtracted_can_and_sample(self):
        reduction_packages = [self.reduction_package_transmissions]
        names_to_save = get_all_names_to_save(reduction_packages, True)

        names_expected = {('lab_workspace', 'transmission', 'transmission_can'),
                          ('hab_workspace', 'transmission', 'transmission_can'),
                          ('reduced_lab_can', '', 'transmission_can'),
                          ('reduced_hab_can', '', 'transmission_can'),
                          ('reduced_lab_sample', 'transmission', ''),
                          ('reduced_hab_sample', 'transmission', '')}

        self.assertEqual(names_to_save, names_expected)

    def test_SANSSingleReduction_selected_when_not_requiring_event_slices(self):
        require_event_slices = False
        compatibility_mode = False
        event_slice_mode = True
        actual_alg, actual_using_event_slice_mode, _ = select_reduction_alg(require_event_slices, compatibility_mode,
                                                                            event_slice_mode, [])
        self.assertEqual(actual_alg, "SANSSingleReduction")
        self.assertEqual(actual_using_event_slice_mode, False)

    @mock.patch("sans.algorithm_detail.batch_execution.split_reduction_packages_for_event_slice_packages")
    def test_SANSSingleReduction_selected_when_compatibility_mode_turned_on(self, event_slice_splitter_mock):
        require_event_slices = True
        compatibility_mode = True
        event_slice_mode = True
        actual_alg, actual_using_event_slice_mode, _ = select_reduction_alg(require_event_slices, compatibility_mode,
                                                                            event_slice_mode, [])
        self.assertEqual(actual_alg, "SANSSingleReduction")
        self.assertEqual(actual_using_event_slice_mode, False)
        # Test that reduction packages have been split into event slices
        event_slice_splitter_mock.assert_called_once_with([])

    @mock.patch("sans.algorithm_detail.batch_execution.split_reduction_packages_for_event_slice_packages")
    def test_SANSSingleReduction_selected_when_not_using_event_slice_mode(self, event_slice_splitter_mock):
        require_event_slices = True
        compatibility_mode = False
        event_slice_mode = False
        actual_alg, actual_using_event_slice_mode, _ = select_reduction_alg(require_event_slices, compatibility_mode,
                                                                            event_slice_mode, [])
        self.assertEqual(actual_alg, "SANSSingleReduction")
        self.assertEqual(actual_using_event_slice_mode, False)
        # Test that reduction packages have been split into event slices
        event_slice_splitter_mock.assert_called_once_with([])

    def test_SANSSingleReductionEventSlice_selected_when_using_event_slice_mode(self):
        require_event_slices = True
        compatibility_mode = False
        event_slice_mode = True
        actual_alg, actual_using_event_slice_mode, _ = select_reduction_alg(require_event_slices, compatibility_mode,
                                                                            event_slice_mode, [])
        self.assertEqual(actual_alg, "SANSSingleReductionEventSlice")
        self.assertEqual(actual_using_event_slice_mode, True)


if __name__ == '__main__':
    unittest.main()
