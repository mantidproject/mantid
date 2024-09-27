# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from unittest import mock

from unittest.mock import call

from sans_core.common.enums import DetectorType
from mantidqtinterfaces.sans_isis.gui_logic.models.async_workers.beam_centre_async import BeamCentreAsync
from mantidqtinterfaces.sans_isis.gui_logic.presenter.beam_centre_presenter import BeamCentrePresenter


@mock.patch("mantidqtinterfaces.sans_isis.gui_logic.models.async_workers.beam_centre_async.SANSCentreFinder")
class BeamCentreAsyncTest(unittest.TestCase):
    def setUp(self) -> None:
        self.mocked_presenter = mock.create_autospec(BeamCentrePresenter)

        self.worker = BeamCentreAsync(parent_presenter=self.mocked_presenter)
        self.worker.set_unit_test_mode(True)

    def test_that_find_beam_centre_calls_centre_finder_once_when_COM_is_False(self, mocked_alg):
        state = mock.NonCallableMock()
        fields = mock.NonCallableMock()
        fields.centre_of_mass = False
        fields.component = DetectorType.LAB
        self.worker.find_beam_centre(state, settings=fields)

        mocked_instance = mocked_alg.return_value
        mocked_instance.assert_called_once_with(
            state,
            r_min=fields.r_min,
            r_max=fields.r_max,
            max_iter=fields.max_iterations,
            x_start=fields.lab_pos_1,
            y_start=fields.lab_pos_2,
            tolerance=fields.tolerance,
            find_direction=fields.find_direction,
            verbose=fields.verbose,
            component=fields.component,
            reduction_method=True,
        )

    def test_that_find_beam_centre_calls_centre_finder_twice_when_COM_is_TRUE(self, mocked_alg):
        state = mock.NonCallableMock()
        fields = mock.NonCallableMock()
        fields.centre_of_mass = True
        fields.component = DetectorType.HAB
        self.worker.find_beam_centre(state, settings=fields)

        mocked_instance = mocked_alg.return_value
        self.assertEqual(mocked_instance.call_count, 2)

        mocked_pos_1 = mocked_instance.return_value["pos1"]
        mocked_pos_2 = mocked_instance.return_value["pos2"]
        expected_calls = [
            call(
                state,
                r_min=fields.r_min,
                r_max=fields.r_max,
                max_iter=fields.max_iterations,
                x_start=fields.hab_pos_1,
                y_start=fields.hab_pos_2,
                tolerance=fields.tolerance,
                find_direction=fields.find_direction,
                component=fields.component,
                reduction_method=False,
            ),
            call(
                state,
                r_min=fields.r_min,
                r_max=fields.r_max,
                max_iter=fields.max_iterations,
                x_start=mocked_pos_1,
                y_start=mocked_pos_2,
                tolerance=fields.tolerance,
                find_direction=fields.find_direction,
                verbose=fields.verbose,
                component=fields.component,
                reduction_method=True,
            ),
        ]

        mocked_instance.assert_has_calls(expected_calls, any_order=True)

    def test_returned_value_is_forwarded_on_success(self, mocked_alg):
        state = mock.NonCallableMock()
        fields = mock.NonCallableMock()
        fields.centre_of_mass = True
        fields.component = DetectorType.HAB

        mocked_instance = mocked_alg.return_value
        expected = {"pos1": 1.0, "pos2": 3.0}
        mocked_instance.return_value = expected

        self.worker.find_beam_centre(state, settings=fields)
        self.mocked_presenter.on_update_centre_values.assert_called_once_with(expected)

    def test_beam_centre_errors_with_no_direction(self, mocked_alg):
        state = mock.NonCallableMock()
        fields = mock.NonCallableMock()
        fields.find_direction = None

        self.worker._logger = mock.create_autospec(self.worker._logger)
        self.worker.find_beam_centre(state, fields)
        self.assertEqual(0, mocked_alg.return_value.call_count)
        self.worker._logger.error.assert_called_once()
