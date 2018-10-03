# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
"""
MantidPlot module with functions specific to the traditional,
qti-based MantidPlot Python plotting interface

As with other MantidPlot modules, this has to run from within MantidPlot

"""
from __future__ import (absolute_import, division,
                        print_function)

# Require MantidPlot
try:
    import _qti
except ImportError:
    raise ImportError('The "mantidplot.qtiplot" module can only be used from within MantidPlot.')

import pymantidplot
from PyQt4 import QtCore

#-----------------------------------------------------------------------------
# Intercept qtiplot "plot" command and forward to plotSpectrum for a workspace
def plot(source, *args, **kwargs):
    """Create a new plot given a workspace, table or matrix.

    Args:
        source: what to plot; if it is a Workspace, will
                call plotSpectrum()

    Returns:
        A handle to the created Graph widget.
    """
    if hasattr(source, '_getHeldObject') and isinstance(source._getHeldObject(), QtCore.QObject):
        return pymantidplot.proxies.new_proxy(pymantidplot.proxies.Graph,_qti.app.plot, source._getHeldObject(), *args, **kwargs)
    else:
        return pymantidplot.plotSpectrum(source, *args, **kwargs)
