# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.
#
#
import sys
import glob
import os
from mantid.kernel import ConfigService
from mantid.plots.utility import mpl_version_info, get_current_cmap
from mantidqt.MPLwidgets import FigureCanvas
from matplotlib.colorbar import Colorbar
from matplotlib.figure import Figure
from matplotlib.colors import ListedColormap, Normalize, SymLogNorm, PowerNorm, LogNorm
from matplotlib import cm
import numpy as np
from qtpy.QtWidgets import QWidget, QVBoxLayout, QHBoxLayout, QLineEdit, QComboBox, QCheckBox, QLabel
from qtpy.QtCore import Signal, Qt
from qtpy.QtGui import QDoubleValidator

NORM_OPTS = ["Linear", "Log", "SymmetricLog10", "Power"]
MIN_LOG_VALUE = 1e-4


def register_customized_colormaps():
    """register customized colormaps to matplotlib runtime"""
    cmap_path = os.path.dirname(os.path.realpath(__file__))
    cmap_files = glob.glob(os.path.join(cmap_path, "*.map"))
    for cmap_file in cmap_files:
        cmap_name = os.path.basename(cmap_file).split(".")[0]
        cmap_data = np.loadtxt(cmap_file)/255.0
        cmap = ListedColormap(cmap_data, name=cmap_name)
        cm.register_cmap(name=cmap_name, cmap=cmap)
        cmap_data_r = np.flipud(cmap_data)
        cmap_r = ListedColormap(cmap_data_r, name=f"{cmap_name}_r")
        cm.register_cmap(name=f"{cmap_name}_r", cmap=cmap_r)


