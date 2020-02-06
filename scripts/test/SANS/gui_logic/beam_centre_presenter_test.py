# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)

import unittest

from mantid.py3compat import mock
from sans.common.enums import SANSInstrument
from sans.gui_logic.presenter.beam_centre_presenter import BeamCentrePresenter
from sans.test_helper.mock_objects import create_mock_beam_centre_tab
from sans.test_helper.mock_objects import (create_run_tab_presenter_mock)
from ui.sans_isis.work_handler import WorkHandler


class BeamCentrePresenterTest(unittest.TestCase):

    def setUp(self):
        self.parent_presenter = create_run_tab_presenter_mock(use_fake_state = False)
        self.view = create_mock_beam_centre_tab()
        self.WorkHandler = mock.MagicMock()
        self.BeamCentreModel = mock.MagicMock()
        self.SANSCentreFinder = mock.MagicMock()
        self.presenter = BeamCentrePresenter(self.parent_presenter, self.SANSCentreFinder,
                                             work_handler=self.WorkHandler, beam_centre_model=self.BeamCentreModel)
        self.presenter.connect_signals = mock.Mock()
        self.presenter.set_view(self.view)

    def test_that_on_run_clicked_calls_find_beam_centre(self):
        self.presenter.on_run_clicked()

        self.assertEqual(self.presenter._work_handler.process.call_count, 1)
        self.assertEqual(self.presenter._work_handler.process.call_args[0][1],  self.presenter._beam_centre_model.find_beam_centre)

    def test_that_on_run_clicked_updates_model_from_view(self):
        self.view.left_right = False
        self.view.lab_pos_1 = 100
        self.view.lab_pos_2 = -100
        self.presenter.set_view(self.view)
        self.presenter._beam_centre_model.scale_1 = 1000
        self.presenter._beam_centre_model.scale_2 = 1000

        self.presenter.on_run_clicked()

        self.assertFalse(self.presenter._beam_centre_model.left_right)
        self.assertEqual(self.presenter._beam_centre_model.lab_pos_1, 0.1)
        self.assertEqual(self.presenter._beam_centre_model.lab_pos_2, -0.1)

    def test_on_update_rows_skips_no_rows(self):
        self.parent_presenter.num_rows.return_value = 0
        self.assertFalse(self.WorkHandler.process.called)
        self.presenter.on_update_rows()

    def test_reset_inst_defaults_called_on_update_rows(self):
        self.parent_presenter.num_rows.return_value = 1
        self.parent_presenter.instrument = SANSInstrument.SANS2D

        mock_row = mock.Mock()
        self.parent_presenter.get_row.return_value = mock_row

        self.presenter.on_update_rows()

        self.WorkHandler.process.assert_called_with(caller=mock.ANY,
                                                    func=self.BeamCentreModel.reset_inst_defaults,
                                                    id=mock.ANY,
                                                    # Kwargs:
                                                    instrument=SANSInstrument.SANS2D, row_entry=mock_row)

    def test_that_model_reset_to_defaults_for_instrument_is_called_on_update_rows(self):
        self.parent_presenter.num_rows.return_value = 1
        self.parent_presenter.get_row.return_value = mock.Mock()

        self.presenter.on_update_rows()

        mock_args = self.WorkHandler.process.call_args
        args, kwargs = mock_args
        self.assertTrue('caller' in kwargs)

        callback = kwargs['caller']
        self.assertTrue(isinstance(callback, WorkHandler.WorkListener))

        # Check the success path
        self.presenter._logger = mock.Mock()
        callback.on_processing_finished(None)
        self.view.set_options.assert_called_with(mock.ANY)
        self.presenter._logger.debug.assert_called_with(mock.ANY)

        # Check the failure path
        self.presenter._logger = mock.Mock()
        callback.on_processing_error("Foo")
        self.presenter._logger.warning.assert_called_with(mock.ANY)


    def test_that_set_scaling_is_called_on_update_instrument(self):
        self.presenter.set_view(self.view)

        self.presenter.on_update_instrument(SANSInstrument.LARMOR)
        self.presenter._beam_centre_model.set_scaling.assert_called_once_with(SANSInstrument.LARMOR)

    def test_that_view_on_update_instrument_is_called_on_update_instrument(self):
        self.presenter.set_view(self.view)

        self.presenter.on_update_instrument(SANSInstrument.LARMOR)
        self.view.on_update_instrument.assert_called_once_with(SANSInstrument.LARMOR)

    def test_that_on_processing_finished_updates_view_and_model(self):
        self.presenter.set_view(self.view)
        result = {'pos1': 0.1, 'pos2': -0.1}
        self.presenter._beam_centre_model.scale_1 = 1000
        self.presenter._beam_centre_model.scale_2 = 1000
        self.presenter._beam_centre_model.update_lab = True
        self.presenter._beam_centre_model.update_hab = True

        self.presenter.on_processing_finished_centre_finder(result)
        self.assertEqual(result['pos1'], self.presenter._beam_centre_model.lab_pos_1)
        self.assertEqual(result['pos2'], self.presenter._beam_centre_model.lab_pos_2)
        self.assertEqual(self.view.lab_pos_1, self.presenter._beam_centre_model.scale_1*result['pos1'])
        self.assertEqual(self.view.lab_pos_2, self.presenter._beam_centre_model.scale_2*result['pos2'])
        self.assertEqual(result['pos1'], self.presenter._beam_centre_model.hab_pos_1)
        self.assertEqual(result['pos2'], self.presenter._beam_centre_model.hab_pos_2)
        self.assertEqual(self.view.hab_pos_1, self.presenter._beam_centre_model.scale_1 * result['pos1'])
        self.assertEqual(self.view.hab_pos_2, self.presenter._beam_centre_model.scale_2 * result['pos2'])
        self.view.set_run_button_to_normal.assert_called_once_with()

    def test_that_on_processing_finished_updates_does_not_update_view_and_model_when_update_disabled(self):
        self.presenter.set_view(self.view)
        result = {'pos1': 0.1, 'pos2': -0.1}
        result_1 = {'pos1': 0.2, 'pos2': -0.2}
        self.presenter._beam_centre_model.scale_1 = 1000
        self.presenter._beam_centre_model.scale_2 = 1000
        self.presenter._beam_centre_model.update_lab = True
        self.presenter._beam_centre_model.update_hab = True
        self.presenter.on_processing_finished_centre_finder(result)
        self.presenter._beam_centre_model.update_lab = False
        self.presenter._beam_centre_model.update_hab = False

        self.presenter.on_processing_finished_centre_finder(result_1)

        self.assertEqual(result['pos1'], self.presenter._beam_centre_model.lab_pos_1)
        self.assertEqual(result['pos2'], self.presenter._beam_centre_model.lab_pos_2)
        self.assertEqual(self.view.lab_pos_1, self.presenter._beam_centre_model.scale_1*result['pos1'])
        self.assertEqual(self.view.lab_pos_2, self.presenter._beam_centre_model.scale_2*result['pos2'])
        self.assertEqual(result['pos1'], self.presenter._beam_centre_model.hab_pos_1)
        self.assertEqual(result['pos2'], self.presenter._beam_centre_model.hab_pos_2)
        self.assertEqual(self.view.hab_pos_1, self.presenter._beam_centre_model.scale_1 * result['pos1'])
        self.assertEqual(self.view.hab_pos_2, self.presenter._beam_centre_model.scale_2 * result['pos2'])
        self.assertEqual(self.view.set_run_button_to_normal.call_count, 2)

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


if __name__ == '__main__':
    unittest.main()
