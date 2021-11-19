# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantidqt package
#
#
import unittest

from mantid.simpleapi import CreateSampleWorkspace
from mantidqt.widgets.instrumentview.presenter import InstrumentViewManager
from mantidqt.widgets.instrumentview.api import get_instrumentview

from mantidqt.utils.qt.testing import start_qapplication


@start_qapplication
class InstrumentViewManagerTest(unittest.TestCase):

    def test_manager_removes_instrument_view_on_close(self):
        """
        Check that the instrument view manager removes the dictionary entry for a workspace
        when the widget is closed directly, without using the workspace name.
        """
        ws = CreateSampleWorkspace()
        i = get_instrumentview(ws)
        self.assertEqual(len(InstrumentViewManager.view_dict), 1)
        i.show_view()
        # Close the instrument view widget directly without using ws name.
        i.container.close()
        # Manager should have removed the instrument view from the dictionary.
        self.assertEqual(len(InstrumentViewManager.view_dict), 0)


if __name__ == '__main__':
    unittest.main()
