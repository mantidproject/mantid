# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.
#
#
from __future__ import (absolute_import, division, print_function)
from qtpy.QtWidgets import QWidget, QVBoxLayout, QHBoxLayout, QLineEdit, QComboBox, QCheckBox, QLabel
from qtpy.QtCore import Signal
from qtpy.QtGui import QDoubleValidator
from matplotlib.colorbar import Colorbar
from matplotlib.figure import Figure
from mantidqt.MPLwidgets import FigureCanvas
from matplotlib.colors import Normalize, SymLogNorm, PowerNorm
from matplotlib import cm
import numpy as np


NORM_OPTS = ["Linear", "SymmetricLog10", "Power"]


class ColorbarWidget(QWidget):
    colorbarChanged = Signal() # The parent should simply redraw their canvas

    def __init__(self, parent=None):
        super(ColorbarWidget, self).__init__(parent)

        self.setWindowTitle("Colorbar")
        self.setMaximumWidth(200)

        self.cmap = QComboBox()
        self.cmap.addItems(cm.cmap_d.keys())
        self.cmap.currentTextChanged.connect(self.cmap_changed)

        self.cmin = QLineEdit()
        self.cmin_value = 0
        self.cmin.setMaximumWidth(100)
        self.cmin.editingFinished.connect(self.clim_changed)
        self.cmin_layout = QHBoxLayout()
        self.cmin_layout.addStretch()
        self.cmin_layout.addWidget(self.cmin)
        self.cmin_layout.addStretch()

        self.cmax = QLineEdit()
        self.cmax_value = 1
        self.cmax.setMaximumWidth(100)
        self.cmax.editingFinished.connect(self.clim_changed)
        self.cmin.setValidator(QDoubleValidator())
        self.cmax.setValidator(QDoubleValidator())
        self.cmax_layout = QHBoxLayout()
        self.cmax_layout.addStretch()
        self.cmax_layout.addWidget(self.cmax)
        self.cmax_layout.addStretch()

        self.norm_layout = QHBoxLayout()
        self.norm = QComboBox()
        self.norm.addItems(NORM_OPTS)
        self.norm.currentIndexChanged.connect(self.norm_changed)

        self.powerscale = QLineEdit()
        self.powerscale_value = 2
        self.powerscale.setText("2")
        self.powerscale.setValidator(QDoubleValidator(0.001,100,3))
        self.powerscale.setMaximumWidth(50)
        self.powerscale.editingFinished.connect(self.norm_changed)
        self.powerscale.hide()
        self.powerscale_label = QLabel("n=")
        self.powerscale_label.hide()

        self.norm_layout.addStretch()
        self.norm_layout.addWidget(self.norm)
        self.norm_layout.addStretch()
        self.norm_layout.addWidget(self.powerscale_label)
        self.norm_layout.addWidget(self.powerscale)

        self.autoscale = QCheckBox("Autoscaling")
        self.autoscale.setChecked(True)
        self.autoscale.stateChanged.connect(self.update_clim)

        self.canvas = FigureCanvas(Figure())
        if parent:
            # Set facecolor to match parent
            self.canvas.figure.set_facecolor(parent.palette().window().color().getRgbF())
        self.ax = self.canvas.figure.add_axes([0.4,0.05,0.2,0.9])

        # layout
        self.layout = QVBoxLayout(self)
        self.layout.addWidget(self.cmap)
        self.layout.addLayout(self.cmax_layout)
        self.layout.addWidget(self.canvas, stretch=1)
        self.layout.addLayout(self.cmin_layout)
        self.layout.addLayout(self.norm_layout)
        self.layout.addWidget(self.autoscale)

    def set_mappable(self, mappable):
        """
        When a new plot is created this method should be called with the new mappable
        """
        self.ax.clear()
        try: # Use current cmap
            cmap = self.colorbar.get_cmap()
        except AttributeError:
            try: # else use viridis
                cmap = cm.viridis
            except AttributeError: # else default
                cmap = None
        self.colorbar = Colorbar(ax=self.ax, mappable=mappable)
        self.cmin_value, self.cmax_value = self.colorbar.get_clim()
        self.update_clim_text()
        self.cmap_changed(cmap)
        self.cmap.setCurrentText(self.colorbar.get_cmap().name)
        self.redraw()

    def cmap_changed(self, name):
        self.colorbar.set_cmap(name)
        self.colorbar.mappable.set_cmap(name)
        self.redraw()

    def norm_changed(self):
        """
        Called when a different normalization is selected
        """
        idx = self.norm.currentIndex()
        if NORM_OPTS[idx] == 'Power':
            self.powerscale.show()
            self.powerscale_label.show()
        else:
            self.powerscale.hide()
            self.powerscale_label.hide()
        self.colorbar.mappable.set_norm(self.get_norm())
        self.set_mappable(self.colorbar.mappable)

    def get_norm(self):
        """
        This will create a matplotlib.colors.Normalize from selected idx, limits and powerscale
        """
        idx = self.norm.currentIndex()
        if self.autoscale.isChecked():
            cmin = cmax = None
        else:
            cmin = self.cmin_value
            cmax = self.cmax_value
        if NORM_OPTS[idx] == 'Power':
            if self.powerscale.hasAcceptableInput():
                self.powerscale_value = float(self.powerscale.text())
            return PowerNorm(gamma=self.powerscale_value, vmin=cmin, vmax=cmax)
        elif NORM_OPTS[idx] == "SymmetricLog10":
            return SymLogNorm(1e-8 if cmin is None else max(1e-8, abs(cmin)*1e-3),
                              vmin=cmin, vmax=cmax)
        else:
            return Normalize(vmin=cmin, vmax=cmax)

    def clim_changed(self):
        """
        Called when either the min or max is changed. Will unset the autoscale.
        """
        self.autoscale.blockSignals(True)
        self.autoscale.setChecked(False)
        self.autoscale.blockSignals(False)
        self.update_clim()

    def update_clim(self):
        """
        This will update the clim of the plot based on min, max, and autoscale
        """
        if self.autoscale.isChecked():
            data = self.colorbar.mappable.get_array()
            try:
                try:
                    self.cmin_value = data[~data.mask].min()
                    self.cmax_value = data[~data.mask].max()
                except AttributeError:
                    self.cmin_value = np.nanmin(data)
                    self.cmax_value = np.nanmax(data)
            except (ValueError, RuntimeWarning):
                # all values mask
                pass
            self.update_clim_text()
        else:
            if self.cmin.hasAcceptableInput():
                cmin = float(self.cmin.text())
                if cmin < self.cmax_value:
                    self.cmin_value = cmin
                else: # reset values back
                    self.update_clim_text()
            if self.cmax.hasAcceptableInput():
                cmax = float(self.cmax.text())
                if cmax > self.cmin_value:
                    self.cmax_value = cmax
                else: #reset values back
                    self.update_clim_text()
        self.colorbar.set_clim(self.cmin_value, self.cmax_value)
        self.redraw()

    def update_clim_text(self):
        """
        Update displayed limit values based on stored ones
        """
        self.cmin.setText("{:.4}".format(self.cmin_value))
        self.cmax.setText("{:.4}".format(self.cmax_value))

    def redraw(self):
        """
        Redraws the colobar and emits signal to cause the parent to redraw
        """
        self.colorbar.update_ticks()
        self.colorbar.draw_all()
        self.canvas.draw_idle()
        self.colorbarChanged.emit()
