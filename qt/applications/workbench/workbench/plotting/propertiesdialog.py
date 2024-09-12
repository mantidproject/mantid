# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.
#
#

# 3rdparty imports

from mantid.plots.datafunctions import update_colorbar_scale, get_images_from_figure
from mantidqt.plotting.figuretype import FigureType, figure_type
from mantidqt.utils.qt import load_ui

from matplotlib.colors import LogNorm, Normalize
from matplotlib.ticker import ScalarFormatter, LogFormatterSciNotation
from mpl_toolkits.mplot3d.axes3d import Axes3D
from qtpy.QtGui import QDoubleValidator, QIcon
from qtpy.QtWidgets import QDialog, QWidget

TREAT_LOG_NEGATIVE_VALUES = "clip"
DECIMAL_FORMAT = "Decimal Format"
SCIENTIFIC_FORMAT = "Scientific Format"


class PropertiesEditorBase(QDialog):
    """Base class for all dialogs responsible for providing
    access to change figure properties by clicking on the canvas"""

    def __init__(self, ui_file, canvas):
        """
        :param canvas: A reference to the canvas to be updated
        """
        super(PropertiesEditorBase, self).__init__()
        self.canvas = canvas
        self.ui = load_ui(__file__, ui_file, baseinstance=self)
        self.ui.buttonBox.accepted.connect(self.on_ok)
        self.ui.buttonBox.rejected.connect(self.reject)
        self.ui.setWindowIcon(QIcon(":/images/MantidIcon.ico"))

    def on_ok(self):
        try:
            self.changes_accepted()
            self.canvas.draw()
        except Exception as exc:
            # restore canvas and display error
            self.error_occurred(exc)
            self.canvas.draw()
        else:
            self.accept()

    def changes_accepted(self):
        raise NotImplementedError("Derived classes should override changes_accepted()")

    def error_occurred(self, exc):
        """Indicates a redraw error occurred. Derived classes should override this
        and revert the state of the canvas and display the error
        """
        raise NotImplementedError("Derived classes should override error_occurred")


class LabelEditorModel(object):
    def __init__(self, label_text):
        self.label_text = label_text


class LabelEditor(PropertiesEditorBase):
    """Provides a dialog box to edit a single label"""

    def __init__(self, canvas, target):
        """
        :param target: A reference to the label being edited
        """
        super(LabelEditor, self).__init__("labeleditor.ui", canvas)
        self.ui.errors.hide()

        self.target = target
        self._memento = LabelEditorModel(target.get_text())
        self.ui.editor.setText(self._memento.label_text)

    def changes_accepted(self):
        self.ui.errors.hide()
        self.target.set_text(self.ui.editor.text())

    def error_occurred(self, exc):
        """
        Display errors to user and reset state
        :param exc: The exception that occurred
        """
        self.target.set_text(self._memento.label_text)
        self.ui.errors.setText(str(exc).strip())
        self.ui.errors.show()


class LegendEditorModel(object):
    def __init__(self, label_text):
        self.label_text = label_text


class LegendEditor(PropertiesEditorBase):
    """Provides a dialog box to edit a legend label"""

    def __init__(self, canvas, target, target_curve):
        """
        :param target: A reference to the label being edited
        :param target_curve: A reference to the curve whose legend is being edited
        """
        super().__init__("labeleditor.ui", canvas)
        self.ui.errors.hide()

        self.target = target
        self.target_curve = target_curve
        self._memento = LegendEditorModel(target.get_text())
        self.ui.editor.setText(self._memento.label_text)

    def changes_accepted(self):
        self.ui.errors.hide()
        self.target.set_text(self.ui.editor.text())
        self.target_curve.set_label(self.ui.editor.text())

    def error_occurred(self, exc):
        """
        Display errors to user and reset state
        :param exc: The exception that occurred
        """
        self.target.set_text(self._memento.label_text)
        self.ui.errors.setText(str(exc).strip())
        self.ui.errors.show()


class AxisEditorModel(object):
    min = None
    max = None
    log = None
    grid = None
    formatter = None


