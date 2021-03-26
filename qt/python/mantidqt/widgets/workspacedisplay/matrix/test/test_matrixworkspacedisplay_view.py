# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.
import unittest
from unittest import mock
from unittest.mock import patch

from mantid.simpleapi import CreateSampleWorkspace
from mantidqt.utils.qt.testing import start_qapplication
from mantidqt.widgets.workspacedisplay.matrix.presenter import MatrixWorkspaceDisplay
from mantidqt.utils.qt.testing.qt_widget_finder import QtWidgetFinder
from qtpy.QtWidgets import QApplication


@start_qapplication
class MatrixWorkspaceDisplayViewTest(unittest.TestCase, QtWidgetFinder):
    def test_window_deleted_correctly(self):
        ws = CreateSampleWorkspace()

        p = MatrixWorkspaceDisplay(ws)
        self.assert_widget_created()
        p.close(ws.name())

        self.assert_widget_created()

        QApplication.sendPostedEvents()

        self.assertEqual(None, p.ads_observer)
        self.assert_widget_not_present("work")
        self.assert_no_toplevel_widgets()

    def test_window_force_deleted_correctly(self):
        ws = CreateSampleWorkspace()

        p = MatrixWorkspaceDisplay(ws)
        self.assert_widget_created()

        p.force_close()

        self.assert_widget_created()

        QApplication.sendPostedEvents()

        self.assertEqual(None, p.ads_observer)
        self.assert_widget_not_present("work")
        self.assert_no_toplevel_widgets()

    def test_context_has_expected_function_when_plotting(self):
        ws = CreateSampleWorkspace()
        mock_plot = mock.MagicMock()
        presenter = MatrixWorkspaceDisplay(ws, plot=mock_plot)
        view = presenter.view
        table = view.currentWidget()
        table.selectColumn(1)

        context_menu = view.setup_bin_context_menu(table)
        actions = context_menu.actions()
        # check we have 8 actions 2 separators - 2 copy - 2plot - 2 overplot
        self.assertEqual(len(actions), 8)

        # check triggering action 4 & 5 calls plot
        actions[3].trigger()
        presenter.plot.assert_called_with(mock.ANY,
                                          wksp_indices=mock.ANY,
                                          errors=False,
                                          overplot=False,
                                          plot_kwargs=mock.ANY)
        actions[4].trigger()
        presenter.plot.assert_called_with(mock.ANY,
                                          wksp_indices=mock.ANY,
                                          errors=True,
                                          overplot=False,
                                          plot_kwargs=mock.ANY)
        presenter.close(ws.name())

    @patch('mantidqt.widgets.workspacedisplay.matrix.view.can_overplot')
    def test_context_has_expected_function_when_overplotting(self, mock_can_overplot):
        mock_can_overplot.return_value = True
        ws = CreateSampleWorkspace()
        mock_plot = mock.MagicMock()
        presenter = MatrixWorkspaceDisplay(ws, plot=mock_plot)
        view = presenter.view
        table = view.currentWidget()
        table.selectColumn(1)

        context_menu = view.setup_bin_context_menu(table)
        actions = context_menu.actions()

        # check triggering action 6 & 7 calls plot
        actions[6].trigger()
        presenter.plot.assert_called_with(mock.ANY,
                                          wksp_indices=mock.ANY,
                                          errors=False,
                                          overplot=True,
                                          plot_kwargs=mock.ANY)
        actions[7].trigger()
        presenter.plot.assert_called_with(mock.ANY,
                                          wksp_indices=mock.ANY,
                                          errors=True,
                                          overplot=True,
                                          plot_kwargs=mock.ANY)
        presenter.close(ws.name())

    @patch('mantidqt.widgets.workspacedisplay.matrix.view.can_overplot')
    def test_context_menu_correctly_disables_and_enables_overplot_options(self, mock_can_overplot):
        mock_can_overplot.return_value = False
        ws = CreateSampleWorkspace()
        mock_plot = mock.MagicMock()
        presenter = MatrixWorkspaceDisplay(ws, plot=mock_plot)
        view = presenter.view
        table = view.currentWidget()

        context_menu = view.setup_bin_context_menu(table)
        actions = context_menu.actions()
        self.assertEqual(actions[6].isEnabled(), False)
        self.assertEqual(actions[7].isEnabled(), False)

        mock_can_overplot.return_value = True
        context_menu = view.setup_bin_context_menu(table)
        actions = context_menu.actions()
        self.assertEqual(actions[6].isEnabled(), True)
        self.assertEqual(actions[7].isEnabled(), True)

        presenter.close(ws.name())


if __name__ == "__main__":
    unittest.main()
