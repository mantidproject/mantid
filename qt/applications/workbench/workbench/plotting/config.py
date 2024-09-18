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

# local imports
from .globalfiguremanager import GlobalFigureManager

# Our backend. We keep this separate from the rc params as it can only be set once
MPL_BACKEND = "module://workbench.plotting.backend_workbench"

# Our style defaults
DEFAULT_RCPARAMS = {"figure.facecolor": "w", "figure.max_open_warning": 200}


def initialize_matplotlib():
    """
    Configure our defaults for matplotlib.
    :param figure_window_parent: An QWidget that will become the parent of any figure window. Can be None
    :param figure_window_flags: A Qt.WindowFlags enumeration defining the window flags for a figure window
    """
    # Set our defaults
    reset_rcparams_to_default()
    # Set figure DPI scaling to monitor DPI
    mpl.rcParams["figure.dpi"] = QApplication.instance().desktop().physicalDpiX()
    # Hide warning made by matplotlib before checking our backend.
    warnings.filterwarnings("ignore", message="Starting a Matplotlib GUI outside of the main thread will likely fail.")
    # Disabling default key shortcuts for toggling axes scale
    mpl.rcParams["keymap.xscale"].remove("k")
    mpl.rcParams["keymap.xscale"].remove("L")
    mpl.rcParams["keymap.yscale"].remove("l")


def init_mpl_gcf():
    """
    Replace vanilla Gcf with our custom manager
    """
    # It is very important this assertion is met. If the matplotlib backend is imported
    # before we set the 'Gcf' object to our custom global figure manager, then the plotting
    # in Mantid will be broken.
    assert "matplotlib.backend_bases" not in sys.modules

    setattr(_pylab_helpers, "Gcf", GlobalFigureManager)


def reset_rcparams_to_default():
    """
    Reset the rcParams to the default settings.
    """
    mpl.rcParams.clear()
    mpl.rc_file_defaults()
    set_rcparams(DEFAULT_RCPARAMS)
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
