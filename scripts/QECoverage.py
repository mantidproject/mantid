# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import sys
from mantidqt.gui_helper import get_qapplication
from QECoverage.QECoverageGUI import QECoverageGUI

app, within_mantid = get_qapplication()
mainForm = QECoverageGUI()
mainForm.show()
if not within_mantid:
    sys.exit(app.exec_())
