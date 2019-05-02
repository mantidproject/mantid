# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#pylint: disable=invalid-name,unused-import
from __future__ import (absolute_import, division, print_function)
import sys
from DGSPlanner import DGSPlannerGUI
from mantidqt.gui_helper import get_qapplication

app, within_mantid = get_qapplication()
planner = DGSPlannerGUI.DGSPlannerGUI()
planner.show()
if not within_mantid:
    sys.exit(app.exec_())
