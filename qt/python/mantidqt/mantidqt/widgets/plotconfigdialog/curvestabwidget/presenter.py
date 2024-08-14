# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.


from matplotlib.lines import Line2D
from matplotlib.container import ErrorbarContainer

from mantid.plots.legend import LegendProperties
from mantid.plots import datafunctions, MantidAxes
from mantidqt.utils.qt import block_signals
from mantidqt.widgets.plotconfigdialog import get_axes_names_dict, curve_in_ax
from mantidqt.widgets.plotconfigdialog.curvestabwidget import CurveProperties, curve_has_errors, remove_curve_from_ax
from mantidqt.widgets.plotconfigdialog.curvestabwidget.view import CurvesTabWidgetView
from workbench.plotting.figureerrorsmanager import FigureErrorsManager
from mantid.kernel import logger


class CurvesTabWidgetPresenter:
    def __init__(self, fig, view=None, parent_view=None, parent_presenter=None, legend_tab=None):
        self.fig = fig

        # The legend tab is passed in so that it can be removed if all curves are removed.
        self.legend_tab = legend_tab
        self.legend_props = None

        if not view:
            self.view = CurvesTabWidgetView(parent_view)
        else:
            self.view = view

        self.current_view_properties = None
        self.curve_names_dict = {}
        self.axes_names_dict = None
        self.update_view()

        # Signals
        self.view.select_axes_combo_box.currentIndexChanged.connect(self.on_axes_index_changed)
        self.view.select_curve_list.currentRowChanged.connect(self.on_curves_row_changed)
        self.view.remove_curve_button.clicked.connect(self.remove_selected_curves)
        self.view.delete_key_pressed.connect(self.remove_selected_curves)
        self.view.line.apply_to_all_button.clicked.connect(self.line_apply_to_all)
        self.view.marker.apply_to_all_button.clicked.connect(self.marker_apply_to_all)
        self.view.marker.marker_style_combo_box.currentTextChanged.connect(self.view.marker.set_colour_fields_enabled)
        self.view.errorbars.apply_to_all_button.clicked.connect(self.errorbars_apply_to_all)
        self.view.select_curve_list.itemSelectionChanged.connect(self.on_curves_selection_changed)

        self.parent_presenter = parent_presenter

    def apply_properties(self):
        """Take properties from views and set them on the selected curve"""
        ax = self.get_selected_ax()
        if ax.legend_:
            self.legend_props = LegendProperties.from_legend(ax.legend_)

        view_props = self.get_view_properties()
        if view_props == self.current_view_properties:
            return

        plot_kwargs = view_props.get_plot_kwargs()
        # Re-plot curve
        self._replot_current_curve(plot_kwargs)
        curve = self.get_current_curve()
        # Set the curve's new name in the names dict and combo box
        self.set_new_curve_name_in_dict_and_list(curve, view_props.label)
        FigureErrorsManager.toggle_errors(curve, view_props)
        self.current_view_properties = view_props

        FigureErrorsManager.update_limits_and_legend(ax, self.legend_props)

    def close_tab(self):
        """Close the tab and set the view to None"""
        self.parent_presenter.forget_tab_from_presenter(self)
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

    def get_current_curve(self):
        """Get selected Line2D or ErrorbarContainer object"""
        name = self.view.get_current_curve_name()
        return self.curve_names_dict[name]

    def get_selected_curves(self):
        """Get a list of selected Line2D or ErrorbarContainer objects"""
        names = self.view.get_selected_curves_names()
        return map(lambda name: self.curve_names_dict[name], names)

    def get_current_curve_properties(self):
        """Get a CurveProperties object from the selected curve"""
        return CurveProperties.from_curve(self.get_current_curve())

    def get_view_properties(self):
        """Get top level properties from view"""
        return self.view.get_properties()

    def _replot_current_curve(self, plot_kwargs):
        """Replot the selected curve with the given plot kwargs"""
        ax = self.get_selected_ax()
        curve = self.get_current_curve()

        waterfall = False
        if isinstance(ax, MantidAxes):
            waterfall = ax.is_waterfall()
        check_line_colour = False
        # If the plot is a waterfall plot and the user has set it so the area under each line is filled, and the fill
        # colour for each line is set as the line colour, after the curve is updated we need to check if its colour has
        # changed so the fill can be updated accordingly.
        if waterfall and ax.waterfall_has_fill() and datafunctions.waterfall_fill_is_line_colour(ax):
            check_line_colour = True

        if isinstance(curve, Line2D):
            curve_index = ax.get_lines().index(curve)
            errorbar = False
        elif isinstance(curve, ErrorbarContainer):
            curve_index = ax.get_lines().index(curve[0])
            errorbar = True
        else:
            return

        # When you remove the curve on a waterfall plot, the remaining curves are repositioned so that they are
        # equally spaced apart. However since the curve is being replotted we don't want that to happen, so here
        # the waterfall offsets are set to 0 so the plot appears to be non-waterfall. The offsets are then re-set
        # after the curve is replotted.
        if waterfall:
            x_offset, y_offset = ax.waterfall_x_offset, ax.waterfall_y_offset
            ax.waterfall_x_offset = ax.waterfall_y_offset = 0

        new_curve = FigureErrorsManager.replot_curve(ax, curve, plot_kwargs)
        self.curve_names_dict[self.view.get_current_curve_name()] = new_curve

        if isinstance(ax, MantidAxes):
            errorbar_cap_lines = datafunctions.remove_and_return_errorbar_cap_lines(ax)
        else:
            errorbar_cap_lines = []

        # When a curve is redrawn it is moved to the back of the list of curves so here it is moved back to its previous
        # position. This is so that the correct offset is applied to the curve if the plot is a waterfall plot, but it
        # also just makes sense for the curve order to remain unchanged.
        # Since mpl 3.7 made ax.lines immutable, we have to do this workaround.
        lines_to_remove = ax.get_lines()[curve_index:]
        if lines_to_remove:
            for line in lines_to_remove:
                line.remove()
            ax.add_line(lines_to_remove.pop())
            for line in lines_to_remove:
                ax.add_line(line)

        if waterfall:
            # Set the waterfall offsets to what they were previously.
            ax.waterfall_x_offset, ax.waterfall_y_offset = x_offset, y_offset
            if check_line_colour:
                # curve can be either a Line2D or an ErrorContainer and the colour is accessed differently for each.
                if not errorbar:
                    # if the line colour hasn't changed then the fill colour doesn't need to be updated.
                    update_fill = curve.get_color() != new_curve[0].get_color()
                else:
                    update_fill = curve[0].get_color() != new_curve[0].get_color()
                datafunctions.convert_single_line_to_waterfall(ax, curve_index, need_to_update_fill=update_fill)
            else:
                # the curve has been reset to its original position so for a waterfall plot it needs to be re-offset.
                datafunctions.convert_single_line_to_waterfall(ax, curve_index)

            datafunctions.set_waterfall_fill_visible(ax, curve_index)

        for cap in errorbar_cap_lines:
            ax.add_line(cap)

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
        names = sorted(names, key=lambda x: x[x.rfind("(") :])
        self.view.populate_select_axes_combo_box(names)

    def remove_selected_curves(self):
        """
        Remove selected curves from figure and curves list. If there are no
        curves left on the axes remove that axes from the axes combo box
        """
        ax = self.get_selected_ax()
        if ax.legend_:
            self.legend_props = LegendProperties.from_legend(ax.legend_)

        # Remove curves from ax and remove from curve names dictionary
        curves_to_remove_names = self.view.get_selected_curves_names()
        for name in curves_to_remove_names:
            remove_curve_from_ax(self.curve_names_dict[name])
            self.curve_names_dict.pop(name)

        self.set_apply_to_all_buttons_enabled()

        ax = self.get_selected_ax()
        # Update the legend and redraw
        FigureErrorsManager.update_limits_and_legend(ax, self.legend_props)
        try:
            ax.figure.canvas.draw()
        except ValueError as ex:
            logger.error("Error redrawing figure canvas: \n" + str(ex))

        # Remove the curve from the curve selection list
        if self.remove_selected_curve_list_entry():
            return
        self.update_view()

    def remove_selected_curve_list_entry(self):
        """
        Remove selected entry in 'select_curve_list'. If no curves remain
        on the axes remove the axes entry from the 'select_axes_combo_box'. If
        no axes with curves remain close the tab and return True
        """
        with block_signals(self.view.select_curve_list), block_signals(self.view.select_axes_combo_box):
            self.view.remove_select_curve_list_selected_items()
            if self.view.select_curve_list.count() == 0:
                self.view.remove_select_axes_combo_box_selected_item()
                if self.view.select_axes_combo_box.count() == 0:
                    self.close_tab()
                    if self.legend_tab:
                        self.legend_tab.close_tab()
                    return True

    def set_new_curve_name_in_dict_and_list(self, curve, new_label):
        """Update a curve name in the curve names dict and combo box"""
        old_name = self.view.get_current_curve_name()
        if new_label:
            curve_name = self._generate_curve_name(curve, new_label)
            self.view.set_selected_curve_selector_text(curve_name)
            self.curve_names_dict[curve_name] = self.curve_names_dict.pop(old_name)

    def set_errorbars_tab_enabled(self):
        """Enable/disable the errorbar tab for selected curve"""
        enable_errorbars = curve_has_errors(self.get_current_curve())
        self.view.set_errorbars_tab_enabled(enable_errorbars)

    def on_axes_index_changed(self):
        # No axes properties have changed, but we need to update the rest of
        # the view to match the curve that are on the new axes.
        self.update_view(update_axes=False)

    def on_curves_row_changed(self):
        # No properties about the axes or the curves have changed, but we need to
        # update the rest of the view so the information matches the new selected curve.
        self.update_view(update_axes=False, update_curves=False)

    def update_view(self, update_axes=True, update_curves=True):
        """Update the view with the selected axes and curve properties.
        By default we update everything since, if we changed something about
        the axes (e.g. title), we need to ensure these propagate to the curves tab.

        update_axes=True -> the axes combo will be updated
        update_curves=True -> the curves combo will be updated

        Regardless of the two parameters, the rest of the curves tab will update to show
        the properties of the selected curve."""

        if update_axes:
            # Update the 'select axes' combo box. Do this if axes properties have changed.
            self.axes_names_dict = get_axes_names_dict(self.fig, curves_only=True)
            self.populate_select_axes_combo_box()
        if update_curves:
            # Update the 'select curves' combo box. Do this if curve properties have changed, or
            # if user selects a different set of axes.
            self._populate_select_curve_list()

        self.set_apply_to_all_buttons_enabled()

        # Then update the rest of the view to reflect the selected combo items.
        curve_props = CurveProperties.from_curve(self.get_current_curve())
        self.view.update_fields(curve_props)
        # only enable error bar tabs if we do not have multiple curves selected
        if not len(self.view.select_curve_list.selectedItems()) > 1:
            self.set_errorbars_tab_enabled()

        self.current_view_properties = curve_props

    # Private methods
    def _generate_curve_name(self, curve, label):
        if label:
            if label == "_nolegend_":
                return None
            else:
                name = label
        else:
            name = "_nolabel_"
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
        return FigureErrorsManager.get_errorbars_from_ax(ax)

    def _populate_select_curve_list(self):
        """
        Add curves on selected axes to the 'select curves' combo box.
        Return False if there are no lines on the axes (this can occur
        when a user uses the "Remove Curve" button), else return True.
        """
        with block_signals(self.view.select_curve_list):
            self.view.select_curve_list.clear()
        selected_ax = self.get_selected_ax()
        if not selected_ax:
            self.view.close()
            return False

        # Get the lines in the order that they are listed on the legend.
        active_lines = datafunctions.get_legend_handles(selected_ax)

        # This dict is about to be repopulated with curves on the new selected axes
        self.curve_names_dict = {}
        for line in active_lines:
            self._update_selected_curve_name(line)
        self.view.populate_select_curve_list(list(self.curve_names_dict))
        return True

    def _update_selected_curve_name(self, curve):
        """Update the selected curve's name in the curve_names_dict"""
        name = self._generate_curve_name(curve, curve.get_label())
        if name:
            self.curve_names_dict[name] = curve

    def set_apply_to_all_buttons_enabled(self):
        """
        Enables the Apply to All buttons in the line, marker, and errorbar tabs
        if there is more than one curve.
        """
        if len(self.curve_names_dict) > 1:
            self.view.line.set_apply_to_all_enabled(True)
            self.view.marker.set_apply_to_all_enabled(True)
            self.view.errorbars.set_apply_to_all_enabled(True)
        else:
            self.view.line.set_apply_to_all_enabled(False)
            self.view.marker.set_apply_to_all_enabled(False)
            self.view.errorbars.set_apply_to_all_enabled(False)

    def line_apply_to_all(self):
        """
        Applies the settings in the line tab for the current curve to all other curves.
        """
        current_curve_index = self.view.select_curve_list.currentRow()

        line_style = self.view.line.get_style()
        draw_style = self.view.line.get_draw_style()
        width = self.view.line.get_width()

        for i in range(len(self.curve_names_dict)):
            self.view.select_curve_list.setCurrentRow(i)

            self.view.line.set_style(line_style)
            self.view.line.set_draw_style(draw_style)
            self.view.line.set_width(width)

            self.apply_properties()

        self.fig.canvas.draw()
        self.view.select_curve_list.setCurrentRow(current_curve_index)

    def marker_apply_to_all(self):
        current_curve_index = self.view.select_curve_list.currentRow()

        marker_style = self.view.marker.get_style()
        marker_size = self.view.marker.get_size()

        for i in range(len(self.curve_names_dict)):
            self.view.select_curve_list.setCurrentRow(i)

            self.view.marker.set_style(marker_style)
            self.view.marker.set_size(marker_size)

            self.apply_properties()

        self.fig.canvas.draw()
        self.view.select_curve_list.setCurrentRow(current_curve_index)

    def errorbars_apply_to_all(self):
        current_curve_index = self.view.select_curve_list.currentRow()

        checked = self.view.errorbars.get_hide()

        if not checked:
            width = self.view.errorbars.get_width()
            capsize = self.view.errorbars.get_capsize()
            cap_thickness = self.view.errorbars.get_cap_thickness()
            error_every = self.view.errorbars.get_error_every()

        for i in range(len(self.curve_names_dict)):
            self.view.select_curve_list.setCurrentRow(i)

            self.view.errorbars.set_hide(checked)

            if not checked:
                self.view.errorbars.set_width(width)
                self.view.errorbars.set_capsize(capsize)
                self.view.errorbars.set_cap_thickness(cap_thickness)
                self.view.errorbars.set_error_every(error_every)

            self.apply_properties()

        self.fig.canvas.draw()
        self.view.select_curve_list.setCurrentRow(current_curve_index)

    def on_curves_selection_changed(self):
        if len(self.view.select_curve_list.selectedItems()) > 1:
            self.view.enable_curve_config(False)
        else:
            self.view.enable_curve_config(True)
            self.set_errorbars_tab_enabled()

    def set_axes_from_object(self, axes_to_set):
        """
        Given the axes object, sets the selected index of the axes combo
        to be the index corresponding to this object.
        """
        index_to_set = None
        for index, axes in enumerate(self.axes_names_dict.values()):
            if axes_to_set == axes:
                index_to_set = index
                break

        if index_to_set is None:  # could be 0, incorrectly raising error.
            raise ValueError("Axes object does not exist in curves tab")

        self.view.select_axes_combo_box.setCurrentIndex(index_to_set)

    def set_curve_from_object(self, curve_to_set):
        """
        Given the curve object, sets the selected index of the curves list
        to be the index corresponding to this object.
        """
        index_to_set = None
        for index, curve in enumerate(self.curve_names_dict.values()):
            if curve_to_set == curve:
                index_to_set = index
                break

        if index_to_set is None:
            raise ValueError("Curve object does not exist in curves tab")

        self.view.select_curve_list.setCurrentRow(index_to_set)
        self.view.select_curve_list.item(index_to_set).setSelected(True)
