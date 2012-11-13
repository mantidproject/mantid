"""
    Classes for each reduction step. Those are kept separately 
    from the the interface class so that the DgsReduction class could 
    be used independently of the interface implementation
"""
import os
import time
import xml.dom.minidom

from reduction_gui.reduction.scripter import BaseScriptElement

class PdAndScConversionScript(BaseScriptElement):
    
    pd_q_range_low = ""
    pd_q_range_width = ""
    pd_q_range_high = ""
    
    def __init__(self, inst_name):
        super(PdAndScConversionScript, self).__init__()
        self.reset()
        
    def to_script(self):
        script = ""
        return script
        
    def to_xml(self):
        """
            Create XML from the current data.
        """
        xml = "<PdAndScConversion>\n"
        xml += "</PdAndScConversion>\n"
        
        return xml

    def from_xml(self, xml_str):
        """
            Read in data from XML
            @param xml_str: text to read the data from
        """       
        dom = xml.dom.minidom.parseString(xml_str)
        element_list = dom.getElementsByTagName("PdAndScConversion")
        if len(element_list)>0:
            instrument_dom = element_list[0]
             
    def reset(self):
        self.pd_q_range_low = PdAndScConversionScript.pd_q_range_low
        self.pd_q_range_width = PdAndScConversionScript.pd_q_range_width
        self.pd_q_range_high = PdAndScConversionScript.pd_q_range_high