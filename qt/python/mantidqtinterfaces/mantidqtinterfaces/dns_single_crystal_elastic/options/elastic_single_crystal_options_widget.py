# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2023 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

"""
DNS single crystal elastic options widget of DNS reduction GUI.
"""

from mantidqtinterfaces.dns_powder_tof.data_structures.dns_widget import DNSWidget
from mantidqtinterfaces.dns_powder_tof.options.common_options_model import DNSCommonOptionsModel
from mantidqtinterfaces.dns_single_crystal_elastic.options.elastic_single_crystal_options_presenter import DNSElasticSCOptionsPresenter
from mantidqtinterfaces.dns_single_crystal_elastic.options.elastic_single_crystal_options_view import DNSElasticSCOptionsView


class DNSElasticSCOptionsWidget(DNSWidget):
    def __init__(self, name, parent):
        super().__init__(name, parent)
        self.view = DNSElasticSCOptionsView(parent=parent.view)
        self.model = DNSCommonOptionsModel(parent=self)
        self.presenter = DNSElasticSCOptionsPresenter(parent=self, view=self.view, model=self.model, name=name)
