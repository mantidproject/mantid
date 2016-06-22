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

# import pyplot and also bring it into the standard MantidPlot namespace
import pymantidplot.pyplot
from pymantidplot.pyplot import *
# and the old qtiplot stuff
import pymantidplot.qtiplot
