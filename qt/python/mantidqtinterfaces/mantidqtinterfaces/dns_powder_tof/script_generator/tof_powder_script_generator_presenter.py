# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

"""
DNS script generator presenter for powder TOF data.
"""

from mantidqtinterfaces.dns_powder_tof.script_generator.common_script_generator_presenter import DNSScriptGeneratorPresenter


class DNSTofPowderScriptGeneratorPresenter(DNSScriptGeneratorPresenter):
    def __init__(self, name=None, parent=None, view=None, model=None):
        super().__init__(parent=parent, name=name, view=view, model=model)
