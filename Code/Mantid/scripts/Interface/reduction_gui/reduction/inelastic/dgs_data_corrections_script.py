"""
    Classes for each reduction step. Those are kept separately 
    from the the interface class so that the DgsReduction class could 
    be used independently of the interface implementation
"""
import os
import time
import xml.dom.minidom

from reduction_gui.reduction.scripter import BaseScriptElement

class DataCorrectionsScript(BaseScriptElement):

    filter_bad_pulses = False
    
    def __init__(self):
        super(DataCorrectionsScript, self).__init__()
        self.reset()
        
    def to_script(self):
        script =  "FilterBadPulses=%s,\n" % self.filter_bad_pulses
        return script
    
    def to_xml(self):
        """
            Create XML from the current data.
        """
        xml =  "<DataCorrections>\n"
        xml += "  <filter_bad_pulses>%s</filter_bad_pulses>\n" % str(self.filter_bad_pulses)
        xml += "</DataCorrections>\n"
        return xml
    
    def from_xml(self, xml_str):
        """
            Read in data from XML
            @param xml_str: text to read the data from
        """       
        dom = xml.dom.minidom.parseString(xml_str)
        element_list = dom.getElementsByTagName("DataCorrections")
        if len(element_list)>0:
            instrument_dom = element_list[0]
            self.filter_bad_pulses = BaseScriptElement.getBoolElement(instrument_dom, 
                                                                      "filter_bad_pulses",
                                                                      default=DataCorrectionsScript.filter_bad_pulses)

    def reset(self):
        """
            Reset state
        """
        self.filter_bad_pulses = DataCorrectionsScript.filter_bad_pulses