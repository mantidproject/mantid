from __future__ import print_function

from qtpy.QtCore import QPoint

from mantid.simpleapi import *
from mantidqt.utils.qt.test.gui_window_test import *

from workbench.plotting.functions import plot
from workbench.plotting.globalfiguremanager import GlobalFigureManager
from workbench.test.workbenchtests import runTests


class TestFitPropertyBrowser(WorkbenchGuiTest):

    def test_fit_range(self):
        ws = Load(r'irs26176_graphite002_conv_1LFixF_s0_to_9_Result.nxs', OutputWorkspace='ws')
        plot([ws], [1])
        manager = GlobalFigureManager.get_active()
        w = manager.window
        trigger_action(find_action_with_text(w, 'Fit'))
        yield 0.1
        fit_browser = manager.fit_browser
        start_x = fit_browser.startX()
        start_x_pxl = fit_browser.tool.fit_start_x.get_x_in_pixels()
        end_x = fit_browser.endX()
        end_x_pxl = fit_browser.tool.fit_end_x.get_x_in_pixels()
        self.assertAlmostEqual(start_x, 0.5318, 4)
        self.assertAlmostEqual(end_x, 1.8186, 4)
        pos = w._canvas.geometry().center()
        canvas = w.childAt(pos)
        pos.setX(fit_browser.tool.fit_start_x.get_x_in_pixels())
        new_pos = pos + QPoint(100, 0)
        yield drag_mouse(canvas, pos, new_pos)
        self.assertAlmostEqual(fit_browser.startX(), start_x + 100.0 / (end_x_pxl - start_x_pxl) * (end_x - start_x), 2)
        pos.setX(fit_browser.tool.fit_end_x.get_x_in_pixels())
        new_pos = pos - QPoint(30, 0)
        yield drag_mouse(canvas, pos, new_pos)
        self.assertAlmostEqual(fit_browser.endX(), end_x - 30.0 / (end_x_pxl - start_x_pxl) * (end_x - start_x), 2)


runTests(TestFitPropertyBrowser)