class AxisEditor(PropertiesEditorBase):
    def __init__(self, canvas, axes, axis_id):
        """

        :param canvas: A reference to the target canvas
        :param axes: The axes object holding the properties to be edited
        :param axis_id: A string ID for the axis
        """
        super(AxisEditor, self).__init__("axiseditor.ui", canvas)
        # suppress errors
        self.ui.errors.hide()
        # Ensure that only floats can be entered
        self.ui.editor_min.setValidator(QDoubleValidator())
        self.ui.editor_max.setValidator(QDoubleValidator())
        if figure_type(canvas.figure) in [FigureType.Surface, FigureType.Wireframe, FigureType.Mesh]:
            self.ui.logBox.hide()
            self.ui.gridBox.hide()
        self.ui.editor_format.addItem(DECIMAL_FORMAT)
        self.ui.editor_format.addItem(SCIENTIFIC_FORMAT)
        self.axes = axes
        self.axis_id = axis_id
        self.lim_getter = getattr(axes, "get_{}lim".format(axis_id))

        if isinstance(axes, Axes3D):
            self.lim_setter = getattr(axes, "set_{}lim3d".format(axis_id))
        else:
            self.lim_setter = getattr(axes, "set_{}lim".format(axis_id))

        self.scale_setter = getattr(axes, "set_{}scale".format(axis_id))
        self.nonposkw = "nonpositive"

        # Store the axis for attributes that can't be directly accessed
        # from axes object (e.g. grid and tick parameters).
        self.axis = getattr(axes, "{}axis".format(axis_id))

    def create_model(self):
        memento = AxisEditorModel()
        self._memento = memento
        memento.min, memento.max = getattr(self.axes, "get_{}lim".format(self.axis_id))()
        memento.log = getattr(self.axes, "get_{}scale".format(self.axis_id))() != "linear"
        memento.grid = self.axis.grid_on() if hasattr(self.axis, "grid_on") else self.axis._major_tick_kw.get("gridOn", False)
        if type(self.axis.get_major_formatter()) is ScalarFormatter:
            memento.formatter = DECIMAL_FORMAT
        elif type(self.axis.get_major_formatter()) is LogFormatterSciNotation:
            memento.formatter = SCIENTIFIC_FORMAT
        self._fill(memento)

    def changes_accepted(self):
        self.ui.errors.hide()
        # apply properties
        axes = self.axes

        self.limit_min, self.limit_max = float(self.ui.editor_min.text()), float(self.ui.editor_max.text())
        if self.ui.logBox.isChecked():
            self.scale_setter("log", **{self.nonposkw: TREAT_LOG_NEGATIVE_VALUES})
            self.limit_min, self.limit_max = self._check_log_limits(self.limit_min, self.limit_max)
        else:
            self.scale_setter("linear")
        self.lim_setter(self.limit_min, self.limit_max)
        self._set_tick_format()
        which = "both" if hasattr(axes, "show_minor_gridlines") and axes.show_minor_gridlines else "major"
        axes.grid(visible=self.ui.gridBox.isChecked(), axis=self.axis_id, which=which)

    def error_occurred(self, exc):
        # revert
        self._fill(self._memento)
        # show error
        self.ui.errors.setText(str(exc).strip())
        self.ui.errors.show()

    def _fill(self, model):
        self.ui.editor_min.setText(str(model.min))
        self.ui.editor_max.setText(str(model.max))
        self.ui.logBox.setChecked(model.log)
        self.ui.gridBox.setChecked(model.grid)
        self.ui.editor_format.setCurrentText(model.formatter)

    def _check_log_limits(self, editor_min, editor_max):
        # Check that the limits from the editor are sensible for a log graph
        # These limits are not necessarily in numeric order we have to check both
        lim_min, lim_max = self.lim_getter()
        if editor_min <= 0:
            editor_min = lim_min
        if editor_max <= 0:
            editor_max = lim_max
        return editor_min, editor_max

    def _set_tick_format(self):
        formatter = self.ui.editor_format.currentText()
        if formatter == DECIMAL_FORMAT:
            fmt = ScalarFormatter(useOffset=True)
        elif formatter == SCIENTIFIC_FORMAT:
            fmt = LogFormatterSciNotation()
        getattr(self.axes, "{}axis".format(self.axis_id)).set_major_formatter(fmt)
        return