class ColorbarWidget(QWidget):
    colorbarChanged = Signal()  # The parent should simply redraw their canvas
    scaleNormChanged = Signal()
    # register additional color maps from file
    register_customized_colormaps()
    # create the list
    cmap_list = sorted([cmap for cmap in cm.cmap_d.keys() if not cmap.endswith('_r')])

    def __init__(self, parent=None, default_norm_scale=None):
        """
        :param default_scale: None uses linear, else either a string or tuple(string, other arguments), e.g. tuple('Power', exponent)
        """

        super(ColorbarWidget, self).__init__(parent)

        self.setWindowTitle("Colorbar")
        self.setMaximumWidth(100)

        self.cmap = QComboBox()
        self.cmap.addItems(self.cmap_list)
        self.cmap.currentIndexChanged.connect(self.cmap_index_changed)
        self.crev = QCheckBox('Reverse')
        self.crev.stateChanged.connect(self.crev_checked_changed)

        self.cmin = QLineEdit()
        self.cmin_value = 0
        self.cmin.setMaximumWidth(100)
        self.cmin.editingFinished.connect(self.clim_changed)
        self.cmin_layout = QHBoxLayout()
        self.cmin_layout.addStretch()
        self.cmin_layout.addWidget(self.cmin)
        self.cmin_layout.addStretch()

        self.linear_validator = QDoubleValidator(parent=self)
        self.log_validator = QDoubleValidator(MIN_LOG_VALUE, sys.float_info.max, 3, self)
        self.cmax = QLineEdit()
        self.cmax_value = 1
        self.cmax.setMaximumWidth(100)
        self.cmax.editingFinished.connect(self.clim_changed)
        self.cmax_layout = QHBoxLayout()
        self.cmax_layout.addStretch()
        self.cmax_layout.addWidget(self.cmax)
        self.cmax_layout.addStretch()

        norm_scale = 'Linear'
        powerscale_value = 2
        if default_norm_scale in NORM_OPTS:
            norm_scale = default_norm_scale
        if isinstance(default_norm_scale, tuple) and default_norm_scale[0] in NORM_OPTS:
            norm_scale = default_norm_scale[0]
            if norm_scale == 'Power':
                powerscale_value = float(default_norm_scale[1])

        self.norm_layout = QHBoxLayout()
        self.norm = QComboBox()
        self.norm.addItems(NORM_OPTS)
        self.norm.setCurrentText(norm_scale)
        self.norm.currentIndexChanged.connect(self.norm_changed)
        self.update_clim_validator()

        self.powerscale = QLineEdit()
        self.powerscale_value = powerscale_value
        self.powerscale.setText(str(powerscale_value))
        self.powerscale.setValidator(QDoubleValidator(0.001, 100, 3))
        self.powerscale.setMaximumWidth(50)
        self.powerscale.editingFinished.connect(self.norm_changed)
        self.powerscale_label = QLabel("n=")
        if norm_scale != 'Power':
            self.powerscale.hide()
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
        self.ax = self.canvas.figure.add_axes([0.0, 0.02, 0.2, 0.97])

        # layout
        self.layout = QVBoxLayout(self)
        self.layout.setContentsMargins(0, 0, 0, 0)
        self.layout.setSpacing(2)
        self.layout.addWidget(self.cmap)
        self.layout.addWidget(self.crev)
        self.layout.addLayout(self.cmax_layout)
        self.layout.addWidget(self.canvas, stretch=1)
        self.layout.addLayout(self.cmin_layout)
        self.layout.addLayout(self.norm_layout)
        self.layout.addWidget(self.autoscale)

    def set_mappable(self, mappable):
        """
        When a new plot is created this method should be called with the new mappable
        """
        # sanity check the mappable
        mappable = self._validate_mappable(mappable)
        self.ax.clear()
        try:  # Use current cmap
            cmap = get_current_cmap(self.colorbar)
        except AttributeError:
            # else use default
            cmap = ConfigService.getString("plots.images.Colormap")
        self.colorbar = Colorbar(ax=self.ax, mappable=mappable)
        self.cmin_value, self.cmax_value = mappable.get_clim()
        self.update_clim_text()
        self.cmap_changed(cmap, False)

        mappable_cmap = get_current_cmap(mappable)

        if mappable_cmap.name.endswith('_r'):
            self.crev.setChecked(True)
        else:
            self.crev.setChecked(False)
        self.cmap.setCurrentIndex(self.cmap_list.index(mappable_cmap.name.replace('_r', '')))

        self.redraw()

    def cmap_index_changed(self):
        self.cmap_changed(self.cmap.currentText(), self.crev.isChecked())

    def crev_checked_changed(self):
        self.cmap_changed(self.cmap.currentText(), self.crev.isChecked())

    def cmap_changed(self, name, rev):
        if rev:
            name += '_r'
        self.colorbar.mappable.set_cmap(name)
        if mpl_version_info() >= (3, 1):
            self.colorbar.update_normal(self.colorbar.mappable)
        else:
            self.colorbar.set_cmap(name)
        self.redraw()

    def mappable_changed(self):
        """
        Updates the colormap and min/max values of the colorbar
        when the plot changes via settings.
        """
        mappable_cmap = get_current_cmap(self.colorbar.mappable)
        low, high = self.colorbar.mappable.get_clim()
        self.cmin_value = low
        self.cmax_value = high
        self.update_clim_text()
        self.cmap.setCurrentIndex(sorted(cm.cmap_d.keys()).index(mappable_cmap.name))
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
        self.update_clim_validator()
        self.scaleNormChanged.emit()

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
            return SymLogNorm(1e-8 if cmin is None else max(1e-8, abs(cmin) * 1e-3),
                              vmin=cmin, vmax=cmax)
        elif NORM_OPTS[idx] == "Log":
            cmin = MIN_LOG_VALUE if cmin is not None and cmin <= 0 else cmin
            return LogNorm(vmin=cmin, vmax=cmax)
        else:
            return Normalize(vmin=cmin, vmax=cmax)

    def get_colorbar_scale(self):
        norm = self.colorbar.norm
        scale = 'linear'
        kwargs = {}
        if isinstance(norm, SymLogNorm):
            scale = 'symlog'
        elif isinstance(norm, LogNorm):
            scale = 'log'
        elif isinstance(norm, PowerNorm):
            scale = 'function'
            kwargs = {'functions': (lambda x: np.power(x, norm.gamma), lambda x: np.power(x, 1 / norm.gamma))}
        return scale, kwargs

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
            self._autoscale_clim()
        else:
            self._manual_clim()

        self.colorbar.mappable.set_clim(self.cmin_value, self.cmax_value)
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

    def update_clim_validator(self):
        if NORM_OPTS[self.norm.currentIndex()] == "Log":
            self.cmin.setValidator(self.log_validator)
            self.cmax.setValidator(self.log_validator)
        else:
            self.cmin.setValidator(self.linear_validator)
            self.cmax.setValidator(self.linear_validator)

    def _autoscale_clim(self):
        """Update stored colorbar limits
        The new limits are found from the colobar data """
        if hasattr(self.colorbar.mappable, "get_array_clipped_to_bounds"):
            data = self.colorbar.mappable.get_array_clipped_to_bounds()
        else:
            # in nonorthog view get passed a QuadMesh that doesn't have the above method
            # however the data from get_array for MDEvent ws is already clipped (not for MDHisto)
            data = self.colorbar.mappable.get_array()
        norm = NORM_OPTS[self.norm.currentIndex()]
        try:
            try:
                masked_data = data[~data.mask]
                # use the smallest positive value as vmin when using log scale,
                # matplotlib will take care of the data skipping part.
                masked_data = masked_data[masked_data > 0] if norm == "Log" else masked_data

                # If any dimension is zero then we have no data in the display area
                data_is_empty = any(map(lambda dim: dim == 0, masked_data.shape))

                self.cmin_value = 0. if data_is_empty else masked_data.min()
                self.cmax_value = 0. if data_is_empty else masked_data.max()
            except (AttributeError, IndexError):
                data = data[np.nonzero(data)] if norm == "Log" else data
                self.cmin_value = np.nanmin(data)
                self.cmax_value = np.nanmax(data)
        except (ValueError, RuntimeWarning):
            # all values mask
            pass
        self.update_clim_text()

    def _manual_clim(self):
        """Update stored colorbar limits
        The new limits are found from user input"""
        if self.cmin.hasAcceptableInput():
            cmin = float(self.cmin.text())
            if cmin < self.cmax_value:
                self.cmin_value = cmin
            else:  # reset values back
                self.update_clim_text()
        if self.cmax.hasAcceptableInput():
            cmax = float(self.cmax.text())
            if cmax > self.cmin_value:
                self.cmax_value = cmax
            else:  # reset values back
                self.update_clim_text()

    def _create_linear_normalize_object(self):
        if self.autoscale.isChecked():
            cmin = cmax = None
        else:
            cmin = self.cmin_value
            cmax = self.cmax_value
        return Normalize(vmin=cmin, vmax=cmax)

    def _validate_mappable(self, mappable):
        """Disable the Log option if no positive value can be found from given data (image)"""
        index = NORM_OPTS.index("Log")
        if mappable.get_array() is not None:
            if np.all(mappable.get_array() <= 0):
                self.norm.model().item(index, 0).setEnabled(False)
                self.norm.setItemData(index, "Log scale is disabled for non-positive data",
                                      Qt.ToolTipRole)
                if isinstance(mappable.norm, LogNorm):
                    mappable.norm = self._create_linear_normalize_object()
                    self.norm.blockSignals(True)
                    self.norm.setCurrentIndex(0)
                    self.norm.blockSignals(False)
            else:
                if not self.norm.model().item(index, 0).isEnabled():
                    self.norm.model().item(index, 0).setEnabled(True)
                    self.norm.setItemData(index, "", Qt.ToolTipRole)

        return mappable
