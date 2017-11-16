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
"""A selection of utility functions related to Qt functionality
"""
from __future__ import absolute_import

# stdlib modules
import os

# 3rd-party modules
from qtpy.uic import loadUi


def load_ui(caller_filename, ui_relfilename, baseinstance=None):
    """Load a ui file assuming it is relative to a given callers filepath

    :param caller_filename: An absolute path to a file whose basename when
    joined with ui_relfilename gives the full path to the .ui file. Generally
    this called with __file__
    :param ui_relfilename: A relative filepath that when joined with the
    basename of caller_filename gives the absolute path to the .ui file.
    :param baseinstance: An instance of a widget to pass to uic.loadUi
    that becomes the base class rather than a new widget being created.
    :return: A new instance of the form class
    """
    filepath = os.path.join(os.path.dirname(caller_filename), ui_relfilename)
    return loadUi(filepath, baseinstance=baseinstance)
