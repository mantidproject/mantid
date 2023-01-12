# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantidqt.interfacemanager import InterfaceManager


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
            try:
                output = self.model.calculate(input_dict)
                statistics = self.model.calculate_statistics(output["y"])
                self.view.set_output_table(statistics, output["scattering"])
                self.view.plot(output["x"], output["y"])
                self.view.set_validation_label("")
            except RuntimeError:
                self.view.set_error_indicator("chemical_formula")
                self.view.set_validation_label("Unable to parse chemical formula.")
        else:
            warning = ""
            for key in validations:
                self.view.set_error_indicator(key)
                warning += validations[key] + " "
            self.view.set_validation_label(warning)

    def help_window(self):
        gui_name = "Sample Transmission Calculator"
        InterfaceManager().showCustomInterfaceHelp(gui_name, "general")
