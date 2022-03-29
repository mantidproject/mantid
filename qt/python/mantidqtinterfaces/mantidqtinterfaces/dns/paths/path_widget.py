# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
"""
DNS Path widget
"""
from mantidqtinterfaces.dns.data_structures.dns_widget import DNSWidget
from mantidqtinterfaces.dns.paths.path_model import DNSPathModel
from mantidqtinterfaces.dns.paths.path_presenter import DNSPathPresenter
from mantidqtinterfaces.dns.paths.path_view import DNSPathView


class DNSPathWidget(DNSWidget):

    def __init__(self, name, parent):
        super().__init__(name, parent)
        self.view = DNSPathView(parent=parent.view)
        self.model = DNSPathModel(parent=self)
        self.presenter = DNSPathPresenter(name=name,
                                          parent=self,
                                          view=self.view,
                                          model=self.model)
