#  This file is part of the mantid workbench.
#
#  Copyright (C) 2017 mantidproject
#
#  This program is free software: you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation, either version 3 of the License, or
#  (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program.  If not, see <http://www.gnu.org/licenses/>.
from __future__ import (absolute_import)

# system imports

# 3rd-party imports
import matplotlib as mpl
import matplotlib._pylab_helpers as _pylab_helpers

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
