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
"""Defines a collection of functions to support plotting workspaces with
our custom window.
"""

# std imports

# 3rd party imports
import matplotlib.pyplot as plt

# local imports

# -----------------------------------------------------------------------------
# Constants
# -----------------------------------------------------------------------------
PROJECTION = 'mantid'


# -----------------------------------------------------------------------------
# Functions
# -----------------------------------------------------------------------------
def plot_spectrum(workspaces, spectrum_nums=None, wksp_indices=None):
    # check inputs
    if spectrum_nums is not None and wksp_indices is not None:
        raise ValueError("plot_spectrum: Both spectrum_nums and wksp_indices supplied. "
                         "Please supply only 1.")

    # do plotting
    fig = plt.figure()
    ax = fig.add_subplot(111, projection=PROJECTION)
    if spectrum_nums is not None:
        kw, rows = 'specNum', spectrum_nums
    else:
        kw, rows = 'wkspIndex', wksp_indices
    for ws in workspaces:
        for row in rows:
            ax.plot(ws, **{kw: row})
    fig.show()
    return fig
