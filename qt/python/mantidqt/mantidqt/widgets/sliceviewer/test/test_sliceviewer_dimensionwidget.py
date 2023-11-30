# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2023 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

import unittest
from typing import Dict

from mantidqt.widgets.sliceviewer.views.dimensionwidget import DimensionWidget
from mantidqt.utils.qt.testing import start_qapplication


@start_qapplication
class DimensionWidgetTest(unittest.TestCase):
    def setUp(self):
        dimensions = ["H", "K", "L", "E"]
        self.dim_info = [self.generate_dim_info(d) for d in dimensions]

    def generate_dim_info(self, dimension: str) -> Dict:
        return {
            "minimum": -1.0,
            "maximum": 1.0,
            "number_of_bins": 1,
            "width": 2.0,
            "name": dimension,
            "units": "r.l.u.",
            "type": "MDE",
            "can_rebin": True,
            "qdim": True,
        }

    def test_spinbox_updated_for_slicepoint_outside_range(self):
        dimension_widget = DimensionWidget(self.dim_info)
        spinbox = dimension_widget.dims[2].spinbox
        self.assertEqual(spinbox.value(), 0.0)
        dimension_widget.set_slicepoint([None, None, 3.0, 0.0])
        self.assertEqual(spinbox.value(), 3.0)
        self.assertEqual(spinbox.minimum(), -0.99)
        self.assertEqual(spinbox.maximum(), 3.0)


if __name__ == "__main__":
    unittest.main()
