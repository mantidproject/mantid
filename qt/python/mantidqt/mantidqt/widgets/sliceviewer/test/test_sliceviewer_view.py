# Mantid Repository : https://github.com/mantidproject/mantid
# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.
import io
import sys
import unittest
from threading import Thread
from unittest import mock
from unittest.mock import patch, DEFAULT

from mantid.simpleapi import (
    CreateMDHistoWorkspace, CreateMDWorkspace, CreateSampleWorkspace, SetUB)
from qtpy.QtWidgets import QApplication

from mantidqt.utils.qt.testing import start_qapplication
from mantidqt.utils.qt.testing.qt_widget_finder import QtWidgetFinder
from mantidqt.widgets.sliceviewer.presenter import SliceViewer
from mantidqt.widgets.sliceviewer.view import SCALENORM, SliceViewerView


class MockConfig(object):
    def get(self, name):
        if name == SCALENORM:
            return "Log"

    def set(self, name):
        pass

    def has(self, name):
        if name == SCALENORM:
            return True


class SliceViewerViewShim(SliceViewerView):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)

    def setLayout(self, _):
        return

    def addWidget(self, _):
        return


@start_qapplication
class SliceViewerViewTest(unittest.TestCase, QtWidgetFinder):
    @classmethod
    def setUpClass(cls):
        cls.histo_ws = CreateMDHistoWorkspace(Dimensionality=3,
                                              Extents='-3,3,-10,10,-1,1',
                                              SignalInput=range(100),
                                              ErrorInput=range(100),
                                              NumberOfBins='5,5,4',
                                              Names='Dim1,Dim2,Dim3',
                                              Units='MomentumTransfer,EnergyTransfer,Angstrom',
                                              OutputWorkspace='ws_MD_2d')
        cls.histo_ws_positive = CreateMDHistoWorkspace(Dimensionality=3,
                                                       Extents='-3,3,-10,10,-1,1',
                                                       SignalInput=range(1, 101),
                                                       ErrorInput=range(100),
                                                       NumberOfBins='5,5,4',
                                                       Names='Dim1,Dim2,Dim3',
                                                       Units='MomentumTransfer,EnergyTransfer,Angstrom',
                                                       OutputWorkspace='ws_MD_2d_pos')
        cls.hkl_ws = CreateMDWorkspace(Dimensions=3,
                                       Extents='-10,10,-9,9,-8,8',
                                       Names='A,B,C',
                                       Units='r.l.u.,r.l.u.,r.l.u.',
                                       Frames='HKL,HKL,HKL',
                                       OutputWorkspace='hkl_ws')
        expt_info = CreateSampleWorkspace()
        cls.hkl_ws.addExperimentInfo(expt_info)
        SetUB('hkl_ws', 1, 1, 1, 90, 90, 90)

    def setUp(self) -> None:
        self._patch_qt_elements()
        self.presenter_mock = mock.Mock()
        self.view = SliceViewerViewShim(presenter=self.presenter_mock, dims_info=mock.Mock(), can_normalise=mock.Mock())

    def _patch_qt_elements(self):
        self.dep_patcher = patch.multiple("mantidqt.widgets.sliceviewer.view",
                                          SliceViewerDataView=DEFAULT,
                                          QSplitter=DEFAULT,
                                          QHBoxLayout=DEFAULT
                                          )
        self.patched = self.dep_patcher.start()

    def tearDown(self):
        for ii in QApplication.topLevelWidgets():
            ii.close()
        QApplication.sendPostedEvents()
        QApplication.sendPostedEvents()
        self.assert_no_toplevel_widgets()
        self.dep_patcher.stop()

    def test_close_event(self):
        # TODO wire up presenter.close()
        ws = CreateSampleWorkspace()
        pres = SliceViewer(ws, model=mock.MagicMock())
        self.assert_widget_created()

        pres.view.close()
        pres = None
        QApplication.sendPostedEvents()

        self.assert_no_toplevel_widgets()

    # private methods
    def _assertNoErrorInADSHandlerFromSeparateThread(self, operation):
        """Check the error stream for any errors when calling operation.
        Raise an assertion error if errors are detected
        """
        try:
            # the ads handler catches all exceptions so that the handlers don't
            # bring down the sliceviewer. Check if anything is written to stderr
            stderr_capture = io.StringIO()
            stderr_orig = sys.stderr
            sys.stderr = stderr_capture
            op_thread = Thread(target=operation)
            op_thread.start()
            op_thread.join()
            self.assertTrue('Error occurred in handler' not in stderr_capture.getvalue(), msg=stderr_capture.getvalue())
        finally:
            sys.stderr = stderr_orig


if __name__ == '__main__':
    unittest.main()
