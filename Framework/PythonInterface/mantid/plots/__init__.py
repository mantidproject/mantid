# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid package
#
#
"""
Functionality for unpacking mantid objects for plotting with matplotlib.
"""

# This file should be left free of PyQt imports to allow quick importing
# of the main package.
from __future__ import (absolute_import, division, print_function)


try:
   from collections.abc import Iterable
except ImportError:
   # check Python 2 location
   from collections import Iterable
from matplotlib.projections import register_projection
from matplotlib.scale import register_scale

from mantid.plots import datafunctions, axesfunctions, axesfunctions3D
from mantid.plots.legend import convert_color_to_hex, LegendProperties
from mantid.plots.datafunctions import get_normalize_by_bin_width
from mantid.plots.scales import PowerScale, SquareScale
from mantid.plots.mantidaxes import MantidAxes, MantidAxes3D, WATERFALL_XOFFSET_DEFAULT, WATERFALL_YOFFSET_DEFAULT
from mantid.plots.utility import (artists_hidden, autoscale_on_update,
                                  legend_set_draggable, MantidAxType)

register_projection(MantidAxes)
register_projection(MantidAxes3D)
register_scale(PowerScale)
register_scale(SquareScale)
