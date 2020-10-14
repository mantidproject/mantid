# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from unittest import mock
from SampleTransmissionCalculator.stc_presenter import SampleTransmissionCalculatorPresenter


class SampleTransmissionCalculatorPresenterTest(unittest.TestCase):

    def test_button_connection(self):
        view = mock.Mock()
        view.calculate_button.clicked.connect = mock.Mock()
        model = mock.Mock()
        self.presenter = SampleTransmissionCalculatorPresenter(view, model)
        self.presenter.view.calculate_button.clicked.connect.assert_called_once_with(self.presenter.calculate)

    def test_calculate_with_invalid(self):
        view = mock.Mock()
        view.get_input_dict.return_value = {'val_1': 1.0,
                                            'val_2': 2.0}
        model = mock.Mock()
        model.validate.return_value = {'val_1': 'error_string_1',
                                       'val_2': 'error_string_2'}
        self.presenter = SampleTransmissionCalculatorPresenter(view, model)

        self.presenter.calculate()
        call_list = [mock.call('val_1'),
                     mock.call('val_2')]
        view.set_error_indicator.assert_has_calls(call_list, any_order=True)
        view.set_validation_label.assert_called_once_with('error_string_1 error_string_2 ')

    def test_calculate_with_valid(self):
        view = mock.Mock()
        view.get_input_dict.return_value = {'val_1': 1.0,
                                            'val_2': 2.0}
        model = mock.Mock()
        model.validate.return_value = {}
        calculated_values = {'x': [1.0, 2.0],
                             'y': [1.0, 2.0],
                             'scattering': 1.0}
        model.calculate.return_value = calculated_values
        statistics = {"Min": 1.0,
                      "Max": 2.0,
                      "Mean": 1.5,
                      "Median": 1.0,
                      "Std. Dev.": 1.0}
        model.calculate_statistics.return_value = statistics
        self.presenter = SampleTransmissionCalculatorPresenter(view, model)

        self.presenter.calculate()

        view.set_output_table.assert_called_with(statistics, calculated_values['scattering'])
        view.plot.assert_called_with(calculated_values['x'], calculated_values['y'])


if __name__ == '__main__':
    unittest.main()
