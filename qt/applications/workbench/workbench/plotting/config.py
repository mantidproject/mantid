# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.
#
#
from __future__ import (absolute_import)

# system imports

# 3rd-party imports
import matplotlib as mpl
import matplotlib._pylab_helpers as _pylab_helpers
from qtpy.QtWidgets import QApplication

# local imports
from .globalfiguremanager import GlobalFigureManager

# Our backend. We keep this separate from the rc params as it can only be set once
MPL_BACKEND = 'module://workbench.plotting.backend_workbench'

# Our style defaults
DEFAULT_RCPARAMS = {
    'figure.facecolor':  'w',
    'figure.max_open_warning': 200
}


def initialize_matplotlib():
    """Configures our defaults"""
    # Replace vanilla Gcf with our custom manager
    setattr(_pylab_helpers, 'Gcf', GlobalFigureManager)
    # Set our defaults
    reset_rcparams_to_default()
    # Set figure DPI scaling to monitor DPI
    mpl.rcParams['figure.dpi'] = QApplication.instance().desktop().physicalDpiX()


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
    assert 'backend' not in rcp
    mpl.rcParams.update(rcp)
