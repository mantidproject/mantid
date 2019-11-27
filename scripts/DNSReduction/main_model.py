# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
"""
Main model which saves all parameters of the gui
this is a dictionary of dictionaries, with each dictionary belonging
to a tab in the reduction window or the header
"""
from __future__ import (absolute_import, division, print_function)
from collections import OrderedDict


class DNSReductionGUI_model(object):
    def __init__(self):
        self.param_dict = OrderedDict()

    def update(self, name, options_dict):
        if options_dict is not None:
            self.param_dict[name] = options_dict

    def set_whole_dict(self, param_dict):
        self.param_dict = param_dict

    def get(self):
        return self.param_dict
