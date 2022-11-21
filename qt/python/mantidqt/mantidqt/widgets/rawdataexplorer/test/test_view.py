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

from mantidqt.widgets.rawdataexplorer.view import PreviewView

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

        # TODO check also for closing connection ?

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
        plt_close.assert_has_calls([mock.call(self.preview_view._widget)])

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
        plt_close.assert_has_calls([mock.call(self.preview_view._widget)])

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
        plt_close.assert_has_calls([mock.call(self.preview_view._widget)])

    @mock.patch('mantidqt.widgets.rawdataexplorer.view.get_window_config')
    @mock.patch('mantidqt.widgets.rawdataexplorer.view.get_instrumentview')
    def test_get_widget_iview(self, get_instrumentview, get_window_config):
        mocked_widget = mock.Mock()
        get_instrumentview.return_value = mocked_widget
        mocked_window_config = mock.Mock(), mock.Mock()
        get_window_config.return_value = mocked_window_config
        self.preview_view.set_type(self.preview_view.IVIEW)

        self.preview_view.get_widget("ws")

        get_instrumentview.assert_has_calls([mock.call("ws", True, mocked_window_config[0], mocked_window_config[1],
                                                       use_thread=False)])
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

        slice_viewer.assert_has_calls([mock.call(ws=mocked_ws)])
        self.assertEqual(self.preview_view._widget, mocked_widget)

    @mock.patch('mantidqt.widgets.rawdataexplorer.view.plotBin')
    def test_get_widget_plot1d(self, plot_bin):
        mocked_widget = mock.Mock(), mock.Mock()
        plot_bin.return_value = mocked_widget
        self.preview_view.set_type(self.preview_view.PLOT1D)

        self.preview_view.get_widget("ws")

        plot_bin.assert_has_calls([mock.call("ws", 0, error_bars=True)])
        self.assertEqual(self.preview_view._widget, mocked_widget)

    @mock.patch('mantidqt.widgets.rawdataexplorer.view.pcolormesh')
    def test_get_widget_plot2d(self, pcolormesh):
        mocked_widget = mock.Mock(), mock.Mock()
        pcolormesh.return_value = mocked_widget
        self.preview_view.set_type(self.preview_view.PLOT2D)

        self.preview_view.get_widget("ws")

        pcolormesh.assert_has_calls([mock.call(["ws"])])
        self.assertEqual(self.preview_view._widget, mocked_widget)

    @mock.patch('mantidqt.widgets.rawdataexplorer.view.plotSpectrum')
    def test_get_widget_plot_spectrum(self, plot_spectrum):
        mocked_widget = mock.Mock(), mock.Mock()
        plot_spectrum.return_value = mocked_widget
        self.preview_view.set_type(self.preview_view.PLOTSPECTRUM)

        self.preview_view.get_widget("ws")

        plot_spectrum.assert_has_calls([mock.call("ws", 0, error_bars=True)])
        self.assertEqual(self.preview_view._widget, mocked_widget)

    def test_change_workspace_iview(self):
        self.preview_view.set_type(self.preview_view.IVIEW)
        self.preview_view._widget = mock.MagicMock()
        ws_name = 'ws'

        self.preview_view.change_workspace(ws_name)

        self.preview_view._widget.replace_workspace.assert_has_calls([mock.call(ws_name)])

    def test_change_workspace_sview(self):
        # TODO first write the relevant code then test it
        pass

    @mock.patch('mantidqt.widgets.rawdataexplorer.view.plotBin')
    def test_change_workspace_plot1d(self, plot_bin):
        self.preview_view.set_type(self.preview_view.PLOT1D)
        self.preview_view._widget = mock.MagicMock()
        self.preview_view.set_presenter(mock.MagicMock())
        ws_name = "ws"

        self.preview_view.change_workspace(ws_name)

        plot_bin.assert_has_calls([mock.call(ws_name, 0, error_bars=True, window=self.preview_view._widget,
                                             clearWindow=True)])
        self.preview_view._presenter.get_main_view().fileTree.set_ignore_next_focus_out.\
            assert_has_calls([mock.call(True)])

    @mock.patch('mantidqt.widgets.rawdataexplorer.view.pcolormesh')
    def test_change_workspace_plot2d(self, pcolor):
        self.preview_view.set_type(self.preview_view.PLOT2D)
        self.preview_view._widget = mock.MagicMock()
        self.preview_view.set_presenter(mock.MagicMock())
        ws_name = "ws"

        self.preview_view.change_workspace(ws_name)

        pcolor.assert_has_calls([mock.call([ws_name], self.preview_view._widget)])
        self.preview_view._presenter.get_main_view().fileTree.set_ignore_next_focus_out.\
            assert_has_calls([mock.call(True)])

    @mock.patch('mantidqt.widgets.rawdataexplorer.view.plotSpectrum')
    def test_change_workspace_plotspectrum(self, plot_spec):
        self.preview_view.set_type(self.preview_view.PLOTSPECTRUM)
        self.preview_view._widget = mock.MagicMock()
        self.preview_view.set_presenter(mock.MagicMock())
        ws_name = "ws"

        self.preview_view.change_workspace(ws_name)

        plot_spec.assert_has_calls([mock.call(ws_name, 0, error_bars=True, window=self.preview_view._widget,
                                              clearWindow=True)])
        self.preview_view._presenter.get_main_view().fileTree.set_ignore_next_focus_out.\
            assert_has_calls([mock.call(True)])

    def test_on_close(self):
        mocked_presenter = mock.Mock()
        self.preview_view.set_presenter(mocked_presenter)

        self.preview_view.on_close("ws", True)
        mocked_presenter.close_preview.assert_has_calls([mock.call(True)])


if __name__ == "__main__":
    unittest.main()
