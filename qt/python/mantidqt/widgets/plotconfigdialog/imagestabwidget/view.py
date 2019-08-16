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
from qtpy.QtCore import Qt
from qtpy.QtGui import QPixmap, QIcon, QImage
from qtpy.QtWidgets import QWidget

from mantidqt.utils.qt import load_ui
from mantidqt.widgets.plotconfigdialog.imagestabwidget import ImageProperties

CMAPS = [
    'viridis', 'plasma', 'inferno', 'magma', 'cividis', 'Greys', 'Purples', 'Blues', 'Greens',
    'Oranges', 'Reds', 'YlOrBr', 'YlOrRd', 'OrRd', 'PuRd', 'RdPu', 'BuPu', 'GnBu', 'PuBu', 'YlGnBu',
    'PuBuGn', 'BuGn', 'YlGn', 'binary', 'gist_yarg', 'gist_gray', 'gray', 'bone', 'pink', 'spring',
    'summer', 'autumn', 'winter', 'cool', 'Wistia', 'hot', 'afmhot', 'gist_heat', 'copper', 'PiYG',
    'PRGn', 'BrBG', 'PuOr', 'RdGy', 'RdBu', 'RdYlBu', 'RdYlGn', 'Spectral', 'coolwarm', 'bwr',
    'seismic', 'hsv', 'Pastel1', 'Pastel2', 'Paired', 'Accent', 'Dark2', 'Set1', 'Set2', 'Set3',
    'tab10', 'tab20', 'tab20b', 'tab20c', 'flag', 'prism', 'ocean', 'gist_earth', 'terrain',
    'gist_stern', 'gnuplot', 'gnuplot2', 'CMRmap', 'cubehelix', 'brg', 'gist_rainbow', 'rainbow',
    'jet', 'nipy_spectral', 'gist_ncar'
]

INTERPOLATIONS = [
    'None', 'Nearest', 'Bilinear', 'Bicubic', 'Spline16', 'Spline36', 'Hanning', 'Hamming',
    'Hermite', 'Kaiser', 'Quadric', 'Catrom', 'Gaussian', 'Bessel', 'Mitchell', 'Sinc', 'Lanczos'
]

SCALES = {'Linear': Normalize, 'Logarithmic': LogNorm}


def create_colormap_img(cmap_name, save=False):
    import matplotlib.pyplot as plt
    try:
        cmap = plt.get_cmap(cmap_name)
    except ValueError:
        print("Colormap '{}' not found!".format(cmap_name))
        return

    row_colours = np.zeros((256, 3), dtype=np.uint8)
    for i in range(256):
        row_colours[i] = 255 * np.array(cmap(i * cmap.N / 255))[:3]

    img_array = np.tile(row_colours, (100, 1, 1))
    img = QImage(img_array.tobytes(), 256, 100, QImage.Format_RGB888)
    if save:
        img.save('D:\\ejo7321303\\Mantid\\Testing\\Colormaps\\' 'colormap-{}.png'.format(cmap_name))
    return img


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

        # Make sure min scale value always less than max
        self.min_value_spin_box.valueChanged.connect(self._check_max_min_consistency_min_changed)
        self.max_value_spin_box.valueChanged.connect(self._check_max_min_consistency_max_changed)

    def _check_max_min_consistency_min_changed(self):
        """Check min value smaller than max value after min_value changed"""
        if self.get_min_value() >= self.get_max_value():
            self.set_max_value(self.get_min_value() + 0.01)

    def _check_max_min_consistency_max_changed(self):
        """Check min value smaller than max value after max value changed"""
        if self.get_min_value() >= self.get_max_value():
            self.set_min_value(self.get_max_value() - 0.01)

    def _populate_colormap_combo_box(self):
        from time import time
        start = time()
        for cmap_name in CMAPS:
            img = create_colormap_img(cmap_name)
            if img:
                pixmap = QPixmap.fromImage(img)
                self.colormap_combo_box.addItem(QIcon(pixmap), cmap_name)
        print("Time taken to poulate colormap drop down: {}s" "".format(time() - start))

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
