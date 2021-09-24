#!/usr/bin/python
# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import sys
from mslice.app import show_gui
from mantidqt.gui_helper import get_qapplication

app, within_mantid = get_qapplication()
show_gui()
if not within_mantid:
    sys.exit(app.exec_())
