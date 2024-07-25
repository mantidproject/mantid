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
from mantid.plots.utility import get_current_cmap
from mantidqt.MPLwidgets import FigureCanvas
from matplotlib.colorbar import Colorbar
from matplotlib.figure import Figure
from matplotlib.colors import ListedColormap, Normalize, SymLogNorm, PowerNorm, LogNorm
from matplotlib import colormaps
import numpy as np
from qtpy.QtWidgets import QWidget, QVBoxLayout, QHBoxLayout, QLineEdit, QComboBox, QCheckBox, QLabel
from qtpy.QtCore import Signal
from qtpy.QtGui import QDoubleValidator

NORM_OPTS = ["Linear", "Log", "SymmetricLog10", "Power"]
AUTO_SCALE_OPTS = ["Min/Max", "3-Sigma", "1.5-Interquartile Range", "1.5-Median Absolute Deviation"]


def register_customized_colormaps():
    """register customized colormaps to matplotlib runtime"""
    cmap_path = os.path.dirname(os.path.realpath(__file__))
    cmap_files = glob.glob(os.path.join(cmap_path, "*.map"))
    for cmap_file in cmap_files:
        cmap_name = os.path.basename(cmap_file).split(".")[0]
        cmap_data = np.loadtxt(cmap_file) / 255.0
        cmap = ListedColormap(cmap_data, name=cmap_name)
        colormaps.register(name=cmap_name, cmap=cmap)
        cmap_data_r = np.flipud(cmap_data)
        cmap_r = ListedColormap(cmap_data_r, name=f"{cmap_name}_r")
        colormaps.register(name=f"{cmap_name}_r", cmap=cmap_r)


