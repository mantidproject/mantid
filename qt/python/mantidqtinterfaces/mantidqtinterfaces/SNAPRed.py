# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=invalid-name,unused-import
from pathlib import Path
import sys

from mantidqt.gui_helper import get_qapplication

try:
    from snapred.ui.main import SNAPRedGUI
except ImportError:
    from mantid.kernel import logger

    logger.warning("SNAPRed is not available")

    SNAPRedGUI = None

if SNAPRedGUI is not None:
    app, within_mantid = get_qapplication()
    if "workbench" in sys.modules:
        from workbench.config import get_window_config

        parent, flags = get_window_config()
    else:
        parent, flags = None, None

    # Recent versions of SNAPRed have required stylesheet settings, but it's important to apply these
    #   only to the SNAPRed GUI and its children.
    from snapred.meta.Config import Resource

    # turn off translucent background when running in mantid
    s = SNAPRedGUI(parent, window_flags=flags, translucentBackground=(not within_mantid))

    # "workbench_style.qss" may not exist in all versions of SNAPRed.
    qssFilePath = Resource.getPath("workbench_style.qss" if within_mantid else "style.qss")
    if Path(qssFilePath).exists():
        with open(qssFilePath, "r") as styleSheet:
            s.setStyleSheet(styleSheet.read())
    s.show()

    if not within_mantid:
        # start the main application
        sys.exit(app.exec_())
