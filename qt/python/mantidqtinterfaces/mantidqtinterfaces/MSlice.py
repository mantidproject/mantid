#!/usr/bin/python
# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import sys
from mantidqt.gui_helper import get_qapplication

MSLICE_URL = "https://mantidproject.github.io/mslice/quickstart.html#starting-mslice"

try:
    from mslice.app import show_gui
except ImportError:
    from mantid.kernel import logger

    logger.warning(
        f"MSlice is not available.\n"
        f"If this is a conda installation of MantidWorkbench,\n"
        f"Mslice must be installed as a separate package.\n"
        f"Check <{MSLICE_URL}> for detailed installation instructions."
    )
    show_gui = None

if show_gui is not None:
    app, within_mantid = get_qapplication()
    show_gui()
    if not within_mantid:
        sys.exit(app.exec_())
