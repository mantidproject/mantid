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
from mantidqt.widgets.plotconfigdialog.legendtabwidget import LegendProperties


class FigureErrorsManager(object):
    ERROR_BARS_MENU_TEXT = "Error Bars"
    SHOW_ERROR_BARS_BUTTON_TEXT = "Show all errors"
    HIDE_ERROR_BARS_BUTTON_TEXT = "Hide all errors"

    AXES_NOT_MANTIDAXES_ERR_MESSAGE = "Plot axes are not MantidAxes. There is no way to automatically load error data."

    def __init__(self, canvas):
        self.canvas = canvas
        self.active_lines = []

    def toggle_all_errors(self, ax, make_visible):
        for line in self.active_lines:
            if curve_has_errors(line):
                self.toggle_error_bars_for(ax, line, make_visible)

    @staticmethod
    def toggle_error_bars_for(ax, curve, make_visible=None):
        # get legend properties
        if ax.legend_:
            legend_props = LegendProperties.from_legend(ax.legend_)
        else:
            legend_props = None

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
        CurvesTabWidgetPresenter.update_limits_and_legend(ax, legend_props)

    def update_plot_after(self, func, *args, **kwargs):
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

    @staticmethod
    def get_errorbars_from_ax(ax):
        return [cont for cont in ax.containers if isinstance(cont, ErrorbarContainer)]

    @staticmethod
    def get_curves_from_ax(ax):
        return ax.get_lines() + CurvesTabWidgetPresenter.get_errorbars_from_ax(ax)
