# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

"""
DNS GUI main widget.
"""

from mantidqtinterfaces.dns_powder_tof.data_structures.dns_widget import DNSWidget
from mantidqtinterfaces.dns_powder_tof.dns_modus import DNSModus
from mantidqtinterfaces.dns_powder_tof.main_presenter import DNSReductionGUIPresenter
from mantidqtinterfaces.dns_powder_tof.main_view import DNSReductionGUIView
from mantidqtinterfaces.dns_powder_tof.parameter_abo import ParameterAbo


class DNSReductionGuiWidget(DNSWidget):
    """
    Main DNS GUI widget, host, view, presenter, model.
    """

    def __init__(self, name=None, parent=None, app=None, within_mantid=None):
        super().__init__(name, parent)
        self.name = name
        self.view = DNSReductionGUIView(parent=self, app=app, within_mantid=within_mantid)
        self.parameter_abo = ParameterAbo()
        self.model = self.parameter_abo
        self.modus = DNSModus(name="single_crystal_elastic", parent=self)
        self.presenter = DNSReductionGUIPresenter(
            parent=self, view=self.view, modus=self.modus, name=name, parameter_abo=self.parameter_abo
        )
