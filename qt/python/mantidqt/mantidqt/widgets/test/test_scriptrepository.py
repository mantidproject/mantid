# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.
#
#


import unittest

from mantidqt.widgets.scriptrepository import ScriptRepositoryView
from mantidqt.utils.qt.testing import start_qapplication


@start_qapplication
class ScriptRepositoryTest(unittest.TestCase):
    """Minimal testing as it is exported from C++"""

    def test_widget_creation(self):
        display = ScriptRepositoryView()
        self.assertNotEqual(display, None)


if __name__ == "__main__":
    unittest.main()
