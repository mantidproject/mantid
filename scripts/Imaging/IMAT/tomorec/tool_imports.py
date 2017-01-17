from __future__ import (absolute_import, division, print_function)
# Copyright &copy; 2014-2015 ISIS Rutherford Appleton Laboratory, NScD
# Oak Ridge National Laboratory & European Spallation Source
#
# This file is part of Mantid.
# Mantid is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3 of the License, or
# (at your option) any later version.
#
# Mantid is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#
# File change history is stored at: <https://github.com/mantidproject/mantid>.
# Code Documentation is available at: <http://doxygen.mantidproject.org>


def _import_tool_tomopy():
    try:
        import tomopy
        import tomopy.prep
        import tomopy.recon
        import tomopy.misc
        import tomopy.io

    except ImportError as exc:
        raise ImportError("Could not import the tomopy package and its subpackages. Details: {0}".
                          format(exc))

    return tomopy


def _import_tool_astra():
    # current astra distributions install here, so check there by default
    ASTRA_LOCAL_PATH = '/usr/local/python/'
    import sys
    sys.path.append(ASTRA_LOCAL_PATH)
    try:
        import astra
    except ImportError as exc:
        raise ImportError("Cannot find and import the astra toolbox package: {0}".
                          format(exc))

    MIN_ASTRA_VERSION = 106
    vers = astra.astra.version()
    if isinstance(vers, int) and vers >= MIN_ASTRA_VERSION:
        print("Imported astra successfully. Version: {0}".format(astra.astra.version()))
    else:
        raise RuntimeError("Could not find the required version of astra. Found version: {0}".format(vers))

    print("Astra using cuda: {0}". format(astra.astra.use_cuda()))
    return astra


def import_tomo_tool(tool):
    if not tool or not isinstance(tool, str):
        raise ValueError("The name of a reconstruction tool is required as a string. Got: {0}".
                         format(tool))
    if 'tomopy' == tool:
        return _import_tool_tomopy()
    elif 'astra' == tool:
        return _import_tool_astra()
    else:
        raise ValueError("Internal inconsistency. Tried to import unknown tool: {0}".format(tool))
