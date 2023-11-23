# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
"""
DNS Path widget
"""
# yapf: disable
from mantidqtinterfaces.dns_powder_tof.data_structures.dns_widget \
    import DNSWidget
from mantidqtinterfaces.dns_powder_tof.script_generator.\
    common_script_generator_view import DNSScriptGeneratorView
from mantidqtinterfaces.dns_sc_elastic.script_generator.\
    elastic_sc_script_generator_model import DNSElasticSCScriptGeneratorModel
from mantidqtinterfaces.dns_sc_elastic.script_generator.\
    elastic_sc_script_generator_presenter import \
    DNSElasticSCScriptGeneratorPresenter
# yapf: enable


class DNSElasticSCScriptGeneratorWidget(DNSWidget):
    def __init__(self, name, parent):
        super().__init__(name, parent)
        self.view = DNSScriptGeneratorView(parent=parent.view)
        self.model = DNSElasticSCScriptGeneratorModel(parent=self)
        self.presenter = DNSElasticSCScriptGeneratorPresenter(
            parent=self, view=self.view, model=self.model, name=name)
