# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.
"""
Controls the dynamic displaying of errors for line on the plot
"""

from matplotlib.container import ErrorbarContainer
from matplotlib.lines import Line2D

from mantid.plots import datafunctions, MantidAxes
from mantid.plots.datafunctions import get_data_from_errorbar_container, set_errorbars_hidden
from mantid.plots.legend import LegendProperties
from mantidqt.widgets.plotconfigdialog.curvestabwidget import curve_has_errors, CurveProperties, remove_curve_from_ax


class FigureErrorsManager(object):
    AXES_NOT_MANTIDAXES_ERR_MESSAGE = "Plot axes are not MantidAxes. There is no way to automatically load error data."

    def __init__(self, canvas):
        self.canvas = canvas

    @classmethod
    def toggle_all_errors(cls, ax, make_visible):
        active_lines = cls.get_curves_from_ax(ax)
        for line in active_lines:
            if curve_has_errors(line):
                cls.toggle_error_bars_for(ax, line, make_visible)

    @classmethod
    def toggle_error_bars_for(cls, ax, curve, make_visible=None):
        # get legend properties
        if ax.legend_:
            legend_props = LegendProperties.from_legend(ax.legend_)
        else:
            legend_props = None

        if isinstance(curve, Line2D):
            curve_index = ax.get_lines().index(curve)
        else:
            curve_index = ax.get_lines().index(curve[0])

        # get all curve properties
        curve_props = CurveProperties.from_curve(curve)
        # and remove the ones that matplotlib doesn't recognise
        plot_kwargs = curve_props.get_plot_kwargs()
        new_curve = cls.replot_curve(ax, curve, plot_kwargs)

        if isinstance(ax, MantidAxes):
            errorbar_cap_lines = datafunctions.remove_and_return_errorbar_cap_lines(ax)
        else:
            errorbar_cap_lines = []

        # Since mpl 3.7 made ax.lines immutable, we have to do this workaround.
        lines_to_remove = ax.get_lines()[curve_index:]
        for line in lines_to_remove:
            line.remove()
        ax.add_line(lines_to_remove.pop())
        for line in lines_to_remove:
            ax.add_line(line)

        if isinstance(ax, MantidAxes) and ax.is_waterfall():
            datafunctions.convert_single_line_to_waterfall(ax, curve_index)

        for cap in errorbar_cap_lines:
            ax.add_line(cap)

        # Inverts either the current state of hide_errors
        # or the make_visible kwarg that forces a state:
        # If make visible is True, then hide_errors must be False
        # for the intended effect
        curve_props.hide_errors = not curve_props.hide_errors if make_visible is None else not make_visible

        cls.toggle_errors(new_curve, curve_props)
        cls.update_limits_and_legend(ax, legend_props)

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
        return hasattr(ax, "creation_args")

    @staticmethod
    def get_errorbars_from_ax(ax):
        return [cont for cont in ax.containers if isinstance(cont, ErrorbarContainer)]

    @classmethod
    def get_curves_from_ax(cls, ax):
        return ax.get_lines() + cls.get_errorbars_from_ax(ax)

    @staticmethod
    def update_limits_and_legend(ax, legend_props=None):
        ax.relim()
        if legend_props:
            LegendProperties.create_legend(legend_props, ax)

    @staticmethod
    def toggle_errors(curve, view_props):
        if curve_has_errors(curve):
            hide_errors = view_props.hide_errors or view_props.hide
            setattr(curve, "hide_errors", hide_errors)
            set_errorbars_hidden(curve, hide_errors)

    @classmethod
    def replot_curve(cls, ax, curve, plot_kwargs):
        if isinstance(ax, MantidAxes):
            if ax.creation_args:
                axis = ax.creation_args[0].get("axis", None)
                if axis:
                    plot_kwargs["axis"] = axis
            try:
                new_curve = ax.replot_artist(curve, errorbars=curve_has_errors(curve), **plot_kwargs)
            except ValueError:  # ValueError raised if Artist not tracked by Axes
                new_curve = cls._replot_mpl_curve(ax, curve, plot_kwargs)
        else:
            new_curve = cls._replot_mpl_curve(ax, curve, plot_kwargs)
        if hasattr(new_curve, "errorevery"):
            setattr(new_curve, "errorevery", plot_kwargs.get("errorevery", 1))
        return new_curve

    @staticmethod
    def _replot_mpl_curve(ax, curve, plot_kwargs):
        """
        Replot the given matplotlib curve with new kwargs
        :param ax: The axis that the curve will be plotted on
        :param curve: The curve that will be replotted
        :param plot_kwargs: Kwargs for the plot that will be passed onto matplotlib
        """
        remove_curve_from_ax(curve)
        if isinstance(curve, Line2D):
            [plot_kwargs.pop(arg, None) for arg in ["capsize", "capthick", "ecolor", "elinewidth", "errorevery"]]
            new_curve = ax.plot(curve.get_xdata(), curve.get_ydata(), **plot_kwargs)[0]
        elif isinstance(curve, ErrorbarContainer):
            # Because of "error every" option, we need to store the original
            # error bar data on the curve or we will lose data on re-plotting
            x, y, xerr, yerr = getattr(curve, "errorbar_data", get_data_from_errorbar_container(curve))
            new_curve = ax.errorbar(x, y, xerr=xerr, yerr=yerr, **plot_kwargs)
            setattr(new_curve, "errorbar_data", [x, y, xerr, yerr])
        else:
            raise ValueError("Curve must have type 'Line2D' or 'ErrorbarContainer'. Found '{}'".format(type(curve)))
        return new_curve
