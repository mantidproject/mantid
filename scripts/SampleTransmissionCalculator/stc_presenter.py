# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantid.simpleapi import CalculateSampleTransmission


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
        input_key = self.view.get_input_key()
        
        if input_key['binning_type'] == 0:
            # single binning
            binning = str(input_key['single_low']) + ',' + str(input_key['single_width']) + ',' + \
                      str(input_key['single_high'])
            print(binning)
        if input_key['binning_type'] == 1:
            # multiple binning
            binning = input_key['multiple_bin']

        transmission_ws = CalculateSampleTransmission(
            WavelengthRange=binning,
            ChemicalFormula=input_key['chemical_formula'],
            DensityType=input_key['density_type'],
            density=input_key['density'],
            thickness=input_key['thickness']
        )

