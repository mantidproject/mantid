# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.
#
#
from __future__ import (absolute_import, unicode_literals)

# std imports

# 3rdparty imports
from mantid.kernel import logger
from mantidqt.plotting.figuretype import FigureType, figure_type
from mantidqt.utils.qt import load_ui
from matplotlib.collections import QuadMesh
from matplotlib.colors import LogNorm
from matplotlib.ticker import LogLocator
from numpy import arange
from qtpy.QtGui import QDoubleValidator, QIcon
from qtpy.QtWidgets import QDialog, QWidget

SYMLOG_LIN_THRESHOLD = 0.01


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
        self.ui.setWindowIcon(QIcon(':/images/MantidIcon.ico'))

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
        super(LabelEditor, self).__init__('labeleditor.ui', canvas)
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


class AxisEditorModel(object):

    min = None
    max = None
    log = None
    grid = None


class AxisEditor(PropertiesEditorBase):

    def __init__(self, canvas, axes, axis_id):
        """

        :param canvas: A reference to the target canvas
        :param axes: The axes object holding the properties to be edited
        :param axis_id: A string ID for the axis
        """
        super(AxisEditor, self).__init__('axiseditor.ui', canvas)
        # suppress errors
        self.ui.errors.hide()
        # Ensure that only floats can be entered
        self.ui.editor_min.setValidator(QDoubleValidator())
        self.ui.editor_max.setValidator(QDoubleValidator())
        if figure_type(canvas.figure) == FigureType.Image:
            self.ui.logBox.hide()
            self.ui.gridBox.hide()

        self.axes = axes
        self.axis_id = axis_id
        self.lim_setter = getattr(axes, 'set_{}lim'.format(axis_id))
        self.scale_setter = getattr(axes, 'set_{}scale'.format(axis_id))
        self.linthresholdkw = 'linthres' + axis_id
        # Grid has no direct accessor from the axes
        self.axis = axes.xaxis if axis_id == 'x' else axes.yaxis

    def create_model(self):
        memento = AxisEditorModel()
        self._memento = memento
        memento.min, memento.max = getattr(self.axes, 'get_{}lim'.format(self.axis_id))()
        memento.log = getattr(self.axes, 'get_{}scale'.format(self.axis_id))() != 'linear'
        memento.grid = self.axis.majorTicks[0].gridOn

        self._fill(memento)

    def changes_accepted(self):
        self.ui.errors.hide()
        # apply properties
        axes = self.axes

        self.limit_min, self.limit_max = float(self.ui.editor_min.text()), float(self.ui.editor_max.text())
        self.lim_setter(self.limit_min, self.limit_max)
        if self.ui.logBox.isChecked():
            self.scale_setter('symlog', **{self.linthresholdkw: SYMLOG_LIN_THRESHOLD})
        else:
            self.scale_setter('linear')
        axes.grid(self.ui.gridBox.isChecked(), axis=self.axis_id)

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


class XAxisEditor(AxisEditor):

    def __init__(self, canvas, axes):
        super(XAxisEditor, self).__init__(canvas, axes, 'x')
        self.create_model()


class YAxisEditor(AxisEditor):

    def __init__(self, canvas, axes):
        super(YAxisEditor, self).__init__(canvas, axes, 'y')
        self.create_model()


class ColorbarAxisEditor(AxisEditor):

    def __init__(self, canvas, axes):
        super(ColorbarAxisEditor, self).__init__(canvas, axes, 'y')

        self.images = self.canvas.figure.gca().images
        if len(self.images) == 0:
            self.images = [col for col in self.canvas.figure.gca().collections if isinstance(col, QuadMesh)]

        self.create_model()

    def changes_accepted(self):
        super(ColorbarAxisEditor, self).changes_accepted()
        cb = self.images[0]
        cb.set_clim(self.limit_min, self.limit_max)
        if isinstance(self.images[0].norm, LogNorm):
            locator = LogLocator(subs=arange(1, 10))
            if locator.tick_values(vmin=self.limit_min, vmax=self.limit_max).size == 0:
                locator = LogLocator()
                logger.warning("Minor ticks on colorbar scale cannot be shown "
                               "as the range between min value and max value is too large")
            cb.colorbar.locator = locator
            cb.colorbar.update_ticks()

    def create_model(self):
        memento = AxisEditorModel()
        self._memento = memento
        memento.min, memento.max = self.images[0].get_clim()
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
            raise RuntimeError("Marker names cannot be duplicated.\n Another marker is named '{}'"
                               .format(new_name))
        try:
            marker.set_name(new_name)
        except:
            marker.set_name(old_name)
            raise RuntimeError("Invalid label '{}'".format(new_name))

        marker.set_position(float(self.widget.position.text()))
        marker.draggable = not self.widget.fixed_marker.isChecked()
        marker.set_style(self.widget.style.currentText())
        marker.set_color(self.colors.get(self.widget.color.currentText(), 'C2'))
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
        super(SingleMarkerEditor, self).__init__('singlemarkereditor.ui', canvas)
        self.ui.errors.hide()

        self._widget = MarkerEditor('markeredit.ui', valid_style, valid_colors, used_names)
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
        super(GlobalMarkerEditor, self).__init__('globalmarkereditor.ui', canvas)
        self.ui.errors.hide()
        self.ui.marker.currentIndexChanged.connect(self.update_marker_data)

        self.markers = sorted(markers, key=lambda _marker: _marker.name)
        self._names = [str(_marker.name) for _marker in self.markers]

        self._widget = MarkerEditor('markeredit.ui', valid_style, valid_colors, self._names)
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
