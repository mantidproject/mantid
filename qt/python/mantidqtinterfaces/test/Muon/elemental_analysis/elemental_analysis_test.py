# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import copy
import unittest

from unittest import mock
from mantidqt.utils.qt.testing import start_qapplication
from testhelpers import assertRaisesNothing

from qtpy.QtGui import QCloseEvent

import matplotlib
from mantidqtinterfaces.Muon.GUI.ElementalAnalysis.elemental_analysis import ElementalAnalysisGui
from mantidqtinterfaces.Muon.GUI.ElementalAnalysis.LoadWidget.load_utils import spectrum_index
from mantidqtinterfaces.Muon.GUI.ElementalAnalysis.elemental_analysis import gen_name
from mantidqtinterfaces.MultiPlotting.multi_plotting_widget import MultiPlotWindow
from mantidqtinterfaces.MultiPlotting.multi_plotting_widget import MultiPlotWidget
from mantidqtinterfaces.MultiPlotting.label import Label


@start_qapplication
class ElementalAnalysisTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        super(ElementalAnalysisTest, cls).setUpClass()
        cls.gui = ElementalAnalysisGui()

    @classmethod
    def tearDownClass(cls):
        cls.gui = None

    def setUp(self):
        self.gui.plot_window = None
        self.gui.used_colors = {}
        self.gui.element_lines = {}
        self.has_raise_ValueError_been_called_once = False
        self.gui.detectors = mock.Mock()
        self.gui.detectors.detectors = [mock.Mock(), mock.Mock(), mock.Mock(), mock.Mock()]
        self.gui.lines = mock.Mock()
        self.gui.ptable.set_peak_datafile = mock.Mock()

    def raise_ValueError_once(self):
        if not self.has_raise_ValueError_been_called_once:
            self.has_raise_ValueError_been_called_once = True
            raise ValueError()

    def test_that_get_color_returns_C0_as_first_color(self):
        self.assertEqual("C0", self.gui.get_color("bla"))

    def test_that_default_colour_cycle_is_used(self):
        cycle = len(matplotlib.rcParams["axes.prop_cycle"])
        self.assertEqual(cycle, self.gui.num_colors)

        expected = ["C%d" % i for i in range(cycle)]
        returned = [self.gui.get_color("el_{}".format(i)) for i in range(cycle)]
        self.assertEqual(expected, returned)

    def test_that_get_color_returns_the_same_color_for_the_same_element(self):
        self.assertEqual(self.gui.used_colors, {})

        colors = [self.gui.get_color("Cu") for _ in range(10)]
        colors += [self.gui.get_color("Fe") for _ in range(10)]
        colors = sorted(list(set(colors)))

        self.assertEqual(colors, ["C0", "C1"])

    def test_that_get_color_returns_a_color_if_it_has_been_freed(self):
        elements = ["Cu", "Fe", "Ni"]
        colors = [self.gui.get_color(el) for el in elements]

        self.assertEqual(colors, ["C0", "C1", "C2"])

        del self.gui.used_colors["Fe"]

        new_col = self.gui.get_color("O")
        self.assertEqual(new_col, "C1")

    def test_that_get_color_wraps_around_when_all_colors_in_cycle_have_been_used(self):
        [self.gui.get_color(i) for i in range(self.gui.num_colors)]

        self.assertEqual(self.gui.get_color("Cu"), "C0")
        self.assertEqual(self.gui.get_color("Fe"), "C1")

    def test_that_closing_with_no_plot_will_not_throw(self):
        self.gui.plot_window = None

        assertRaisesNothing(self, self.gui.closeEvent, QCloseEvent())

    def test_that_closing_with_a_plot_will_close_the_window(self):
        self.gui.plot_window = mock.create_autospec(MultiPlotWindow)
        self.gui.closeEvent(QCloseEvent())

        self.assertEqual(self.gui.plot_window.closeEvent.call_count, 1)

    def test_that_gen_label_does_not_throw_with_non_float_x_values(self):
        assertRaisesNothing(self, self.gui._gen_label, "name", "not_a_float", "Cu")
        assertRaisesNothing(self, self.gui._gen_label, "name", "not_a_float", "Cu")
        assertRaisesNothing(self, self.gui._gen_label, "name", None, "Cu")
        assertRaisesNothing(self, self.gui._gen_label, "name", ("not", "a", "float"), "Cu")
        assertRaisesNothing(self, self.gui._gen_label, "name", ["not", "a", "float"], "Cu")

    def test_gen_label_output_is_Label_type(self):
        name = "string"
        x_value_in = 1.0
        self.gui.element_lines["H"] = []
        gen_label_output = self.gui._gen_label(name, x_value_in, element="H")
        self.assertEqual(Label, type(gen_label_output))

    def test_that_gen_label_with_element_none_returns_none(self):
        self.assertEqual(self.gui._gen_label("label", 0.0), None)

    @mock.patch("mantidqtinterfaces.Muon.GUI.ElementalAnalysis.elemental_analysis.Label")
    def test_that_gen_label_name_is_appended_to_list_if_not_present(self, mock_label):
        element = "Cu"
        name = "new_label"
        self.gui.element_lines = {"Cu": ["old_label"]}
        self.gui._gen_label(name, 1.0, element)

        self.assertIn(name, self.gui.element_lines[element])
        mock_label.assert_called_with(str(name), 1.0, False, 0.9, True, rotation=-90, protected=True)

    @mock.patch("mantidqtinterfaces.Muon.GUI.ElementalAnalysis.elemental_analysis.Label")
    def test_that_gen_label_name_is_not_duplicated_in_list_if_already_present(self, mock_label):
        element = "Cu"
        name = "old_label"
        self.gui.element_lines = {"Cu": ["old_label"]}
        self.gui._gen_label(name, 1.0, element)

        self.assertIn(name, self.gui.element_lines[element])
        self.assertEqual(self.gui.element_lines[element].count(name), 1)
        mock_label.assert_called_with(str(name), 1.0, False, 0.9, True, rotation=-90, protected=True)

    @mock.patch("mantidqtinterfaces.Muon.GUI.ElementalAnalysis.elemental_analysis.ElementalAnalysisGui._gen_label")
    @mock.patch("mantidqtinterfaces.Muon.GUI.ElementalAnalysis.elemental_analysis.ElementalAnalysisGui._plot_line_once")
    def test_that_plot_line_returns_if_plot_window_is_none(self, mock_plot_line_once, mock_gen_label):
        self.gui.plot_window = None
        mock_gen_label.return_value = "name of the label"
        self.gui._plot_line("name", 1.0, "C0", None)

        self.assertEqual(mock_plot_line_once.call_count, 0)

    @mock.patch("mantidqtinterfaces.Muon.GUI.ElementalAnalysis.elemental_analysis.ElementalAnalysisGui._gen_label")
    @mock.patch("mantidqtinterfaces.Muon.GUI.ElementalAnalysis.elemental_analysis.ElementalAnalysisGui._plot_line_once")
    def test_that_plot_line_calls_plot_line_once_if_window_not_none(self, mock_plot_line_once, mock_gen_label):
        self.gui.plot_window = mock.create_autospec(MultiPlotWindow)
        self.gui.plotting = MultiPlotWidget(mock.Mock())
        self.gui.plotting.get_subplots = mock.Mock(return_value=["plot1"])
        mock_gen_label.return_value = "name of the label"
        self.gui._plot_line("name", 1.0, "C0", None)

        self.assertEqual(mock_plot_line_once.call_count, 1)
        mock_plot_line_once.assert_called_with("plot1", 1.0, "name of the label", "C0")

    def test_plot_line_once_calls_correct_multiplot_function(self):
        self.gui.plotting = mock.create_autospec(MultiPlotWidget)
        self.gui._plot_line_once("GE1", 1.0, "label", "C0")

        self.assertEqual(self.gui.plotting.add_vline_and_annotate.call_count, 1)

    def test_that_rm_line_returns_if_plot_window_is_none(self):
        self.gui.plotting = MultiPlotWidget(mock.Mock())
        self.gui.plotting.get_subplots = mock.Mock(return_value=["plot1", "plot2", "plot3"])
        self.gui.plot_window = None

        self.gui._rm_line("line")

        self.assertEqual(self.gui.plotting.get_subplots.call_count, 0)

    def test_that_rm_line_calls_correct_function_if_window_not_none(self):
        self.gui.plotting = MultiPlotWidget(mock.Mock())
        self.gui.plotting.get_subplots = mock.Mock(return_value=["plot1", "plot2", "plot3"])
        self.gui.plotting.rm_vline_and_annotate = mock.Mock()
        self.gui.plot_window = mock.create_autospec(MultiPlotWindow)
        self.gui._rm_line("line")

        self.assertEqual(self.gui.plotting.get_subplots.call_count, 1)
        self.assertEqual(self.gui.plotting.rm_vline_and_annotate.call_count, 3)
        self.gui.plotting.rm_vline_and_annotate.assert_called_with("plot3", "line")

    def test_that_generate_element_widgets_creates_widget_once_for_each_element(self):
        elem = len(self.gui.ptable.peak_data)
        self.assertEqual(len(self.gui.element_widgets), elem)

    @mock.patch("mantidqtinterfaces.Muon.GUI.ElementalAnalysis.elemental_analysis.ElementalAnalysisGui._add_element_lines")
    def test_table_left_clicked_adds_lines_if_element_selected(self, mock_add_element_lines):
        self.gui.ptable.is_selected = mock.Mock(return_value=True)
        self.gui._add_element_lines(mock.Mock())
        self.assertEqual(mock_add_element_lines.call_count, 1)

    @mock.patch("mantidqtinterfaces.Muon.GUI.ElementalAnalysis.elemental_analysis.ElementalAnalysisGui._remove_element_lines")
    def test_table_left_clicked_removed_lines_if_element_not_selected(self, mock_remove_element_lines):
        self.gui.ptable.is_selected = mock.Mock(return_value=False)
        self.gui.table_left_clicked(mock.Mock())
        self.assertEqual(mock_remove_element_lines.call_count, 1)

    @mock.patch("mantidqtinterfaces.Muon.GUI.ElementalAnalysis.elemental_analysis.ElementalAnalysisGui.get_color")
    def test_that_add_element_lines_will_call_get_color(self, mock_get_color):
        self.gui._add_element_lines("Cu")
        self.assertEqual(mock_get_color.call_count, 1)

    @mock.patch("mantidqtinterfaces.Muon.GUI.ElementalAnalysis.elemental_analysis.ElementalAnalysisGui._plot_line")
    def test_that_add_element_lines_will_call_plot_line(self, mock_plot_line):
        data = {"line1": 10.0, "line2": 20.0, "line3": 30.0}
        self.gui._add_element_lines("Cu", data)
        self.assertEqual(mock_plot_line.call_count, 3)
        call_list = [
            mock.call(gen_name("Cu", "line3"), 30.0, "C0", "Cu"),
            mock.call(gen_name("Cu", "line2"), 20.0, "C0", "Cu"),
            mock.call(gen_name("Cu", "line1"), 10.0, "C0", "Cu"),
        ]
        mock_plot_line.assert_has_calls(call_list, any_order=True)

    @mock.patch("mantidqtinterfaces.Muon.GUI.ElementalAnalysis.elemental_analysis.ElementalAnalysisGui._rm_line")
    def test_remove_element_lines_does_nothing_if_element_not_in_element_lines(self, mock_rm_line):
        self.gui.element_lines["H"] = ["alpha", "beta"]
        self.gui._remove_element_lines("He")
        self.assertEqual(mock_rm_line.call_count, 0)

    @mock.patch("mantidqtinterfaces.Muon.GUI.ElementalAnalysis.elemental_analysis.ElementalAnalysisGui._rm_line")
    def test_remove_element_lines_removes_all_values_for_a_given_element(self, mock_rm_line):
        self.gui.element_lines["H"] = ["alpha", "beta"]
        self.gui._remove_element_lines("H")
        self.assertEqual(mock_rm_line.call_count, 2)

    @mock.patch("mantidqtinterfaces.Muon.GUI.ElementalAnalysis.elemental_analysis.MultiPlotWindow")
    def test_load_run_opens_new_plot_window_if_none_open(self, mock_multi_plot_window):
        self.gui.add_detector_to_plot = mock.Mock()
        self.gui.load_run("GE1", "2695")
        self.assertEqual(mock_multi_plot_window.call_count, 1)

    @mock.patch("mantidqtinterfaces.Muon.GUI.ElementalAnalysis.elemental_analysis.MultiPlotWindow")
    def test_load_run_does_not_open_new_plot_window_if_one_is_open(self, mock_multi_plot_window):
        self.gui.add_detector_to_plot = mock.Mock()
        self.gui.plot_window = MultiPlotWindow(str("2695"))
        self.gui.load_run("GE1", 2695)
        self.assertEqual(mock_multi_plot_window.call_count, 0)

    @mock.patch("mantidqtinterfaces.Muon.GUI.ElementalAnalysis.Detectors.detectors_view.QtWidgets.QWidget")
    def test_loading_finished_returns_nothing_if_no_run_loaded(self, mock_qwidget):
        self.gui.load_widget.last_loaded_run = mock.Mock(return_value=None)
        mock_qwidget.return_value = True
        self.gui.plot_window = mock.Mock()
        self.gui.plotting = mock.Mock()

        self.gui.loading_finished()
        self.assertEqual(self.gui.plotting.remove_subplot.call_count, 0)

    @mock.patch("mantidqtinterfaces.Muon.GUI.ElementalAnalysis.Detectors.detectors_view.QtWidgets.QWidget")
    def test_loading_finished_returns_correctly_if_no_plot_window_but_has_to_plot(self, mock_qwidget):
        self.gui.load_widget.last_loaded_run = mock.Mock(return_value=2695)
        self.gui.load_widget.get_run_num_loaded_detectors = mock.Mock(return_value=4)
        self.gui.detectors.getNames.return_value = ["1", "2", "3"]
        self.gui.plot_window = None
        self.gui.plotting = mock.Mock()
        self.gui.plotting.get_subplots.return_value = ["1", "2", "3"]
        mock_qwidget.return_value = True

        self.gui.loading_finished()
        self.assertEqual(self.gui.detectors.setStateQuietly.call_count, 3)
        for detector in self.gui.detectors.detectors:
            self.assertEqual(detector.setChecked.call_count, 1)

    @mock.patch("mantidqtinterfaces.Muon.GUI.ElementalAnalysis.Detectors.detectors_view.QtWidgets.QWidget")
    def test_loading_finished_returns_correctly_if_no_to_plot_but_has_plot_window(self, mock_qwidget):
        self.gui.load_widget.last_loaded_run = mock.Mock(return_value=2695)
        self.gui.load_widget.get_run_num_loaded_detectors = mock.Mock(return_value=4)
        self.gui.detectors.getNames.return_value = ["1", "2", "3"]
        mock_qwidget.return_value = True
        self.gui.plot_window = mock.Mock()

        self.gui.loading_finished()
        self.assertEqual(self.gui.plotting.remove_subplot.call_count, 3)

    @mock.patch("mantidqtinterfaces.Muon.GUI.ElementalAnalysis.Detectors.detectors_view.QtWidgets.QWidget")
    def test_loading_finished_correctly_disables_detectors_if_less_detectors_are_loaded(self, mock_qwidget):
        num_loaded_detectors = 1
        num_detectors = 4
        self.gui.load_widget.last_loaded_run = mock.Mock(return_value=2695)
        self.gui.load_widget.get_run_num_loaded_detectors = mock.Mock(return_value=num_loaded_detectors)
        self.gui.detectors.getNames.return_value = ["1", "2", "3", "4"]
        self.gui.plotting.get_subplots.return_value = ["1", "2", "3", "4"]
        mock_qwidget.return_value = True
        self.gui.plot_window = mock.Mock()

        self.gui.loading_finished()
        # should have set the states of num_detectors - num_loaded_detectors
        self.assertEqual(self.gui.detectors.setStateQuietly.call_count, num_detectors - num_loaded_detectors)
        # should have only enabled the detector we have loaded
        self.assertEqual(self.gui.detectors.enableDetector.call_count, num_loaded_detectors)
        # Should disable (num_detectors - num_loaded_detectors) detectors
        self.assertEqual(self.gui.detectors.disableDetector.call_count, num_detectors - num_loaded_detectors)

    @mock.patch("mantidqtinterfaces.Muon.GUI.ElementalAnalysis.elemental_analysis.ElementalAnalysisGui.add_peak_data")
    @mock.patch("mantidqtinterfaces.Muon.GUI.ElementalAnalysis.elemental_analysis.mantid")
    def test_add_detectors_to_plot_plots_all_given_ws_and_all_selected_elements(self, mock_mantid, mock_add_peak_data):
        mock_mantid.mtd = {
            "name1": mock.Mock(),
            "name2": mock.Mock(),
        }
        self.gui.plotting = mock.Mock()
        self.gui.lines = mock.Mock()
        self.gui.lines.total.isChecked.return_value = True
        self.gui.lines.prompt.isChecked.return_value = False
        self.gui.lines.delayed.isChecked.return_value = True
        mock_mantid.mtd["name1"].name.return_value = "Detector 1"
        self.gui.add_detector_to_plot("GE1", "name1")
        self.assertEqual(self.gui.plotting.add_subplot.call_count, 1)
        self.assertEqual(self.gui.plotting.plot.call_count, 2)
        self.assertEqual(mock_add_peak_data.call_count, 0)

    def test_unset_detectors_resets_plot_window_and_detectors(self):
        self.gui.plot_window = mock.Mock()
        self.gui.detectors = mock.Mock()
        self.gui.detectors.getNames.return_value = ["name1", "name2", "name3"]
        self.gui._unset_detectors()
        self.assertEqual(self.gui.detectors.setStateQuietly.call_count, 3)
        self.assertEqual(self.gui.plot_window, None)

    @mock.patch("mantidqtinterfaces.Muon.GUI.ElementalAnalysis.elemental_analysis.ElementalAnalysisGui._gen_label")
    @mock.patch("mantidqtinterfaces.Muon.GUI.ElementalAnalysis.elemental_analysis.ElementalAnalysisGui._plot_line_once")
    def test_add_peak_data_plot_line_called_with_correct_terms(self, mock_plot_line_once, mock_gen_label):
        mock_subplot = mock.Mock()
        mock_gen_label.return_value = "label"
        test_data = {"name1": 1.0}
        self.gui.add_peak_data("H", mock_subplot, data=test_data)
        mock_plot_line_once.assert_called_with(mock_subplot, 1.0, "label", "C0")

    @mock.patch("mantidqtinterfaces.Muon.GUI.ElementalAnalysis.elemental_analysis.ElementalAnalysisGui._add_element_lines")
    @mock.patch("mantidqtinterfaces.Muon.GUI.ElementalAnalysis.elemental_analysis.ElementalAnalysisGui._remove_element_lines")
    def test_update_peak_data_element_is_selected(self, mock_remove_element_lines, mock_add_element_lines):
        self.gui.ptable.is_selected = mock.Mock(return_value=True)
        self.gui._update_peak_data("test_element")
        mock_remove_element_lines.assert_called_with("test_element")
        mock_add_element_lines.assert_called_with("test_element")

    @mock.patch("mantidqtinterfaces.Muon.GUI.ElementalAnalysis.elemental_analysis.ElementalAnalysisGui._add_element_lines")
    @mock.patch("mantidqtinterfaces.Muon.GUI.ElementalAnalysis.elemental_analysis.ElementalAnalysisGui._remove_element_lines")
    def test_update_peak_data_element_is_not_selected(self, mock_remove_element_lines, mock_add_element_lines):
        self.gui.ptable.is_selected = mock.Mock(return_value=False)
        self.gui._update_peak_data("test_element")
        mock_remove_element_lines.assert_called_with("test_element")
        self.assertEqual(mock_add_element_lines.call_count, 0)

    @mock.patch("mantidqtinterfaces.Muon.GUI.ElementalAnalysis.elemental_analysis.ElementalAnalysisGui.load_run")
    def test_add_plot_does_nothing_is_no_loaded_run(self, mock_load_run):
        self.gui.load_widget.last_loaded_run = mock.Mock(return_value=None)
        self.gui.add_plot(mock.Mock())
        self.assertEqual(mock_load_run.call_count, 0)

    @mock.patch("mantidqtinterfaces.Muon.GUI.ElementalAnalysis.elemental_analysis.ElementalAnalysisGui.add_peak_data")
    @mock.patch("mantidqtinterfaces.Muon.GUI.ElementalAnalysis.elemental_analysis.ElementalAnalysisGui.load_run")
    def test_add_plot_loads_run_and_electron_peaks_not_plotted(self, mock_load_run, add_peak_data):
        self.gui.load_widget.last_loaded_run = mock.Mock(return_value=2695)
        self.gui.peaks.electron.isChecked = mock.Mock(return_value=False)
        mock_checkbox = mock.Mock()
        mock_checkbox.name = "GE1"

        self.gui.add_plot(mock_checkbox)

        mock_load_run.assert_called_with("GE1", 2695)
        self.assertEqual(add_peak_data.call_count, 0)

    @mock.patch("mantidqtinterfaces.Muon.GUI.ElementalAnalysis.elemental_analysis.ElementalAnalysisGui.load_run")
    def test_add_plot_loads_run(self, mock_load_run):
        self.gui.load_widget.last_loaded_run = mock.Mock(return_value=2695)
        mock_checkbox = mock.Mock()
        mock_checkbox.name = "GE1"

        self.gui.add_plot(mock_checkbox)

        mock_load_run.assert_called_with("GE1", 2695)

    def test_del_plot_does_nothing_if_no_loaded_run(self):
        self.gui.load_widget.last_loaded_run = mock.Mock(return_value=None)
        self.gui.plotting.remove_subplot = mock.Mock()
        mock_checkbox = mock.Mock()
        mock_checkbox.name = "GE1"

        self.gui.del_plot(mock_checkbox)

        self.assertEqual(self.gui.plotting.remove_subplot.call_count, 0)

    def test_del_plot_removes_subplot_only_if_other_subplots_exist(self):
        self.gui.load_widget.last_loaded_run = mock.Mock(return_value=2695)
        self.gui.plotting.remove_subplot = mock.Mock()
        self.gui.plotting.get_subplots = mock.Mock(return_value=True)
        self.gui.plot_window = "plot_window"
        mock_checkbox = mock.Mock()
        mock_checkbox.name = "GE1"

        self.gui.del_plot(mock_checkbox)

        self.assertEqual(self.gui.plotting.remove_subplot.call_count, 1)
        self.assertEqual(self.gui.plot_window, "plot_window")

    def test_del_plot_closes_plot_if_no_subplots_left(self):
        self.gui.load_widget.last_loaded_run = mock.Mock(return_value=2695)
        self.gui.plotting.remove_subplot = mock.Mock()
        self.gui.plotting.get_subplots = mock.Mock(return_value=False)
        self.gui.plot_window = mock.Mock()
        mock_checkbox = mock.Mock()
        mock_checkbox.name = "GE1"

        self.gui.del_plot(mock_checkbox)

        self.assertEqual(self.gui.plotting.remove_subplot.call_count, 1)
        self.assertEqual(self.gui.plot_window, None)

    def test_subplot_removed_changes_state_only_if_other_subplots_exist(self):
        self.gui.detectors.setStateQuietly = mock.Mock()
        self.gui.plotting.get_subplots = mock.Mock(return_value=True)
        self.gui.plot_window = "plot_window"

        self.gui.subplot_removed("name")

        self.assertEqual(self.gui.detectors.setStateQuietly.call_count, 1)
        self.assertEqual(self.gui.plot_window, "plot_window")

    def test_subplot_removed_closes_plot_if_no_other_subplots_exist(self):
        self.gui.detectors.setStateQuietly = mock.Mock()
        self.gui.plotting.get_subplots = mock.Mock(return_value=False)
        self.gui.plot_window = mock.Mock()

        self.gui.subplot_removed("name")

        self.assertEqual(self.gui.detectors.setStateQuietly.call_count, 1)
        self.assertEqual(self.gui.plot_window, None)

    @mock.patch("mantidqtinterfaces.Muon.GUI.ElementalAnalysis.elemental_analysis.message_box.warning")
    @mock.patch("mantidqtinterfaces.Muon.GUI.ElementalAnalysis.elemental_analysis.QtWidgets.QFileDialog.getOpenFileName")
    def test_that_set_peak_datafile_is_called_with_select_data_file(self, mock_get_open_file_name, mock_warning):
        name = "data to load"
        mock_get_open_file_name.return_value = name
        self.gui.select_data_file()
        self.assertEqual(self.gui.ptable.set_peak_datafile.call_count, 1)
        self.assertEqual(0, mock_warning.call_count)

    @mock.patch("mantidqtinterfaces.Muon.GUI.ElementalAnalysis.elemental_analysis.QtWidgets.QFileDialog.getOpenFileName")
    def test_that_select_data_file_uses_the_first_element_of_a_tuple_when_given_as_a_filename(self, mock_get_open_file_name):
        mock_get_open_file_name.return_value = ("string1", "string2")
        self.gui.select_data_file()
        self.gui.ptable.set_peak_datafile.assert_called_with("string1")

    @mock.patch("mantidqtinterfaces.Muon.GUI.ElementalAnalysis.elemental_analysis.PeriodicTablePresenter.peak_data")
    @mock.patch("mantidqtinterfaces.Muon.GUI.ElementalAnalysis.elemental_analysis.ElementalAnalysisGui._create_peak_selector")
    def test_generate_element_widget_pass(self, create_mock, peak_mock):
        inputs = {"H": 3, "Cu": 4, "Ag": 2}
        peak_mock.return_value = inputs
        peak_mock.keys = mock.Mock(return_value=inputs.keys())
        create_mock.return_value = mock.Mock()
        any_elements = self.gui._generate_element_widgets()
        self.assertEqual(any_elements, True)
        self.assertEqual(self.gui._create_peak_selector.call_count, 3)

    @mock.patch("mantidqtinterfaces.Muon.GUI.ElementalAnalysis.elemental_analysis.PeriodicTablePresenter.peak_data")
    @mock.patch("mantidqtinterfaces.Muon.GUI.ElementalAnalysis.elemental_analysis.ElementalAnalysisGui._create_peak_selector")
    def test_generate_element_widget_partial(self, create_mock, peak_mock):
        inputs = {"Hi": 3, "Cu": 4, "unit": 2}
        peak_mock.return_value = inputs
        peak_mock.keys = mock.Mock(return_value=inputs.keys())
        create_mock.return_value = mock.Mock()
        any_elements = self.gui._generate_element_widgets()
        self.assertEqual(any_elements, True)
        self.assertEqual(self.gui._create_peak_selector.call_count, 1)

    @mock.patch("mantidqtinterfaces.Muon.GUI.ElementalAnalysis.elemental_analysis.PeriodicTablePresenter.peak_data")
    @mock.patch("mantidqtinterfaces.Muon.GUI.ElementalAnalysis.elemental_analysis.ElementalAnalysisGui._create_peak_selector")
    def test_generate_element_widget_fail(self, create_mock, peak_mock):
        inputs = {"Hi": 3, "test": 4, "unit": 2}
        peak_mock.return_value = inputs
        peak_mock.keys = mock.Mock(return_value=inputs.keys())
        create_mock.return_value = mock.Mock()
        any_elements = self.gui._generate_element_widgets()
        self.assertEqual(any_elements, False)
        self.assertEqual(self.gui._create_peak_selector.call_count, 0)

    @mock.patch("mantidqtinterfaces.Muon.GUI.ElementalAnalysis.elemental_analysis.message_box.warning")
    @mock.patch("mantidqtinterfaces.Muon.GUI.ElementalAnalysis.elemental_analysis.ElementalAnalysisGui._generate_element_widgets")
    @mock.patch("mantidqtinterfaces.Muon.GUI.ElementalAnalysis.elemental_analysis.QtWidgets.QFileDialog.getOpenFileName")
    def test_that_select_data_file_raises_warning_with_correct_text(
        self, mock_get_open_file_name, mock_generate_element_widgets, mock_warning
    ):
        mock_get_open_file_name.return_value = "filename"
        mock_generate_element_widgets.side_effect = self.raise_ValueError_once
        self.gui.select_data_file()
        warning_text = (
            "The file does not contain correctly formatted data, resetting to default data file."
            'See "https://docs.mantidproject.org/nightly/interfaces/muon/'
            'Muon%20Elemental%20Analysis.html" for more information.'
        )
        mock_warning.assert_called_with(warning_text)

    @mock.patch("mantidqtinterfaces.Muon.GUI.ElementalAnalysis.elemental_analysis.ElementalAnalysisGui._reset_data_file_warning_and_action")
    @mock.patch("mantidqtinterfaces.Muon.GUI.ElementalAnalysis.elemental_analysis.QtWidgets.QFileDialog.getOpenFileName")
    @mock.patch("mantidqtinterfaces.Muon.GUI.ElementalAnalysis.elemental_analysis.ElementalAnalysisGui._generate_element_widgets")
    def test_that_select_data_file_raises_warning_with_bad_input(self, mock_generate, mock_get_open_file_name, mock_warning):
        mock_get_open_file_name.return_value = "filename"
        mock_generate.return_value = False
        self.gui.select_data_file()
        self.assertEqual(mock_warning.call_count, 1)

    @mock.patch("mantidqtinterfaces.Muon.GUI.ElementalAnalysis.elemental_analysis.message_box.warning")
    @mock.patch("mantidqtinterfaces.Muon.GUI.ElementalAnalysis.elemental_analysis.QtWidgets.QFileDialog.getOpenFileName")
    def test_select_data_file_calls_update_checked_data(self, mock_getOpenFileName, mock_warning):
        mock_getOpenFileName.return_value = "filename"
        tmp = self.gui._update_checked_data
        self.gui._update_checked_data = mock.Mock()
        self.gui.select_data_file()

        self.assertEqual(1, self.gui._update_checked_data.call_count)
        self.gui._update_checked_data = tmp

    def test_update_checked_data_calls_the_right_functions(self):
        self.gui.major_peaks_changed = mock.Mock()
        self.gui.minor_peaks_changed = mock.Mock()
        self.gui.gammas_changed = mock.Mock()
        self.gui.electrons_changed = mock.Mock()

        self.gui._update_checked_data()

        self.assertEqual(1, self.gui.major_peaks_changed.call_count)
        self.assertEqual(1, self.gui.minor_peaks_changed.call_count)
        self.assertEqual(1, self.gui.gammas_changed.call_count)
        self.assertEqual(1, self.gui.electrons_changed.call_count)
        self.gui.major_peaks_changed.assert_called_with(self.gui.peaks.major)
        self.gui.minor_peaks_changed.assert_called_with(self.gui.peaks.minor)
        self.gui.gammas_changed.assert_called_with(self.gui.peaks.gamma)
        self.gui.electrons_changed.assert_called_with(self.gui.peaks.electron)

    def test_major_changed_calls_checked_data_for_each_element(self):
        # need to do this to reset the elements
        self.gui._generate_element_widgets()

        elem = len(self.gui.ptable.peak_data)
        self.gui.checked_data = mock.Mock()
        self.gui.major_peaks_changed(self.gui.peaks.major)
        self.assertEqual(self.gui.checked_data.call_count, elem)

    def test_minor_changed_calls_checked_data_for_each_element(self):
        # need to do this to reset the elements
        self.gui._generate_element_widgets()

        elem = len(self.gui.ptable.peak_data)
        self.gui.checked_data = mock.Mock()
        self.gui.minor_peaks_changed(self.gui.peaks.minor)
        self.assertEqual(self.gui.checked_data.call_count, elem)

    def test_gammas_changed_calls_checked_data_for_each_element(self):
        elem = len(self.gui.ptable.peak_data)
        self.gui.checked_data = mock.Mock()
        self.gui.gammas_changed(self.gui.peaks.gamma)
        self.assertEqual(self.gui.checked_data.call_count, elem)

    def test_electrons_changed_calls_checked_data_for_each_element(self):
        elem = len(self.gui.ptable.peak_data)
        self.gui.checked_data = mock.Mock()
        self.gui.electrons_changed(self.gui.peaks.electron)
        self.assertEqual(self.gui.checked_data.call_count, elem)

    @mock.patch("mantidqtinterfaces.Muon.GUI.ElementalAnalysis.elemental_analysis.ElementalAnalysisGui._update_peak_data")
    def test_checked_data_changes_all_states_in_list(self, mock_update_peak_data):
        selection = [mock.Mock() for i in range(10)]
        self.gui.checked_data("Cu", selection, True)

        self.assertTrue(all(map(lambda m: m.setChecked.call_count == 1, selection)))
        mock_update_peak_data.assert_called_with("Cu")

    def test_that_ptable_contains_no_peak_not_part_of_an_element(self):
        self.assertTrue("Electrons" not in self.gui.ptable.peak_data)
        self.assertTrue("Gammas" not in self.gui.ptable.peak_data)

    def test_that_add_line_by_type_enables_all_detectors(self):
        self.gui.detectors.detectors = [mock.Mock(), mock.Mock(), mock.Mock(), mock.Mock()]
        for detector in self.gui.detectors.detectors:
            detector.isEnabled.return_value = False

        self.gui.add_line_by_type(2695, "Total")

        for detector in self.gui.detectors.detectors:
            detector.setEnabled.assert_called_with(True)

    def texst_that_add_line_by_type_returns_is_plot_window_is_none(self):
        self.gui.plotting = mock.Mock()
        self.gui.add_line_by_type(2695, "Total")

        self.assertEqual(0, self.gui.plotting.get_subplots.call_count)

    @mock.patch("mantidqtinterfaces.Muon.GUI.ElementalAnalysis.elemental_analysis.mantid")
    def test_that_add_line_by_type_plots_correct_lines_with_correct_colour(self, mock_mantid):
        self.gui.plotting = mock.Mock()
        self.gui.plot_window = mock.Mock()
        self.gui.plotting.get_subplots.return_value = ["1", "2"]
        mock_mantid.mtd = {"2695; Detector 1": mock.Mock(), "2695; Detector 2": mock.Mock(), "2695; Detector 3": mock.Mock()}
        mock_mantid.mtd["2695; Detector 1"].name.return_value = "2695; Detector 1"
        mock_mantid.mtd["2695; Detector 2"].name.return_value = "2695; Detector 2"
        expected_calls = [
            mock.call("1", "2695; Detector 1", color="C0", spec_num=spectrum_index["Total"]),
            mock.call("2", "2695; Detector 2", color="C0", spec_num=spectrum_index["Total"]),
        ]
        self.gui.add_line_by_type(2695, "Total")

        self.assertEqual(1, self.gui.plotting.get_subplots.call_count)
        self.gui.plotting.plot.assert_has_calls(expected_calls)

    def test_remove_line_type_returns_if_no_plot_open(self):
        self.gui.plot_window = None
        self.gui.plotting = mock.Mock()
        self.gui.remove_line_type(2695, "Total")

        self.assertEqual(0, self.gui.plotting.get_subplots.call_count)

    @mock.patch("mantidqtinterfaces.Muon.GUI.ElementalAnalysis.elemental_analysis.mantid")
    def test_remove_line_type_calls_remove_line(self, mock_mantid):
        self.gui.plot_window = mock.Mock()
        self.gui.plotting = mock.Mock()
        self.gui.plotting.get_subplots.return_value = ["1", "2"]
        mock_mantid.mtd = {"2695; Detector 1": mock.Mock(), "2695; Detector 2": mock.Mock(), "2695; Detector 3": mock.Mock()}
        mock_mantid.mtd["2695; Detector 1"].name.return_value = "2695; Detector 1"
        mock_mantid.mtd["2695; Detector 2"].name.return_value = "2695; Detector 2"
        expected_calls = [
            mock.call("1", "2695; Detector 1", spec=spectrum_index["Total"]),
            mock.call("2", "2695; Detector 2", spec=spectrum_index["Total"]),
        ]
        self.gui.remove_line_type(2695, "Total")

        self.assertEqual(1, self.gui.plotting.get_subplots.call_count)
        self.gui.plotting.remove_line.assert_has_calls(expected_calls)

    def test_that_uncheck_detectors_if_no_line_plotted_unchecks_all_detectors(self):
        self.gui.lines.total.isChecked.return_value = False
        self.gui.lines.prompt.isChecked.return_value = False
        self.gui.lines.delayed.isChecked.return_value = False

        self.gui.uncheck_detectors_if_no_line_plotted()

        for detector in self.gui.detectors.detectors:
            detector.setEnabled.assert_called_with(False)
            self.assertEqual(1, detector.setEnabled.call_count)

    def test_that_uncheck_detectors_if_no_line_plotted_does_not_uncheck_if_plotting_some_lines(self):
        self.gui.lines.total.isChecked.return_value = False
        self.gui.lines.prompt.isChecked.return_value = True
        self.gui.lines.delayed.isChecked.return_value = False

        self.gui.uncheck_detectors_if_no_line_plotted()

        for detector in self.gui.detectors.detectors:
            self.assertEqual(0, detector.setEnabled.call_count)

    def test_that_uncheck_on_removed_uncheck_correct_lines(self):
        rem_lines = ["line Total", "line Prompt", "line 3"]
        tmp_detectors = copy.deepcopy(self.gui.detectors.detectors)
        for detector in self.gui.detectors.detectors:
            detector.isChecked.return_value = False
        self.gui.uncheck_on_removed(rem_lines)

        self.gui.lines.total.setChecked.assert_called_with(False)
        self.gui.lines.prompt.setChecked.assert_called_with(False)
        self.assertEqual(0, self.gui.lines.delayed.setChecked.call_count)

        self.gui.detectors.detectors = tmp_detectors

    def test_that_uncheck_on_removed_blocks_signals_and_calls_right_function(self):
        rem_lines = ["line Total", "line Prompt", "line 3"]
        calls = [mock.call(True), mock.call(False)]
        self.gui.uncheck_detectors_if_no_line_plotted = mock.Mock()
        tmp_detectors = copy.deepcopy(self.gui.detectors.detectors)
        for detector in self.gui.detectors.detectors:
            detector.isChecked.return_value = False

        self.gui.uncheck_on_removed(rem_lines)

        self.gui.lines.total.blockSignals.assert_has_calls(calls)
        self.gui.lines.prompt.blockSignals.assert_has_calls(calls)
        self.gui.lines.delayed.blockSignals.assert_has_calls([])
        self.assertEqual(1, self.gui.uncheck_detectors_if_no_line_plotted.call_count)

        self.gui.detectors.detectors = tmp_detectors

    @mock.patch("mantidqtinterfaces.Muon.GUI.ElementalAnalysis.elemental_analysis.ElementalAnalysisGui.add_line_by_type")
    def test_line_total_checked_checks_line_and_calls_add_line_by_type(self, mock_add_line_by_type):
        self.gui.load_widget.last_loaded_run = mock.Mock(return_value=2695)
        mock_line = mock.Mock()
        mock_line.isChecked.return_value = True
        self.gui.line_total_changed(mock_line)

        self.gui.lines.total.setChecked.assert_called_with(True)
        self.assertEqual(1, mock_add_line_by_type.call_count)

    @mock.patch("mantidqtinterfaces.Muon.GUI.ElementalAnalysis.elemental_analysis.ElementalAnalysisGui.add_line_by_type")
    def test_line_prompt_checked_checks_line_and_calls_add_line_by_type(self, mock_add_line_by_type):
        self.gui.load_widget.last_loaded_run = mock.Mock(return_value=2695)
        mock_line = mock.Mock()
        mock_line.isChecked.return_value = True
        self.gui.line_prompt_changed(mock_line)

        self.gui.lines.prompt.setChecked.assert_called_with(True)
        self.assertEqual(1, mock_add_line_by_type.call_count)

    @mock.patch("mantidqtinterfaces.Muon.GUI.ElementalAnalysis.elemental_analysis.ElementalAnalysisGui.add_line_by_type")
    def test_line_delayed_checked_checks_line_and_calls_add_line_by_type(self, mock_add_line_by_type):
        self.gui.load_widget.last_loaded_run = mock.Mock(return_value=2695)
        mock_line = mock.Mock()
        mock_line.isChecked.return_value = True
        self.gui.line_delayed_changed(mock_line)

        self.gui.lines.delayed.setChecked.assert_called_with(True)
        self.assertEqual(1, mock_add_line_by_type.call_count)

    @mock.patch("mantidqtinterfaces.Muon.GUI.ElementalAnalysis.elemental_analysis.ElementalAnalysisGui._remove_element_lines")
    def test_deselect_elements(self, mock_remove_element_lines):
        self.gui.ptable.deselect_element = mock.Mock()

        self.gui.peaks.enable_deselect_elements_btn = mock.Mock()
        self.gui.peaks.disable_deselect_elements_btn = mock.Mock()

        self.gui.deselect_elements()

        self.assertEqual(self.gui.peaks.enable_deselect_elements_btn.call_count, 1)
        self.assertEqual(self.gui.peaks.disable_deselect_elements_btn.call_count, 1)

        self.assertEqual(self.gui.ptable.deselect_element.call_count, len(self.gui.element_widgets))
        self.assertEqual(mock_remove_element_lines.call_count, len(self.gui.element_widgets))

        calls = [mock.call(element) for element in self.gui.element_widgets.keys()]
        self.gui.ptable.deselect_element.assert_has_calls(calls)
        mock_remove_element_lines.assert_has_calls(calls)

    @mock.patch("mantidqtinterfaces.Muon.GUI.ElementalAnalysis.elemental_analysis.ElementalAnalysisGui._remove_element_lines")
    def test_deselect_elements_fails(self, mock_remove_element_lines):
        self.gui.ptable.deselect_element = mock.Mock()

        self.gui.peaks.enable_deselect_elements_btn = mock.Mock()
        self.gui.peaks.disable_deselect_elements_btn = mock.Mock()

        self.gui.deselect_elements()
        # Test passes only if deselect_element is not called with "Hydrogen"
        with self.assertRaises(AssertionError):
            self.gui.ptable.deselect_element.assert_any_call("Hydrogen")

        with self.assertRaises(AssertionError):
            mock_remove_element_lines.assert_any_call("Hydrogen")


if __name__ == "__main__":
    unittest.main()
