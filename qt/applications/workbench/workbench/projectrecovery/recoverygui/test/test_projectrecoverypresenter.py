# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantidqt package
#

from __future__ import (absolute_import, unicode_literals)

import unittest

from mantid.py3compat import mock
from workbench.projectrecovery.recoverygui.projectrecoverypresenter import ProjectRecoveryPresenter


PATCH_PROJECT_RECOVERY_VIEW = 'workbench.projectrecovery.recoverygui.projectrecoverypresenter.ProjectRecoveryWidgetView'
PATCH_PROJECT_RECOVERY_MODEL = 'workbench.projectrecovery.recoverygui.projectrecoverypresenter.ProjectRecoveryModel'
PATCH_PROJECT_RECOVERY_FAILURE_VIEW = \
    'workbench.projectrecovery.recoverygui.projectrecoverypresenter.RecoveryFailureView'


def raise_exception():
    raise Exception


class ProjectRecoveryPresenterTest(unittest.TestCase):
    def setUp(self):
        self.prp = ProjectRecoveryPresenter(mock.MagicMock(), model=mock.MagicMock())
        self.prp.current_view = mock.MagicMock()

    @mock.patch(PATCH_PROJECT_RECOVERY_VIEW)
    def test_start_recovery_view_exception_raised(self, view):
        self.prp.current_view = None
        view.side_effect = raise_exception

        self.assertTrue(not self.prp.start_recovery_view())
        self.assertIsNone(self.prp.current_view)

    @mock.patch(PATCH_PROJECT_RECOVERY_VIEW)
    def test_start_recovery_view_start_mantid_normally_called(self, _):
        self.prp.current_view = None
        self.prp.start_mantid_normally_called = True

        self.assertTrue(self.prp.start_recovery_view())
        self.assertEqual(1, self.prp.current_view.exec_.call_count)

    @mock.patch(PATCH_PROJECT_RECOVERY_VIEW)
    def test_start_recovery_view_model_has_a_failed_run(self, _):
        self.prp.current_view = None
        self.prp.model.has_failed_run = True

        self.assertTrue(not self.prp.start_recovery_view())
        self.assertEqual(1, self.prp.current_view.exec_.call_count)

    @mock.patch(PATCH_PROJECT_RECOVERY_VIEW)
    def test_start_recovery_view_successful_run(self, _):
        self.prp.current_view = None
        self.prp.model.has_failed_run = False

        self.assertTrue(self.prp.start_recovery_view())
        self.assertEqual(1, self.prp.current_view.exec_.call_count)

    @mock.patch(PATCH_PROJECT_RECOVERY_MODEL)
    @mock.patch(PATCH_PROJECT_RECOVERY_FAILURE_VIEW)
    def test_start_recovery_failure_start_mantid_normally_called(self, _, __):
        self.prp.current_view = None
        self.prp.start_mantid_normally_called = True

        self.assertTrue(self.prp.start_recovery_failure())
        self.assertEqual(1, self.prp.current_view.exec_.call_count)

    @mock.patch(PATCH_PROJECT_RECOVERY_MODEL)
    @mock.patch(PATCH_PROJECT_RECOVERY_FAILURE_VIEW)
    def test_start_recovery_failure_exception_raised(self, view, _):
        self.prp.current_view = None
        view.side_effect = raise_exception

        self.assertTrue(not self.prp.start_recovery_failure())

    @mock.patch(PATCH_PROJECT_RECOVERY_FAILURE_VIEW)
    @mock.patch(PATCH_PROJECT_RECOVERY_MODEL)
    def test_start_recovery_failure_model_has_a_failed_run(self, model, _):
        self.prp.current_view = None
        model.return_value.has_failed_run = True

        self.assertTrue(not self.prp.start_recovery_failure())
        self.assertEqual(1, self.prp.current_view.exec_.call_count)

    @mock.patch(PATCH_PROJECT_RECOVERY_FAILURE_VIEW)
    @mock.patch(PATCH_PROJECT_RECOVERY_MODEL)
    def test_start_recovery_failure_successful_run(self, model, _):
        self.prp.current_view = None
        model.return_value.has_failed_run = False

        self.assertTrue(self.prp.start_recovery_failure())
        self.assertEqual(1, self.prp.current_view.exec_.call_count)

    def test_get_row_empty_list(self):
        self.prp.model.get_row.return_value = []

        self.assertEqual(self.prp.get_row(0), ["", "", ""])

    def test_get_row_actual_list(self):
        self.prp.model.get_row.return_value = ["Checkpoint1", "20", "No"]

        self.assertEqual(self.prp.get_row(0), ["Checkpoint1", "20", "No"])

    def test_recover_last_recovery_started(self):
        self.prp.model.is_recovery_running = True

        self.assertIsNone(self.prp.recover_last())

        self.prp.model.decide_last_checkpoint.assert_not_called()
        self.prp.model.recover_selected_checkpoint.assert_not_called()

    def test_recover_last(self):
        self.prp.model.is_recovery_running = False

        self.prp.recover_last()

        self.assertEqual(1, self.prp.model.decide_last_checkpoint.call_count)
        self.assertEqual(1, self.prp.model.recover_selected_checkpoint.call_count)

    def test_open_last_in_editor_recovery_started(self):
        self.prp.model.is_recovery_running = True

        self.assertIsNone(self.prp.open_last_in_editor())

        self.prp.model.decide_last_checkpoint.assert_not_called()
        self.prp.model.open_selected_in_editor.assert_not_called()

    def test_open_last_in_editor(self):
        self.prp.model.is_recovery_running = False

        self.prp.open_last_in_editor()

        self.assertEqual(1, self.prp.model.decide_last_checkpoint.call_count)
        self.assertEqual(1, self.prp.model.open_selected_in_editor.call_count)

    def test_start_mantid_normally_self_not_allow_start_mantid_normally_and_current_view_is_none(self):
        self.prp.current_view = None
        self.prp.allow_start_mantid_normally = False

        self.assertIsNone(self.prp.start_mantid_normally())

    def test_start_mantid_normally_self_not_allow_start_mantid_normally_and_current_view_is_not_none(self):
        self.prp.current_view = mock.MagicMock()
        self.prp.allow_start_mantid_normally = False

        self.prp.start_mantid_normally()

        self.assertEqual(1, self.prp.current_view.emit_abort_script.call_count)
        self.assertTrue(self.prp.start_mantid_normally_called)
        self.assertEqual(1, self.prp.model.start_mantid_normally.call_count)

    def test_start_mantid_normally_self_allow_start_mantid_normally(self):
        self.prp.current_view = None
        self.prp.allow_start_mantid_normally = True

        self.prp.start_mantid_normally()

        self.assertTrue(self.prp.start_mantid_normally_called)
        self.assertEqual(1, self.prp.model.start_mantid_normally.call_count)

    def test_recover_selected_checkpoint_recovery_has_started(self):
        self.prp.model.is_recovery_running = True

        self.prp.recover_selected_checkpoint("1")

        self.prp.model.recover_selected_checkpoint.assert_not_called()

    def test_recover_selected_checkpoint(self):
        self.prp.model.is_recovery_running = False

        self.prp.recover_selected_checkpoint("1")

        self.prp.model.recover_selected_checkpoint.assert_called_with("1")

    def test_open_selected_checkpoint_in_editor(self):
        self.prp.model.is_recovery_running = False

        self.prp.open_selected_checkpoint_in_editor("1")

        self.assertTrue(self.prp.open_selected_in_editor_selected)
        self.prp.model.open_selected_in_editor.assert_called_with("1")

    def test_open_selected_checkpoint_in_editor_recovery_has_started(self):
        self.prp.model.is_recovery_running = True

        self.prp.open_selected_checkpoint_in_editor("1")

        self.assertTrue(not self.prp.open_selected_in_editor_selected)
        self.prp.model.open_selected_in_editor.assert_not_called()

    def test_close_view_is_none(self):
        self.prp.current_view = None

        self.assertIsNone(self.prp.close_view())

    def test_close_view_is_not_none(self):
        view = mock.MagicMock()
        self.prp.current_view = view

        self.prp.close_view()

        view.setVisible.assert_called_with(False)
        self.assertIsNone(self.prp.current_view)

    def test_connect_progress_bar_to_recovery_view(self):
        self.prp.connect_progress_bar_to_recovery_view()

        self.assertEqual(1, self.prp.current_view.connect_progress_bar.call_count)

    def test_change_start_mantid_to_cancel_label(self):
        self.prp.change_start_mantid_to_cancel_label()

        self.prp.allow_start_mantid_normally = False
        self.prp.current_view.change_start_mantid_button.assert_called_with("Cancel Recovery")

    def test_set_up_progress_bar(self):
        self.prp.set_up_progress_bar(1)

        self.prp.current_view.set_progress_bar_maximum.assert_called_with(1)

    def test_get_number_of_checkpoints(self):
        self.prp.model.get_number_of_checkpoints.return_value = 1

        self.assertEqual(self.prp.get_number_of_checkpoints(), 1)
