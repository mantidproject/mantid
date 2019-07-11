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

import matplotlib

matplotlib.use("AGG")

import matplotlib.pyplot as plt  # noqa: E402
from qtpy.QtWidgets import QMenu  # noqa: E402
from mantid.plots import MantidAxes  # noqa: E402
from mantid.simpleapi import CreateWorkspace  # noqa: E402
from mantidqt.utils.qt.testing import GuiTest  # noqa: E402
from workbench.plotting.figureerrorsmanager import FigureErrorsManager  # noqa: E402


def plot_things(make_them_errors):
    def function_reference(func):
        def function_parameters(self):
            if not make_them_errors:
                # plot this line with specNum
                self.ax.plot(self.ws2d_histo, specNum=1)
                # and another one with wkspIndex
                self.ax.plot(self.ws2d_histo, wkspIndex=2)
            else:
                self.ax.errorbar(self.ws2d_histo, specNum=1)
                self.ax.errorbar(self.ws2d_histo, wkspIndex=2)

            anonymous_menu = QMenu()
            # this initialises some of the class internals
            self.errors_manager.add_error_bars_menu(anonymous_menu)
            return func(self)

        return function_parameters

    return function_reference


class FigureErrorsManagerTest(GuiTest):
    """
    Test class that covers the interaction of the FigureErrorsManager with plots
    that use the mantid projection and have MantidAxes
    """

    @classmethod
    def setUpClass(cls):
        cls.ws2d_histo = CreateWorkspace(DataX=[10, 20, 30, 10, 20, 30, 10, 20, 30],
                                         DataY=[2, 3, 4, 5, 3, 5],
                                         DataE=[1, 2, 3, 4, 1, 1],
                                         NSpec=3,
                                         Distribution=True,
                                         UnitX='Wavelength',
                                         VerticalAxisUnit='DeltaE',
                                         VerticalAxisValues=[4, 6, 8],
                                         OutputWorkspace='ws2d_histo')
        # initialises the QApplication
        super(cls, FigureErrorsManagerTest).setUpClass()

    def setUp(self):
        self.fig, self.ax = plt.subplots(subplot_kw={'projection': 'mantid'})

        self.errors_manager = FigureErrorsManager(self.fig.canvas)

    def tearDown(self):
        plt.close('all')
        del self.fig
        del self.ax
        del self.errors_manager

    def test_add_error_bars_menu(self):
        main_menu = QMenu()
        self.errors_manager.add_error_bars_menu(main_menu)

        # Check the expected sub-menu with buttons is added
        added_menu = main_menu.children()[1]
        self.assertTrue(
            any(FigureErrorsManager.SHOW_ERROR_BARS_BUTTON_TEXT == child.text() for child in added_menu.children()))
        self.assertTrue(
            any(FigureErrorsManager.HIDE_ERROR_BARS_BUTTON_TEXT == child.text() for child in added_menu.children()))

    @plot_things(make_them_errors=False)
    def test_show_all_errors(self):
        # assert plot does not have errors
        self.assertEqual(0, len(self.ax.containers))

        self.errors_manager._toggle_all_errors(self.ax, make_visible=True)

        # check that the errors have been added
        self.assertEqual(2, len(self.ax.containers))

    @plot_things(make_them_errors=True)
    def test_hide_all_errors(self):
        self.assertEqual(2, len(self.ax.containers))
        self.errors_manager._toggle_all_errors(self.ax, make_visible=False)
        # errors still exist
        self.assertEqual(2, len(self.ax.containers))
        # they are just invisible
        self.assertFalse(self.ax.containers[0][2][0].get_visible())


