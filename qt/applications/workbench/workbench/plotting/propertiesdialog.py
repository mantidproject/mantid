#  This file is part of the mantid workbench.
#
#  Copyright (C) 2017 mantidproject
#
#  This program is free software: you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation, either version 3 of the License, or
#  (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program.  If not, see <http://www.gnu.org/licenses/>.
from __future__ import (absolute_import, unicode_literals)

# std imports

# 3rdparty imports
from mantidqt.utils.qt import load_ui
from qtpy.QtGui import QDoubleValidator
from qtpy.QtWidgets import QDialog


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

        self.axes = axes
        self.axis_id = axis_id
        self.lim_setter = getattr(axes, 'set_{}lim'.format(axis_id))
        self.scale_setter = getattr(axes, 'set_{}scale'.format(axis_id))
        self.linthresholdkw = 'linthres' + axis_id
        # Grid has no direct accessor from the axes
        axis = axes.xaxis if axis_id == 'x' else axes.yaxis

        memento = AxisEditorModel()
        self._momento = memento
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
