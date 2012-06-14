"""
    Classes for each reduction step. Those are kept separately 
    from the the interface class so that the DgsReduction class could 
    be used independently of the interface implementation
"""
import os
import time
import xml.dom.minidom

from reduction_gui.reduction.scripter import BaseScriptElement

class DiagnoseDetectorsScript(BaseScriptElement):

    find_bad_detectors = False

    def __init__(self):
        super(DiagnoseDetectorsScript, self).__init__()
        self.reset()
        
    def to_script(self):
        script = "FindBadDetectors=%s,\n" % self.find_bad_detectors        
        return script
    
    def to_xml(self):
        """
            Create XML from the current data.
        """
        xml =  "<DiagnoseDetectors>"
        xml += "  <find_bad_detectors>%s</find_bad_detectors>\n" % self.find_bad_detectors
        xml += "</DiagnoseDetectors>"
        return xml
    
    def from_xml(self, xml_str):
        """
            Read in data from XML
            @param xml_str: text to read the data from
        """       
        dom = xml.dom.minidom.parseString(xml_str)
        element_list = dom.getElementsByTagName("DiagnoseDetectors")
        if len(element_list)>0:
            instrument_dom = element_list[0]
            self.find_bad_detectors = BaseScriptElement.getBoolElement(instrument_dom,
                                                                       "find_bad_detectors",
                                                                       default=DiagnoseDetectorsScript.find_bad_detectors)

    def reset(self):
        """
            Reset state
        """
        self.find_bad_detectors = DiagnoseDetectorsScript.find_bad_detectors