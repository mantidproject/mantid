# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2023 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

"""
DNS script generator widget for elastic single crystal data.
"""

from mantidqtinterfaces.dns_powder_tof.data_structures.dns_widget import DNSWidget
from mantidqtinterfaces.dns_powder_tof.script_generator.common_script_generator_view import DNSScriptGeneratorView
from mantidqtinterfaces.dns_single_crystal_elastic.script_generator.elastic_single_crystal_script_generator_model import (
    DNSElasticSCScriptGeneratorModel,
)
from mantidqtinterfaces.dns_single_crystal_elastic.script_generator.elastic_single_crystal_script_generator_presenter import (
    DNSElasticSCScriptGeneratorPresenter,
)


class DNSElasticSCScriptGeneratorWidget(DNSWidget):
    def __init__(self, name, parent):
        super().__init__(name, parent)
        self.view = DNSScriptGeneratorView(parent=parent.view)
        self.model = DNSElasticSCScriptGeneratorModel(parent=self)
        self.presenter = DNSElasticSCScriptGeneratorPresenter(parent=self, view=self.view, model=self.model, name=name)
