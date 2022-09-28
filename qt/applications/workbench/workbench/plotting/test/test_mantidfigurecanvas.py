# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
# NScD Oak Ridge National Laboratory, European Spallation Source,
# Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

import unittest
from unittest.mock import MagicMock
from workbench.plotting.mantidfigurecanvas import MantidFigureCanvas


class MantidFigureCanvasTest(unittest.TestCase):

    def test_dpi_attribute_exists(self):
        fig = MagicMock()
        fig.bbox.max = [1, 1]
        canvas = MantidFigureCanvas(fig)

        self.assertTrue(hasattr(canvas, '_dpi_ratio'))


if __name__ == "__main__":
    unittest.main()
