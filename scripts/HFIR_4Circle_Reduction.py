# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#pylint: disable=invalid-name
from __future__ import (absolute_import, division, print_function)
import sys
from mantidqt.gui_helper import set_matplotlib_backend, get_qapplication
set_matplotlib_backend()  # must be called before anything tries to use matplotlib
from HFIR_4Circle_Reduction import reduce4circleGUI  # noqa

app, within_mantid = get_qapplication()
reducer = reduce4circleGUI.MainWindow()  # the main ui class in this file
reducer.show()
if not within_mantid:
    sys.exit(app.exec_())
