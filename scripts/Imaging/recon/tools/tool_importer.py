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

def do_importing(tool):
    if not tool or not isinstance(tool, str):
        raise ValueError("The name of a reconstruction tool is required as a string. Got: {0}".
                         format(tool))
    if 'tomopy' == tool:
        from recon.tools.tomopy_tool import TomoPyTool
        return TomoPyTool()

    elif 'astra' == tool:
        from recon.tools.astra_tool import AstraTool

        return AstraTool()

    else:
        raise ValueError("Internal inconsistency. Tried to import unknown tool: {0}".format(tool))
