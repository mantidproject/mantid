from __future__ import print_function
import unittest

from qtpy.QtCore import QPoint
from qtpy.QtGui import QCursor, QContextMenuEvent
from qtpy.QtWidgets import QApplication

from mantid.simpleapi import *
from mantidqt.utils.qt.test.gui_window_test import *

from workbench.plotting.functions import plot, pcolormesh
from workbench.plotting.globalfiguremanager import GlobalFigureManager
from workbench.test.workbenchtests import runTests


def on_darwin():
    return sys.platform == 'darwin'


@unittest.skipIf(on_darwin(), "Couldn't make it work for a mac")
class TestFitPropertyBrowser(WorkbenchGuiTest):

    def start(self, ws_name='ws', fit=True):
        if ws_name not in mtd:
            ws = Load(r'irs26176_graphite002_conv_1LFixF_s0_to_9_Result.nxs', OutputWorkspace=ws_name)
        else:
            ws = mtd[ws_name]
        plot([ws], [1])
        self.figure_manager = GlobalFigureManager.get_active()
        self.fit_browser = self.figure_manager.fit_browser
        self.w = self.figure_manager.window
        self.fit_action = find_action_with_text(self.w, 'Fit')
        if fit:
            yield self.start_fit()

    def start_fit(self):
        trigger_action(self.fit_action)
        yield 0.1

    def start_draw_calls_count(self):
        self.draw_count = 0

        def increment(ev):
            self.draw_count += 1
        self.fit_browser.canvas.mpl_connect('draw_event', increment)

    def get_canvas(self):
        pos = self.w._canvas.geometry().center()
        return self.w.childAt(pos)

    def move_marker(self, canvas, marker, pos, dx, try_other_way_if_failed):
        tr = self.fit_browser.tool.ax.get_xaxis_transform()
        x0 = tr.transform((0, 0))[0]
        dx_pxl = tr.transform((dx, 0))[0] - x0
        pos.setX(marker.get_x_in_pixels())
        new_pos = pos + QPoint(dx_pxl, 0)
        yield drag_mouse(canvas, pos, new_pos)
        pos1 = canvas.mapFromGlobal(QCursor.pos())
        if try_other_way_if_failed and pos1 != new_pos:
            new_x = marker.x + dx
            marker.button_press_callback(pos.x())
            marker.mouse_move(new_x)
            yield 0.1
            marker.stop()

    def drag_mouse(self, x, y, x1, y1):
        canvas = self.get_canvas()
        h = canvas.height()
        tr = self.fit_browser.tool.get_transform()
        x1_pxl, y1_pxl = tr.transform_affine((x1, y1))
        x_pxl, y_pxl = tr.transform_affine((x, y))
        pos = QPoint(x_pxl, h - y_pxl)
        new_pos = QPoint(x1_pxl, h - y1_pxl)
        yield drag_mouse(canvas, pos, new_pos)

    def mouse_click(self, x, y):
        canvas = self.get_canvas()
        h = canvas.height()
        tr = self.fit_browser.tool.get_transform()
        x_pxl, y_pxl = tr.transform_affine((x, y))
        pos = QPoint(x_pxl, h - y_pxl)
        yield mouse_click(canvas, pos)

    def context_menu(self):
        canvas = self.get_canvas()
        QApplication.instance().postEvent(canvas, QContextMenuEvent(2, QPoint()))
        yield self.wait_for_popup()

    def move_start_x(self, canvas, pos, dx, try_other_way_if_failed=True):
        return self.move_marker(canvas, self.fit_browser.tool.fit_start_x, pos, dx,
                                try_other_way_if_failed=try_other_way_if_failed)

    def move_end_x(self, canvas, pos, dx, try_other_way_if_failed=True):
        return self.move_marker(canvas, self.fit_browser.tool.fit_end_x, pos, dx,
                                try_other_way_if_failed=try_other_way_if_failed)

    def test_fit_on_off(self):
        yield self.start()
        self.start_draw_calls_count()
        self.assertTrue(self.fit_browser.isVisible())
        trigger_action(self.fit_action)
        yield self.wait_for_true(lambda: not self.fit_browser.isVisible())
        self.assertFalse(self.fit_browser.isVisible())
        self.assertEqual(self.draw_count, 1)

    def test_dock_undock(self):
        yield self.start()
        self.start_draw_calls_count()
        self.fit_browser.setFloating(True)
        yield self.wait_for_true(lambda: not self.fit_browser.isFloating())
        trigger_action(self.fit_action)
        yield self.wait_for_true(lambda: not self.fit_browser.isVisible())
        self.assertFalse(self.fit_browser.isVisible())
        self.assertTrue(self.fit_browser.tool is None)
        self.assertEqual(self.draw_count, 2)
        trigger_action(self.fit_action)
        yield self.wait_for_true(self.fit_browser.isVisible)
        self.assertTrue(self.fit_browser.isVisible())
        self.assertFalse(self.fit_browser.tool is None)
        self.assertEqual(self.draw_count, 3)

    def test_zoom_on_off(self):
        yield self.start(fit=False)
        self.figure_manager.toolbar.zoom()
        yield self.wait_for_true(self.fit_browser.toolbar_state_checker.is_tool_active)
        yield self.start_fit()
        self.assertFalse(self.fit_browser.toolbar_state_checker.is_tool_active())

    def test_pan_on_off(self):
        yield self.start(fit=False)
        self.figure_manager.toolbar.pan()
        yield self.wait_for_true(self.fit_browser.toolbar_state_checker.is_tool_active)
        yield self.start_fit()
        self.assertFalse(self.fit_browser.toolbar_state_checker.is_tool_active())

    def test_zoom_active_fit_inactive(self):
        yield self.start()
        self.figure_manager.toolbar.zoom()
        yield self.wait_for_true(self.fit_browser.toolbar_state_checker.is_tool_active)
        self.assertTrue(self.fit_browser.toolbar_state_checker.is_tool_active())
        start_x = self.fit_browser.startX()
        pos = self.w._canvas.geometry().center()
        canvas = self.w.childAt(pos)
        yield self.move_start_x(canvas, pos, 0.5)
        self.assertAlmostEqual(self.fit_browser.startX(), start_x, 2)

    def test_pan_active_fit_inactive(self):
        yield self.start()
        self.figure_manager.toolbar.pan()
        yield self.wait_for_true(self.fit_browser.toolbar_state_checker.is_tool_active)
        self.assertTrue(self.fit_browser.toolbar_state_checker.is_tool_active())
        start_x = self.fit_browser.startX()
        pos = self.w._canvas.geometry().center()
        canvas = self.w.childAt(pos)
        yield self.move_start_x(canvas, pos, 0.5)
        self.assertAlmostEqual(self.fit_browser.startX(), start_x, 2)

    def test_resize(self):
        yield self.start()
        self.start_draw_calls_count()
        width = (self.fit_browser.width() + self.w.width()) / 2
        self.w.resize(width, 400)
        yield
        self.assertEqual(self.draw_count, 1)
        width = self.fit_browser.width() - 10
        self.w.resize(width, 400)
        yield
        self.assertEqual(self.draw_count, 1)

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
        self.assertAlmostEqual(self.fit_browser.startX(), start_x + 0.5, 1)
        yield self.move_end_x(canvas, pos, -0.25)
        self.assertAlmostEqual(self.fit_browser.endX(), end_x - 0.25, 1)

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
        self.assertAlmostEqual(new_end_x, end_x - 0.5, 1)
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
        self.assertAlmostEqual(new_start_x, start_x + 0.5, 1)
        yield self.move_end_x(canvas, pos, -1.0)
        self.assertAlmostEqual(self.fit_browser.endX(), new_start_x)

    def test_fit_range_moved_start_outside(self):
        yield self.start()
        start_x_pxl = self.fit_browser.tool.fit_start_x.get_x_in_pixels()
        pos = self.w._canvas.geometry().center()
        canvas = self.w.childAt(pos)
        yield self.move_start_x(canvas, pos, -2.0, try_other_way_if_failed=False)
        self.assertTrue(abs(start_x_pxl - self.fit_browser.tool.fit_start_x.get_x_in_pixels()) < 5)

    def test_fit_range_moved_end_outside(self):
        yield self.start()
        end_x_pxl = self.fit_browser.tool.fit_end_x.get_x_in_pixels()
        pos = self.w._canvas.geometry().center()
        canvas = self.w.childAt(pos)
        yield self.move_end_x(canvas, pos, 2.0, try_other_way_if_failed=False)
        self.assertTrue(abs(end_x_pxl - self.fit_browser.tool.fit_end_x.get_x_in_pixels()) < 5)

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
        yield self.wait_for_true(lambda: len(self.fit_browser.fit_result_lines) == 2)
        # self.assertEqual(self.fit_browser.getFittingFunction(), "name=LinearBackground,A0=4.74354,A1=-0.442138")
        self.assertEqual(len(self.fit_browser.fit_result_lines), 2)
        del mtd['ws_Workspace']
        self.fit_browser.fit()
        yield self.wait_for_true(lambda: len(self.fit_browser.fit_result_lines) == 2)
        self.assertEqual(len(self.fit_browser.fit_result_lines), 2)

    def start_emu(self):
        res1 = Load(r'emu00006473.nxs', OutputWorkspace='ws1')
        Load(r'emu00006475.nxs', OutputWorkspace='ws2')
        plot([res1[0]], [3, 5, 7])
        manager = GlobalFigureManager.get_active()
        self.w = manager.window
        trigger_action(find_action_with_text(self.w, 'Fit'))
        yield 0.1
        self.fit_browser = manager.fit_browser

    def test_ws_index(self):
        yield self.start_emu()
        self.assertEqual(self.fit_browser.getWorkspaceNames(), ['ws1'])
        self.assertEqual(self.fit_browser.workspaceIndex(), 2)
        self.fit_browser.setWorkspaceIndex(3)
        yield self.wait_for_true(lambda: self.fit_browser.workspaceIndex() == 4)
        self.assertEqual(self.fit_browser.workspaceIndex(), 4)
        self.fit_browser.setWorkspaceIndex(3)
        yield self.wait_for_true(lambda: self.fit_browser.workspaceIndex() == 2)
        self.assertEqual(self.fit_browser.workspaceIndex(), 2)
        self.fit_browser.setWorkspaceIndex(10)
        yield self.wait_for_true(lambda: self.fit_browser.workspaceIndex() == 6)
        self.assertEqual(self.fit_browser.workspaceIndex(), 6)
        self.fit_browser.setWorkspaceIndex(0)
        yield self.wait_for_true(lambda: self.fit_browser.workspaceIndex() == 2)
        self.assertEqual(self.fit_browser.workspaceIndex(), 2)

    def test_hidden_fit_for_images(self):
        if 'ws1' in mtd:
            ws1 = mtd['ws1']
        else:
            ws1 = Load(r'emu00006473.nxs', OutputWorkspace='ws1')[0]
        pcolormesh([ws1])
        manager = GlobalFigureManager.get_active()
        action = manager.toolbar._actions['toggle_fit']
        self.assertFalse(action.isVisible())
        self.assertFalse(action.isEnabled())

    def test_fit_names_with_underscores(self):
        """
        Test that fit browser doesn't crash if the workspace name contains underscores
        """
        yield self.start(ws_name='ws_1')
        self.fit_browser.loadFunction('name=LinearBackground')
        self.fit_browser.fit()
        yield self.wait_for_true(lambda: len(self.fit_browser.fit_result_lines) == 2)
        self.assertEqual(len(self.fit_browser.fit_result_lines), 2)

    def test_fit_names_ending_with_Raw(self):
        """
        Test that fit browser doesn't crash if the workspace name ends on _Raw
        """
        yield self.start(ws_name='ws_Raw')
        self.fit_browser.loadFunction('name=LinearBackground')
        self.fit_browser.fit()
        yield self.wait_for_true(lambda: len(self.fit_browser.fit_result_lines) == 2)
        self.assertEqual(len(self.fit_browser.fit_result_lines), 2)

    def test_fit_output_name_non_default(self):
        yield self.start(ws_name='ws')
        self.fit_browser.setOutputName('out')
        self.fit_browser.loadFunction('name=LinearBackground')
        self.fit_browser.fit()
        yield self.wait_for_true(lambda: len(self.fit_browser.fit_result_lines) == 2)
        self.assertEqual(len(self.fit_browser.fit_result_lines), 2)

    def test_add_peak(self):
        yield self.start()
        self.fit_browser.tool.add_peak(1.0, 4.3, 4.1)
        self.assertEqual(self.fit_browser.sizeOfFunctionsGroup(), 3)
        self.assertEqual(self.fit_browser.getFittingFunction(), 'name=Gaussian,Height=0.2,PeakCentre=1,Sigma=0')

    def test_move_peak(self):
        yield self.start()
        self.fit_browser.tool.add_peak(1.0, 4.3, 4.1)
        yield self.drag_mouse(1.0, 4.2, 1.5, 4.2)
        self.assertAlmostEqual(self.fit_browser.getPeakCentreOf('f0'), 1.5, 1)
        self.assertAlmostEqual(self.fit_browser.getPeakHeightOf('f0'), 0.2, 1)
        yield self.drag_mouse(1.5, 4.4, 1.2, 4.4)
        self.assertAlmostEqual(self.fit_browser.getPeakCentreOf('f0'), 1.5, 1)
        self.assertAlmostEqual(self.fit_browser.getPeakHeightOf('f0'), 0.2, 1)
        yield self.drag_mouse(1.5, 4.05, 1.2, 4.05)
        self.assertAlmostEqual(self.fit_browser.getPeakCentreOf('f0'), 1.5, 1)
        self.assertAlmostEqual(self.fit_browser.getPeakHeightOf('f0'), 0.2, 1)

    def test_add_two_peaks(self):
        yield self.start()
        self.fit_browser.tool.add_peak(1.0, 4.3, 4.1)
        self.fit_browser.tool.add_peak(1.5, 4.4)
        self.assertEqual(self.fit_browser.sizeOfFunctionsGroup(), 4)
        self.assertEqual(self.fit_browser.getFittingFunction(),
                         'name=Gaussian,Height=0.2,PeakCentre=1,Sigma=0;'
                         'name=Gaussian,Height=4.4,PeakCentre=1.5,Sigma=0')
        yield self.drag_mouse(1.0, 4.295, 1.75, 4.45)
        yield self.drag_mouse(1.5, 4.395, 0.9, 4.12)
        self.assertAlmostEqual(self.fit_browser.getPeakCentreOf('f0'), 1.75, 1)
        self.assertAlmostEqual(self.fit_browser.getPeakHeightOf('f0'), 0.35, 1)
        self.assertAlmostEqual(self.fit_browser.getPeakCentreOf('f1'), 0.9, 1)
        self.assertAlmostEqual(self.fit_browser.getPeakHeightOf('f1'), 4.12, 1)

    def test_update_peaks(self):
        yield self.start()
        self.fit_browser.tool.add_peak(1.0, 4.3, 4.1)
        self.fit_browser.tool.add_peak(1.5, 4.4)
        self.assertTrue(self.fit_browser.tool.get_override_cursor(1.23, 4.2) is None)
        self.assertFalse(self.fit_browser.tool.get_override_cursor(1.5, 4.3) is None)
        self.start_draw_calls_count()
        self.fit_browser.setPeakCentreOf('f0', 1.23)
        self.fit_browser.setPeakHeightOf('f1', 4.22)
        self.assertEqual(self.draw_count, 4)
        self.assertFalse(self.fit_browser.tool.get_override_cursor(1.23, 4.2) is None)
        self.assertTrue(self.fit_browser.tool.get_override_cursor(1.5, 4.3) is None)

    def test_context_menu_no_crash(self):
        yield self.start(fit=False)
        yield self.context_menu()

    def test_add_peak_context_menu(self):
        yield self.start()
        yield self.context_menu()
        menu = self.get_active_popup_widget()
        action = find_action_with_text(menu, 'Add peak')
        trigger_action(action)
        yield
        menu.close()
        yield self.mouse_click(1.0, 4.2)
        self.assertEqual(self.fit_browser.sizeOfFunctionsGroup(), 3)
        self.assertAlmostEqual(self.fit_browser.getPeakCentreOf('f0'), 1.0, 1)
        self.assertAlmostEqual(self.fit_browser.getPeakHeightOf('f0'), 4.2, 1)


runTests(TestFitPropertyBrowser)
