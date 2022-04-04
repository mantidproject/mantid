# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
"""
DNS Observer Class, they share a common data model DNSReductionGui_model
and are updated by DNSReductionGui_presenter
"""


class DNSObsModel:
    """
    Defines a model for DNS Observers
    """

    def __init__(self, parent):
        super().__init__()
        self.parent = parent
