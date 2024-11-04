# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

from unittest import mock
from mantidqtinterfaces.MultiPlotting.subplot.subplot_ADS_observer import SubplotADSObserver


class SubplotADSObserverTest(unittest.TestCase):
    def setUp(self):
        self.subplot = mock.Mock()
        self.obs = SubplotADSObserver(self.subplot)

    def test_delete(self):
        self.subplot._rm_ws_from_plots = mock.Mock()
        self.obs.deleteHandle("test", mock.Mock())

        self.assertEqual(self.subplot._rm_ws_from_plots.call_count, 1)
        self.subplot._rm_ws_from_plots.assert_called_with("test")

    def test_replace(self):
        self.subplot._replaced_ws = mock.Mock()
        ws = mock.Mock()
        self.obs.replaceHandle("test", ws)

        self.assertEqual(self.subplot._replaced_ws.call_count, 1)
        self.subplot._replaced_ws.assert_called_with(ws)


if __name__ == "__main__":
    unittest.main()
