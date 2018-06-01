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
import matplotlib._pylab_helpers as pylab_helpers

# local imports
from .currentfigure import CurrentFigure

# Our backend. We keep this separate from the rc params as it can only be set once
MPL_BACKEND = 'module://workbench.plotting.backend_workbench'

# Our style defaults
DEFAULT_RCPARAMS = {
    'figure.facecolor':  'w'
}


def setup_matplotlib():
    """Configures our defaults"""
    # Replace vanilla Gcf with our custom instance
    setattr(pylab_helpers, 'Gcf', CurrentFigure)
    # Our backend
    mpl.use(MPL_BACKEND)
    # Set our defaults
    reset_rcparams_to_default()


def reset_rcparams_to_default():
    """
    Reset the rcParams to the default settings.

    :param rcp: A dictionary containing new rcparams values
    """
    mpl.rcParams.clear()
    mpl.rc_file_defaults()
    set_rcparams(DEFAULT_RCPARAMS)


def set_rcparams(rcp):
    """
    Update the current rcParams with the given set
    :param rcp: A dictionary containing new rcparams values
    """
    mpl.rcParams.update(rcp)
