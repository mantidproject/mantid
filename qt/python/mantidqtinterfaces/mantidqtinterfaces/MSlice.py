#!/usr/bin/python
# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import sys
from mantidqt.gui_helper import get_qapplication

try:
    from mslice.app import show_gui
except ImportError:
    from mantid.kernel import logger

    logger.warning("MSlice is not available")
    show_gui = None

if show_gui is not None:
    app, within_mantid = get_qapplication()
    show_gui()
    if not within_mantid:
        sys.exit(app.exec_())