class XAxisEditor(AxisEditor):
    def __init__(self, canvas, axes):
        super(XAxisEditor, self).__init__(canvas, axes, "x")
        self.create_model()


class YAxisEditor(AxisEditor):
    def __init__(self, canvas, axes):
        super(YAxisEditor, self).__init__(canvas, axes, "y")
        self.create_model()


class ZAxisEditor(AxisEditor):
    def __init__(self, canvas, axes):
        super(ZAxisEditor, self).__init__(canvas, axes, "z")
        self.create_model()


class ColorbarAxisEditor(AxisEditor):
    def __init__(self, canvas, axes):
        super(ColorbarAxisEditor, self).__init__(canvas, axes, "y")

        self.ui.gridBox.hide()

        self.images = []

        images = get_images_from_figure(canvas.figure)
        # If there are an equal number of plots and colorbars so apply changes to plot with the selected colorbar
        # Otherwise apply changes to all the plots in the figure
        if len(images) != len(self.canvas.figure.axes) / 2:
            self.images = images
        else:
            # apply changes to selected axes
            for img in images:
                if img.colorbar and img.colorbar.ax == axes:
                    self.images.append(img)

        self.create_model()
        self.ui.editor_format.setEnabled(False)

    def changes_accepted(self):
        self.ui.errors.hide()

        if len(self.images) == 0:
            raise RuntimeError("Cannot find any plot linked to this colorbar")

        limit_min, limit_max = float(self.ui.editor_min.text()), float(self.ui.editor_max.text())

        scale = LogNorm if self.ui.logBox.isChecked() else Normalize

        if scale == LogNorm and (limit_min <= 0 or limit_max <= 0):
            raise ValueError("Limits must be positive\nwhen scale is logarithmic.")

        self.lim_setter(limit_min, limit_max)
        for img in self.images:
            update_colorbar_scale(self.canvas.figure, img, scale, limit_min, limit_max)

    def create_model(self):
        memento = AxisEditorModel()
        self._memento = memento
        if len(self.images) > 0:
            memento.min, memento.max = self.images[0].get_clim()
            memento.log = isinstance(self.images[0].norm, LogNorm)
        else:
            memento.log = False
        memento.grid = False

        self._fill(memento)


class MarkerEditor(QWidget):
    def __init__(self, filename, valid_style, valid_colors, used_names=None):
        """
        Widget to edit a marker properties
        :param filename: name of the ui file for this widget
        :param valid_style: list of valid line styles (eg. 'solid', 'dashed'...) used by matplotlib
        :param valid_colors: dictionary of valid colours
            keys = name of the colour
            value = corresponding matplotlib name (eg. {'red': 'C4'})
        """
        super(MarkerEditor, self).__init__()
        self.widget = load_ui(__file__, filename, baseinstance=self)
        self.widget.position.setValidator(QDoubleValidator())
        self.widget.label_x_pos.setValidator(QDoubleValidator())
        self.widget.label_y_pos.setValidator(QDoubleValidator())
        self.colors = valid_colors
        if used_names is None:
            self.used_names = []
        else:
            self.used_names = used_names

        self.widget.style.addItems(valid_style)
        self.widget.color.addItems(list(valid_colors.keys()))

    def set_defaults(self, marker):
        """
        Set the values of all fields to the ones of the marker
        """
        _color = [name for name, symbol in self.colors.items() if symbol == marker.color][0]
        self.widget.name.setText(str(marker.name))
        self.widget.position.setText(str(marker.get_position()))
        self.widget.style.setCurrentText(str(marker.style))
        self.widget.color.setCurrentText(_color)
        self.widget.display_label.setChecked(marker.label_visible)
        self.widget.label_x_pos.setText(str(marker.label_x_offset))
        self.widget.label_y_pos.setText(str(marker.label_y_offset))
        self.fixed_marker.setChecked(not marker.draggable)

    def update_marker(self, marker):
        """
        Update the properties of the marker with the values from the widget
        """
        old_name = str(marker.name)
        new_name = self.widget.name.text()
        if new_name == "":
            raise RuntimeError("Marker names cannot be empty")
        if new_name in self.used_names and new_name != old_name:
            raise RuntimeError("Marker names cannot be duplicated.\n Another marker is named '{}'".format(new_name))
        try:
            marker.set_name(new_name)
        except:
            marker.set_name(old_name)
            raise RuntimeError("Invalid label '{}'".format(new_name))

        marker.set_position(float(self.widget.position.text()))
        marker.draggable = not self.widget.fixed_marker.isChecked()
        marker.set_style(self.widget.style.currentText())
        marker.set_color(self.colors.get(self.widget.color.currentText(), "C2"))
        marker.set_label_visible(self.widget.display_label.isChecked())

        x_pos = float(self.widget.label_x_pos.text())
        y_pos = float(self.widget.label_y_pos.text())
        marker.set_label_position(x_pos, y_pos)


