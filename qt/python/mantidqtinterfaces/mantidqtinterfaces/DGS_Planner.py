# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=invalid-name,unused-import
import sys
from mantidqtinterfaces.DGSPlanner import DGSPlannerGUI
from mantidqt.gui_helper import get_qapplication

app, within_mantid = get_qapplication()
if "workbench" in sys.modules:
    from workbench.config import get_window_config

    parent, flags = get_window_config()
else:
    parent, flags = None, None
planner = DGSPlannerGUI.DGSPlannerGUI(parent, flags)
planner.show()
if not within_mantid:
    sys.exit(app.exec_())
