# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.

from __future__ import (absolute_import, unicode_literals)

import numpy as np
from matplotlib.colors import LogNorm, Normalize
from matplotlib import cm
from qtpy.QtCore import Qt
from qtpy.QtGui import QPixmap, QIcon, QImage
from qtpy.QtWidgets import QWidget

from mantid.plots.utility import get_colormap_names
from mantidqt.utils.qt import load_ui
from mantidqt.widgets.plotconfigdialog.imagestabwidget import ImageProperties

INTERPOLATIONS = [
    'None', 'Nearest', 'Bilinear', 'Bicubic', 'Spline16', 'Spline36', 'Hanning', 'Hamming',
    'Hermite', 'Kaiser', 'Quadric', 'Catrom', 'Gaussian', 'Bessel', 'Mitchell', 'Sinc', 'Lanczos'
]
SCALES = {'Linear': Normalize, 'Logarithmic': LogNorm}


def create_colormap_img(cmap_name, width=50, height=20):
    colormap = cm.get_cmap(cmap_name)
    gradient_array = np.tile(np.linspace(0, 1, width), height)
    img_array = (colormap(gradient_array)*255).astype(np.uint8)
    return QImage(img_array, width, height, QImage.Format_RGBA8888_Premultiplied)


class ImagesTabWidgetView(QWidget):
    def __init__(self, parent=None):
        super(ImagesTabWidgetView, self).__init__(parent=parent)

        self.ui = load_ui(__file__, 'images_tab.ui', baseinstance=self)
        self._populate_colormap_combo_box()
        self._populate_interpolation_combo_box()
        self._populate_scale_combo_box()

        # Set maximum and minimum for the min/max spin boxes
        for bound in ['min', 'max']:
            spin_box = getattr(self, '%s_value_spin_box' % bound)
            spin_box.setRange(0, np.finfo(np.float32).max)

    def _populate_colormap_combo_box(self):
        for cmap_name in get_colormap_names():
            qt_img = create_colormap_img(cmap_name)
            pixmap = QPixmap.fromImage(qt_img)
            self.colormap_combo_box.addItem(QIcon(pixmap), cmap_name)

    def _populate_interpolation_combo_box(self):
        self.interpolation_combo_box.addItems(INTERPOLATIONS)

    def _populate_scale_combo_box(self):
        self.scale_combo_box.addItems(sorted(SCALES.keys()))

    def enable_interpolation(self, enable):
        self.interpolation_combo_box.setEnabled(enable)

    def interpolation_enabled(self):
        return self.interpolation_combo_box.isEnabled()

    def populate_select_image_combo_box(self, image_names):
        self.select_image_combo_box.addItems(image_names)

    def get_properties(self):
        return ImageProperties.from_view(self)

    def get_selected_image_name(self):
        return self.select_image_combo_box.currentText()

    def get_label(self):
        return self.label_line_edit.text()

    def set_label(self, label):
        self.label_line_edit.setText(label)

    def get_colormap(self):
        return self.colormap_combo_box.currentText()

    def set_colormap(self, colormap):
        self.colormap_combo_box.setCurrentText(colormap)

    def get_reverse_colormap(self):
        return self.reverse_colormap_check_box.checkState() == Qt.Checked

    def set_reverse_colormap(self, checked):
        qt_checked = Qt.Checked if checked else Qt.Unchecked
        self.reverse_colormap_check_box.setCheckState(qt_checked)

    def get_min_value(self):
        return self.min_value_spin_box.value()

    def set_min_value(self, value):
        self.min_value_spin_box.setValue(value)

    def get_max_value(self):
        return self.max_value_spin_box.value()

    def set_max_value(self, value):
        self.max_value_spin_box.setValue(value)

    def get_interpolation(self):
        return self.interpolation_combo_box.currentText()

    def set_interpolation(self, interpolation):
        self.interpolation_combo_box.setCurrentText(interpolation)

    def get_scale(self):
        return self.scale_combo_box.currentText()

    def set_scale(self, scale):
        self.scale_combo_box.setCurrentText(scale)

    def replace_selected_image_name(self, name):
        current_index = self.select_image_combo_box.currentIndex()
        self.select_image_combo_box.setItemText(current_index, name)
