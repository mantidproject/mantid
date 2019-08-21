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
from mantidqt.plotting.figuretype import FigureType, figure_type
from mantidqt.utils.qt import load_ui
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
        axis = axes.xaxis if axis_id == 'x' else axes.yaxis

        memento = AxisEditorModel()
        self._memento = memento
        memento.min, memento.max = getattr(axes, 'get_{}lim'.format(axis_id))()
        memento.log = getattr(axes, 'get_{}scale'.format(axis_id))() != 'linear'
        memento.grid = axis.majorTicks[0].gridOn

        self._fill(memento)

    def changes_accepted(self):
        self.ui.errors.hide()
        # apply properties
        axes = self.axes

        limit_min, limit_max = float(self.ui.editor_min.text()), float(self.ui.editor_max.text())
        self.lim_setter(limit_min, limit_max)
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


class YAxisEditor(AxisEditor):

    def __init__(self, canvas, axes):
        super(YAxisEditor, self).__init__(canvas, axes, 'y')


class MarkerEditor(QWidget):
    def __init__(self, filename, valid_style, valid_colors):
        super(MarkerEditor, self).__init__()
        self.widget = load_ui(__file__, filename, baseinstance=self)
        self.widget.position.setValidator(QDoubleValidator())
        self.colors = valid_colors

        self.widget.style.addItems(valid_style)
        self.widget.color.addItems(list(valid_colors.keys()))

    def set_defaults(self, marker):
        _color = [name for name, symbol in self.colors.items() if symbol == marker.color][0]
        self.widget.name.setText(str(marker.name))
        self.widget.position.setText(str(marker.get_position()))
        self.widget.style.setCurrentText(str(marker.style))
        self.widget.color.setCurrentText(_color)

    def update_marker(self, marker):
        old_name = str(marker.name)
        try:
            marker.set_name(self.widget.name.text())
        except:
            marker.set_name(old_name)
            raise RuntimeError("Invalid label '{}'".format(self.widget.name.text()))

        marker.set_position(float(self.widget.position.text()))
        marker.set_style(self.widget.style.currentText())
        marker.set_color(self.colors.get(self.widget.color.currentText(), 'C2'))


class SingleMarkerEditor(PropertiesEditorBase):
    def __init__(self, canvas, marker, valid_style, valid_colors):
        super(SingleMarkerEditor, self).__init__('singlemarkereditor.ui', canvas)
        self.ui.errors.hide()

        self._widget = MarkerEditor('markeredit.ui', valid_style, valid_colors)
        layout = self.ui.layout()
        layout.replaceWidget(self.ui.placeHolder, self._widget)
        self.ui.placeHolder.setParent(None)

        self.marker = marker
        self._widget.set_defaults(self.marker)

    def changes_accepted(self):
        self.ui.errors.hide()
        self._widget.update_marker(self.marker)

    def error_occurred(self, exc):
        self.ui.errors.setText(str(exc).strip())
        self.ui.errors.show()


class GlobalMarkerEditor(PropertiesEditorBase):
    def __init__(self, canvas, markers, valid_style, valid_colors):
        super(GlobalMarkerEditor, self).__init__('globalmarkereditor.ui', canvas)
        self.ui.errors.hide()
        self.ui.marker.currentIndexChanged.connect(self.update_marker_data)

        self._widget = MarkerEditor('markeredit.ui', valid_style, valid_colors)
        layout = self.ui.layout()
        layout.replaceWidget(self.ui.placeHolder, self._widget)
        self.ui.placeHolder.setParent(None)

        self.markers = sorted(markers, key=lambda x: x.name)
        self._names = [str(_marker.name) for _marker in self.markers]
        self.ui.marker.addItems(self._names)

    def changes_accepted(self):
        self.ui.errors.hide()
        idx = self.ui.marker.currentIndex()
        self._widget.update_marker(self.markers[idx])

    def error_occurred(self, exc):
        self.ui.errors.setText(str(exc).strip())
        self.ui.errors.show()

    def update_marker_data(self, idx):
        self._widget.set_defaults(self.markers[idx])
