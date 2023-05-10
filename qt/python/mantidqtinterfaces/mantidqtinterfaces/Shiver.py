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
    from shiver import Shiver
except ImportError:
    from mantid.kernel import logger

    logger.warning("Shiver is not available")
    Shiver = None

if Shiver is not None:
    # NOTE: Shiver my need to use parent and flags in the future
    app, within_mantid = get_qapplication()
    if "workbench" in sys.modules:
        from workbench.config import get_window_config

        parent, flags = get_window_config()
    else:
        parent, flags = None, None
    #
    s = Shiver()
    s.show()
    if not within_mantid:
        sys.exit(app.exec_())
