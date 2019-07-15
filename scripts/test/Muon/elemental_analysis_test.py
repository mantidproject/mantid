from __future__ import absolute_import, print_function

import unittest
import matplotlib

from mantid.py3compat import mock
from Muon.GUI.ElementalAnalysis.elemental_analysis import ElementalAnalysisGui


class ElementalAnalysisTest(unittest.TestCase):
    def setup(self):
        self.gui = ElementalAnalysisGui()


    def test_that_color_cycle_is_matplotlib_default_one(self):
        number_colors = len(matplotlib.rcParams['axes.prop_cycle'])

        self.assertEqual(self.gui.num_colors, number_colors)



if __name__ == '__main__':
    unittest.main()