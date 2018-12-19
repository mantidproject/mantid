from __future__ import print_function

from qtpy.QtCore import QPoint
from qtpy.QtGui import QCursor

from mantid.simpleapi import *
from mantidqt.utils.qt.test.gui_window_test import *

from workbench.plotting.functions import plot
from workbench.plotting.globalfiguremanager import GlobalFigureManager
from workbench.test.workbenchtests import runTests


class TestFitPropertyBrowser(WorkbenchGuiTest):

    def start(self):
        if 'ws' not in mtd:
            ws = Load(r'irs26176_graphite002_conv_1LFixF_s0_to_9_Result.nxs', OutputWorkspace='ws')
        else:
            ws = mtd['ws']
        plot([ws], [1])
        manager = GlobalFigureManager.get_active()
        self.w = manager.window
        trigger_action(find_action_with_text(self.w, 'Fit'))
        yield 0.1
        self.fit_browser = manager.fit_browser

    def move_marker(self, canvas, marker, pos, dx):
        tr = self.fit_browser.tool.ax.get_xaxis_transform()
        x0 = tr.transform((0, 0))[0]
        dx_pxl = tr.transform((dx, 0))[0] - x0
        pos0 = QCursor.pos()
        pos.setX(marker.get_x_in_pixels())
        new_pos = pos + QPoint(dx_pxl, 0)
        yield drag_mouse(canvas, pos, new_pos)
        pos1 = QCursor.pos()
        if pos0 == pos1:
            new_x = marker.x + dx
            marker.on_click(pos.x())
            marker.mouse_move(new_x)
            marker.stop()

    def move_start_x(self, canvas, pos, dx):
        return self.move_marker(canvas, self.fit_browser.tool.fit_start_x, pos, dx)

    def move_end_x(self, canvas, pos, dx):
        return self.move_marker(canvas, self.fit_browser.tool.fit_end_x, pos, dx)

    def test_fit_range(self):
        yield self.start()
        start_x = self.fit_browser.startX()
        end_x = self.fit_browser.endX()
        self.assertGreater(end_x, start_x)
        self.assertGreater(start_x, 0.3)
        self.assertGreater(2.0, end_x)
        pos = self.w._canvas.geometry().center()
        canvas = self.w.childAt(pos)
        yield self.move_start_x(canvas, pos, 0.5)
        self.assertAlmostEqual(self.fit_browser.startX(), start_x + 0.5, 2)
        yield self.move_end_x(canvas, pos, -0.25)
        self.assertAlmostEqual(self.fit_browser.endX(), end_x - 0.25, 2)

    def test_fit_range_start_moved_too_far(self):
        yield self.start()
        start_x = self.fit_browser.startX()
        end_x = self.fit_browser.endX()
        self.assertGreater(end_x, start_x)
        self.assertGreater(start_x, 0.3)
        self.assertGreater(2.0, end_x)
        pos = self.w._canvas.geometry().center()
        canvas = self.w.childAt(pos)
        yield self.move_end_x(canvas, pos, -0.5)
        new_end_x = self.fit_browser.endX()
        self.assertAlmostEqual(new_end_x, end_x - 0.5, 2)
        yield self.move_start_x(canvas, pos, 1.0)
        self.assertAlmostEqual(self.fit_browser.startX(), new_end_x)

    def test_fit_range_end_moved_too_far(self):
        yield self.start()
        start_x = self.fit_browser.startX()
        end_x = self.fit_browser.endX()
        self.assertGreater(end_x, start_x)
        self.assertGreater(start_x, 0.3)
        self.assertGreater(2.0, end_x)
        pos = self.w._canvas.geometry().center()
        canvas = self.w.childAt(pos)
        yield self.move_start_x(canvas, pos, 0.5)
        new_start_x = self.fit_browser.startX()
        self.assertAlmostEqual(new_start_x, start_x + 0.5, 2)
        yield self.move_end_x(canvas, pos, -1.0)
        self.assertAlmostEqual(self.fit_browser.endX(), new_start_x)

    def test_fit_range_moved_start_outside(self):
        yield self.start()
        start_x_pxl = self.fit_browser.tool.fit_start_x.get_x_in_pixels()
        pos = self.w._canvas.geometry().center()
        canvas = self.w.childAt(pos)
        yield self.move_start_x(canvas, pos, -2.0)
        self.assertAlmostEqual(start_x_pxl, self.fit_browser.tool.fit_start_x.get_x_in_pixels())

    def test_fit_range_moved_end_outside(self):
        yield self.start()
        end_x_pxl = self.fit_browser.tool.fit_end_x.get_x_in_pixels()
        pos = self.w._canvas.geometry().center()
        canvas = self.w.childAt(pos)
        yield self.move_end_x(canvas, pos, 2.0)
        self.assertAlmostEqual(end_x_pxl, self.fit_browser.tool.fit_end_x.get_x_in_pixels())

    def test_fit_range_set_start(self):
        yield self.start()
        self.fit_browser.setStartX(0.7)
        self.assertAlmostEqual(self.fit_browser.tool.fit_start_x.x, 0.7)

    def test_fit_range_set_start_outside(self):
        yield self.start()
        self.fit_browser.setStartX(0.1)
        self.assertAlmostEqual(self.fit_browser.tool.fit_start_x.x, 0.1)

    def test_fit_range_set_start_outside_right(self):
        yield self.start()
        self.fit_browser.setStartX(2.0)
        self.assertAlmostEqual(self.fit_browser.tool.fit_start_x.x, self.fit_browser.endX())

    def test_fit_range_set_end(self):
        yield self.start()
        self.fit_browser.setEndX(1.0)
        self.assertAlmostEqual(self.fit_browser.tool.fit_end_x.x, 1.0)

    def test_fit_range_set_end_outside(self):
        yield self.start()
        self.fit_browser.setEndX(2.0)
        self.assertAlmostEqual(self.fit_browser.tool.fit_end_x.x, 2.0)

    def test_fit_range_set_end_outside_left(self):
        yield self.start()
        self.fit_browser.setEndX(0.3)
        self.assertAlmostEqual(self.fit_browser.tool.fit_end_x.x, self.fit_browser.startX())

    def test_fit(self):
        yield self.start()
        self.fit_browser.loadFunction('name=LinearBackground')
        self.fit_browser.fit()
        yield self.wait_for_true(lambda: 'ws_Workspace' in mtd)
        # self.assertEqual(self.fit_browser.getFittingFunction(), "name=LinearBackground,A0=4.74354,A1=-0.442138")
        self.assertEqual(len(self.fit_browser.fit_result_lines), 2)
        del mtd['ws_Workspace']
        self.fit_browser.fit()
        yield self.wait_for_true(lambda: 'ws_Workspace' in mtd)
        self.assertEqual(len(self.fit_browser.fit_result_lines), 2)


runTests(TestFitPropertyBrowser)
