# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from Muon.GUI.Common.plot_widget.raw_pane.base_pane_model import RawPaneModel
from Muon.GUI.Common.test_helpers.context_setup import setup_context
from mantidqt.utils.qt.testing import start_qapplication


@start_qapplication
class RawPaneModelTest(unittest.TestCase):

    def setUp(self):
        self.model = RawPaneModel(context=setup_context(False))


if __name__ == '__main__':
    unittest.main(buffer=False, verbosity=2)
