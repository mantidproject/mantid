# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
import uuid
from unittest import mock

from mantid.api import WorkspaceGroup, AnalysisDataService
from mantid.simpleapi import CreateSampleWorkspace, GroupWorkspaces
from sans.algorithm_detail.batch_execution import (
    get_all_names_to_save,
    get_transmission_names_to_save,
    ReductionPackage,
    select_reduction_alg,
    save_workspace_to_file,
    delete_reduced_workspaces,
    create_scaled_background_workspace,
    subtract_scaled_background,
)
from sans.common.enums import SaveType, ReductionMode


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
        workspaces = ["Sample", "Transmission", "Direct"]
        monitors = ["monitor1"]
        self.reduction_package_merged = ReductionPackage(state, workspaces, monitors)
        self.reduction_package = ReductionPackage(state, workspaces, monitors)
        self.reduction_package_transmissions = ReductionPackage(state, workspaces, monitors)
        self.reduction_package_transmissions.unfitted_transmission_base_name = "Base"
        self.reduction_package_transmissions.unfitted_transmission_name = ["transmission"]
        self.reduction_package_transmissions.unfitted_transmission_can_base_name = "Base"
        self.reduction_package_transmissions.unfitted_transmission_can_name = ["transmission_can"]

        def _create_sample_ws_group(ws_name):
            # This has to be done as two steps or the simple API can't figure out the output name
            ws_group = WorkspaceGroup()
            ws_group.addWorkspace(
                CreateSampleWorkspace(
                    OutputWorkspace=ws_name,
                    Function="Flat background",
                    NumBanks=1,
                    BankPixelWidth=1,
                    NumEvents=1,
                    XMin=1,
                    XMax=14,
                    BinWidth=2,
                )
            )
            return ws_group

        # Some tests rely on the names being in the ADS - so manually inject them so we don't have to
        # re-test the ADS injection code each time
        merged_workspace = _create_sample_ws_group("merged_workspace")
        lab_workspace = _create_sample_ws_group("lab_workspace")
        hab_workspace = _create_sample_ws_group("hab_workspace")
        bgsub_workspace_name = ["bgsub_workspace"]
        reduced_lab_can = _create_sample_ws_group("reduced_lab_can")
        reduced_hab_can = _create_sample_ws_group("reduced_hab_can")
        reduced_lab_sample = _create_sample_ws_group("reduced_lab_sample")
        reduced_hab_sample = _create_sample_ws_group("reduced_hab_sample")
        transmission = _create_sample_ws_group("transmission")
        transmission_can = _create_sample_ws_group("transmission_can")

        self.reduction_package_merged.reduced_merged = merged_workspace
        self.reduction_package_merged.reduced_lab = lab_workspace
        self.reduction_package_merged.reduced_hab = hab_workspace
        self.reduction_package_merged.reduced_bgsub_name = bgsub_workspace_name
        self.reduction_package_merged.reduced_lab_can = reduced_lab_can
        self.reduction_package_merged.reduced_hab_can = reduced_hab_can
        self.reduction_package_merged.reduced_lab_sample = reduced_lab_sample
        self.reduction_package_merged.reduced_hab_sample = reduced_hab_sample
        self.reduction_package_merged.unfitted_transmission = ""
        self.reduction_package_merged.unfitted_transmission_can = ""

        self.reduction_package.reduced_lab = lab_workspace
        self.reduction_package.reduced_hab = hab_workspace
        self.reduction_package.reduced_bgsub_name = bgsub_workspace_name
        self.reduction_package.reduced_lab_can = reduced_lab_can
        self.reduction_package.reduced_hab_can = reduced_hab_can
        self.reduction_package.reduced_lab_sample = reduced_lab_sample
        self.reduction_package.reduced_hab_sample = reduced_hab_sample
        self.reduction_package.unfitted_transmission = ""
        self.reduction_package.unfitted_transmission_can = ""

        self.reduction_package_transmissions.reduced_lab = lab_workspace
        self.reduction_package_transmissions.reduced_hab = hab_workspace
        self.reduction_package_transmissions.reduced_bgsub_name = bgsub_workspace_name
        self.reduction_package_transmissions.reduced_lab_can = reduced_lab_can
        self.reduction_package_transmissions.reduced_hab_can = reduced_hab_can
        self.reduction_package_transmissions.reduced_lab_sample = reduced_lab_sample
        self.reduction_package_transmissions.reduced_hab_sample = reduced_hab_sample
        self.reduction_package_transmissions.unfitted_transmission = transmission
        self.reduction_package_transmissions.unfitted_transmission_can = transmission_can

    def test_returns_merged_name_if_present(self):
        reduction_packages = [self.reduction_package_merged]
        names_to_save = get_all_names_to_save(reduction_packages, save_can=False)

        self.assertEqual(names_to_save, [(["merged_workspace"], [], []), (["bgsub_workspace"], [], [])])

    def test_hab_and_lab_workspaces_returned_if_merged_workspace_not_present(self):
        reduction_packages = [self.reduction_package]
        names_to_save = get_all_names_to_save(reduction_packages, save_can=False)

        self.assertEqual(names_to_save, [(["lab_workspace"], [], []), (["hab_workspace"], [], []), (["bgsub_workspace"], [], [])])

    def test_can_workspaces_returned_if_save_can_selected(self):
        names_to_save = get_all_names_to_save([self.reduction_package_merged], True)
        names = {
            "merged_workspace",
            "lab_workspace",
            "hab_workspace",
            "bgsub_workspace",
            "reduced_lab_can",
            "reduced_hab_can",
            "reduced_lab_sample",
            "reduced_hab_sample",
        }
        names_expected = [([name], [], []) for name in names]
        self.assertEqual(sorted(names_to_save), sorted(names_expected))

    def test_can_workspaces_returned_if_save_can_selected_no_merged(self):
        reduction_packages = [self.reduction_package]
        names_to_save = get_all_names_to_save(reduction_packages, True)

        names = {
            "lab_workspace",
            "hab_workspace",
            "bgsub_workspace",
            "reduced_lab_can",
            "reduced_hab_can",
            "reduced_lab_sample",
            "reduced_hab_sample",
        }
        names_expected = [([name], [], []) for name in names]
        self.assertEqual(sorted(names_to_save), sorted(names_expected))

    def test_that_empty_string_returned_if_transmission_base_name_is_none(self):
        # Test when base name is None
        reduction_package = self.reduction_package
        reduction_package.unfitted_transmission_base_name = None
        returned_name = get_transmission_names_to_save(reduction_package, False)
        self.assertEqual(
            returned_name,
            [],
            "Should have returned an empty string because "
            "transmission sample base name was None. "
            "Returned {} instead".format(returned_name),
        )

        # Test when base name is not None but name is None
        reduction_package.unfitted_transmission_base_name = "Base"
        reduction_package.unfitted_transmission_name = None
        returned_name = get_transmission_names_to_save(reduction_package, False)
        self.assertEqual(
            returned_name,
            [],
            "Should have returned an empty string because "
            "transmission sample name was None. "
            "Returned {} instead".format(returned_name),
        )

    @mock.patch("sans.algorithm_detail.batch_execution.AnalysisDataService", new=ADSMock(False))
    def test_no_transmission_workspace_names_returned_if_not_in_ADS(self):
        # Check transmission sample
        returned_name = get_transmission_names_to_save(self.reduction_package_transmissions, False)
        self.assertEqual(
            returned_name,
            [],
            "Should have returned an empty string because "
            "transmission sample was not in the ADS. "
            "Returned {} instead.".format(returned_name),
        )

        # Check transmission_can
        returned_name = get_transmission_names_to_save(self.reduction_package_transmissions, True)
        self.assertEqual(
            returned_name,
            [],
            "Should have returned an empty string because "
            "transmission can was not in the ADS. "
            "Returned {} instead".format(returned_name),
        )

    @mock.patch("sans.algorithm_detail.batch_execution.AnalysisDataService", new=ADSMock(True))
    def test_transmission_workspace_names_from_reduction_package_are_returned_if_in_ADS(self):
        reduction_package = self.reduction_package_transmissions
        reduction_package.unfitted_transmission_base_name = "Base"
        reduction_package.unfitted_transmission_name = ["transmission"]
        reduction_package.unfitted_transmission_can_base_name = "Base"
        reduction_package.unfitted_transmission_can_name = ["transmission_can"]

        # Check transmission sample
        returned_name = get_transmission_names_to_save(reduction_package, False)
        self.assertEqual(
            returned_name,
            ["transmission"],
            "Should have transmission as name because " "transmission sample was in the ADS. " "Returned {} instead.".format(returned_name),
        )

        # Check transmission_can
        returned_name = get_transmission_names_to_save(reduction_package, True)
        self.assertEqual(
            returned_name,
            ["transmission_can"],
            "Should have returned transmission can as name because "
            "transmission can was not in the ADS. "
            "Returned {} instead.".format(returned_name),
        )

    @mock.patch("sans.algorithm_detail.batch_execution.AnalysisDataService", new=ADSMock(True))
    def test_transmission_names_added_to_correct_workspaces_when_not_saving_can(self):
        reduction_packages = [self.reduction_package_transmissions]
        names_to_save = get_all_names_to_save(reduction_packages, False)

        names_expected = [
            (["lab_workspace"], ["transmission"], ["transmission_can"]),
            (["hab_workspace"], ["transmission"], ["transmission_can"]),
            (["bgsub_workspace"], ["transmission"], ["transmission_can"]),
        ]

        self.assertEqual(names_to_save, names_expected)

    @mock.patch("sans.algorithm_detail.batch_execution.AnalysisDataService", new=ADSMock(True))
    def test_transmission_names_added_to_unsubtracted_can_and_sample(self):
        reduction_packages = [self.reduction_package_transmissions]
        names_to_save = get_all_names_to_save(reduction_packages, True)

        names_expected = [
            (["lab_workspace"], ["transmission"], ["transmission_can"]),
            (["hab_workspace"], ["transmission"], ["transmission_can"]),
            (["bgsub_workspace"], ["transmission"], ["transmission_can"]),
            (["reduced_lab_can"], [], ["transmission_can"]),
            (["reduced_hab_can"], [], ["transmission_can"]),
            (["reduced_lab_sample"], ["transmission"], []),
            (["reduced_hab_sample"], ["transmission"], []),
        ]

        self.assertEqual(names_to_save, names_expected)

    def test_does_not_use_event_slice_optimisation_when_not_requiring_event_slices(self):
        require_event_slices = False
        compatibility_mode = False
        event_slice_optimisation_checkbox = True
        actual_using_event_slice_optimisation, _ = select_reduction_alg(
            require_event_slices, compatibility_mode, event_slice_optimisation_checkbox, []
        )
        self.assertEqual(actual_using_event_slice_optimisation, False)

    @mock.patch("sans.algorithm_detail.batch_execution.split_reduction_packages_for_event_slice_packages")
    def test_does_not_use_event_slice_optimisation_when_compatibility_mode_turned_on(self, event_slice_splitter_mock):
        require_event_slices = True
        compatibility_mode = True
        event_slice_optimisation_checkbox = True
        actual_using_event_slice_optimisation, _ = select_reduction_alg(
            require_event_slices, compatibility_mode, event_slice_optimisation_checkbox, []
        )
        self.assertEqual(actual_using_event_slice_optimisation, False)
        # Test that reduction packages have been split into event slices
        event_slice_splitter_mock.assert_called_once_with([])

    @mock.patch("sans.algorithm_detail.batch_execution.split_reduction_packages_for_event_slice_packages")
    def test_does_not_use_event_slice_optimisation_when_optimisation_not_selected(self, event_slice_splitter_mock):
        require_event_slices = True
        compatibility_mode = False
        event_slice_optimisation_checkbox = False
        actual_using_event_slice_optimisation, _ = select_reduction_alg(
            require_event_slices, compatibility_mode, event_slice_optimisation_checkbox, []
        )
        self.assertEqual(actual_using_event_slice_optimisation, False)
        # Test that reduction packages have been split into event slices
        event_slice_splitter_mock.assert_called_once_with([])

    def test_use_event_slice_optimisation_when_using_event_slice_optimisation_is_checked(self):
        require_event_slices = True
        compatibility_mode = False
        event_slice_optimisation_checkbox = True
        actual_using_event_slice_optimisation, _ = select_reduction_alg(
            require_event_slices, compatibility_mode, event_slice_optimisation_checkbox, []
        )
        self.assertEqual(actual_using_event_slice_optimisation, True)

    @mock.patch("sans.algorithm_detail.batch_execution.create_unmanaged_algorithm")
    def test_that_save_workspace_to_file_includes_run_numbers_in_options(self, mock_alg_manager):
        ws_name = "wsName"
        filename = "fileName"
        additional_run_numbers = {
            "SampleTransmissionRunNumber": "5",
            "SampleDirectRunNumber": "6",
            "CanScatterRunNumber": "7",
            "CanDirectRunNumber": "8",
        }

        save_workspace_to_file(ws_name, [], filename, additional_run_numbers)

        expected_options = {
            "InputWorkspace": ws_name,
            "Filename": filename,
            "Transmission": "",
            "TransmissionCan": "",
            "SampleTransmissionRunNumber": "5",
            "SampleDirectRunNumber": "6",
            "CanScatterRunNumber": "7",
            "CanDirectRunNumber": "8",
        }
        mock_alg_manager.assert_called_once_with("SANSSave", **expected_options)

    @mock.patch("sans.algorithm_detail.batch_execution.create_unmanaged_algorithm")
    def test_that_save_workspace_to_file_can_set_file_types(self, mock_alg_manager):
        ws_name = "wsName"
        filename = "fileName"
        additional_run_numbers = {}
        file_types = [SaveType.NEXUS, SaveType.CAN_SAS, SaveType.NX_CAN_SAS, SaveType.NIST_QXY, SaveType.RKH, SaveType.CSV]

        save_workspace_to_file(ws_name, file_types, filename, additional_run_numbers)

        expected_options = {
            "InputWorkspace": ws_name,
            "Filename": filename,
            "Transmission": "",
            "TransmissionCan": "",
            "Nexus": True,
            "CanSAS": True,
            "NXcanSAS": True,
            "NistQxy": True,
            "RKH": True,
            "CSV": True,
        }
        mock_alg_manager.assert_called_once_with("SANSSave", **expected_options)

    @mock.patch("sans.algorithm_detail.batch_execution.create_unmanaged_algorithm")
    def test_that_save_workspace_to_file_can_set_transmission_workspace_names(self, mock_alg_manager):
        ws_name = "wsName"
        filename = "fileName"
        additional_run_numbers = {}
        file_types = []
        transmission_name = "transName"
        transmission_can_name = "transCanName"

        save_workspace_to_file(
            ws_name,
            file_types,
            filename,
            additional_run_numbers,
            transmission_name=transmission_name,
            transmission_can_name=transmission_can_name,
        )

        expected_options = {
            "InputWorkspace": ws_name,
            "Filename": filename,
            "Transmission": transmission_name,
            "TransmissionCan": transmission_can_name,
        }
        mock_alg_manager.assert_called_once_with("SANSSave", **expected_options)

    def test_get_scaled_background_workspace_no_background(self):
        state = mock.MagicMock()
        state.background_subtraction.workspace = None

        result = create_scaled_background_workspace(state)

        state.background_subtraction.validate.assert_called_once()
        self.assertIsNone(result, "When no background ws is set, this should return None.")

    @mock.patch("sans.algorithm_detail.batch_execution.create_unmanaged_algorithm")
    def test_get_scaled_background_workspace_calls_algs(self, mock_alg_manager):
        state = mock.MagicMock()
        ws_name = "workspace"
        scale_factor = 1.12
        expected_out_name = "__" + ws_name + "_scaled"
        state.background_subtraction.workspace = ws_name
        state.background_subtraction.scale_factor = scale_factor

        result = create_scaled_background_workspace(state)

        state.background_subtraction.validate.assert_called_once()

        expected_options = {
            "InputWorkspace": ws_name,
            "Factor": scale_factor,
            "OutputWorkspace": expected_out_name,
        }
        mock_alg_manager.assert_called_once_with("Scale", **expected_options)
        self.assertEqual(result, expected_out_name, "Should output the scaled ws name.")

    def test_subtract_scaled_background_with_all_detectors_fails(self):
        reduction_package = mock.MagicMock()
        scaled_ws_name = "__workspace_scaled"
        reduction_package.reduction_mode = ReductionMode.ALL
        self.assertRaisesRegex(
            ValueError,
            f"Reduction Mode '{ReductionMode.ALL}' is incompatible with scaled background reduction. The ReductionMode "
            f"must be set to '{ReductionMode.MERGED}', '{ReductionMode.HAB}', or '{ReductionMode.LAB}'.",
            subtract_scaled_background,
            reduction_package,
            scaled_ws_name,
        )

    @mock.patch("sans.algorithm_detail.batch_execution.AnalysisDataService", new=ADSMock(True))
    @mock.patch("sans.algorithm_detail.batch_execution.create_unmanaged_algorithm")
    def test_subtract_background_from_merged_calls_algorithms_correctly(self, mock_alg_manager):
        mock_minus = mock.MagicMock()
        reduction_package = mock.MagicMock()
        mock_alg_manager.return_value = mock_minus
        scaled_ws_name = "__workspace_scaled"
        reduction_package.reduction_mode = ReductionMode.MERGED
        reduction_package.reduced_merged_name = ["ws1", "ws2"]

        created_workspaces, created_workspace_names = subtract_scaled_background(reduction_package, scaled_ws_name)

        mock_alg_manager.assert_any_call("Minus", **{"RHSWorkspace": scaled_ws_name, "LHSWorkspace": "ws1", "OutputWorkspace": "ws1_bgsub"})
        mock_alg_manager.assert_any_call("Minus", **{"RHSWorkspace": scaled_ws_name, "LHSWorkspace": "ws2", "OutputWorkspace": "ws2_bgsub"})
        self.assertEqual(["ws1_bgsub", "ws2_bgsub"], created_workspace_names)
        self.assertEqual(len(created_workspaces), len(created_workspace_names))
        self.assertEqual(mock_minus.execute.call_count, 2)


