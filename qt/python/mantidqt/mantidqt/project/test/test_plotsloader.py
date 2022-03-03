# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantidqt package
#
import unittest  # noqa
from unittest import mock  # noqa

import matplotlib
import matplotlib.axis
import matplotlib.pyplot as plt  # noqa
import matplotlib.figure  # noqa
import matplotlib.text  # noqa
from matplotlib.ticker import LogFormatterSciNotation, ScalarFormatter, NullFormatter
matplotlib.use('AGG')

from mantidqt.project.plotsloader import PlotsLoader  # noqa
import mantid.plots.axesfunctions  # noqa
from mantid.api import AnalysisDataService as ADS  # noqa
from mantid.dataobjects import Workspace2D  # noqa


def pass_func():
    pass


class PlotsLoaderTest(unittest.TestCase):
    def setUp(self):
        self.plots_loader = PlotsLoader()
        plt.plot = mock.MagicMock()
        mantid.plots.axesfunctions.plot = mock.MagicMock()
        self.dictionary = {u'legend': {u'exists': False}, u'lines': [],
                           u'properties': {u'axisOn': True, u'bounds': (0.0, 0.0, 0.0, 0.0), u'dynamic': True,
                                           u'frameOn': True, u'visible': True, u'facecolor': None,
                                           u'xAxisProperties': {u'fontSize': 10.0,
                                                                u'gridStyle': {u'gridOn': False},
                                                                u'majorTickFormat': None,
                                                                u'majorTickFormatter': 'ScalarFormatter',
                                                                u'majorTickLocator': 'AutoLocator',
                                                                u'majorTickLocatorValues': None,
                                                                u'minorTickFormat': None,
                                                                u'minorTickFormatter': 'NullFormatter',
                                                                u'minorTickLocator': 'NullLocator',
                                                                u'minorTickLocatorValues': None,
                                                                u'visible': True},
                                           u'xAxisScale': u'linear', u'xLim': (0.0, 1.0),
                                           u'yAxisProperties': {u'fontSize': 10.0,
                                                                u'gridStyle': {u'gridOn': False},
                                                                u'majorTickFormat': None,
                                                                u'majorTickFormatter': 'ScalarFormatter',
                                                                u'majorTickLocator': 'AutoLocator',
                                                                u'majorTickLocatorValues': None,
                                                                u'minorTickFormat': None,
                                                                u'minorTickFormatter': 'NullFormatter',
                                                                u'minorTickLocator': 'NullLocator',
                                                                u'minorTickLocatorValues': None,
                                                                u'visible': True},
                                           u'yAxisScale': u'linear', u'yLim': (0.0, 1.0), u'showMinorGrid': False,
                                           u"xAutoScale": False, u"yAutoScale": False,
                                           u'tickParams': {
                                               'xaxis': {
                                                   'major': {
                                                       'bottom': True,
                                                       'top': True,
                                                       'labelbottom': True,
                                                       'labeltop': True,
                                                       'direction': 'inout',
                                                       'width': 1,
                                                       'size': 6},
                                                   'minor': {
                                                       'bottom': True,
                                                       'top': True,
                                                       'labelbottom': True,
                                                       'labeltop': True,
                                                       'direction': 'inout',
                                                       'width': 1,
                                                       'size': 3}},
                                               'yaxis': {
                                                   'major': {
                                                       'left': True,
                                                       'right': True,
                                                       'labelleft': True,
                                                       'labelright': True,
                                                       'direction': 'inout',
                                                       'width': 1, 'size': 6},
                                                   'minor': {
                                                       'left': True,
                                                       'right': True,
                                                       'labelleft': True,
                                                       'labelright': True,
                                                       'direction': 'inout',
                                                       'width': 1,
                                                       'size': 3}}},
                                           u'spineWidths': {'left': 0.4, 'right': 0.4, 'bottom': 0.4, 'top': 0.4}},
                           u'textFromArtists': {}, u'texts': [], u'title': u'', u'xAxisTitle': u'', u'yAxisTitle': u''}

    def test_load_plots_does_the_right_calls(self):
        self.plots_loader.make_fig = mock.MagicMock()
        self.plots_loader.load_plots(["plot1", "plot2"])

        self.assertEqual(self.plots_loader.make_fig.call_count, 2)

    @mock.patch("matplotlib.figure.Figure.show")
    def test_make_fig_makes_the_right_calls(self, pass_func):
        ws = Workspace2D()
        ADS.add("ws", ws)
        plot_dict = {"label": "plot", "creationArguments": [[
            {"workspaces": "ws", "wkspIndex": 0},
            {"function": "axhline", "args": [10, 0, 1], "kwargs": {}},
            {"function": "axvline", "args": [], "kwargs": {"x": 0, "ymin": 0, "ymax": 1}}
        ]]}
        self.plots_loader.workspace_plot_func = mock.MagicMock()
        self.plots_loader.plot_func = mock.MagicMock()
        self.plots_loader.restore_figure_data = mock.MagicMock()

        self.plots_loader.make_fig(plot_dict)

        self.assertEqual(self.plots_loader.workspace_plot_func.call_count, 1)
        self.assertEqual(self.plots_loader.plot_func.call_count, 2)
        self.assertEqual(self.plots_loader.restore_figure_data.call_count, 1)

    def test_restore_fig_properties(self):
        matplotlib.figure.Figure.set_figheight = mock.MagicMock()
        matplotlib.figure.Figure.set_figwidth = mock.MagicMock()
        matplotlib.figure.Figure.set_dpi = mock.MagicMock()
        self.plots_loader.restore_fig_properties(matplotlib.figure.Figure(), {"figHeight": 1, "figWidth": 1, "dpi": 1})

        self.assertEqual(matplotlib.figure.Figure.set_figheight.call_count, 1)
        self.assertEqual(matplotlib.figure.Figure.set_figwidth.call_count, 1)
        self.assertEqual(matplotlib.figure.Figure.set_dpi.call_count, 1)

    def test_restore_fig_axes(self):
        self.plots_loader.update_properties = mock.MagicMock()
        self.plots_loader.update_lines = mock.MagicMock()
        self.plots_loader.create_text_from_dict = mock.MagicMock()
        self.plots_loader.update_legend = mock.MagicMock()

        fig = matplotlib.figure.Figure()
        self.plots_loader.restore_fig_axes(matplotlib.axes.Axes(fig=fig, rect=[0, 0, 0, 0]), self.dictionary)

        self.assertEqual(self.plots_loader.update_properties.call_count, 1)
        self.assertEqual(self.plots_loader.update_lines.call_count, 0)
        self.assertEqual(self.plots_loader.create_text_from_dict.call_count, 0)
        self.assertEqual(self.plots_loader.update_legend.call_count, 1)

    def test_update_properties_limits(self):
        dic = self.dictionary[u"properties"]
        dic.pop("spineWidths", None)  # Not needed for this test and causes error on mock.
        mock_ax = mock.Mock()

        plots_loader = self.plots_loader
        with mock.patch.object(plots_loader, "update_axis", mock.Mock()):
            plots_loader.update_properties(mock_ax, dic)

        mock_ax.set_xlim.assert_called_once_with(dic['xLim'])
        mock_ax.set_xlim.assert_called_once_with(dic['yLim'])

    def test_update_properties_limits_autoscale(self):
        dic = self.dictionary[u"properties"]
        dic.pop("spineWidths", None)  # Not needed for this test and causes error on mock.
        dic.update({"xAutoScale": True, "yAutoScale": True})
        mock_ax = mock.Mock()

        plots_loader = self.plots_loader
        with mock.patch.object(plots_loader, "update_axis", mock.Mock()):
            plots_loader.update_properties(mock_ax, dic)

        mock_ax.autoscale.assert_has_calls([mock.call(True, axis="x"), mock.call(True, axis="y")])
        mock_ax.set_xlim.assert_not_called()
        mock_ax.set_xlim.assert_not_called()

    def test_update_properties_sets_tick_params_and_spine_widths(self):
        dic = self.dictionary[u"properties"]
        mock_ax = mock.Mock()
        mock_ax.spines = {
            "left": mock.Mock(),
            "right": mock.Mock(),
            "top": mock.Mock(),
            "bottom": mock.Mock(),
        }

        plots_loader = self.plots_loader
        with mock.patch.object(plots_loader, "update_axis", mock.Mock()):
            plots_loader.update_properties(mock_ax, dic)

        mock_ax.xaxis.set_tick_params.assert_has_calls([
            mock.call(which="major", **dic["tickParams"]["xaxis"]["major"]),
            mock.call(which="minor", **dic["tickParams"]["xaxis"]["minor"])
        ])
        mock_ax.yaxis.set_tick_params.assert_has_calls([
            mock.call(which="major", **dic["tickParams"]["yaxis"]["major"]),
            mock.call(which="minor", **dic["tickParams"]["yaxis"]["minor"])
        ])

        for (spine_name, mock_spine) in mock_ax.spines.items():
            mock_spine.set_linewidth.assert_called_with(dic["spineWidths"][spine_name])

    def test_create_text_from_dict(self):
        fig = matplotlib.figure.Figure()
        ax = matplotlib.axes.Axes(fig=fig, rect=[0, 0, 0, 0])
        ax.text = mock.MagicMock()

        self.plots_loader.create_text_from_dict(ax=ax, dic={"text": "text", "position": (1, 1), "useTeX": 1,
                                                            "style": {"alpha": 1, "textSize": 1, "color": 1,
                                                                      "hAlign": 1, "vAlign": 1, "rotation": 1,
                                                                      "zOrder": 1}})
        self.assertEqual(ax.text.call_count, 1)
        ax.text.assert_called_once_with(fontdict={u'zorder': 1, u'fontsize': 1, u'color': 1, u'alpha': 1,
                                                  u'rotation': 1, u'verticalalignment': 1, u'usetex': 1,
                                                  u'horizontalalignment': 1}, s=u'text', x=1, y=1)

    @mock.patch("matplotlib.figure.Figure.show")
    def test_load_plot_from_dict(self, pass_func):
        # The fact this runs is the test
        self.plots_loader.load_plots([self.dictionary])

    def test_update_axis_ticks_format(self):
        fig, ax = plt.subplots()
        x_axis = ax.xaxis
        x_axis.set_major_formatter(LogFormatterSciNotation())
        x_axis.set_minor_formatter(LogFormatterSciNotation())
        PlotsLoader.update_axis_ticks(x_axis, self.dictionary['properties']['xAxisProperties'])

        self.assertIsInstance(x_axis.get_major_formatter(), ScalarFormatter)
        self.assertIsInstance(x_axis.get_minor_formatter(), NullFormatter)

    @mock.patch("matplotlib.colors.LogNorm", autospec=True)
    def test_restore_normalise_obj_from_dict_creates_correct_norm_instance_from_supported_norm(self, mock_LogNorm):
        norm_dict = {'type': 'LogNorm', 'vmin': 1, 'vmax': 2, 'clip': True}

        _ = self.plots_loader.restore_normalise_obj_from_dict(norm_dict)

        mock_LogNorm.assert_called_once_with(norm_dict['vmin'], norm_dict['vmax'], norm_dict['clip'])

    def test_restore_normalise_obj_from_dict_returns_none_with_unsupported_norm(self):
        norm_dict = {'type': 'unsupported_norm', 'vmin': 1, 'vmax': 2, 'clip': True}

        return_value = self.plots_loader.restore_normalise_obj_from_dict(norm_dict)

        self.assertIsNone(return_value)

    @mock.patch("matplotlib.colors.Normalize", autospec=True)
    def test_restore_normalise_obj_from_dict_returns_Normalize_type_with_unspecified_norm(self, mock_Normalize):
        """If the type of the norm is unspecified, the method should return a Normalize object,
        which is the most general norm and is subclassed by LogNorm, etc."""
        norm_dict = {'vmin': 1, 'vmax': 2, 'clip': True}

        _ = self.plots_loader.restore_normalise_obj_from_dict(norm_dict)

        mock_Normalize.assert_called_once_with(norm_dict['vmin'], norm_dict['vmax'], norm_dict['clip'])

    def test_update_properties_without_spineWidths(self):
        # Test that old versions of the .mtdproj file that don't include spine widths
        # still load. The fact this runs is the test
        props = self.dictionary['properties']
        props.pop("spineWidths")
        mock_ax = mock.Mock()

        plots_loader = self.plots_loader
        with mock.patch.object(plots_loader, "update_axis", mock.Mock()):
            plots_loader.update_properties(mock_ax, props)

    def test_update_axis_with_old_tick_position(self):
        # Test that old versions of the .mtdproj file that represented which side of the
        # plot had ticks differently.
        mock_xaxis = mock.MagicMock(spec=matplotlib.axis.XAxis)
        mock_yaxis = mock.MagicMock(spec=matplotlib.axis.YAxis)
        mock_ax = mock.MagicMock()
        mock_ax.xaxis = mock_xaxis
        mock_ax.yaxis = mock_yaxis
        dic = self.dictionary["properties"]
        dic.pop("tickParams")
        dic["xAxisProperties"]["position"] = "top"
        dic["yAxisProperties"]["position"] = "right"

        self.plots_loader.update_properties(mock_ax, dic)

        mock_ax.set_tick_params.assert_not_called()
        mock_xaxis.tick_top.assert_called()
        mock_yaxis.tick_right.assert_called()


if __name__ == "__main__":
    unittest.main()
