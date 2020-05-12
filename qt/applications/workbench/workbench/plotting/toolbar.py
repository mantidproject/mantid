# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#    This file is part of the mantid workbench.
#
#

from matplotlib.backends.backend_qt5 import NavigationToolbar2QT
from matplotlib.collections import LineCollection
from qtpy import QtCore, QtGui, QtPrintSupport, QtWidgets

from mantid.plots import MantidAxes
from mantid.plots.legend import convert_color_to_hex
from mantidqt.icons import get_icon
from mantidqt.plotting.figuretype import FigureType, figure_type
from mantidqt.widgets.plotconfigdialog import curve_in_ax


class WorkbenchNavigationToolbar(NavigationToolbar2QT):
    sig_home_clicked = QtCore.Signal()
    sig_grid_toggle_triggered = QtCore.Signal(bool)
    sig_active_triggered = QtCore.Signal()
    sig_hold_triggered = QtCore.Signal()
    sig_toggle_fit_triggered = QtCore.Signal()
    sig_plot_options_triggered = QtCore.Signal()
    sig_generate_plot_script_file_triggered = QtCore.Signal()
    sig_generate_plot_script_clipboard_triggered = QtCore.Signal()
    sig_waterfall_reverse_order_triggered = QtCore.Signal()
    sig_waterfall_offset_amount_triggered = QtCore.Signal()
    sig_waterfall_fill_area_triggered = QtCore.Signal()
    sig_waterfall_conversion = QtCore.Signal(bool)
    sig_change_line_collection_colour_triggered = QtCore.Signal(QtGui.QColor)

    toolitems = (
        ('Home', 'Reset axes limits', 'mdi.home', 'on_home_clicked', None),
        ('Back', 'Back to previous view', 'mdi.arrow-left', 'back', None),
        ('Forward', 'Forward to next view', 'mdi.arrow-right', 'forward', None),
        (None, None, None, None, None),
        ('Pan', 'Pan: L-click \nStretch: R-click', 'mdi.arrow-all', 'pan', False),
        ('Zoom', 'Zoom \n In: L-click+drag \n Out: R-click+drag', 'mdi.magnify', 'zoom', False),
        (None, None, None, None, None),
        ('Grid', 'Grids on/off', 'mdi.grid', 'toggle_grid', False),
        ('Save', 'Save image file', 'mdi.content-save', 'save_figure', None),
        ('Print', 'Print image', 'mdi.printer', 'print_figure', None),
        (None, None, None, None, None),
        ('Customize', 'Options menu', 'mdi.settings', 'launch_plot_options', None),
        (None, None, None, None, None),
        ('Create Script', 'Generate script to recreate the current figure',
         'mdi.script-text-outline', 'generate_plot_script', None),
        (None, None, None, None, None),
        ('Fit', 'Open/close fitting tab', None, 'toggle_fit', False),
        (None, None, None, None, None),
        ('Offset', 'Adjust curve offset %', 'mdi.arrow-expand-horizontal',
         'waterfall_offset_amount', None),
        ('Reverse Order', 'Reverse curve order', 'mdi.swap-horizontal', 'waterfall_reverse_order', None),
        ('Fill Area', 'Fill area under curves', 'mdi.format-color-fill', 'waterfall_fill_area', None)
    )

    def _init_toolbar(self):
        for text, tooltip_text, mdi_icon, callback, checked in self.toolitems:
            if text is None:
                self.addSeparator()
            else:
                if text == 'Create Script':
                    # Add a QMenu under the QToolButton for "Create Script"
                    a = self.addAction(get_icon(mdi_icon), text, lambda: None)
                    # This is the only way I could find of getting hold of the QToolButton object
                    button = [child for child in self.children()
                              if isinstance(child, QtWidgets.QToolButton)][-1]
                    menu = QtWidgets.QMenu("Menu", parent=button)
                    menu.addAction("Script to file",
                                   self.sig_generate_plot_script_file_triggered.emit)
                    menu.addAction("Script to clipboard",
                                   self.sig_generate_plot_script_clipboard_triggered.emit)
                    button.setMenu(menu)
                    button.setPopupMode(QtWidgets.QToolButton.InstantPopup)
                elif mdi_icon:
                    a = self.addAction(get_icon(mdi_icon), text, getattr(self, callback))
                else:
                    a = self.addAction(text, getattr(self, callback))
                self._actions[callback] = a
                if checked is not None:
                    a.setCheckable(True)
                    a.setChecked(checked)
                if tooltip_text is not None:
                    a.setToolTip(tooltip_text)

        # Add the x,y location widget at the right side of the toolbar
        # The stretch factor is 1 which means any resizing of the toolbar
        # will resize this label instead of the buttons.
        if self.coordinates:
            self.locLabel = QtWidgets.QLabel("", self)
            self.locLabel.setAlignment(QtCore.Qt.AlignRight | QtCore.Qt.AlignTop)
            self.locLabel.setSizePolicy(
                QtWidgets.QSizePolicy(QtWidgets.Expanding, QtWidgets.QSizePolicy.Ignored))
            labelAction = self.addWidget(self.locLabel)
            labelAction.setVisible(True)

        # Adjust icon size or they are too small in PyQt5 by default
        dpi_ratio = QtWidgets.QApplication.instance().desktop().physicalDpiX() / 100
        self.setIconSize(QtCore.QSize(24 * dpi_ratio, 24 * dpi_ratio))

    def launch_plot_options(self):
        self.sig_plot_options_triggered.emit()

    def toggle_grid(self):
        enable = self._actions['toggle_grid'].isChecked()
        self.sig_grid_toggle_triggered.emit(enable)

    def toggle_fit(self):
        fit_action = self._actions['toggle_fit']
        if fit_action.isChecked():
            if self._actions['zoom'].isChecked():
                self.zoom()
            if self._actions['pan'].isChecked():
                self.pan()
        self.sig_toggle_fit_triggered.emit()

    def trigger_fit_toggle_action(self):
        self._actions['toggle_fit'].trigger()

    def print_figure(self):
        printer = QtPrintSupport.QPrinter(QtPrintSupport.QPrinter.HighResolution)
        printer.setOrientation(QtPrintSupport.QPrinter.Landscape)
        print_dlg = QtPrintSupport.QPrintDialog(printer)
        if print_dlg.exec_() == QtWidgets.QDialog.Accepted:
            painter = QtGui.QPainter(printer)
            page_size = printer.pageRect()
            pixmap = self.canvas.grab().scaled(page_size.width(), page_size.height(),
                                               QtCore.Qt.KeepAspectRatio)
            painter.drawPixmap(0, 0, pixmap)
            painter.end()

    def contextMenuEvent(self, event):
        pass

    def on_home_clicked(self):
        self.sig_home_clicked.emit()
        self.push_current()

    def waterfall_conversion(self, is_waterfall):
        self.sig_waterfall_conversion.emit(is_waterfall)

    def set_waterfall_options_enabled(self, on):
        for action in ['waterfall_offset_amount', 'waterfall_reverse_order', 'waterfall_fill_area']:
            toolbar_action = self._actions[action]
            toolbar_action.setEnabled(on)
            toolbar_action.setVisible(on)

        # show/hide separator
        fit_action = self._actions['toggle_fit']
        self.toggle_separator_visibility(fit_action, on)

    def set_generate_plot_script_enabled(self, enabled):
        action = self._actions['generate_plot_script']
        action.setEnabled(enabled)
        action.setVisible(enabled)
        # Show/hide the separator between this button and the "Fit" button
        self.toggle_separator_visibility(action, enabled)

    def _set_fit_enabled(self, on):
        action = self._actions['toggle_fit']
        action.setEnabled(on)
        action.setVisible(on)

    def waterfall_offset_amount(self):
        self.sig_waterfall_offset_amount_triggered.emit()

    def waterfall_reverse_order(self):
        self.sig_waterfall_reverse_order_triggered.emit()

    def waterfall_fill_area(self):
        self.sig_waterfall_fill_area_triggered.emit()

    def adjust_for_3d_plots(self):
        self._actions['toggle_grid'].setChecked(True)

        for action in ['back', 'forward', 'pan', 'zoom']:
            toolbar_action = self._actions[action]
            toolbar_action.setEnabled(False)
            toolbar_action.setVisible(False)

        action = self._actions['forward']
        self.toggle_separator_visibility(action, False)

    def toggle_separator_visibility(self, action, enabled):
        # shows/hides the separator positioned immediately after the action
        for i, toolbar_action in enumerate(self.actions()):
            if toolbar_action == action:
                self.actions()[i + 1].setVisible(enabled)
                break

    def set_buttons_visiblity(self, fig):
        if figure_type(fig) not in [FigureType.Line, FigureType.Errorbar] and len(fig.get_axes()) > 1:
            self._set_fit_enabled(False)

        # For plot-to-script button to show, every axis must be a MantidAxes with lines in it
        # Plot-to-script currently doesn't work with waterfall plots so the button is hidden for that plot type.
        if not all((isinstance(ax, MantidAxes) and curve_in_ax(ax)) for ax in fig.get_axes()) or \
                fig.get_axes()[0].is_waterfall():
            self.set_generate_plot_script_enabled(False)

        # Only show options specific to waterfall plots if the axes is a MantidAxes and is a waterfall plot.
        if not isinstance(fig.get_axes()[0], MantidAxes) or not fig.get_axes()[0].is_waterfall():
            self.set_waterfall_options_enabled(False)

        # For contour and wireframe plots, add a toolbar option to change the colour of the lines.
        if figure_type(fig) in [FigureType.Wireframe, FigureType.Contour]:
            self.set_up_color_selector_toolbar_button(fig)

        if figure_type(fig) in [FigureType.Surface, FigureType.Wireframe]:
            self.adjust_for_3d_plots()

    def set_up_color_selector_toolbar_button(self, fig):
        # check if the action is already in the toolbar
        if self._actions.get('line_colour'):
            return

        a = self.addAction(get_icon('mdi.palette'), "Line Colour", lambda: None)
        self._actions['line_colour'] = a

        if figure_type(fig) == FigureType.Wireframe:
            a.setToolTip("Set the colour of the wireframe.")
        else:
            a.setToolTip("Set the colour of the contour lines.")

        line_collection = next(col for col in fig.get_axes()[0].collections if isinstance(col, LineCollection))
        initial_colour = convert_color_to_hex(line_collection.get_color()[0])

        colour_dialog = QtWidgets.QColorDialog(QtGui.QColor(initial_colour))
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
        return self._toolbar._actions['zoom'].isChecked()

    def is_pan_active(self):
        """
        Check if the Pan button is checked
        """
        return self._toolbar._actions['pan'].isChecked()

    def is_tool_active(self):
        """
        Check if any of the zoom buttons are checked
        """
        return self.is_pan_active() or self.is_zoom_active()

    def toggle_fit_button_checked(self):
        fit_action = self._toolbar._actions['toggle_fit']
        if fit_action.isChecked():
            fit_action.setChecked(False)
        else:
            fit_action.setChecked(True)

    def home_button_connect(self, slot):
        self._toolbar.sig_home_clicked.connect(slot)

    def emit_sig_home_clicked(self):
        self._toolbar.sig_home_clicked.emit()
