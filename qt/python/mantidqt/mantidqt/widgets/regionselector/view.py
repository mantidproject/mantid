# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.
from qtpy.QtCore import Qt
from qtpy.QtGui import QCursor
from qtpy.QtWidgets import QApplication, QHBoxLayout, QWidget

from mantidqt.widgets.sliceviewer.views.dataview import SliceViewerDataView


class RegionSelectorView(QWidget):
    """The view for the data portion of the sliceviewer"""

    def __init__(self, presenter, parent=None, dims_info=None, image_info_widget=None, add_extents=False):
        super().__init__(parent)
        self.setWindowFlags(Qt.Window)
        self._data_view = SliceViewerDataView(presenter, dims_info, None, self, None, image_info_widget, add_extents)
        self._data_view.help_button.hide()

        layout = QHBoxLayout(self)
        layout.setContentsMargins(0, 0, 0, 0)
        layout.addWidget(self._data_view)
        self.setLayout(layout)

        self.setWindowTitle("Region Selector")

    @property
    def data_view(self):
        return self._data_view

    def set_workspace(self, workspace):
        self._data_view.image_info_widget.setWorkspace(workspace)

    def create_dimensions(self, dims_info):
        self._data_view.create_dimensions(dims_info=dims_info, custom_image_info=True)

    def create_axes_orthogonal(self, redraw_on_zoom):
        self._data_view.create_axes_orthogonal(redraw_on_zoom=redraw_on_zoom)

    def clear_figure(self):
        self._data_view.clear_figure()
        self._data_view.canvas.draw()

    @staticmethod
    def set_override_cursor(override: bool, override_cursor: Qt.CursorShape = Qt.SizeAllCursor) -> None:
        """
        Sets the override cursor if an override cursor doesn't exist. Otherwise, restores the override cursor
        :param override: A boolean for whether to set the override cursor or not
        :param override_cursor: The override cursor to use if the override parameter is true
        """
        cursor = QApplication.overrideCursor()
        if override and cursor is None:
            QApplication.setOverrideCursor(QCursor(override_cursor))
        elif not override and cursor is not None:
            QApplication.restoreOverrideCursor()
