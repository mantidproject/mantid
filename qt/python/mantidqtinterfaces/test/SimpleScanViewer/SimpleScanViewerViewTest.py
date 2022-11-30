# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

import unittest
from unittest import mock
import sys

from qtpy.QtTest import QTest
from qtpy.QtCore import Qt
from qtpy.QtWidgets import QApplication

from mantid.simpleapi import config

from mantidqtinterfaces.simplescanviewer.view import SimpleScanViewerView
from mantidqt.widgets.sliceviewer.views.view import SliceViewerDataView

app = QApplication(sys.argv)


class SimpleScanViewerViewTest(unittest.TestCase):

    def setUp(self) -> None:
        self.facility = config['default.facility']
        self.instrument = config['default.instrument']
        config['default.facility'] = "ILL"
        config['default.instrument'] = "D16"

        patch = mock.patch(
                'mantidqtinterfaces.simplescanviewer.presenter.SimpleScanViewerPresenter')
        self.mocked_presenter = patch.start()
        self.addCleanup(patch.stop)

        self.view = SimpleScanViewerView()
        self.view.presenter = self.mocked_presenter

    def tearDown(self) -> None:
        config['default.facility'] = self.facility
        config['default.instrument'] = self.instrument

    def test_browse_button_file_provided(self):
        self.view._open_file_dialog = mock.Mock()  # we don't want a dialog opening during the test
        self.view._open_file_dialog.return_value = "/file.nxs"
        self.view.sig_file_selected = mock.Mock()

        QTest.mouseClick(self.view.browse_button, Qt.LeftButton)
        self.view._open_file_dialog.assert_called_once()
        self.view.sig_file_selected.emit.assert_called_once()

    def test_browse_button_no_file(self):
        self.view._open_file_dialog = mock.Mock()  # we don't want a dialog opening during the test
        self.view._open_file_dialog.return_value = None
        self.view.sig_file_selected = mock.Mock()

        QTest.mouseClick(self.view.browse_button, Qt.LeftButton)
        self.view._open_file_dialog.assert_called_once()
        self.view.sig_file_selected.emit.assert_not_called()

    def test_set_background_button_file_provided(self):
        self.view._open_file_dialog = mock.Mock()  # we don't want a dialog opening during the test
        self.view._open_file_dialog.return_value = "/file.nxs"
        self.view.sig_background_selected = mock.Mock()

        QTest.mouseClick(self.view.background_button, Qt.LeftButton)
        self.view._open_file_dialog.assert_called_once()
        self.view.sig_background_selected.emit.assert_called_once()

    def test_background_button_no_file(self):
        self.view._open_file_dialog = mock.Mock()  # we don't want a dialog opening during the test
        self.view._open_file_dialog.return_value = None
        self.view.sig_background_selected = mock.Mock()

        QTest.mouseClick(self.view.background_button, Qt.LeftButton)
        self.view._open_file_dialog.assert_called_once()
        self.view.sig_background_selected.emit.assert_not_called()

    def test_initialize_rect_manager(self):
        self.patcher = mock.patch.multiple("mantidqt.widgets.sliceviewer.views.dataview",
                                           DimensionWidget=mock.DEFAULT,
                                           QGridLayout=mock.DEFAULT,
                                           QHBoxLayout=mock.DEFAULT,
                                           QVBoxLayout=mock.DEFAULT,
                                           QWidget=mock.DEFAULT,
                                           QLabel=mock.DEFAULT,
                                           QStatusBar=mock.DEFAULT,
                                           CurveLinearSubPlot=mock.DEFAULT,
                                           ColorbarWidget=mock.DEFAULT,
                                           SliceViewerNavigationToolbar=mock.DEFAULT)
        self.patched_objs = self.patcher.start()
        self.view._data_view = SliceViewerDataView(self.mocked_presenter, mock.MagicMock(), mock.Mock())
        self.view._data_view.add_line_plots = lambda x, y: 0
        self.view.initialize_rectangle_manager()

        self.assertEqual(self.view.lower_splitter.count(), 1)


if __name__ == "__main__":
    unittest.main()
