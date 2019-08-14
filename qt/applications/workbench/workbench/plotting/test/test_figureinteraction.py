# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.
#
from __future__ import (absolute_import, division, print_function,
                        unicode_literals)

# system imports
import unittest

# third-party library imports
import matplotlib
matplotlib.use('AGG')  # noqa
import numpy as np
from qtpy.QtCore import Qt
from testhelpers import assert_almost_equal

# local package imports
from mantid.plots import MantidAxes
from mantid.py3compat.mock import MagicMock, PropertyMock, call, patch
from mantid.simpleapi import CreateWorkspace
from mantidqt.plotting.figuretype import FigureType
from mantidqt.plotting.functions import plot
from workbench.plotting.figureinteraction import FigureInteraction


class FigureInteractionTest(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        cls.ws = CreateWorkspace(
            DataX=np.array([10, 20, 30], dtype=np.float64),
            DataY=np.array([2, 3], dtype=np.float64),
            DataE=np.array([0.02, 0.02], dtype=np.float64),
            Distribution=False,
            UnitX='Wavelength',
            YUnitLabel='Counts',
            OutputWorkspace='ws')
        cls.ws1 = CreateWorkspace(
            DataX=np.array([11, 21, 31], dtype=np.float64),
            DataY=np.array([3, 4], dtype=np.float64),
            DataE=np.array([0.03, 0.03], dtype=np.float64),
            Distribution=False,
            UnitX='Wavelength',
            YUnitLabel='Counts',
            OutputWorkspace='ws1')

    @classmethod
    def tearDownClass(cls):
        cls.ws.delete()
        cls.ws1.delete()

    # Success tests
    def test_construction_registers_handler_for_button_press_event(self):
        fig_manager = MagicMock()
        fig_manager.canvas = MagicMock()
        interactor = FigureInteraction(fig_manager)
        fig_manager.canvas.mpl_connect.assert_called_once_with('button_press_event',
                                                               interactor.on_mouse_button_press)

    def test_disconnect_called_for_each_registered_handler(self):
        fig_manager = MagicMock()
        canvas = MagicMock()
        fig_manager.canvas = canvas
        interactor = FigureInteraction(fig_manager)
        interactor.disconnect()
        self.assertEqual(interactor.nevents, canvas.mpl_disconnect.call_count)

    @patch('workbench.plotting.figureinteraction.QMenu',
           autospec=True)
    @patch('workbench.plotting.figureinteraction.figure_type',
           autospec=True)
    def test_right_click_gives_no_context_menu_for_empty_figure(self, mocked_figure_type,
                                                                mocked_qmenu):
        fig_manager = self._create_mock_fig_manager_to_accept_right_click()
        interactor = FigureInteraction(fig_manager)
        mouse_event = self._create_mock_right_click()
        mocked_figure_type.return_value = FigureType.Empty

        with patch.object(interactor.toolbar_manager, 'is_tool_active',
                          lambda: False):
            interactor.on_mouse_button_press(mouse_event)
            self.assertEqual(0, mocked_qmenu.call_count)

    @patch('workbench.plotting.figureinteraction.QMenu',
           autospec=True)
    @patch('workbench.plotting.figureinteraction.figure_type',
           autospec=True)
    def test_right_click_gives_no_context_menu_for_color_plot(self, mocked_figure_type,
                                                              mocked_qmenu):
        fig_manager = self._create_mock_fig_manager_to_accept_right_click()
        interactor = FigureInteraction(fig_manager)
        mouse_event = self._create_mock_right_click()
        mocked_figure_type.return_value = FigureType.Image

        with patch.object(interactor.toolbar_manager, 'is_tool_active',
                          lambda: False):
            interactor.on_mouse_button_press(mouse_event)
            self.assertEqual(0, mocked_qmenu.call_count)

    @patch('workbench.plotting.figureinteraction.QMenu',
           autospec=True)
    @patch('workbench.plotting.figureinteraction.figure_type',
           autospec=True)
    def test_right_click_gives_context_menu_for_plot_without_fit_enabled(self, mocked_figure_type,
                                                                         mocked_qmenu_cls):
        fig_manager = self._create_mock_fig_manager_to_accept_right_click()
        fig_manager.fit_browser.tool = None
        interactor = FigureInteraction(fig_manager)
        mouse_event = self._create_mock_right_click()
        mocked_figure_type.return_value = FigureType.Line

        # Expect a call to QMenu() for the outer menu followed by two more calls
        # for the Axes and Normalization menus
        qmenu_call1 = MagicMock()
        qmenu_call2 = MagicMock()
        qmenu_call3 = MagicMock()
        mocked_qmenu_cls.side_effect = [qmenu_call1, qmenu_call2, qmenu_call3]

        with patch('workbench.plotting.figureinteraction.QActionGroup',
                   autospec=True):
            with patch.object(interactor.toolbar_manager, 'is_tool_active',
                              lambda: False):
                with patch.object(interactor.errors_manager, 'add_error_bars_menu', MagicMock()):
                    interactor.on_mouse_button_press(mouse_event)
                    self.assertEqual(0, qmenu_call1.addSeparator.call_count)
                    self.assertEqual(0, qmenu_call1.addAction.call_count)
                    expected_qmenu_calls = [call(),
                                            call("Axes", qmenu_call1),
                                            call("Normalization", qmenu_call1)]
                    self.assertEqual(expected_qmenu_calls, mocked_qmenu_cls.call_args_list)
                    # 4 actions in Axes submenu
                    self.assertEqual(4, qmenu_call2.addAction.call_count)
                    # 2 actions in Normalization submenu
                    self.assertEqual(2, qmenu_call3.addAction.call_count)

    def test_toggle_normalization_no_errorbars(self):
        self._test_toggle_normalization(errorbars_on=False, plot_kwargs={'distribution': True})

    def test_toggle_normalization_with_errorbars(self):
        self._test_toggle_normalization(errorbars_on=True, plot_kwargs={'distribution': True})

    def test_correct_yunit_label_when_overplotting_after_normaliztion_toggle(self):
        fig = plot([self.ws], spectrum_nums=[1], errors=True,
                   plot_kwargs={'distribution': True})
        mock_canvas = MagicMock(figure=fig)
        fig_manager_mock = MagicMock(canvas=mock_canvas)
        fig_interactor = FigureInteraction(fig_manager_mock)

        ax = fig.axes[0]
        fig_interactor._toggle_normalization(ax)
        self.assertEqual("Counts ($\AA$)$^{-1}$", ax.get_ylabel())
        plot([self.ws1], spectrum_nums=[1], errors=True, overplot=True, fig=fig)
        self.assertEqual("Counts ($\AA$)$^{-1}$", ax.get_ylabel())

    def test_normalization_toggle_with_no_autoscale_on_update_no_errors(self):
        self._test_toggle_normalization(errorbars_on=False, plot_kwargs={'distribution': True, 'autoscale_on_update': False})

    def test_normalization_toggle_with_no_autoscale_on_update_with_errors(self):
        self._test_toggle_normalization(errorbars_on=True, plot_kwargs={'distribution': True, 'autoscale_on_update': False})

    # Failure tests
    def test_construction_with_non_qt_canvas_raises_exception(self):
        class NotQtCanvas(object):
            pass

        class FigureManager(object):
            def __init__(self):
                self.canvas = NotQtCanvas()

        self.assertRaises(RuntimeError, FigureInteraction, FigureManager())

    # Private methods
    def _create_mock_fig_manager_to_accept_right_click(self):
        fig_manager = MagicMock()
        canvas = MagicMock()
        type(canvas).buttond = PropertyMock(return_value={Qt.RightButton: 3})
        fig_manager.canvas = canvas
        return fig_manager

    def _create_mock_right_click(self):
        mouse_event = MagicMock(inaxes=MagicMock(spec=MantidAxes))
        type(mouse_event).button = PropertyMock(return_value=3)
        return mouse_event

    def _test_toggle_normalization(self, errorbars_on, plot_kwargs):
        fig = plot([self.ws], spectrum_nums=[1], errors=errorbars_on,
                   plot_kwargs=plot_kwargs)
        mock_canvas = MagicMock(figure=fig)
        fig_manager_mock = MagicMock(canvas=mock_canvas)
        fig_interactor = FigureInteraction(fig_manager_mock)

        # Earlier versions of matplotlib do not store the data assciated with a
        # line with high precision and hence we need to set a lower tolerance
        # when making comparisons of this data
        if matplotlib.__version__ < "2":
            decimal_tol = 1
        else:
            decimal_tol = 7

        ax = fig.axes[0]
        fig_interactor._toggle_normalization(ax)
        assert_almost_equal(ax.lines[0].get_xdata(), [15, 25])
        assert_almost_equal(ax.lines[0].get_ydata(), [0.2, 0.3], decimal=decimal_tol)
        self.assertEqual("Counts ($\\AA$)$^{-1}$", ax.get_ylabel())
        fig_interactor._toggle_normalization(ax)
        assert_almost_equal(ax.lines[0].get_xdata(), [15, 25])
        assert_almost_equal(ax.lines[0].get_ydata(), [2, 3], decimal=decimal_tol)
        self.assertEqual("Counts", ax.get_ylabel())


if __name__ == '__main__':
    unittest.main()