class ColorbarWidget(QWidget):
    colorbarChanged = Signal()  # The parent should simply redraw their canvas
    scaleNormChanged = Signal()
    # register additional color maps from file
    register_customized_colormaps()
    # create the list
    cmap_list = sorted([cmap for cmap in colormaps.keys() if not cmap.endswith("_r")])

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
        self.crev = QCheckBox("Reverse")
        self.crev.stateChanged.connect(self.crev_checked_changed)

        self.cmin = QLineEdit()
        self.cmin_value = 0
        self.cmin.setMaximumWidth(100)
        self.cmin.editingFinished.connect(self.clim_changed)
        self.cmin.textEdited.connect(lambda: self._set_cmin_cmax_box_outline(self.cmin))
        self.cmin_layout = QHBoxLayout()
        self.cmin_layout.addStretch()
        self.cmin_layout.addWidget(self.cmin)
        self.cmin_layout.addStretch()

        self.cmin_cmax_default_style_sheet = self.cmin.styleSheet()
        self.cmin_cmax_red_outline_style_sheet = "border: 1px solid red"

        self.linear_validator = QDoubleValidator(parent=self)
        self.log_validator = QDoubleValidator(sys.float_info.min, sys.float_info.max, 3, self)
        self.cmax = QLineEdit()
        self.cmax_value = 1
        self.cmax.setMaximumWidth(100)
        self.cmax.editingFinished.connect(self.clim_changed)
        self.cmax.textEdited.connect(lambda: self._set_cmin_cmax_box_outline(self.cmax))
        self.cmax_layout = QHBoxLayout()
        self.cmax_layout.addStretch()
        self.cmax_layout.addWidget(self.cmax)
        self.cmax_layout.addStretch()

        norm_scale = "Linear"
        powerscale_value = 2
        if default_norm_scale in NORM_OPTS:
            norm_scale = default_norm_scale
        if isinstance(default_norm_scale, tuple) and default_norm_scale[0] in NORM_OPTS:
            norm_scale = default_norm_scale[0]
            if norm_scale == "Power":
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
        if norm_scale != "Power":
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

        self.auto_layout = QHBoxLayout()
        self.autotype = QComboBox()
        self.autotype.addItems(AUTO_SCALE_OPTS)
        self.autotype.setCurrentIndex(0)
        self.autotype.currentIndexChanged.connect(self.auto_scale_type_changed)

        self.auto_layout.addWidget(self.autotype)
        self.auto_layout.addStretch()

        self.canvas = FigureCanvas(Figure())
        if parent:
            # Set facecolor to match parent
            self.canvas.figure.set_facecolor(parent.palette().window().color().getRgbF())
        self.ax = None
        self._reset_figure_axes()

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
        self.layout.addLayout(self.auto_layout)

    def _reset_figure_axes(self):
        """
        Adds axes to the figure. If axes already exist then these are removed and replaced with new ones.
        """
        if self.ax:
            self.ax.clear()
            self.canvas.figure.delaxes(self.ax)
        self.ax = self.canvas.figure.add_axes([0.0, 0.02, 0.2, 0.97])

    def set_mappable(self, mappable):
        """
        When a new plot is created this method should be called with the new mappable
        """
        # The matplotlib.Colorbar doesn't seem to be garbage collected very reliably. If we recreate the axes then
        # this seems to improve the garbage collection.
        self._reset_figure_axes()

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

        if mappable_cmap.name.endswith("_r"):
            self.crev.setChecked(True)
            self.cmap.setCurrentIndex(self.cmap_list.index(mappable_cmap.name.replace("_r", "")))
        else:
            self.crev.setChecked(False)
            self.cmap.setCurrentIndex(self.cmap_list.index(mappable_cmap.name))

        self.redraw()

    def cmap_index_changed(self):
        self.cmap_changed(self.cmap.currentText(), self.crev.isChecked())

    def crev_checked_changed(self):
        self.cmap_changed(self.cmap.currentText(), self.crev.isChecked())

    def cmap_changed(self, name, rev):
        if rev:
            name += "_r"
        self.colorbar.mappable.set_cmap(name)
        self.colorbar.update_normal(self.colorbar.mappable)
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
        self.cmap.setCurrentIndex(sorted(colormaps.keys()).index(mappable_cmap.name))
        self.redraw()

    def norm_changed(self):
        """
        Called when a different normalization is selected
        """
        idx = self.norm.currentIndex()
        if NORM_OPTS[idx] == "Power":
            self.powerscale.show()
            self.powerscale_label.show()
        else:
            self.powerscale.hide()
            self.powerscale_label.hide()
        self.colorbar.mappable.set_norm(self.get_norm())
        self.set_mappable(self.colorbar.mappable)
        self.update_clim_validator()
        self.update_clim()
        self.scaleNormChanged.emit()

    def auto_scale_type_changed(self):
        """
        Called when a different auto scale type is selected
        """
        self.autoscale.blockSignals(True)
        self.autoscale.setChecked(True)
        self.autoscale.blockSignals(False)
        self.norm_changed()

    def get_norm(self):
        """
        This will create a matplotlib.colors.Normalize from selected idx, limits and powerscale
        """
        idx = self.norm.currentIndex()
        if self.autoscale.isChecked():
            climits = self._calculate_clim()
            if climits is not None:
                cmin, cmax = climits
            else:
                cmin = cmax = None
        else:
            cmin = self.cmin_value
            cmax = self.cmax_value
            # sanity check negative values switching to log-scale without autoscale
            if NORM_OPTS[idx] == "Log" and cmin <= 0:
                climits = self._calculate_clim()
                cmin = self.cmin_value = climits[0]
                # keep previous maximum value if possible
                if cmin >= cmax:
                    cmax = self.cmax_value = climits[1]
        if NORM_OPTS[idx] == "Power":
            if self.powerscale.hasAcceptableInput():
                self.powerscale_value = float(self.powerscale.text())
            return PowerNorm(gamma=self.powerscale_value, vmin=cmin, vmax=cmax)
        elif NORM_OPTS[idx] == "SymmetricLog10":
            return SymLogNorm(1e-8 if cmin is None else max(1e-8, abs(cmin) * 1e-3), vmin=cmin, vmax=cmax)
        elif NORM_OPTS[idx] == "Log":
            return LogNorm(vmin=cmin, vmax=cmax)
        else:
            return Normalize(vmin=cmin, vmax=cmax)

    def get_colorbar_scale(self):
        norm = self.colorbar.norm
        scale = "linear"
        kwargs = {}
        if isinstance(norm, SymLogNorm):
            scale = "symlog"
        elif isinstance(norm, LogNorm):
            scale = "log"
        elif isinstance(norm, PowerNorm):
            scale = "function"
            kwargs = {"functions": (lambda x: np.power(x, norm.gamma), lambda x: np.power(x, 1 / norm.gamma))}
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

        self._set_cmin_cmax_box_outline(self.cmin)
        self._set_cmin_cmax_box_outline(self.cmax)
        self.colorbar.mappable.set_clim(self.cmin_value, self.cmax_value)
        self.redraw()

    def update_clim_text(self):
        """
        Update displayed limit values based on stored ones
        """
        self.cmin.setText(f"{float(self.cmin_value):.4}")
        self.cmax.setText(f"{float(self.cmax_value):.4}")

    def redraw(self):
        """
        Redraws the colobar and emits signal to cause the parent to redraw
        """
        self.colorbar.update_ticks()
        self.canvas.figure.draw_without_rendering()
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
        """Update stored colorbar limits. The new limits are found from the colobar data"""
        climits = self._calculate_clim()
        if climits is not None:
            self.cmin_value, self.cmax_value = climits
        self.update_clim_text()

    def _calculate_clim(self) -> tuple:
        """Calculate the colorbar limits to use when autoscale is turned on."""
        try:
            axes = self.colorbar.mappable
        except AttributeError:
            return None

        try:
            data = axes.get_array_clipped_to_bounds()
        except AttributeError:
            data = axes.get_array()

        if self._is_log_norm():
            mask = np.logical_and(np.isfinite(data), np.greater(data, 0))
        else:
            mask = np.isfinite(data)

        signal = data[mask].filled(np.nan) if np.ma.isMaskedArray(data) else data[mask]

        if mask.any():
            vmin, vmax = self._calculate_auto_color_limits(signal)
            # sparse data
            if np.isclose(vmin, vmax):
                vmin, vmax = self._calculate_auto_color_limits(np.unique(signal))
            return vmin, vmax
        else:
            return (0.1, 1.0)

    def _calculate_auto_color_limits(self, signal):
        """Calculate auto scale limits"""
        scale_type = AUTO_SCALE_OPTS[self.autotype.currentIndex()]
        signal_min, signal_max = np.nanmin(signal), np.nanmax(signal)

        if scale_type == "Min/Max":
            vmin, vmax = signal_min, signal_max
        elif scale_type == "3-Sigma":
            mean, sigma = np.nanmean(signal), np.nanstd(signal)
            vmin, vmax = mean - 3 * sigma, mean + 3 * sigma
        elif scale_type == "1.5-Interquartile Range":
            Q1, Q3 = np.nanpercentile(signal, [25, 75])
            vmin, vmax = Q1 - 1.5 * (Q3 - Q1), Q3 + 1.5 * (Q3 - Q1)
        elif scale_type == "1.5-Median Absolute Deviation":
            med = np.nanmedian(signal)
            mad = np.nanmedian(np.abs(signal - med))
            vmin, vmax = med - 1.5 * mad, med + 1.5 * mad

        vmin = max(vmin, signal_min)
        vmax = min(vmax, signal_max)

        # sanity checks
        if self._is_log_norm():
            if vmax <= 0:
                vmax = 1.0
            if vmin <= 0 or np.isclose(vmin, vmax):
                vmin = 1e-4 * vmax
        return vmin, vmax

    def _is_log_norm(self):
        return NORM_OPTS[self.norm.currentIndex()] == "Log"

    def _manual_clim(self):
        """Update stored colorbar limits
        The new limits are found from user input"""
        self._update_cmin()
        self._update_cmax()

    def _update_cmin(self):
        """Attempt to update cmin with user input. Reset to previous value if invalid input"""
        if not self.cmin.hasAcceptableInput():
            self.update_clim_text()
            return

        # QDoubleValidator can let through some things like 0,111
        try:
            cmin = float(self.cmin.text())
        except ValueError:
            self.update_clim_text()
            return

        if cmin >= self.cmax_value:
            self.update_clim_text()
            return

        self.cmin_value = cmin

    def _update_cmax(self):
        """Attempt to update cmax with user input. Reset to previous value if invalid input"""
        if not self.cmax.hasAcceptableInput():
            self.update_clim_text()
            return

        try:
            cmax = float(self.cmax.text())
        except ValueError:
            self.update_clim_text()
            return

        if cmax <= self.cmin_value:
            self.update_clim_text()
            return

        self.cmax_value = cmax

    def _create_linear_normalize_object(self):
        if self.autoscale.isChecked():
            cmin = cmax = None
        else:
            cmin = self.cmin_value
            cmax = self.cmax_value
        return Normalize(vmin=cmin, vmax=cmax)

    def _set_cmin_cmax_box_outline(self, box: QLineEdit):
        if not box.hasAcceptableInput():
            box.setStyleSheet(self.cmin_cmax_red_outline_style_sheet)
        else:
            box.setStyleSheet(self.cmin_cmax_default_style_sheet)
