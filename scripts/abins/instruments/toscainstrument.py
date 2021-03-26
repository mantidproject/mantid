# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import abins
import abins.parameters
from .indirectinstrument import IndirectInstrument


class ToscaInstrument(IndirectInstrument):
    """
    ISIS TOSCA instrument

    A quadratic model is used for energy resolution
    """
    parameters = abins.parameters.instruments['TOSCA']

    def __init__(self, setting: str = ''):
        super().__init__(name='TOSCA', setting=setting)

    @classmethod
    def get_sigma(cls, frequencies):
        """Frequency-dependent broadening width from empirical fit"""
        a = cls.parameters['a']
        b = cls.parameters['b']
        c = cls.parameters['c']
        sigma = a * frequencies**2 + b * frequencies + c
        return sigma
