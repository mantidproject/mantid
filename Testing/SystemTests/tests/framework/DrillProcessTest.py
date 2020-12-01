# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# from Interface import *

import systemtesting
import sys

from qtpy.QtWidgets import QApplication

from mantid.kernel import config
from Interface.ui.drill.view.DrillView import *


app = QApplication(sys.argv)


class DrillProcessTest(systemtesting.MantidSystemTest):

    def __init__(self):
        super().__init__()

    def validate(self):
        return ['', '']

    def runTest(self):
        config['default.facility'] = 'ILL'
        config['default.instrument'] = 'D11'

        drill = DrillView()
        drill.close()
