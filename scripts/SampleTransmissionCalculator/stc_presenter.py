# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
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

