# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
"""
Definition of DNS measurement modi and functions fo switching
"""
from __future__ import (absolute_import, division, print_function)
from collections import OrderedDict

from DNSReduction.paths.path_presenter import DNSPath_presenter
from DNSReduction.file_selector.file_selector_presenter import DNSFileSelector_presenter
from DNSReduction.options.elastic_powder_options_presenter import DNSElasticPowderOptions_presenter
from DNSReduction.xml_dump.xml_dump_presenter import DNSXMLDump_presenter
from DNSReduction.simulation.simulation_presenter import DNSSimulation_presenter
from DNSReduction.options.tof_powder_options_presenter import DNSTofPowderOptions_presenter
from DNSReduction.script_generator.tof_powder_script_generator_presenter import DNSTofPowderScriptGenerator_presenter
from DNSReduction.plot.tof_powder_plot_presenter import DNSTofPowderPlot_presenter
#from DNSReduction.script_generator.tof_sc_script_generator_presenter import DNSTofScScriptGenerator_presenter
#from DNSReduction.options.tof_sc_options_presenter import DNSTofScOptions_presenter
from DNSReduction.script_generator.elastic_powder_script_generator_presenter import DNSElasticPowderScriptGenerator_presenter
from DNSReduction.plot.elastic_powder_plot_presenter import DNSElasticPowderPlot_presenter


class DNSModus(object):
    def __init__(self, name, parent):
        super(DNSModus, self).__init__()
        self.parent = parent  ##schold be main presenter
        self.name = name  ## only names in the mapping below are allowed
        self.presenters = OrderedDict()

        self.mode_map = {
            'powder_elastic': [
                'path', 'file_selector', 'elastic_powder_options',
                'elastic_powder_script_generator', 'xml_dump',
                'plot_elastic_powder'
            ],
            'powder_tof': [
                'path', 'file_selector', 'tof_powder_options',
                'tof_powder_script_generator', 'xml_dump', 'plot_tof_powder'
            ],
            #'sc_elastic': ['path', 'xml_dump'],
            #'sc_tof': [
            #    'path', 'file_selector', 'tof_sc_options',
            #    'tof_sc_script_generator', 'xml_dump'
            #],
            'simulation': ['simulation'],
        }
        self.mode_map['all'] = set(
            [j for i in self.mode_map.values() for j in i])

        self.presenter_map = {
            'path':
            DNSPath_presenter(self.parent),
            'file_selector':
            DNSFileSelector_presenter(self.parent),
            'elastic_powder_options':
            DNSElasticPowderOptions_presenter(self.parent),
            'tof_powder_options':
            DNSTofPowderOptions_presenter(self.parent),
            'tof_powder_script_generator':
            DNSTofPowderScriptGenerator_presenter(self.parent),
            'elastic_powder_script_generator':
            DNSElasticPowderScriptGenerator_presenter(self.parent),
            'xml_dump':
            DNSXMLDump_presenter(self.parent),
            'simulation':
            DNSSimulation_presenter(self.parent),
            'plot_tof_powder':
            DNSTofPowderPlot_presenter(self.parent),
            #'tof_sc_options':
            #DNSTofScOptions_presenter(self.parent),
            #'tof_sc_script_generator':
            #DNSTofScScriptGenerator_presenter(self.parent),
            'plot_elastic_powder':
            DNSElasticPowderPlot_presenter(self.parent),
        }
        # we initailize all views inside the tabs first
        self.change('all')

    def change(self, name):
        """changes the mode of DNS GUI """
        self.name = name
        self.presenters.clear()
        for key in self.mode_map.get(self.name, []):
            self.presenters[key] = self.presenter_map[key]
