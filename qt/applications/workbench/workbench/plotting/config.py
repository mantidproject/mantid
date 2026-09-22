# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.
#
#
# system imports

# 3rd-party imports
import sys
import warnings

import matplotlib as mpl
import matplotlib._pylab_helpers as _pylab_helpers
from qtpy.QtWidgets import QApplication
from qtpy.QtCore import Qt

# local imports
from .globalfiguremanager import GlobalFigureManager

# Our backend. We keep this separate from the rc params as it can only be set once
MPL_BACKEND = "module://workbench.plotting.backend_workbench"

# Our style defaults
DEFAULT_RCPARAMS = {"figure.facecolor": "w", "figure.max_open_warning": 200}

_DARK_BG_OUTER = "#1d1d1d"  # figure background (window margin)
_DARK_BG_PANEL = "#2b2b2b"  # axes background (the actual plot area)
_DARK_FG_PRIMARY = "#DAD8D8"  # labels, text, legend text - near-white, highest contrast
_DARK_FG_SECONDARY = "#c8c8c8"  # ticks, spines, patch/hatch edges - legible but a step down
_DARK_FG_MUTED = "#555555"  # gridlines - present without competing with the data
_DARK_ACCENT = "#ffa500"  # boxplot median, etc.

DARK_RCPARAMS = {
    "figure.facecolor": _DARK_BG_OUTER,
    "axes.facecolor": _DARK_BG_PANEL,
    "savefig.facecolor": _DARK_BG_PANEL,
    "axes.edgecolor": _DARK_FG_SECONDARY,
    "axes.labelcolor": _DARK_FG_PRIMARY,
    "text.color": _DARK_FG_PRIMARY,
    "xtick.color": _DARK_FG_SECONDARY,
    "xtick.labelcolor": _DARK_FG_PRIMARY,
    "ytick.color": _DARK_FG_SECONDARY,
    "ytick.labelcolor": _DARK_FG_PRIMARY,
    "grid.color": _DARK_FG_MUTED,
    "legend.facecolor": _DARK_BG_PANEL,
    "legend.edgecolor": _DARK_FG_SECONDARY,
    "legend.labelcolor": _DARK_FG_PRIMARY,
    "patch.edgecolor": _DARK_FG_SECONDARY,
    "hatch.color": _DARK_FG_SECONDARY,
    "boxplot.boxprops.color": _DARK_FG_PRIMARY,
    "boxplot.whiskerprops.color": _DARK_FG_PRIMARY,
    "boxplot.capprops.color": _DARK_FG_PRIMARY,
    "boxplot.medianprops.color": _DARK_ACCENT,
    "boxplot.flierprops.markeredgecolor": _DARK_FG_PRIMARY,
    "figure.max_open_warning": 200,
}


def initialize_matplotlib():
    """
    Configure our defaults for matplotlib.
    :param figure_window_parent: An QWidget that will become the parent of any figure window. Can be None
    :param figure_window_flags: A Qt.WindowFlags enumeration defining the window flags for a figure window
    """
    # Set our defaults
    reset_rcparams_to_default()
    # Set figure DPI scaling to monitor DPI
    mpl.rcParams["figure.dpi"] = QApplication.instance().primaryScreen().physicalDotsPerInchX()
    # Hide warning made by matplotlib before checking our backend.
    warnings.filterwarnings("ignore", message="Starting a Matplotlib GUI outside of the main thread will likely fail.")
    # Disabling default key shortcuts for toggling axes scale
    mpl.rcParams["keymap.xscale"].remove("k")
    mpl.rcParams["keymap.xscale"].remove("L")
    mpl.rcParams["keymap.yscale"].remove("l")

    def remove_keys(shortcut_keys, rc_param_key):
        [mpl.rcParams[rc_param_key].remove(k) for k in shortcut_keys if k in mpl.rcParams[rc_param_key]]

    # Disabling to override default shortcuts to navigate backward and forward
    remove_keys(["c", "left", "backspace", "MouseButton.BACK"], "keymap.back")
    remove_keys(["v", "right", "MouseButton.FORWARD"], "keymap.forward")

    # Disabling to override default shortcuts for home
    remove_keys(["h", "r", "home"], "keymap.home")


def init_mpl_gcf():
    """
    Replace vanilla Gcf with our custom manager
    """
    # It is very important this assertion is met. If the matplotlib backend is imported
    # before we set the 'Gcf' object to our custom global figure manager, then the plotting
    # in Mantid will be broken.
    assert "matplotlib.backend_bases" not in sys.modules

    setattr(_pylab_helpers, "Gcf", GlobalFigureManager)


def _is_dark_mode() -> bool:
    app = QApplication.instance()
    return app is not None and app.styleHints().colorScheme() == Qt.ColorScheme.Dark


def reset_rcparams_to_default():
    """
    Reset the rcParams to the default settings.
    """
    mpl.rcParams.clear()
    mpl.rc_file_defaults()
    set_rcparams(DARK_RCPARAMS if _is_dark_mode() else DEFAULT_RCPARAMS)

    # We must keep our backend
    mpl.use(MPL_BACKEND)


def set_rcparams(rcp):
    """
    Update the current rcParams with the given set
    :param rcp: A dictionary containing new rcparams values
    """
    # We must keep our backend
    assert "backend" not in rcp
    mpl.rcParams.update(rcp)
