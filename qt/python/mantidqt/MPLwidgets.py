# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)
from .gui_helper import set_matplotlib_backend

backend = set_matplotlib_backend()  # must be at the top of this file
if backend == 'Qt4Agg':
    from matplotlib.backends.backend_qt4agg import *  # noqa
elif backend == 'Qt5Agg':
    from matplotlib.backends.backend_qt5agg import *  # noqa
else:
    raise RuntimeError('Unrecognized backend {}'.format(backend))