class SingleMarkerEditor(PropertiesEditorBase):
    def __init__(self, canvas, marker, valid_style, valid_colors, used_names):
        """
        Edit the properties of a single marker.
        :param canvas: A reference to the target canvas
        :param marker: The marker to be edited
        :param valid_style: list of valid line styles (eg. 'solid', 'dashed'...) used by matplotlib
        :param valid_colors: dictionary of valid colours
        """
        super(SingleMarkerEditor, self).__init__("singlemarkereditor.ui", canvas)
        self.ui.errors.hide()

        self._widget = MarkerEditor("markeredit.ui", valid_style, valid_colors, used_names)
        layout = self.ui.layout()
        layout.addWidget(self._widget, 1, 0)

        self.marker = marker
        self._widget.set_defaults(self.marker)

    def changes_accepted(self):
        """
        Update the marker properties
        """
        self.ui.errors.hide()
        self._widget.update_marker(self.marker)

    def error_occurred(self, exc):
        self.ui.errors.setText(str(exc).strip())
        self.ui.errors.show()


class GlobalMarkerEditor(PropertiesEditorBase):
    def __init__(self, canvas, markers, valid_style, valid_colors):
        """
        Edit the properties of a marker, this can be chosen from a list of valid markers.
        :param canvas: A reference to the target canvas
        :param markers: List of markers that can be edited
        :param valid_style: list of valid line styles (eg. 'solid', 'dashed'...) used by matplotlib
        :param valid_colors: dictionary of valid colours
        """
        super(GlobalMarkerEditor, self).__init__("globalmarkereditor.ui", canvas)
        self.ui.errors.hide()
        self.ui.marker.currentIndexChanged.connect(self.update_marker_data)

        self.markers = sorted(markers, key=lambda _marker: _marker.name)
        self._names = [str(_marker.name) for _marker in self.markers]

        self._widget = MarkerEditor("markeredit.ui", valid_style, valid_colors, self._names)
        layout = self.ui.layout()
        layout.addWidget(self._widget, 2, 0, 1, 2)

        if self._names:
            self.ui.marker.addItems(self._names)
        else:
            self._widget.setEnabled(False)

    def changes_accepted(self):
        """Update the properties of the currently selected marker"""
        self.ui.errors.hide()
        idx = self.ui.marker.currentIndex()
        self._widget.update_marker(self.markers[idx])

    def error_occurred(self, exc):
        self.ui.errors.setText(str(exc).strip())
        self.ui.errors.show()

    def update_marker_data(self, idx):
        """When changing the selected marker update the properties displayed in the editor window"""
        if self.ui.marker.count == 0:
            self._widget.setEnabled(False)
            return
        self._widget.setEnabled(True)
        self._widget.set_defaults(self.markers[idx])