class DeleteMethodsTest(unittest.TestCase):
    def setUp(self):
        self._ads_names = []

    def _create_ads_sample_workspaces(self):
        ws_ptrs = []
        for i in range(2):
            self._ads_names.append(str(uuid.uuid4()))
            ws_ptrs.append(CreateSampleWorkspace(OutputWorkspace=self._ads_names[-1]))
        ws_group = GroupWorkspaces(InputWorkspaces=ws_ptrs, OutputWorkspace=str(uuid.uuid4()))
        return ws_group

    @staticmethod
    def _create_non_ads_sample_workspaces():
        ws_group = WorkspaceGroup()
        for i in range(2):
            ws_group.addWorkspace(CreateSampleWorkspace(OutputWorkspace=str(uuid.uuid4()), StoreInADS=False))
        return ws_group

    @staticmethod
    def _pack_reduction_package(create_method):
        package = mock.NonCallableMock()
        package.calculated_transmission = create_method()
        package.calculated_transmission_can = create_method()
        package.unfitted_transmission = create_method()
        package.unfitted_transmission_can = create_method()
        return package

    def test_delete_reduced_workspace_in_ads(self):
        package = self._pack_reduction_package(self._create_ads_sample_workspaces)
        current_ads_names = AnalysisDataService.getObjectNames()
        self.assertTrue(all(name in current_ads_names for name in self._ads_names))
        delete_reduced_workspaces(reduction_packages=[package], include_non_transmission=False)
        current_ads_names = AnalysisDataService.getObjectNames()
        self.assertFalse(all(name in current_ads_names for name in self._ads_names))

    def test_delete_reduced_workspaces_not_in_ads(self):
        package = self._pack_reduction_package(self._create_non_ads_sample_workspaces)
        delete_reduced_workspaces(reduction_packages=[package], include_non_transmission=False)

        # Pick two at random to check
        for i in package.unfitted_transmission.getNames():
            self.assertFalse(i)

        for i in package.calculated_transmission_can.getNames():
            self.assertFalse(i)


if __name__ == "__main__":
    unittest.main()
