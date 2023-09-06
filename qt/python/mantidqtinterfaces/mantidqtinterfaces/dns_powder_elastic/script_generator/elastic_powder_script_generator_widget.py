# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2023 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

"""
DNS powder elastic script generator widget.
"""

from mantidqtinterfaces.dns_powder_tof.data_structures.dns_widget import DNSWidget
from mantidqtinterfaces.dns_powder_tof.script_generator.common_script_generator_view import DNSScriptGeneratorView
from mantidqtinterfaces.dns_powder_elastic.script_generator.elastic_powder_script_generator_model import (
    DNSElasticPowderScriptGeneratorModel,
)
from mantidqtinterfaces.dns_powder_elastic.script_generator.elastic_powder_script_generator_presenter import (
    DNSElasticPowderScriptGeneratorPresenter,
)


class DNSElasticPowderScriptGeneratorWidget(DNSWidget):
    def __init__(self, name, parent):
        super().__init__(name, parent)
        self.view = DNSScriptGeneratorView(parent=parent.view)
        self.model = DNSElasticPowderScriptGeneratorModel(parent=self)
        self.presenter = DNSElasticPowderScriptGeneratorPresenter(parent=self, view=self.view, model=self.model, name=name)
