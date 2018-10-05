# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#-------------------------------------------------------------------
# mantidplot.py
#
# Load 'pymantidplot' which cannot be called 'mantidplot' because of
# name conflict with the MantidPlot binary (especially on osx)
#-------------------------------------------------------------------
from __future__ import (absolute_import, division,
                        print_function)

import pymantidplot
from pymantidplot import *

# and the old qtiplot stuff
import pymantidplot.qtiplot
