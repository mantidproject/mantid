"""
    Classes for each reduction step. Those are kept separately
    from the the interface class so that the HFIRReduction class could
    be used independently of the interface implementation
"""
import xml.dom.minidom
import os
import time
from reduction_gui.reduction.scripter import BaseScriptElement

class DataSets(BaseScriptElement):

    data_file = 0
    incident_medium_list = ['H2O']
    incident_medium_index_selected = 0
    number_attenuator = 0
    peak_selection = [0,0]
    back_selection = [0,0]
    lambda_requested = 'N/A'
    s1h = 'N/A'
    s2h = 'N/A'
    s1w = 'N/A'
    s2w = 'N/A'
    scaling_factor_file = ''
    tof_min = 0.
    tof_max = 200000.

    def __init__(self):
        super(DataSets, self).__init__()
        self.reset()

    def to_script(self):
        """
            Generate reduction script
            @param execute: if true, the script will be executed
        """
        script = 'Run number: %s \n' % str(self.data_file)

#        _u_list = self.incident_medium_list
#        _list = str(_u_list[0]).split(',')

        script += 'Incident medium: %s \n' % str(self.incident_medium_list)
#        script += 'Incident medium: %s \n' % str(self.incident_medium_list[self.incident_medium_index_selected])
#        script += 'Incident medium: %s \n' % str(_list[self.incident_medium_index_selected])
        script += 'Incident medium index: %s \n' % str(self.incident_medium_index_selected)
        script += 'TOF from: %s \n' % str(self.tof_min)
        script += 'TOF to: %s \n' % str(self.tof_max)
        script += 'Scaling factor file: %s \n' %str(self.scaling_factor_file)
        script += 'Number of attenuator: %s \n' % str(self.number_attenuator)
        script += 'Peak from pixel: %s \n' % str(self.peak_selection[0])
        script += 'Peak to pixel: %s \n' % str(self.peak_selection[1])
        script += 'Back from pixel: %s \n' % str(self.back_selection[0])
        script += 'Back to pixel: %s \n' % str(self.back_selection[1])

        return script

    def update(self):
        """
            Update transmission from reduction output
        """
        pass

    def to_xml(self):
        """
            Create XML from the current data.
        """
        xml  = "<RefLSFCalculator>\n"
#        xml += "<incident_medium_list>%s</incident_medium_list>\n" % ','.join([str(i) for i in self.incident_medium_list])
        xml += "<incident_medium_list>%s</incident_medium_list>\n" % str(self.incident_medium_list[0])
        xml += "<tof_min>%s</tof_min>\n" % str(self.tof_min)
        xml += "<tof_max>%s</tof_max>\n" % str(self.tof_max)
        xml += "<incident_medium_index_selected>%s</incident_medium_index_selected>\n" % str(self.incident_medium_index_selected)
        xml += "<data_file>%s</data_file>\n" % str(self.data_file)
        xml += "<number_attenuator>%s</number_attenuator>\n" % str(self.number_attenuator)
        xml += "<peak_selection_from_pixel>%s</peak_selection_from_pixel>\n" % str(self.peak_selection[0])
        xml += "<peak_selection_to_pixel>%s</peak_selection_to_pixel>\n" % str(self.peak_selection[1])
        xml += "<back_selection_from_pixel>%s</back_selection_from_pixel>\n" % str(self.back_selection[0])
        xml += "<back_selection_to_pixel>%s</back_selection_to_pixel>\n" % str(self.back_selection[1])
        xml += "<lambda_requested>%s</lambda_requested>\n" % str(self.lambda_requested)
        xml += "<s1h>%s</s1h>\n" % str(self.s1h)
        xml += "<s2h>%s</s2h>\n" % str(self.s2h)
        xml += "<s1w>%s</s1w>\n" % str(self.s1w)
        xml += "<s2w>%s</s2w>\n" % str(self.s2w)
        xml += "<scaling_factor_file>%s</scaling_factor_file>\n" % str(self.scaling_factor_file)
        xml += "</RefLSFCalculator>\n"

        return xml

    def from_xml(self, xml_str):
        self.reset()
        dom = xml.dom.minidom.parseString(xml_str)
        self.from_xml_element(dom)
        element_list = dom.getElementsByTagName("RefLSFCalculator")
        if len(element_list)>0:
            instrument_dom = element_list[0]

    def from_xml_element(self, instrument_dom):
        """
            Read in data from XML
            @param xml_str: text to read the data from
        """
        #incident medium
        self.incident_medium_list = BaseScriptElement.getStringList(instrument_dom, "incident_medium_list")
        self.incident_medium_index_selected = BaseScriptElement.getIntElement(instrument_dom, "incident_medium_index_selected")

        self.tof_min = BaseScriptElement.getFloatElement(instrument_dom, "tof_min")
        self.tof_max = BaseScriptElement.getFloatElement(instrument_dom, "tof_max")

        #run number
        self.data_file = BaseScriptElement.getIntElement(instrument_dom, "data_file")

        #number of attenuator
        self.number_attenuator = BaseScriptElement.getIntElement(instrument_dom, "number_attenuator")

        #peak selection from and to
        self.peak_selection = [BaseScriptElement.getIntElement(instrument_dom, "peak_selection_from_pixel"),
                               BaseScriptElement.getIntElement(instrument_dom, "peak_selection_to_pixel")]

        #background flag and selection from and to
        self.back_selection = [BaseScriptElement.getIntElement(instrument_dom, "back_selection_from_pixel"),
                               BaseScriptElement.getIntElement(instrument_dom, "back_selection_to_pixel")]

        #lambda requested
        self.lambda_requested = BaseScriptElement.getStringElement(instrument_dom, "lambda_requested")

        #s1h, s2h, s1w, s2w
        self.s1h = BaseScriptElement.getStringElement(instrument_dom, "s1h")
        self.s2h = BaseScriptElement.getStringElement(instrument_dom, "s2h")
        self.s1w = BaseScriptElement.getStringElement(instrument_dom, "s1w")
        self.s2w = BaseScriptElement.getStringElement(instrument_dom, "s2w")

        #scaling factor file
        self.scaling_factor_file = BaseScriptElement.getStringElement(instrument_dom, "scaling_factor_file")

    def reset(self):
        """
            Reset state
        """
        self.data_file = DataSets.data_file
        self.incident_medium_list = DataSets.incident_medium_list
        self.incident_medium_index_selected = DataSets.incident_medium_index_selected
        self.number_attenuator = DataSets.number_attenuator
        self.peak_selection = DataSets.peak_selection
        self.back_selection = DataSets.back_selection
        self.lambda_requested = DataSets.lambda_requested
        self.s1h = DataSets.s1h
        self.s2h = DataSets.s2h
        self.s1w = DataSets.s1w
        self.s2w = DataSets.s2w
        self.tof_min = DataSets.tof_min
        self.tof_max = DataSets.tof_max
        self.scaling_factor_file = DataSets.scaling_factor_file
