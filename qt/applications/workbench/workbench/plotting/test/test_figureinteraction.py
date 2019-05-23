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
from qtpy.QtCore import Qt

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
        cls.ws = CreateWorkspace(DataX=[10, 20, 30],
                                 DataY=[2, 3],
                                 Distribution=False,
                                 UnitX='Wavelength',
                                 YUnitLabel='Counts',
                                 OutputWorkspace='ws')

    @classmethod
    def tearDownClass(cls):
        cls.ws.delete()

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
        self._toggle_normalization(errobars_on=False)

    def test_toggle_normalization_with_errorbars(self):
        self._toggle_normalization(errobars_on=True)

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

    def _toggle_normalization(self, errobars_on):
        fig = plot([self.ws], spectrum_nums=[1], errors=errobars_on,
                   plot_kwargs={'plot_as_distribution': False})
        mock_canvas = MagicMock(figure=fig)
        fig_manager_mock = MagicMock(canvas=mock_canvas)
        fig_interactor = FigureInteraction(fig_manager_mock)

        ax = fig.axes[0]
        fig_interactor._toggle_normalization(ax)
        self.assertSequenceEqual(list(ax.lines[0]._x), [15, 25])
        self.assertSequenceEqual(list(ax.lines[0]._y), [0.2, 0.3])
        self.assertEqual("Counts ($\\AA$)$^{-1}$", ax.get_ylabel())
        fig_interactor._toggle_normalization(ax)
        self.assertSequenceEqual(list(ax.lines[0]._x), [15, 25])
        self.assertSequenceEqual(list(ax.lines[0]._y), [2, 3])
        self.assertEqual("Counts", ax.get_ylabel())


if __name__ == '__main__':
    unittest.main()
