# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
"""
Definition of DNS measurement modi and functions fo switching
"""
from collections import OrderedDict

from mantidqtinterfaces.dns.file_selector.file_selector_widget \
    import DNSFileSelectorWidget
from mantidqtinterfaces.dns.options.tof_powder_options_widget import \
    DNSTofPowderOptionsWidget
from mantidqtinterfaces.dns.paths.path_widget import DNSPathWidget
from mantidqtinterfaces.dns.plot.tof_powder_plot_widget import \
    DNSTofPowderPlotWidget
from mantidqtinterfaces.dns.script_generator.\
    tof_powder_script_generator_widget import \
    DNSTofPowderScriptGeneratorWidget
from mantidqtinterfaces.dns.xml_dump.xml_dump_widget import \
    DNSXMLDumpWidget


class DNSModus:
    """defines the different reduction modes and which widgets are used in
       each mode """

    def __init__(self, name, parent):
        super().__init__()
        self.parent = parent  # schould be main widget
        self.name = name  # only names in the mapping below are allowed
        self.widgets = OrderedDict()

        self._mode_map = {
            'powder_tof': [
                'paths', 'file_selector', 'tof_powder_options',
                'tof_powder_script_generator', 'xml_dump', 'plot_tof_powder'
            ]
        }
        # yapf: disable
        self._widget_map = {
            'paths': DNSPathWidget,
            'file_selector': DNSFileSelectorWidget,
            'tof_powder_options': DNSTofPowderOptionsWidget,
            'tof_powder_script_generator': DNSTofPowderScriptGeneratorWidget,
            'xml_dump': DNSXMLDumpWidget,
            'plot_tof_powder': DNSTofPowderPlotWidget,
        }
        # yapf: enable

    def change(self, name):
        """changes the mode of DNS GUI only names from _mode_map  are
           allowed"""
        self.name = name
        self.widgets.clear()
        for key in self._mode_map.get(self.name, []):
            self.widgets[key] = self._widget_map[key](key, self.parent)
