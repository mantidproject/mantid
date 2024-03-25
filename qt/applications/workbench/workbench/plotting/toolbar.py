# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#    This file is part of the mantid workbench.
#
#
from matplotlib.collections import LineCollection, PathCollection
from qtpy import QtCore, QtGui, QtPrintSupport, QtWidgets

from mantid.plots import MantidAxes
from mantid.plots.utility import convert_color_to_hex
from mantidqt.icons import get_icon
from mantidqt.plotting.figuretype import FigureType, figure_type
from mantidqt.plotting.mantid_navigation_toolbar import MantidNavigationToolbar, MantidStandardNavigationTools, MantidNavigationTool
from mantidqt.widgets.plotconfigdialog import curve_in_ax


def _create_script_action(self, text, tooltip_text, mdi_icon, *args):
    # Add a QMenu under the QToolButton for "Create Script"
    a = self.addAction(get_icon(mdi_icon), text, lambda: None)
    # This is the only way I could find of getting hold of the QToolButton object
    button = [child for child in self.children() if isinstance(child, QtWidgets.QToolButton)][-1]
    menu = QtWidgets.QMenu("Menu", parent=button)
    menu.addAction("Script to file", self.sig_generate_plot_script_file_triggered.emit)
    menu.addAction("Script to clipboard", self.sig_generate_plot_script_clipboard_triggered.emit)
    button.setMenu(menu)
    button.setPopupMode(QtWidgets.QToolButton.InstantPopup)
    return a


