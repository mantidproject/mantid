# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

import unittest
from unittest import mock
import sys

from qtpy.QtWidgets import QApplication
from qtpy.QtCore import QDir

from mantidqt.widgets.rawdataexplorer.view import PreviewView, RawDataExplorerView

app = QApplication(sys.argv)


class PreviewViewTest(unittest.TestCase):
    def setUp(self) -> None:
        self.preview_view = PreviewView()

    def test_show_workspace_iview(self):
        self.preview_view.get_widget = mock.Mock()
        self.preview_view._type = self.preview_view.IVIEW
        self.preview_view.set_presenter(mock.MagicMock())

        self.preview_view._widget = mock.MagicMock()
        self.preview_view.on_close = mock.MagicMock()
        ws_name = 'ws'

        self.preview_view.show_workspace(ws_name)

        self.preview_view._widget.show_view.assert_called_once()

        self.preview_view.sig_request_close.emit()

        self.preview_view.on_close.assert_called_once()
        self.preview_view._widget.container.emit_close.assert_called_once()

    def test_show_workspace_sview(self):
        self.preview_view.get_widget = mock.Mock()
        self.preview_view._type = self.preview_view.SVIEW
        self.preview_view.set_presenter(mock.MagicMock())

        self.preview_view._widget = mock.MagicMock()
        self.preview_view.on_close = mock.MagicMock()
        ws_name = 'ws'

        self.preview_view.show_workspace(ws_name)

        self.preview_view.get_widget.assert_called_once()
        self.preview_view._widget.view.show.assert_called_once()

        self.preview_view.sig_request_close.emit()

        self.preview_view.on_close.assert_called_once()
        self.preview_view._widget.view.emit_close.assert_called_once()

    @mock.patch('mantidqt.widgets.rawdataexplorer.view.plt.close')
    def test_show_workspace_plot2d(self, plt_close):
        self.preview_view.get_widget = mock.Mock()
        self.preview_view._type = self.preview_view.PLOT2D
        self.preview_view.set_presenter(mock.MagicMock())

        self.preview_view._widget = mock.MagicMock()
        self.preview_view.on_close = mock.MagicMock()
        ws_name = 'ws'

        self.preview_view.show_workspace(ws_name)

        self.preview_view.get_widget.assert_called_once()

        self.preview_view.sig_request_close.emit()

        self.preview_view.on_close.assert_called_once()
        plt_close.assert_called_once_with(self.preview_view._widget)

    @mock.patch('mantidqt.widgets.rawdataexplorer.view.plt.close')
    def test_show_workspace_plot1d(self, plt_close):
        self.preview_view.get_widget = mock.Mock()
        self.preview_view._type = self.preview_view.PLOT1D
        self.preview_view.set_presenter(mock.MagicMock())

        self.preview_view._widget = mock.MagicMock()
        self.preview_view.on_close = mock.MagicMock()
        ws_name = 'ws'

        self.preview_view.show_workspace(ws_name)

        self.preview_view.get_widget.assert_called_once()

        self.preview_view.sig_request_close.emit()

        self.preview_view.on_close.assert_called_once()
        plt_close.assert_called_once_with(self.preview_view._widget)

    @mock.patch('mantidqt.widgets.rawdataexplorer.view.plt.close')
    def test_show_workspace_plotspectrum(self, plt_close):
        self.preview_view.get_widget = mock.Mock()
        self.preview_view._type = self.preview_view.PLOTSPECTRUM
        self.preview_view.set_presenter(mock.MagicMock())

        self.preview_view._widget = mock.MagicMock()
        self.preview_view.on_close = mock.MagicMock()
        ws_name = 'ws'

        self.preview_view.show_workspace(ws_name)

        self.preview_view.get_widget.assert_called_once()

        self.preview_view.sig_request_close.emit()

        self.preview_view.on_close.assert_called_once()
        plt_close.assert_called_once_with(self.preview_view._widget)

    @mock.patch('mantidqt.widgets.rawdataexplorer.view.get_window_config')
    @mock.patch('mantidqt.widgets.rawdataexplorer.view.get_instrumentview')
    def test_get_widget_iview(self, get_instrumentview, get_window_config):
        mocked_widget = mock.Mock()
        get_instrumentview.return_value = mocked_widget
        mocked_window_config = mock.Mock(), mock.Mock()
        get_window_config.return_value = mocked_window_config
        self.preview_view.set_type(self.preview_view.IVIEW)

        self.preview_view.get_widget("ws")

        get_instrumentview.assert_called_once_with("ws", True, mocked_window_config[0], mocked_window_config[1],
                                                   use_thread=False)
        self.assertEqual(self.preview_view._widget, mocked_widget)

    @mock.patch('mantidqt.widgets.rawdataexplorer.view.mtd')
    @mock.patch('mantidqt.widgets.rawdataexplorer.view.SliceViewer')
    def test_get_widget_sview(self, slice_viewer, mtd):
        mocked_widget = mock.Mock(), mock.Mock()
        slice_viewer.return_value = mocked_widget
        mocked_ws = mock.Mock()
        mtd.__getitem__.return_value = mocked_ws
        self.preview_view.set_type(self.preview_view.SVIEW)

        self.preview_view.get_widget("ws")

        slice_viewer.assert_called_once_with(ws=mocked_ws)
        self.assertEqual(self.preview_view._widget, mocked_widget)

    @mock.patch('mantidqt.widgets.rawdataexplorer.view.plotBin')
    def test_get_widget_plot1d(self, plot_bin):
        mocked_widget = mock.Mock(), mock.Mock()
        plot_bin.return_value = mocked_widget
        self.preview_view.set_type(self.preview_view.PLOT1D)

        self.preview_view.get_widget("ws")

        plot_bin.assert_called_once_with("ws", 0, error_bars=True)
        self.assertEqual(self.preview_view._widget, mocked_widget)

    @mock.patch('mantidqt.widgets.rawdataexplorer.view.pcolormesh')
    def test_get_widget_plot2d(self, pcolormesh):
        mocked_widget = mock.Mock(), mock.Mock()
        pcolormesh.return_value = mocked_widget
        self.preview_view.set_type(self.preview_view.PLOT2D)

        self.preview_view.get_widget("ws")

        pcolormesh.assert_called_once_with(["ws"])
        self.assertEqual(self.preview_view._widget, mocked_widget)

    @mock.patch('mantidqt.widgets.rawdataexplorer.view.plotSpectrum')
    def test_get_widget_plot_spectrum(self, plot_spectrum):
        mocked_widget = mock.Mock(), mock.Mock()
        plot_spectrum.return_value = mocked_widget
        self.preview_view.set_type(self.preview_view.PLOTSPECTRUM)

        self.preview_view.get_widget("ws")

        plot_spectrum.assert_called_once_with("ws", 0, error_bars=True)
        self.assertEqual(self.preview_view._widget, mocked_widget)

    def test_change_workspace_iview(self):
        self.preview_view.set_type(self.preview_view.IVIEW)
        self.preview_view._widget = mock.MagicMock()
        ws_name = 'ws'

        self.preview_view.change_workspace(ws_name)

        self.preview_view._widget.replace_workspace.assert_called_once_with(ws_name)

    def test_change_workspace_sview(self):
        # First write the relevant code for the use of slice viewer, then test it
        pass

    @mock.patch('mantidqt.widgets.rawdataexplorer.view.plotBin')
    def test_change_workspace_plot1d(self, plot_bin):
        self.preview_view.set_type(self.preview_view.PLOT1D)
        self.preview_view._widget = mock.MagicMock()
        self.preview_view.set_presenter(mock.MagicMock())
        ws_name = "ws"

        self.preview_view.change_workspace(ws_name)

        plot_bin.assert_called_once_with(ws_name, 0, error_bars=True, window=self.preview_view._widget,
                                         clearWindow=True)
        self.preview_view._presenter.get_main_view().fileTree.set_ignore_next_focus_out.assert_called_once_with(True)

    @mock.patch('mantidqt.widgets.rawdataexplorer.view.pcolormesh')
    def test_change_workspace_plot2d(self, pcolor):
        self.preview_view.set_type(self.preview_view.PLOT2D)
        self.preview_view._widget = mock.MagicMock()
        self.preview_view.set_presenter(mock.MagicMock())
        ws_name = "ws"

        self.preview_view.change_workspace(ws_name)

        pcolor.assert_called_once_with([ws_name], self.preview_view._widget)
        self.preview_view._presenter.get_main_view().fileTree.set_ignore_next_focus_out.assert_called_once_with(True)

    @mock.patch('mantidqt.widgets.rawdataexplorer.view.plotSpectrum')
    def test_change_workspace_plotspectrum(self, plot_spec):
        self.preview_view.set_type(self.preview_view.PLOTSPECTRUM)
        self.preview_view._widget = mock.MagicMock()
        self.preview_view.set_presenter(mock.MagicMock())
        ws_name = "ws"

        self.preview_view.change_workspace(ws_name)

        plot_spec.assert_called_once_with(ws_name, 0, error_bars=True, window=self.preview_view._widget,
                                          clearWindow=True)
        self.preview_view._presenter.get_main_view().fileTree.set_ignore_next_focus_out.assert_called_once_with(True)

    def test_on_close(self):
        mocked_presenter = mock.Mock()
        self.preview_view.set_presenter(mocked_presenter)

        self.preview_view.on_close("ws", True)
        mocked_presenter.close_preview.assert_called_once_with(True)


