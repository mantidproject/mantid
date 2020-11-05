# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.
import unittest

import matplotlib as mpl

from mantidqt.widgets.colorbar.colorbar import MIN_LOG_VALUE

mpl.use('Agg')  # noqa
from mantid.simpleapi import (CreateMDHistoWorkspace, CreateMDWorkspace, CreateSampleWorkspace,
                              SetUB)
from mantidqt.utils.qt.testing import start_qapplication
from mantidqt.utils.qt.testing.qt_widget_finder import QtWidgetFinder
from mantidqt.widgets.sliceviewer.presenter import SliceViewer
from mantidqt.widgets.sliceviewer.toolbar import ToolItemText
from qtpy.QtWidgets import QApplication
from math import inf


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
        cls.hkl_ws = CreateMDWorkspace(Dimensions=3,
                                       Extents='-10,10,-10,10,-10,10',
                                       Names='A,B,C',
                                       Units='r.l.u.,r.l.u.,r.l.u.',
                                       Frames='HKL,HKL,HKL',
                                       OutputWorkspace='hkl_ws')
        expt_info = CreateSampleWorkspace()
        cls.hkl_ws.addExperimentInfo(expt_info)
        SetUB('hkl_ws', 1, 1, 1, 90, 90, 90)

    def test_deleted_on_close(self):
        pres = SliceViewer(self.histo_ws)
        self.assert_widget_created()
        pres.view.close()

        QApplication.sendPostedEvents()
        QApplication.sendPostedEvents()

        self.assert_no_toplevel_widgets()

    def test_enable_nonorthogonal_view_disables_lineplots_if_enabled(self):
        pres = SliceViewer(self.hkl_ws)
        line_plots_action, region_sel_action, non_ortho_action = toolbar_actions(
            pres,
            (ToolItemText.LINEPLOTS, ToolItemText.REGIONSELECTION, ToolItemText.NONORTHOGONAL_AXES))
        line_plots_action.trigger()
        QApplication.sendPostedEvents()

        non_ortho_action.trigger()
        QApplication.sendPostedEvents()

        self.assertTrue(non_ortho_action.isChecked())
        self.assertFalse(line_plots_action.isChecked())
        self.assertFalse(line_plots_action.isEnabled())
        self.assertFalse(region_sel_action.isChecked())
        self.assertFalse(region_sel_action.isEnabled())

        pres.view.close()

    def test_disable_lineplots_disables_region_selector(self):
        pres = SliceViewer(self.hkl_ws)
        line_plots_action, region_sel_action = toolbar_actions(
            pres, (ToolItemText.LINEPLOTS, ToolItemText.REGIONSELECTION))
        line_plots_action.trigger()
        region_sel_action.trigger()
        QApplication.sendPostedEvents()

        line_plots_action.trigger()
        QApplication.sendPostedEvents()

        self.assertFalse(line_plots_action.isChecked())
        self.assertFalse(region_sel_action.isChecked())

        pres.view.close()

    def test_clim_edits_prevent_negative_values_if_lognorm(self):
        pres = SliceViewer(self.histo_ws)
        colorbar = pres.view.data_view.colorbar
        colorbar.autoscale.setChecked(False)
        colorbar.norm.setCurrentText("Log")
        prev_bottom_limit = colorbar.cmin.text()
        prev_top_limit = colorbar.cmax.text()

        colorbar.cmin.textChanged.emit(str(-10))
        colorbar.cmax.textChanged.emit(str(-5))

        self.assertEqual(colorbar.cmin.text(), prev_bottom_limit)
        self.assertEqual(colorbar.cmax.text(), prev_top_limit)

        pres.view.close()

    def test_changing_norm_updates_clim_validators(self):
        pres = SliceViewer(self.histo_ws)
        colorbar = pres.view.data_view.colorbar
        colorbar.autoscale.setChecked(False)

        colorbar.norm.setCurrentText("Log")
        self.assertEqual(colorbar.cmin.validator().bottom(), MIN_LOG_VALUE)

        colorbar.norm.setCurrentText("Linear")
        self.assertEqual(colorbar.cmin.validator().bottom(), -inf)

        pres.view.close()


# private helper functions


def toolbar_actions(presenter, text_labels):
    """
    Return a list of actions with the given text labels
    :param presenter: Presenter containing view
    :param text_labels: A list of text labels on toolbar buttons
    :return: A list of QAction objects matching the text labels given
    """
    toolbar_actions = presenter.view.data_view.mpl_toolbar.actions()
    return [action for action in toolbar_actions if action.text() in text_labels]


if __name__ == '__main__':
    unittest.main()
