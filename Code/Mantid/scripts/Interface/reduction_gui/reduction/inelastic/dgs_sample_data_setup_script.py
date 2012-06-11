"""
    Classes for each reduction step. Those are kept separately 
    from the the interface class so that the DgsReduction class could 
    be used independently of the interface implementation
"""
import xml.dom.minidom
import os
import time
from reduction_gui.reduction.scripter import BaseScriptElement

class SampleSetupScript(BaseScriptElement):
    
    sample_data = ""
    
    def __init__(self):
        super(SampleSetupScript, self).__init__()
        self.reset()
        
    def to_script(self):
        script = "SampleData=\"%s\"\n" % self.sample_data
        return script
        
    def to_xml(self):
        """
            Create XML from the current data.
        """
        xml = "<SampleSetup>\n"
        xml += "  <sample_data>%s</sample_data>\n" % self.sample_data
        xml += "</SampleSetup>\n"
        return xml
    
    def from_xml(self, xml_str):
        """
            Read in data from XML
            @param xml_str: text to read the data from
        """       
        dom = xml.dom.minidom.parseString(xml_str)
        element_list = dom.getElementsByTagName("SampleSetup")
        if len(element_list)>0:
            instrument_dom = element_list[0]
            self.sample_data = BaseScriptElement.getStringElement(instrument_dom, 
                                                                  "sample_data",
                                                                  default=Background.sample_data)      

    
    def reset(self):
        """
            Reset state
        """
        self.sample_data = SampleSetupScript.sample_data