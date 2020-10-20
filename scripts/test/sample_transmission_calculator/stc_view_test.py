# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from unittest.mock import Mock, patch

from SampleTransmissionCalculator.stc_view import SampleTransmissionCalculatorView
from mantidqt.utils.qt.testing import start_qapplication


@start_qapplication
class SampleTransmissionCalculatorViewTest(unittest.TestCase):
    def setUp(self):
        self.view = SampleTransmissionCalculatorView()

    def test_get_input_dict(self):
        self.view.binning_type_combo_box.setCurrentIndex(0)
        self.view.single_low_spin_box.setValue(0.1)
        self.view.single_width_spin_box.setValue(0.1)
        self.view.single_high_spin_box.setValue(10.0)
        self.view.multiple_line_edit.setText('0.1,0.1,20.0')
        self.view.chemical_formula_line_edit.setText('C')
        self.view.density_combo_box.setCurrentIndex(1)
        self.view.density_spin_box.setValue(0.2)
        self.view.thickness_spin_box.setValue(0.2)

        input_dict = self.view.get_input_dict()

        input_dict_expected = {
            'binning_type': 0,
            'single_low': 0.1,
            'single_width': 0.1,
            'single_high': 10.0,
            'multiple_bin': '0.1,0.1,20.0',
            'chemical_formula': 'C',
            'density_type': 'Number Density',
            'density': 0.2,
            'thickness': 0.2,
        }
        self.assertEqual(input_dict, input_dict_expected)

    def test_set_output_table(self):
        output_dict = {
            'transmission_1': 1.0,
            'transmission_2': 2.0,
            'transmission_3': 3.0,
            'transmission_4': 4.0,
            'transmission_5': 5.0
        }
        statistics = 0.1

        self.view.set_output_table(output_dict, statistics)

        self.assertEqual(self.view.results_tree.topLevelItem(0).text(1), '0.1')
        transmission_items = self.view.results_tree.topLevelItem(1).takeChildren()
        for item in transmission_items:
            key = item.text(0)
            self.assertEqual(item.text(1), str(output_dict[key]))

    @patch('SampleTransmissionCalculator.stc_view.FigureCanvas')
    def test_plot(self, figure_canvas_mock):
        x = [1.0, 2.0]
        y = [1.0, 2.0]
        self.view.plot_frame = figure_canvas_mock
        self.view.axes = Mock()
        self.view.axes.plot = Mock()
        self.view.axes.cla = Mock()

        self.view.plot(x, y)

        self.view.axes.cla.assert_called_once()
        self.view.axes.plot.assert_called_with(x, y)
        figure_canvas_mock.draw.assert_called_once()

    def test_set_validation_label(self):
        warning_text = 'dummy warning text.'
        self.assertEqual(self.view.validation_label.text(), '')

        self.view.set_validation_label(warning_text)

        self.assertEqual(self.view.validation_label.text(), warning_text)

    def test_error_indicator(self):
        self.assertEqual(self.view.histogram_err.text(), '')
        self.view.set_error_indicator('histogram')
        self.assertEqual(self.view.histogram_err.text(), '*')
        self.view.clear_error_indicator()
        self.assertEqual(self.view.histogram_err.text(), '')


if __name__ == '__main__':
    unittest.main()
