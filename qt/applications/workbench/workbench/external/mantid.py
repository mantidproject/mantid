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
"""Defines utility functionality for interacting with the mantid framework
"""
from __future__ import unicode_literals

from imp import find_module
import os


def prepare_mantid_env():
    # Mantid needs to be able to find its .properties file. It looks
    # in the application directory but by default but this is python.exe.
    # MANTIDPATH can be used to override this. If it is not set
    # we currently assume the Mantid.properties is in the same location as the
    # mantid module itself
    if 'MANTIDPATH' not in os.environ:
        _, pkgpath, _ = find_module('mantid')
        os.environ['MANTIDPATH'] = os.path.dirname(pkgpath)
