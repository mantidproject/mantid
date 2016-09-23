"""
Produce output tables from fitting benchmarking results.
"""
# Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory, NScD
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

from __future__ import (absolute_import, division, print_function)

def format_cell_value_rst(value, width, color_scale=None, items_link=None):
    """
    Build the content string for a table cell, adding style/color tags
    if required.

    """
    if not color_scale:
        if not items_link:
            value_text = ' {0:.4g}'.format(value).ljust(width, ' ')
        else:
            value_text = ' :ref:`{0:.4g} <{1}>`'.format(value, items_link).ljust(width, ' ')
    else:
        color = ''
        for color_descr in color_scale:
            if value <= color_descr[0]:
                color = color_descr[1]
                break
        if not color:
            color = color_scale[-1][1]
        value_text = " :{0}:`{1:.4g}`".format(color, value).ljust(width, ' ')

    return value_text
