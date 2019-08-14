# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.

from __future__ import (absolute_import, unicode_literals)

from matplotlib.axes import ErrorbarContainer
from matplotlib.lines import Line2D

from mantid.plots import MantidAxes
from mantid.plots.helperfunctions import get_data_from_errorbar_container
from mantidqt.utils.qt import block_signals
from mantidqt.widgets.plotconfigdialog import get_axes_names_dict, curve_in_ax
from mantidqt.widgets.plotconfigdialog.curvestabwidget import (
    CurveProperties, set_errorbars_hidden, curve_has_errors,
    remove_curve_from_ax)
from mantidqt.widgets.plotconfigdialog.curvestabwidget.view import CurvesTabWidgetView


class CurvesTabWidgetPresenter:

    def __init__(self, fig, view=None, parent=None):
        self.fig = fig
        if not view:
            self.view = CurvesTabWidgetView(parent)
        else:
            self.view = view

        self.current_view_properties = None

        # Fill the fields in the view
        self.axes_names_dict = get_axes_names_dict(self.fig, curves_only=True)
        self.populate_select_axes_combo_box()
        self.curve_names_dict = {}
        self.populate_curve_combo_box_and_update_view()

        # Signals
        self.view.select_axes_combo_box.currentIndexChanged.connect(
            self.populate_curve_combo_box_and_update_view)
        self.view.select_curve_combo_box.currentIndexChanged.connect(
            self.update_view)
        self.view.remove_curve_button.clicked.connect(
            self.remove_selected_curve)

    def apply_properties(self):
        """Take properties from views and set them on the selected curve"""
        view_props = self.get_view_properties()
        if view_props == self.current_view_properties:
            return
        # Re-plot curve
        self._replot_selected_curve(view_props.get_plot_kwargs())
        curve = self.get_selected_curve()
        # Set the curve's new name in the names dict and combo box
        self.set_new_curve_name_in_dict_and_combo_box(curve, view_props.label)
        self.toggle_errors(curve, view_props)
        self.current_view_properties = view_props

        self.update_limits_and_legend(self.get_selected_ax())

    @staticmethod
    def update_limits_and_legend(ax):
        ax.relim()
        ax.autoscale()
        if ax.legend_:
            ax.legend().draggable()

    @staticmethod
    def toggle_errors(curve, view_props):
        setattr(curve, 'hide_errors', view_props.hide_errors)
        set_errorbars_hidden(curve, view_props.hide_errors)

    def close_tab(self):
        """Close the tab and set the view to None"""
        self.view.close()
        self.view = None

    def get_selected_ax(self):
        """
        Get selected axes object from name in combo box.
        If not found return None.
        """
        try:
            return self.axes_names_dict[self.view.get_selected_ax_name()]
        except KeyError:
            return None

    def get_selected_curve(self):
        """Get selected Line2D or ErrorbarContainer object"""
        return self.curve_names_dict[self.view.get_selected_curve_name()]

    def get_selected_curve_properties(self):
        """Get a CurveProperties object from the selected curve"""
        return CurveProperties.from_curve(self.get_selected_curve())

    def get_view_properties(self):
        """Get top level properties from view"""
        return self.view.get_properties()

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
            [plot_kwargs.pop(arg, None) for arg in
             ['capsize', 'capthick', 'ecolor', 'elinewidth', 'errorevery']]
            new_curve = ax.plot(curve.get_xdata(), curve.get_ydata(),
                                **plot_kwargs)[0]
        elif isinstance(curve, ErrorbarContainer):
            # Because of "error every" option, we need to store the original
            # error bar data on the curve or we will lose data on re-plotting
            x, y, xerr, yerr = getattr(curve, 'errorbar_data',
                                       get_data_from_errorbar_container(curve))
            new_curve = ax.errorbar(x, y, xerr=xerr, yerr=yerr, **plot_kwargs)
            setattr(new_curve, 'errorbar_data', [x, y, xerr, yerr])
        else:
            raise ValueError("Curve must have type 'Line2D' or 'ErrorbarContainer'. Found '{}'".format(type(curve)))
        return new_curve

    def _replot_selected_curve(self, plot_kwargs):
        """Replot the selected curve with the given plot kwargs"""
        ax = self.get_selected_ax()
        curve = self.get_selected_curve()
        new_curve = self.replot_curve(ax, curve, plot_kwargs)
        self.curve_names_dict[self.view.get_selected_curve_name()] = new_curve

    @classmethod
    def replot_curve(cls, ax, curve, plot_kwargs):
        if isinstance(ax, MantidAxes):
            try:
                new_curve = ax.replot_artist(curve, errorbars=True, **plot_kwargs)
            except ValueError:  # ValueError raised if Artist not tracked by Axes
                new_curve = cls._replot_mpl_curve(ax, curve, plot_kwargs)
        else:
            new_curve = cls._replot_mpl_curve(ax, curve, plot_kwargs)
        setattr(new_curve, 'errorevery', plot_kwargs.get('errorevery', 1))
        return new_curve

    def populate_curve_combo_box_and_update_view(self):
        """
        Populate curve combo box and update the view with the curve's
        properties.
        """
        self.curve_names_dict = {}
        if self._populate_select_curve_combo_box():
            self.update_view()

    def populate_select_axes_combo_box(self):
        """
        Add Axes names to 'select axes' combo box.
        Names are generated similary to in AxesTabWidgetPresenter
        """
        # Sort names by axes position
        names = []
        for name, ax in self.axes_names_dict.items():
            if curve_in_ax(ax):
                names.append(name)
        names = sorted(names, key=lambda x: x[x.rfind("("):])
        self.view.populate_select_axes_combo_box(names)

    def remove_selected_curve(self):
        """
        Remove selected curve from figure and combobox. If there are no
        curves left on the axes remove that axes from the axes combo box
        """
        # Remove curve from ax and remove from curve names dictionary
        remove_curve_from_ax(self.get_selected_curve())
        self.curve_names_dict.pop(self.view.get_selected_curve_name())

        ax = self.get_selected_ax()
        # Update the legend and redraw
        self.update_limits_and_legend(ax)
        ax.figure.canvas.draw()

        # Remove the curve from the curve selection combo box
        if self.remove_selected_curve_combo_box_entry():
            return
        self.update_view()

    def remove_selected_curve_combo_box_entry(self):
        """
        Remove selected entry in 'select_curve_combo_box'. If no curves remain
        on the axes remove the axes entry from the 'select_axes_combo_box'. If
        no axes with curves remain close the tab and return True
        """
        with block_signals(self.view.select_curve_combo_box):
            self.view.remove_select_curve_combo_box_selected_item()
            if self.view.select_curve_combo_box.count() == 0:
                self.view.remove_select_axes_combo_box_selected_item()
                if self.view.select_axes_combo_box.count() == 0:
                    self.close_tab()
                    return True

    def set_new_curve_name_in_dict_and_combo_box(self, curve, new_label):
        """Update a curve name in the curve names dict and combo box"""
        old_name = self.view.get_selected_curve_name()
        if new_label:
            curve_name = self._generate_curve_name(curve, new_label)
            self.view.set_selected_curve_selector_text(curve_name)
            self.curve_names_dict[curve_name] = self.curve_names_dict.pop(old_name)

    def set_errorbars_tab_enabled(self):
        """Enable/disable the errorbar tab for selected curve"""
        enable_errorbars = curve_has_errors(self.get_selected_curve())
        self.view.set_errorbars_tab_enabled(enable_errorbars)

    def update_view(self):
        """Update the view with the selected curve's properties"""
        curve_props = CurveProperties.from_curve(self.get_selected_curve())
        self.view.update_fields(curve_props)
        self.set_errorbars_tab_enabled()
        self.current_view_properties = curve_props

    # Private methods
    def _generate_curve_name(self, curve, label):
        if label:
            if label == '_nolegend_':
                return None
            else:
                name = label
        else:
            name = '_nolabel_'
        # Deal with case of curves sharing the same label
        idx, base_name = 1, name
        while name in self.curve_names_dict:
            if self.curve_names_dict[name] == curve:
                break
            name = base_name + " ({})".format(idx)
            idx += 1
        return name

    def _get_selected_ax_errorbars(self):
        """Get all errorbar containers in selected axes"""
        ax = self.get_selected_ax()
        return self.get_errorbars_from_ax(ax)

    @staticmethod
    def get_errorbars_from_ax(ax):
        return [cont for cont in ax.containers if isinstance(cont, ErrorbarContainer)]

    def _populate_select_curve_combo_box(self):
        """
        Add curves on selected axes to the 'select curves' combo box.
        Return False if there are no lines on the axes (this can occur
        when a user uses the "Remove Curve" button), else return True.
        """
        with block_signals(self.view.select_curve_combo_box):
            self.view.select_curve_combo_box.clear()
        selected_ax = self.get_selected_ax()
        if not selected_ax:
            self.view.close()
            return False

        active_lines = self.get_curves_from_ax(selected_ax)
        for line in active_lines:
            self._update_selected_curve_name(line)

        self.view.populate_select_curve_combo_box(
            sorted(self.curve_names_dict.keys(), key=lambda s: s.lower()))
        return True

    @staticmethod
    def get_curves_from_ax(ax):
        return ax.get_lines() + CurvesTabWidgetPresenter.get_errorbars_from_ax(ax)

    def _update_selected_curve_name(self, curve):
        """Update the selected curve's name in the curve_names_dict"""
        name = self._generate_curve_name(curve, curve.get_label())
        if name:
            self.curve_names_dict[name] = curve
