# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

"""
DNS xml dump widget.
"""

from mantidqtinterfaces.dns_powder_tof.data_structures.dns_widget import DNSWidget
from mantidqtinterfaces.dns_powder_tof.xml_dump.xml_dump_model import DNSXMLDumpModel
from mantidqtinterfaces.dns_powder_tof.xml_dump.xml_dump_presenter import DNSXMLDumpPresenter
from mantidqtinterfaces.dns_powder_tof.xml_dump.xml_dump_view import DNSXMLDumpView


class DNSXMLDumpWidget(DNSWidget):
    def __init__(self, name, parent):
        super().__init__(name, parent)
        self.view = DNSXMLDumpView(parent=parent.view)
        self.model = DNSXMLDumpModel(parent=self)
        self.presenter = DNSXMLDumpPresenter(parent=self, view=self.view, model=self.model, name=name)