class RawDataExplorerViewTest(unittest.TestCase):

    def setUp(self) -> None:
        self.mocked_presenter = mock.MagicMock()
        self.view = RawDataExplorerView(self.mocked_presenter)

        self.view.browse.clicked.disconnect()
        self.view.fileTree.sig_new_current.disconnect()

    def test_add_preview(self):
        preview = self.view.add_preview()
        self.assertEqual(len(self.view._previews), 1)
        self.assertEqual(self.view._previews[0], preview)

    def test_del_preview_present(self):
        preview1 = self.view.add_preview()
        preview2 = self.view.add_preview()

        self.view.del_preview(preview2)

        self.assertEqual(len(self.view._previews), 1)
        self.assertEqual(self.view._previews[0], preview1)

    def test_del_preview_absent(self):
        preview1 = self.view.add_preview()
        preview2 = self.view.add_preview()

        self.view.del_preview(preview2)
        self.view.del_preview(preview2)

        self.assertEqual(len(self.view._previews), 1)
        self.assertEqual(self.view._previews[0], preview1)

    def test_clear_selection(self):
        self.view.fileTree.clear_selection = mock.Mock()

        self.view.clear_selection()

        self.view.fileTree.clear_selection.assert_called_once()  # rather assert on the state of the file tree ?
        self.assertEqual(self.view._current_selection, set())

    def test_select_last_clicked(self):
        # TODO there really is no satisfactory way to test that without a fake file system so the file model is sensible
        pass

    def test_on_item_selected(self):

        # TODO this only checks the code actually runs (and then not by much since we don't enter the for loop)
        # but without pyfakefs (and possibly even with it, I'm not sure how well it interacts with Qt's C++ backend)
        # testing this function requires basically mocking all high level calls to the file model which is both a pain
        # and stupid since we would in the end only be checking that mock works correctly, having replaced every
        # relevant method
        index = mock.MagicMock()
        self.view.on_item_selected(index)

    def test_set_up_connections(self):
        self.view.show_directory_manager = mock.Mock()
        self.view.on_item_selected = mock.Mock()

        self.view.setup_connections()

        self.view.browse.clicked.emit()
        self.view.show_directory_manager.assert_called_once()

    @mock.patch("mantidqt.widgets.rawdataexplorer.view.QFileDialog")
    def test_show_directory_manager(self, dialog):
        self.view.fileTree.model().setRootPath(QDir.rootPath())
        trigger_check = mock.Mock()
        self.view.file_tree_path_changed.connect(trigger_check)
        path = str(QDir.rootPath())
        dialog().getExistingDirectory.return_value = path  # so qt accepts to send a signal

        self.view.show_directory_manager()

        # check that the options are the correct ones. Because QFileDialog has been overwritten, it is a bit strange
        # for some reason Qt returns the root path in posix format even on windows
        dialog().getExistingDirectory.assert_called_with(parent=self.view,
                                                         caption="Select a directory",
                                                         directory=str(QDir.rootPath()),
                                                         options=dialog.DontUseNativeDialog | dialog.ShowDirsOnly)

        trigger_check.assert_called_with(path)


if __name__ == "__main__":
    unittest.main()
