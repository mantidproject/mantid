# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
"""
DNS Observer Class, they share a common data model DNSReductionGui_model
and are updated by DNSReductionGui_presenter
"""
from __future__ import (absolute_import, division, print_function)


class DNSObsModel(object):
    """
    Defines a model for DNS Observers, so far only used for Simulation Observer
    """
    def __init__(self, parent):
        super(DNSObsModel, self).__init__()
        self.presenter = parent
        self.name = self.presenter.name
        self.param_dict = self.presenter.param_dict
        self.own_dict = self.presenter.own_dict