class WorkbenchNavigationToolbar(MantidNavigationToolbar):
    sig_home_clicked = QtCore.Signal()
    sig_grid_toggle_triggered = QtCore.Signal(bool)
    sig_active_triggered = QtCore.Signal()
    sig_hold_triggered = QtCore.Signal()
    sig_toggle_fit_triggered = QtCore.Signal()
    sig_toggle_superplot_triggered = QtCore.Signal()
    sig_copy_to_clipboard_triggered = QtCore.Signal()
    sig_plot_options_triggered = QtCore.Signal()
    sig_plot_help_triggered = QtCore.Signal()
    sig_generate_plot_script_file_triggered = QtCore.Signal()
    sig_generate_plot_script_clipboard_triggered = QtCore.Signal()
    sig_waterfall_reverse_order_triggered = QtCore.Signal()
    sig_waterfall_offset_amount_triggered = QtCore.Signal()
    sig_waterfall_fill_area_triggered = QtCore.Signal()
    sig_waterfall_conversion = QtCore.Signal(bool)
    sig_change_line_collection_colour_triggered = QtCore.Signal(QtGui.QColor)
    sig_hide_plot_triggered = QtCore.Signal()

    toolitems = (
        MantidNavigationTool("Home", "Reset axes limits", "mdi.home", "on_home_clicked", None),
        MantidStandardNavigationTools.BACK,
        MantidStandardNavigationTools.FORWARD,
        MantidStandardNavigationTools.SEPARATOR,
        MantidStandardNavigationTools.PAN,
        MantidStandardNavigationTools.ZOOM,
        MantidStandardNavigationTools.SEPARATOR,
        MantidNavigationTool("Grid", "Grids on/off", "mdi.grid", "toggle_grid", False),
        MantidNavigationTool("Copy", "Copy image to clipboard", "mdi.content-copy", "copy_to_clipboard", None),
        MantidStandardNavigationTools.SAVE,
        MantidNavigationTool("Print", "Print image", "mdi.printer", "print_figure", None),
        MantidStandardNavigationTools.SEPARATOR,
        MantidNavigationTool("Customize", "Options menu", "mdi.settings", "launch_plot_options", None),
        MantidStandardNavigationTools.SEPARATOR,
        MantidNavigationTool(
            "Create Script",
            "Generate script to recreate the current figure",
            "mdi.script-text-outline",
            "generate_plot_script",
            None,
            _create_script_action,
        ),
        MantidStandardNavigationTools.SEPARATOR,
        MantidNavigationTool("Fit", "Open/close fitting tab", None, "toggle_fit", False),
        MantidStandardNavigationTools.SEPARATOR,
        MantidNavigationTool("Superplot", "Open/close superplot tab", None, "toggle_superplot", False),
        MantidStandardNavigationTools.SEPARATOR,
        MantidNavigationTool("Offset", "Adjust curve offset %", "mdi.arrow-expand-horizontal", "waterfall_offset_amount", None),
        MantidNavigationTool("Reverse Order", "Reverse curve order", "mdi.swap-horizontal", "waterfall_reverse_order", None),
        MantidNavigationTool("Fill Area", "Fill area under curves", "mdi.format-color-fill", "waterfall_fill_area", None),
        MantidStandardNavigationTools.SEPARATOR,
        MantidNavigationTool("Help", "Open plotting help documentation", "mdi.help", "launch_plot_help", None),
        MantidNavigationTool("Hide", "Hide the plot", "mdi.eye", "hide_plot", None),
    )

    def __init__(self, canvas, parent, coordinates=True):
        super().__init__(canvas, parent, coordinates)

        # Adjust icon size or they are too small in PyQt5 by default
        dpi_ratio = QtWidgets.QApplication.instance().desktop().physicalDpiX() / 100
        self.setIconSize(QtCore.QSize(int(24 * dpi_ratio), int(24 * dpi_ratio)))

    def hide_plot(self):
        self.sig_hide_plot_triggered.emit()

    def copy_to_clipboard(self):
        self.sig_copy_to_clipboard_triggered.emit()

    def launch_plot_options(self):
        self.sig_plot_options_triggered.emit()

    def toggle_grid(self, enable=None):
        if enable is None:
            # Toggle grid to whatever state the toolbar button is in
            enable = self._actions["toggle_grid"].isChecked()
        else:
            # Otherwise toggle grid to whatever state we were given
            self._actions["toggle_grid"].setChecked(enable)
        self.sig_grid_toggle_triggered.emit(enable)

    def toggle_fit(self):
        fit_action = self._actions["toggle_fit"]
        if fit_action.isChecked():
            if self._actions["zoom"].isChecked():
                self.zoom()
            if self._actions["pan"].isChecked():
                self.pan()
        self.sig_toggle_fit_triggered.emit()

    def toggle_superplot(self):
        self.sig_toggle_superplot_triggered.emit()

    def trigger_fit_toggle_action(self):
        self._actions["toggle_fit"].trigger()

    def launch_plot_help(self):
        self.sig_plot_help_triggered.emit()

    def print_figure(self):
        printer = QtPrintSupport.QPrinter(QtPrintSupport.QPrinter.HighResolution)
        printer.setOrientation(QtPrintSupport.QPrinter.Landscape)
        print_dlg = QtPrintSupport.QPrintDialog(printer)
        if print_dlg.exec_() == QtWidgets.QDialog.Accepted:
            painter = QtGui.QPainter(printer)
            page_size = printer.pageRect()
            pixmap = self.canvas.grab().scaled(page_size.width(), page_size.height(), QtCore.Qt.KeepAspectRatio)
            painter.drawPixmap(0, 0, pixmap)
            painter.end()

    def contextMenuEvent(self, event):
        pass

    def on_home_clicked(self):
        self.sig_home_clicked.emit()
        self.home()
        self.push_current()

    def waterfall_conversion(self, is_waterfall):
        self.sig_waterfall_conversion.emit(is_waterfall)

    def set_waterfall_options_enabled(self, on):
        for action in ["waterfall_offset_amount", "waterfall_reverse_order", "waterfall_fill_area"]:
            toolbar_action = self._actions[action]
            toolbar_action.setEnabled(on)
            toolbar_action.setVisible(on)

        # Show/hide the separator between this button and help button
        action = self._actions["waterfall_fill_area"]
        self.toggle_separator_visibility(action, on)

    def set_generate_plot_script_enabled(self, enabled):
        action = self._actions["generate_plot_script"]
        action.setEnabled(enabled)
        action.setVisible(enabled)
        # Show/hide the separator between this button and the "Fit" button
        self.toggle_separator_visibility(action, enabled)

    def set_fit_enabled(self, on):
        action = self._actions["toggle_fit"]
        action.setEnabled(on)
        action.setVisible(on)
        # Show/hide the separator between this button and help button / waterfall options
        self.toggle_separator_visibility(action, on)

    def set_superplot_enabled(self, on):
        action = self._actions["toggle_superplot"]
        action.setEnabled(on)
        action.setVisible(on)
        # Show/hide the separator between this button and help button / waterfall options
        self.toggle_separator_visibility(action, on)

    def waterfall_offset_amount(self):
        self.sig_waterfall_offset_amount_triggered.emit()

    def waterfall_reverse_order(self):
        self.sig_waterfall_reverse_order_triggered.emit()

    def waterfall_fill_area(self):
        self.sig_waterfall_fill_area_triggered.emit()

    def adjust_for_3d_plots(self):
        self._actions["toggle_grid"].setChecked(True)

        for action in ["back", "forward", "pan", "zoom"]:
            toolbar_action = self._actions[action]
            toolbar_action.setEnabled(False)
            toolbar_action.setVisible(False)

        action = self._actions["forward"]
        self.toggle_separator_visibility(action, False)

    def toggle_separator_visibility(self, action, enabled):
        # shows/hides the separator positioned immediately after the action
        for i, toolbar_action in enumerate(self.actions()):
            if toolbar_action == action:
                separator = self.actions()[i + 1]
                if separator and separator.isSeparator():
                    separator.setVisible(enabled)
                break

    def set_buttons_visibility(self, fig):
        #  check if fitting and superplot should be enabled
        if figure_type(fig) not in [FigureType.Line, FigureType.Errorbar] or len(fig.get_axes()) > 1:
            self.set_fit_enabled(False)
            self.set_superplot_enabled(False)
        for ax in fig.get_axes():
            for artist in ax.get_lines():
                try:
                    if ax.get_artists_sample_log_plot_details(artist) is not None:
                        self.set_fit_enabled(False)
                        break
                except Exception:
                    # The artist is not tracked - ignore this one and check the rest
                    continue
            if isinstance(ax, MantidAxes):
                for artists in ax.tracked_workspaces.values():
                    if any([artist.workspace_index is None for artist in artists]):
                        self.set_fit_enabled(False)
                        self.set_superplot_enabled(False)

        # Plot-to-script currently doesn't work with waterfall plots so the button is hidden for that plot type.
        # There must be at least one MantidAxis plot with data for to generate a script, others will be skipped
        if not any((isinstance(ax, MantidAxes) and curve_in_ax(ax)) for ax in fig.get_axes()) or fig.get_axes()[0].is_waterfall():
            self.set_generate_plot_script_enabled(False)
            self.set_fit_enabled(False)
            self.set_superplot_enabled(False)

        # reenable script generation for colormaps
        if self.is_colormap(fig):
            self.set_generate_plot_script_enabled(True)

        # Only show options specific to waterfall plots if the axes is a MantidAxes and is a waterfall plot.
        if not isinstance(fig.get_axes()[0], MantidAxes) or not fig.get_axes()[0].is_waterfall():
            self.set_waterfall_options_enabled(False)

        # For contour and wireframe plots, add a toolbar option to change the colour of the lines.
        if figure_type(fig) in [FigureType.Wireframe, FigureType.Contour]:
            self.set_up_color_selector_toolbar_button(fig)

        if figure_type(fig) in [FigureType.Surface, FigureType.Wireframe, FigureType.Mesh]:
            self.adjust_for_3d_plots()

        # Determine the toggle state of the grid button. If all subplots have grids, then set it to on.
        is_major_grid_on = False
        for ax in fig.get_axes():
            # Don't look for grids on colour bars.
            if self._is_colorbar(ax):
                continue
            if hasattr(ax, "grid_on"):
                is_major_grid_on = ax.grid_on()
            else:
                is_major_grid_on = ax.xaxis._major_tick_kw.get("gridOn", False) and ax.yaxis._major_tick_kw.get("gridOn", False)
            # If ANY of the axes have no grid, set the button to unchecked.
            if not is_major_grid_on:
                break
        self._actions["toggle_grid"].setChecked(is_major_grid_on)

    def is_colormap(self, fig):
        """Identify as a single colour map if it has a axes, one with the plot and the other the colorbar"""
        if figure_type(fig) in [FigureType.Image] and len(fig.get_axes()) == 2:
            if (
                len(fig.get_axes()[0].get_images()) == 1
                and len(fig.get_axes()[1].get_images()) == 0
                and self._is_colorbar(fig.get_axes()[1])
            ):
                return True
        else:
            return False

    @classmethod
    def _is_colorbar(cls, ax):
        """Determine whether an axes object is a colorbar"""
        return hasattr(ax, "_colorbar")

    def set_up_color_selector_toolbar_button(self, fig):
        # check if the action is already in the toolbar
        if self._actions.get("line_colour"):
            return

        a = self.addAction(get_icon("mdi.palette"), "Line Colour", lambda: None)
        self._actions["line_colour"] = a

        if figure_type(fig) == FigureType.Wireframe:
            a.setToolTip("Set the colour of the wireframe.")
        else:
            a.setToolTip("Set the colour of the contour lines.")

        for col in fig.get_axes()[0].collections:
            if isinstance(col, LineCollection):
                current_ax_colour = col.get_color()
                break
            elif isinstance(col, PathCollection):
                current_ax_colour = col.get_edgecolor()
                break

        # initial QColorDialog colour
        colour_dialog = QtWidgets.QColorDialog(QtGui.QColor(convert_color_to_hex(current_ax_colour)))
        colour_dialog.setOption(QtWidgets.QColorDialog.NoButtons)
        colour_dialog.setOption(QtWidgets.QColorDialog.DontUseNativeDialog)
        colour_dialog.currentColorChanged.connect(self.change_line_collection_colour)

        button = [child for child in self.children() if isinstance(child, QtWidgets.QToolButton)][-1]

        menu = QtWidgets.QMenu("Menu", parent=button)
        colour_selector_action = QtWidgets.QWidgetAction(menu)
        colour_selector_action.setDefaultWidget(colour_dialog)
        menu.addAction(colour_selector_action)

        button.setMenu(menu)
        button.setPopupMode(QtWidgets.QToolButton.InstantPopup)

    def change_line_collection_colour(self, colour):
        self.sig_change_line_collection_colour_triggered.emit(colour)


class ToolbarStateManager(object):
    """
    An object that lets users check and manipulate the state of the toolbar
    whilst hiding any implementation details.
    """

    def __init__(self, toolbar):
        self._toolbar = toolbar

    def is_zoom_active(self):
        """
        Check if the Zoom button is checked
        """
        return self._toolbar._actions["zoom"].isChecked()

    def is_pan_active(self):
        """
        Check if the Pan button is checked
        """
        return self._toolbar._actions["pan"].isChecked()

    def is_tool_active(self):
        """
        Check if any of the zoom buttons are checked
        """
        return self.is_pan_active() or self.is_zoom_active()

    def is_fit_active(self):
        """
        Check if the fit button is checked
        """
        return self._toolbar._actions["toggle_fit"].isChecked()

    def toggle_fit_button_checked(self):
        fit_action = self._toolbar._actions["toggle_fit"]
        if fit_action.isChecked():
            fit_action.setChecked(False)
        else:
            fit_action.setChecked(True)

    def home_button_connect(self, slot):
        self._toolbar.sig_home_clicked.connect(slot)

    def emit_sig_home_clicked(self):
        self._toolbar.sig_home_clicked.emit()
