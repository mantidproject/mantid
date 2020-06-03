# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantidqt.gui_helper import show_interface_help
import sys
import mantid
import os


class SampleTransmissionCalculatorPresenter(object):
    """
    The sample transmission calculator interface
    """

    def __init__(self, view, model):
        self.view = view
        self.model = model
        self.set_connections()

    def set_connections(self):
        self.view.calculate_button.clicked.connect(self.calculate)
        self.view.help_button.clicked.connect(self.help_window)

    def calculate(self):
        input_dict = self.view.get_input_dict()
        self.view.clear_error_indicator()
        validations = self.model.validate(input_dict)
        if not validations:
            output = self.model.calculate(input_dict)
            statistics = self.model.calculate_statistics(output['y'])
            self.view.set_output_table(statistics, output['scattering'])
            self.view.plot(output['x'], output['y'])
            self.view.set_validation_label('')
        else:
            warning = ''
            for key in validations:
                self.view.set_error_indicator(key)
                warning += validations[key] + ' '
            self.view.set_validation_label(warning)

    def help_window(self):
        gui_name = 'Sample Transmission Calculator'
        collection_file = os.path.join(mantid._bindir, '../docs/qthelp/MantidProject.qhc')
        version = ".".join(mantid.__version__.split(".")[:2])
        qt_url = 'qthelp://org.sphinx.mantidproject.' + version + '/doc/interfaces/Sample Transmission Calculator.html'
        external_url = 'http://docs.mantidproject.org/nightly/interfaces/Sample Transmission Calculator.html'
        show_interface_help(gui_name,
                            self.view.assistant_process,
                            collection_file,
                            qt_url,
                            external_url
                            )
