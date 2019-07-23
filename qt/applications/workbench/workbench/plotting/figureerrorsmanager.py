# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.
"""
Controls the dynamic displaying of errors for line on the plot
"""
from functools import partial
from matplotlib.container import ErrorbarContainer
from qtpy.QtWidgets import QMenu

from mantid.plots import MantidAxes
from mantidqt.widgets.plotconfigdialog.curvestabwidget import curve_has_errors, CurveProperties
from mantidqt.widgets.plotconfigdialog.curvestabwidget.presenter import CurvesTabWidgetPresenter


class FigureErrorsManager(object):
    ERROR_BARS_MENU_TEXT = "Error Bars"
    SHOW_ERROR_BARS_BUTTON_TEXT = "Show all errors"
    HIDE_ERROR_BARS_BUTTON_TEXT = "Hide all errors"

    AXES_NOT_MANTIDAXES_ERR_MESSAGE = "Plot axes are not MantidAxes. There is no way to automatically load error data."

    def __init__(self, canvas):
        self.canvas = canvas
        self.active_lines = []

    def add_error_bars_menu(self, parent_menu, ax):
        """
        Add menu actions to toggle the errors for all lines in the plot.

        Lines without errors are added in the context menu first,
        then lines containing errors are appended afterwards.

        This is done so that the context menu always has
        the same order of curves as the legend is currently showing - and the
        legend always appends curves with errors after the lines without errors.
        Relevant source, as of 10 July 2019:
        https://github.com/matplotlib/matplotlib/blob/154922992722db37a9d9c8680682ccc4acf37f8c/lib/matplotlib/legend.py#L1201

        :param parent_menu: The menu to which the actions will be added
        :type parent_menu: QMenu
        :param ax: The Axes containing lines to toggle errors on
        """
        # if the ax is not a MantidAxes, and there are no errors plotted,
        # then do not add any options for the menu
        if not isinstance(ax, MantidAxes) and len(ax.containers) == 0:
            return

        error_bars_menu = QMenu(self.ERROR_BARS_MENU_TEXT, parent_menu)
        error_bars_menu.addAction(self.SHOW_ERROR_BARS_BUTTON_TEXT,
                                  partial(self._update_plot_after, self._toggle_all_errors, ax, make_visible=True))
        error_bars_menu.addAction(self.HIDE_ERROR_BARS_BUTTON_TEXT,
                                  partial(self._update_plot_after, self._toggle_all_errors, ax, make_visible=False))
        parent_menu.addMenu(error_bars_menu)

        self.active_lines = CurvesTabWidgetPresenter.get_curves_from_ax(ax)

        # if there's more than one line plotted, then
        # add a sub menu, containing an action to hide the
        # error bar for each line
        error_bars_menu.addSeparator()
        add_later = []
        for index, line in enumerate(self.active_lines):
            if curve_has_errors(line):
                curve_props = CurveProperties.from_curve(line)
                # Add lines without errors first, lines with errors are appended later. Read docstring for more info
                if not isinstance(line, ErrorbarContainer):
                    action = error_bars_menu.addAction(line.get_label(), partial(
                        self._update_plot_after, self.toggle_error_bars_for, ax, line))
                    action.setCheckable(True)
                    action.setChecked(not curve_props.hide_errors)
                else:
                    add_later.append((line.get_label(), partial(
                        self._update_plot_after, self.toggle_error_bars_for, ax, line),
                                      not curve_props.hide_errors))

        for label, function, visible in add_later:
            action = error_bars_menu.addAction(label, function)
            action.setCheckable(True)
            action.setChecked(visible)

    def _toggle_all_errors(self, ax, make_visible):
        for line in self.active_lines:
            if curve_has_errors(line):
                self.toggle_error_bars_for(ax, line, make_visible)

    @staticmethod
    def toggle_error_bars_for(ax, curve, make_visible=None):
        # get all curve properties
        curve_props = CurveProperties.from_curve(curve)
        # and remove the ones that matplotlib doesn't recognise
        plot_kwargs = curve_props.get_plot_kwargs()
        new_curve = CurvesTabWidgetPresenter.replot_curve(ax, curve, plot_kwargs)

        # Inverts either the current state of hide_errors
        # or the make_visible kwarg that forces a state:
        # If make visible is True, then hide_errors must be False
        # for the intended effect
        curve_props.hide_errors = not curve_props.hide_errors if make_visible is None else not make_visible

        CurvesTabWidgetPresenter.toggle_errors(new_curve, curve_props)
        CurvesTabWidgetPresenter.update_limits_and_legend(ax)

    def _update_plot_after(self, func, *args, **kwargs):
        """
        Updates the legend and the plot after the function has been executed.
        Used to funnel through the updates through a common place

        :param func: Function to be executed, before updating the plot
        :param args: Arguments forwarded to the function
        :param kwargs: Keyword arguments forwarded to the function
        """
        func(*args, **kwargs)
        self.canvas.draw()

    @staticmethod
    def _supported_ax(ax):
        return hasattr(ax, 'creation_args')
