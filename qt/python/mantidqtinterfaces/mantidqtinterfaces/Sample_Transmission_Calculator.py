# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import sys
from mantidqtinterfaces.SampleTransmissionCalculator import stc_gui
from mantidqt.gui_helper import get_qapplication

app, within_mantid = get_qapplication()
if 'workbench' in sys.modules:
    from workbench.config import get_window_config

    parent, flags = get_window_config()
else:
    parent, flags = None, None
planner = stc_gui.SampleTransmissionCalculator(parent, flags)
planner.show()
if not within_mantid:
    sys.exit(app.exec_())
