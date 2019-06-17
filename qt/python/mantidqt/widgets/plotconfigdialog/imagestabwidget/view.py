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
from qtpy.QtWidgets import QWidget

from mantidqt.utils.qt import load_ui

CMAPS = ['viridis', 'viridis_r', 'plasma', 'plasma_r', 'inferno', 'inferno_r',
         'magma', 'magma_r', 'cividis', 'cividis_r', 'Greys', 'Greys_r',
         'Purples', 'Purples_r', 'Blues', 'Blues_r', 'Greens', 'Greens_r',
         'Oranges', 'Oranges_r', 'Reds', 'Reds_r', 'YlOrBr', 'YlOrBr_r',
         'YlOrRd', 'YlOrRd_r', 'OrRd', 'OrRd_r', 'PuRd', 'PuRd_r', 'RdPu',
         'RdPu_r', 'BuPu', 'BuPu_r', 'GnBu', 'GnBu_r', 'PuBu', 'PuBu_r',
         'YlGnBu', 'YlGnBu_r', 'PuBuGn', 'PuBuGn_r', 'BuGn', 'BuGn_r', 'YlGn',
         'YlGn_r', 'binary', 'binary_r', 'gist_yarg', 'gist_yarg_r',
         'gist_gray', 'gist_gray_r', 'gray', 'gray_r', 'bone', 'bone_r',
         'pink', 'pink_r', 'spring', 'spring_r', 'summer', 'summer_r',
         'autumn', 'autumn_r', 'winter', 'winter_r', 'cool', 'cool_r',
         'Wistia', 'Wistia_r', 'hot', 'hot_r', 'afmhot', 'afmhot_r',
         'gist_heat', 'gist_heat_r', 'copper', 'copper_r', 'PiYG', 'PiYG_r',
         'PRGn', 'PRGn_r', 'BrBG', 'BrBG_r', 'PuOr', 'PuOr_r', 'RdGy',
         'RdGy_r', 'RdBu', 'RdBu_r', 'RdYlBu', 'RdYlBu_r', 'RdYlGn',
         'RdYlGn_r', 'Spectral', 'Spectral_r', 'coolwarm', 'coolwarm_r', 'bwr',
         'bwr_r', 'seismic', 'seismic_r', 'twilight', 'twilight_r',
         'twilight_shifted', 'twilight_shifted_r', 'hsv', 'hsv_r', 'Pastel1',
         'Pastel1_r', 'Pastel2', 'Pastel2_r', 'Paired', 'Paired_r', 'Accent',
         'Accent_r', 'Dark2', 'Dark2_r', 'Set1', 'Set1_r', 'Set2', 'Set2_r',
         'Set3', 'Set3_r', 'tab10', 'tab10_r', 'tab20', 'tab20_r', 'tab20b',
         'tab20b_r', 'tab20c', 'tab20c_r', 'flag', 'flag_r', 'prism',
         'prism_r', 'ocean', 'ocean_r', 'gist_earth', 'gist_earth_r',
         'terrain', 'terrain_r', 'gist_stern', 'gist_stern_r', 'gnuplot',
         'gnuplot_r', 'gnuplot2', 'gnuplot2_r', 'CMRmap', 'CMRmap_r',
         'cubehelix', 'cubehelix_r', 'brg', 'brg_r', 'gist_rainbow',
         'gist_rainbow_r', 'rainbow', 'rainbow_r', 'jet', 'jet_r',
         'nipy_spectral', 'nipy_spectral_r', 'gist_ncar', 'gist_ncar_r']

INTERPOLATIONS = ['None', 'Nearest', 'Bilinear', 'Bicubic', 'Spline16',
                  'Spline36', 'Hanning', 'Hamming', 'Hermite', 'Kaiser',
                  'Quadric','Catrom', 'Gaussian', 'Bessel', 'Mitchell', 'Sinc',
                  'Lanczos']

SCALES = {'Linear': Normalize,
          'Logarithmic': LogNorm}


class ImagesTabWidgetView(QWidget):

    def __init__(self, parent=None):
        super(ImagesTabWidgetView, self).__init__(parent=parent)

        self.ui = load_ui(__file__,
                          'images_tab.ui',
                          baseinstance=self)
        self._populate_colormap_combo_box()
        self._populate_interpolation_combo_box()
        self._populate_scale_combo_box()

        # Set maximum and minimum for the min/max spin boxes
        for bound in ['min', 'max']:
            spin_box = getattr(self, '%s_value_spin_box' % bound)
            spin_box.setRange(0, np.finfo(np.float32).max)

        # Make sure min scale value always less than max
        self.min_value_spin_box.valueChanged.connect(
            self._check_max_min_consistency_min_changed)
        self.max_value_spin_box.valueChanged.connect(
            self._check_max_min_consistency_max_changed)

    def _check_max_min_consistency_min_changed(self):
        """Check min value smaller than max value after min_value changed"""
        if self.get_min_value() >= self.get_max_value():
            self.set_max_value(self.get_min_value() + 0.01)

    def _check_max_min_consistency_max_changed(self):
        """Check min value smaller than max value after max value changed"""
        if self.get_min_value() >= self.get_max_value():
            self.set_min_value(self.get_max_value() - 0.01)

    def _populate_colormap_combo_box(self):
        self.colormap_combo_box.addItems(CMAPS)

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

    def get_min_value(self):
        return self.min_value_spin_box.value()

    def set_min_value(self, value):
        self.min_value_spin_box.setValue(value)

    def get_max_value(self,):
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
