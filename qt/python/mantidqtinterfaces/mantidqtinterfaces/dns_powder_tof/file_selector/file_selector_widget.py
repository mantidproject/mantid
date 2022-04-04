# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
"""
DNS File Selector widget
"""
from mantidqtinterfaces.dns_powder_tof.data_structures.dns_widget import DNSWidget
from mantidqtinterfaces.dns_powder_tof.file_selector.file_selector_model import \
    DNSFileSelectorModel
from mantidqtinterfaces.dns_powder_tof.file_selector.file_selector_presenter\
    import DNSFileSelectorPresenter
from mantidqtinterfaces.dns_powder_tof.file_selector.file_selector_view import \
    DNSFileSelectorView
from mantidqtinterfaces.dns_powder_tof.file_selector.file_selector_watcher \
    import DNSFileSelectorWatcher


class DNSFileSelectorWidget(DNSWidget):

    def __init__(self, name, parent):
        super().__init__(name, parent)
        self.view = DNSFileSelectorView(parent=parent.view)
        self.model = DNSFileSelectorModel(parent=self)
        self.watcher = DNSFileSelectorWatcher(parent=self.view)
        self.presenter = DNSFileSelectorPresenter(parent=self,
                                                  view=self.view,
                                                  model=self.model,
                                                  name=name,
                                                  watcher=self.watcher)
