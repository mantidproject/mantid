# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantidqt package
#
import unittest
from unittest import mock

from mantidqt.widgets.instrumentview.presenter import InstrumentViewPresenter


class InstrumentViewPresenterTest(unittest.TestCase):
    def setUp(self) -> None:
        # Keep import private so we're not tempted to use it in other tests
        from mantidqt.widgets.instrumentview.view import InstrumentView
        self.mock_view = mock.create_autospec(InstrumentView)
        self.ws = mock.NonCallableMock()
        self.presenter = InstrumentViewPresenter(ws=self.ws, view=self.mock_view)

    @mock.patch("mantidqt.widgets.instrumentview.presenter.InstrumentView")
    def test_ws_name_passed_in_constructor(self, mock_view):
        ws = mock.NonCallableMock()
        presenter = InstrumentViewPresenter(ws)

        mock_view.assert_called_once_with(parent=mock.ANY, presenter=presenter,
                                          name=str(ws), window_flags=mock.ANY)

    @mock.patch("mantidqt.widgets.instrumentview.presenter.InstrumentViewManager")
    def test_constructor_registers_with_inst_view_manager(self, mock_manager):
        ws = mock.NonCallableMock()
        # Recreate presenter now we have patched InstrumentViewManager
        presenter = InstrumentViewPresenter(ws, view=mock.NonCallableMock())
        mock_manager.register.assert_called_with(presenter, str(ws))

    def test_current_workspace_equals(self):
        self.assertTrue(self.presenter.current_workspace_equals(str(self.ws)))
        self.assertFalse(self.presenter.current_workspace_equals(str(mock.NonCallableMock())))

    def test_replace_workspace(self):
        expected_str = "test"
        self.presenter.replace_workspace(expected_str)
        self.mock_view.replace_workspace.assert_called_once_with(expected_str, None)

    def test_replace_workspace_new_window_name(self):
        expected_str = "test"
        window_name = "new_window"
        self.presenter.replace_workspace(expected_str, window_name)
        self.mock_view.replace_workspace.assert_called_once_with(expected_str, window_name)

    def test_rename_workspace(self):
        # This is a no-op
        self.assertIsNone(self.presenter.rename_workspace("", ""))

    def test_show_view(self):
        self.presenter.show_view()
        self.mock_view.show.assert_called_once()

    def test_get_current_tab(self):
        self.presenter.get_current_tab()
        self.mock_view.get_current_tab.assert_called_once()

    def test_get_render_tab(self):
        self.presenter.get_render_tab()
        self.mock_view.get_render_tab.assert_called_once()

    def test_get_pick_tab(self):
        self.presenter.get_pick_tab()
        self.mock_view.get_pick_tab.assert_called_once()

    def test_select_render_tab(self):
        self.presenter.select_render_tab()
        self.mock_view.select_tab.assert_called_once_with(0)

    def test_select_pick_tab(self):
        self.presenter.select_pick_tab()
        self.mock_view.select_tab.assert_called_once_with(1)

    def test_set_bin_range(self):
        min_val, max_val = mock.NonCallableMock(), mock.NonCallableMock()
        self.presenter.set_bin_range(min_val, max_val)
        self.mock_view.set_range.assert_called_once_with(min_val, max_val)

    def test_is_thread_running(self):
        self.mock_view.is_thread_running = mock.Mock(return_value=True)
        self.assertTrue(self.presenter.is_thread_running())
        self.mock_view.is_thread_running.assert_called_once()

    def test_wait(self):
        self.presenter.wait()
        self.mock_view.wait.assert_called_once()

    @mock.patch("mantidqt.widgets.instrumentview.presenter.InstrumentViewManager")
    def test_close_with_last_view_self_sets_this_to_none(self, mock_manager):
        mock_manager.last_view = self.presenter
        self.presenter.close("")
        self.assertIsNone(mock_manager.last_view)

    @mock.patch("mantidqt.widgets.instrumentview.presenter.InstrumentViewManager")
    def test_close_with_last_view_other_val_does_nothing(self, mock_manager):
        expected = mock.NonCallableMock()
        mock_manager.last_view = expected
        self.presenter.close("")
        self.assertEqual(expected, mock_manager.last_view,
                         "InstrumentViewManager.last_view was cleared when it should not have been")

    @mock.patch("mantidqt.widgets.instrumentview.presenter.ObservingPresenter.close")
    @mock.patch("mantidqt.widgets.instrumentview.presenter.InstrumentViewManager")
    def test_close_with_matching_ws_name(self, mocked_manager, mocked_super_close):
        ws_name = str(self.ws)
        self.presenter.close(ws_name)
        mocked_super_close.assert_called_once_with(ws_name)
        mocked_manager.remove.assert_called_once_with(self.presenter, ws_name)

    @mock.patch("mantidqt.widgets.instrumentview.presenter.ObservingPresenter.close")
    @mock.patch("mantidqt.widgets.instrumentview.presenter.InstrumentViewManager")
    def test_close_with_different_ws_name(self, mocked_manager, mocked_super_close):
        ws_name = str(mock.NonCallableMock())
        self.presenter.close(ws_name)
        mocked_super_close.assert_not_called()
        mocked_manager.assert_not_called()


if __name__ == '__main__':
    unittest.main()
