# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +


class SampleTransmissionCalculatorPresenter(object):
    """
    The sample transmission calculator interface
    """

    def __init__(self, view, model):
        self.view = view
        self.model = model
        self.set_connections()

    def set_connections(self):
        self.view.pbCalculate.clicked.connect(self.calculate)

    def calculate(self):
        input_dict = self.view.get_input_dict()
        validations = self.model.validate(input_dict)
        if not validations:
            output = self.model.calculate(input_dict)
            statistics = self.model.calculate_statistics(output['y'])
            self.view.set_output_table(statistics, output['scattering'])
            self.view.plot(output['x'], output['y'])
            self.view.set_validation_label()
        else:
            warning = ''
            for key in validations:
                warning += validations[key] + ' '
            self.view.set_validation_label(warning)
