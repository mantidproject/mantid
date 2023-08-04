# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=invalid-name,unused-import
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

    if not within_mantid:
        # set the super-awesome color scheme
        from snapred.meta.Config import Resource

        with Resource.open("style.qss", "r") as styleSheet:
            app.setStyleSheet(styleSheet.read())

    # turn off tranlucent when running in mantid
    s = SNAPRedGUI(parent, window_flags=flags, translucentBackground=(not within_mantid))
    s.show()

    if not within_mantid:
        # start the main application
        sys.exit(app.exec_())
