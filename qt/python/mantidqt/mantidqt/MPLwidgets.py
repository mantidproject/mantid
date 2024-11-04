# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# ruff: noqa: F403   # Allow wild imports
from .gui_helper import set_matplotlib_backend

backend = set_matplotlib_backend()  # must be at the top of this file
if backend == "Qt5Agg":
    from matplotlib.backends.backend_qt5agg import *
else:
    raise RuntimeError(f"Unrecognized backend {backend}")
