# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
"""
DNS TOf powder options widget
"""
from mantidqtinterfaces.dns.data_structures.dns_widget import DNSWidget
from mantidqtinterfaces.dns.options.tof_powder_options_model import \
    DNSTofPowderOptionsModel
from mantidqtinterfaces.dns.options.tof_powder_options_presenter\
    import DNSTofPowderOptionsPresenter
from mantidqtinterfaces.dns.options.tof_powder_options_view import \
    DNSTofPowderOptionsView


class DNSTofPowderOptionsWidget(DNSWidget):

    def __init__(self, name, parent):
        super().__init__(name, parent)
        self.view = DNSTofPowderOptionsView(parent=parent.view)
        self.model = DNSTofPowderOptionsModel(parent=self)
        self.presenter = DNSTofPowderOptionsPresenter(parent=self,
                                                      view=self.view,
                                                      model=self.model,
                                                      name=name)