class ScriptedPlotFigureErrorsManagerTest(GuiTest):
    """
    Test class that covers the interaction of the FigureErrorsManager with plots that
    do not use the MantidAxes, which happens if they are scripted.
    """

    @classmethod
    def setUpClass(cls):
        cls.ws2d_histo = CreateWorkspace(DataX=[10, 20, 30, 10, 20, 30, 10, 20, 30],
                                         DataY=[2, 3, 4, 5, 3, 5],
                                         DataE=[1, 2, 3, 4, 1, 1],
                                         NSpec=3,
                                         Distribution=True,
                                         UnitX='Wavelength',
                                         VerticalAxisUnit='DeltaE',
                                         VerticalAxisValues=[4, 6, 8],
                                         OutputWorkspace='ws2d_histo')
        # initialises the QApplication
        super(cls, ScriptedPlotFigureErrorsManagerTest).setUpClass()

    def setUp(self):
        self.fig, self.ax = plt.subplots()  # type: matplotlib.figure.Figure, MantidAxes

        self.errors_manager = FigureErrorsManager(self.fig.canvas)  # type: FigureErrorsManager

    def tearDown(self):
        plt.close('all')
        del self.fig
        del self.ax
        del self.errors_manager

    def test_context_menu_not_added_for_scripted_plot_without_errors(self):
        self.ax.plot([0, 15000], [0, 15000], label='MyLabel')
        self.ax.plot([0, 15000], [0, 14000], label='MyLabel 2')

        main_menu = QMenu()
        # QMenu always seems to have 1 child when empty,
        # but just making sure the count as expected at this point in the test
        self.assertEqual(1, len(main_menu.children()))

        # plot above doesn't have errors, nor is a MantidAxes
        # so no context menu will be added
        self.errors_manager.add_error_bars_menu(main_menu)

        # number of children should remain unchanged
        self.assertEqual(1, len(main_menu.children()))

    def test_scripted_plot_line_without_label_handled_properly(self):
        # having the special nolabel is usually present on lines with errors,
        # but sometimes can be present on lines without errors, this test covers that case
        self.ax.plot([0, 15000], [0, 15000], label=MantidAxes.MPL_NOLEGEND)
        self.ax.plot([0, 15000], [0, 15000], label=MantidAxes.MPL_NOLEGEND)

        main_menu = QMenu()
        # QMenu always seems to have 1 child when empty,
        # but just making sure the count as expected at this point in the test
        self.assertEqual(1, len(main_menu.children()))

        # plot above doesn't have errors, nor is a MantidAxes
        # so no context menu will be added for error bars
        self.errors_manager.add_error_bars_menu(main_menu)

        # number of children should remain unchanged
        self.assertEqual(1, len(main_menu.children()))

    def test_context_menu_added_for_scripted_plot_with_errors(self):
        self.ax.plot([0, 15000], [0, 15000], label='MyLabel')
        self.ax.errorbar([0, 15000], [0, 14000], yerr=[10, 10000], label='MyLabel 2')

        main_menu = QMenu()
        # QMenu always seems to have 1 child when empty,
        # but just making sure the count as expected at this point in the test
        self.assertEqual(1, len(main_menu.children()))

        # plot above doesn't have errors, nor is a MantidAxes
        # so no context menu will be added
        self.errors_manager.add_error_bars_menu(main_menu)

        added_menu = main_menu.children()[1]

        # actions should have been added now, which for this case are only `Show all` and `Hide all`
        self.assertTrue(
            any(FigureErrorsManager.SHOW_ERROR_BARS_BUTTON_TEXT == child.text() for child in added_menu.children()))
        self.assertTrue(
            any(FigureErrorsManager.HIDE_ERROR_BARS_BUTTON_TEXT == child.text() for child in added_menu.children()))
        self.assertEqual(2, len(added_menu.children()))

    def test_scripted_plot_show_and_hide_all(self):
        self.ax.plot([0, 15000], [0, 15000], label='MyLabel')
        self.ax.errorbar([0, 15000], [0, 14000], yerr=[10, 10000], label='MyLabel 2')

        anonymous_menu = QMenu()
        # this initialises some of the class internals
        self.errors_manager.add_error_bars_menu(anonymous_menu)

        self.assertTrue(self.ax.containers[0][2][0].get_visible())
        self.errors_manager._toggle_all_errors(self.ax, make_visible=False)
        self.assertFalse(self.ax.containers[0][2][0].get_visible())

        # make the menu again, this updates the internal state of the errors manager
        # and is what actually happens when the user opens the menu again
        self.errors_manager.add_error_bars_menu(anonymous_menu)
        self.errors_manager._toggle_all_errors(self.ax, make_visible=True)
        self.assertTrue(self.ax.containers[0][2][0].get_visible())
