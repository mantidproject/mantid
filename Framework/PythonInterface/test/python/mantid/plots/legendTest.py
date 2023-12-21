# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#

import unittest
from unittest import mock
import matplotlib.pyplot as plt

from mantid.plots import MantidAxes
from mantid.plots.legend import LegendProperties

LEGEND_OPTIONS = {"plots.legend.Location": "best", "plots.legend.FontSize": "8.0"}


class MockConfigService(object):
    def __init__(self):
        self.getString = mock.Mock(side_effect=LEGEND_OPTIONS.get)


class LegendTest(unittest.TestCase):
    def test_get_properties_from_legend_returns_none_if_legend_has_no_entries(self):
        legend = plt.legend()

        props = LegendProperties.from_legend(legend)

        self.assertEqual(props, None)

    def test_calling_create_legend_with_no_props_adds_legend_to_plot(self):
        ax = mock.Mock(spec=MantidAxes)
        ax.lines = [mock.Mock()]
        ax.figure = mock.Mock()

        LegendProperties.create_legend(props=None, ax=ax)

        ax.legend.assert_called_once()

    @mock.patch("mantid.plots.legend.ConfigService", new_callable=MockConfigService)
    def test_calling_create_legend_with_no_props_uses_config_values_in_legend(self, mock_ConfigService):
        ax = mock.Mock(spec=MantidAxes)
        ax.lines = [mock.Mock()]
        ax.figure = mock.Mock()

        LegendProperties.create_legend(props=None, ax=ax)

        mock_ConfigService.getString.assert_has_calls([mock.call("plots.legend.Location"), mock.call("plots.legend.FontSize")])
        ax.legend.assert_called_once_with(handles=mock.ANY, loc="best", prop={"size": 8.0})


if __name__ == "__main__":
    unittest.main()
