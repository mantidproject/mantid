# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#

import unittest

import matplotlib.pyplot as plt

from mantid.plots.legend import LegendProperties


class LegendTest(unittest.TestCase):
    def test_get_properties_from_legend_returns_none_if_legend_has_no_entries(self):
        legend = plt.legend()

        props = LegendProperties.from_legend(legend)

        self.assertEqual(props, None)

    def test_calling_create_legend_with_no_props_adds_legend_to_plot(self):
        ax = plt.gca()

        LegendProperties.create_legend(props=None, ax=ax)

        self.assertNotEqual(ax.get_legend(), None)


if __name__ == '__main__':
    unittest.main()