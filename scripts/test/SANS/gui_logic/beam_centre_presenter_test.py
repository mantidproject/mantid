# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
import random
from unittest import mock

from sans.common.enums import SANSInstrument
from sans.gui_logic.presenter.beam_centre_presenter import BeamCentrePresenter
from sans.test_helper.mock_objects import create_mock_beam_centre_tab
from sans.test_helper.mock_objects import (create_run_tab_presenter_mock)


class BeamCentrePresenterTest(unittest.TestCase):

    def setUp(self):
        self.parent_presenter = create_run_tab_presenter_mock(use_fake_state = False)
        self.view = create_mock_beam_centre_tab()
        self.WorkHandler = mock.MagicMock()
        self.BeamCentreModel = mock.MagicMock()
        self.SANSCentreFinder = mock.MagicMock()

        self.presenter = BeamCentrePresenter(self.parent_presenter, beam_centre_model=self.BeamCentreModel)
        self.presenter.connect_signals = mock.Mock()
        self.presenter.set_view(self.view)
        self.presenter._worker = mock.create_autospec(self.presenter._worker)

    def test_that_on_run_clicked_calls_find_beam_centre(self):
        self.presenter.on_run_clicked()
        self.presenter._worker.find_beam_centre.assert_called_once_with(
            mock.ANY, self.BeamCentreModel.pack_beam_centre_settings.return_value)

    def test_beam_centre_finder_update_handles_str(self):
        # Since the view might give us mixed types check we can handle str
        model = self.BeamCentreModel
        model.lab_pos_1 = "1"
        model.lab_pos_2 = 1
        model.hab_pos_1 = 2
        model.hab_pos_2 = "2"

        self.presenter.on_processing_finished_centre_finder()
        for i in [self.view.lab_pos_1, self.view.lab_pos_2]:
            self.assertEquals(1, i)
        for i in [self.view.hab_pos_1, self.view.hab_pos_2]:
            self.assertEquals(2, i)

    def test_that_on_run_clicked_updates_model_from_view(self):
        self.view.left_right = False
        self.view.lab_pos_1 = 100
        self.view.lab_pos_2 = -100
        self.presenter.set_view(self.view)

        self.presenter.on_run_clicked()

        self.assertFalse(self.presenter._beam_centre_model.left_right)
        self.assertEqual(self.presenter._beam_centre_model.lab_pos_1, 100)
        self.assertEqual(self.presenter._beam_centre_model.lab_pos_2, -100)

    def test_on_update_rows_skips_no_rows(self):
        self.parent_presenter.num_rows.return_value = 0
        self.assertFalse(self.WorkHandler.process.called)
        self.presenter.on_update_rows()

    def test_reset_inst_defaults_called_on_update_rows(self):
        self.parent_presenter.num_rows.return_value = 1
        self.parent_presenter.instrument = SANSInstrument.SANS2D

        self.presenter.on_update_rows()

        self.BeamCentreModel.reset_inst_defaults.assert_called_with(SANSInstrument.SANS2D)

    def test_that_on_processing_finished_updates_view(self):
        self.presenter.set_view(self.view)

        self.BeamCentreModel.lab_pos_1 = 1
        self.BeamCentreModel.lab_pos_2 = 2
        self.BeamCentreModel.hab_pos_1 = 3
        self.BeamCentreModel.hab_pos_2 = 4

        self.presenter.on_processing_finished_centre_finder()
        self.assertEqual(self.BeamCentreModel.lab_pos_1, self.view.lab_pos_1)
        self.assertEqual(self.BeamCentreModel.lab_pos_2, self.view.lab_pos_2)

        self.assertEqual(self.BeamCentreModel.hab_pos_1, self.view.hab_pos_1)
        self.assertEqual(self.BeamCentreModel.hab_pos_2, self.view.hab_pos_2)
        self.view.set_run_button_to_normal.assert_called_once_with()

    def test_on_update_positions_handles_strings(self):
        self.BeamCentreModel.lab_pos_1 = "foo"
        self.BeamCentreModel.lab_pos_2 = ""
        self.presenter.update_centre_positions()

        self.assertEqual("foo", self.view.lab_pos_1)
        self.assertEqual("", self.view.lab_pos_2)

    def test_that_update_hab_selected_enabled_hab_and_disabled_lab(self):
        self.presenter.set_view(self.view)
        self.presenter.update_hab_selected()

        # Check that the model has been updated
        self.assertTrue(self.presenter._beam_centre_model.update_hab)
        self.assertFalse(self.presenter._beam_centre_model.update_lab)

        # Check that we called a method to enable the HAB and a method to disable the LAB
        self.presenter._view.enable_update_hab.assert_called_once_with(True)
        self.presenter._view.enable_update_lab.assert_called_once_with(False)

    def test_that_update_lab_selected_enabled_lab_and_disabled_hab(self):
        self.presenter.set_view(self.view)
        self.presenter.update_lab_selected()

        # Check that the model has been updated
        self.assertFalse(self.presenter._beam_centre_model.update_hab)
        self.assertTrue(self.presenter._beam_centre_model.update_lab)

        # Check that we called a method to disable the HAB and a method to enable the LAB
        self.presenter._view.enable_update_hab.assert_called_once_with(False)
        self.presenter._view.enable_update_lab.assert_called_once_with(True)

    def test_that_update_all_selected_enabled_hab_and_lab(self):
        self.presenter.set_view(self.view)
        self.presenter.update_all_selected()

        # Check that the model has been updated
        self.assertTrue(self.presenter._beam_centre_model.update_hab)
        self.assertTrue(self.presenter._beam_centre_model.update_lab)

        # Check that we called a method to enable the HAB and a method to enable the LAB
        self.presenter._view.enable_update_hab.assert_called_once_with(True)
        self.presenter._view.enable_update_lab.assert_called_once_with(True)

    def test_copies_into_internal_model(self):
        mocked_external_model = mock.NonCallableMock()

        attr_list = ["lab_pos_1", "lab_pos_2", "hab_pos_1", "hab_pos_2"]

        for attr in attr_list:
            setattr(mocked_external_model, attr, random.randint(0, 1000)/100)  # Simulate a random FP value
        self.presenter.copy_centre_positions(mocked_external_model)

        for attr in attr_list:
            self.assertEqual(getattr(mocked_external_model, attr), getattr(self.presenter._beam_centre_model, attr))

    def test_copies_without_hab_into_internal_model(self):
        mocked_external_model = mock.NonCallableMock()
        lab_attr = ["lab_pos_1", "lab_pos_2"]
        hab_list = ["hab_pos_1", "hab_pos_2"]

        for attr in hab_list:
            # On instruments with no HAB this can be a blank string
            setattr(mocked_external_model, attr, '')

        for attr in lab_attr:
            setattr(mocked_external_model, attr, random.randint(0, 1000)/100)  # Simulate a random FP value
        self.presenter.copy_centre_positions(mocked_external_model)

        for attr in lab_attr:
            self.assertEqual(getattr(mocked_external_model, attr), getattr(self.presenter._beam_centre_model, attr))

        # When HAB isn't present we should take LAB values
        for hab_attr, lab_attr in zip(hab_list, lab_attr):
            self.assertEqual(getattr(mocked_external_model, lab_attr),
                             getattr(self.presenter._beam_centre_model, hab_attr))

    def test_on_update_rows_updates_centres(self):
        # As the LAB centres can update when the rows change (due to different run number groups)
        # we should always update from the model
        self.presenter.update_centre_positions = mock.Mock()
        self.presenter.on_update_rows()
        self.presenter.update_centre_positions.assert_called_once()


if __name__ == '__main__':
    unittest.main()
