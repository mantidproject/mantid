"""
    Classes for each reduction step. Those are kept separately 
    from the the interface class so that the DgsReduction class could 
    be used independently of the interface implementation
"""
import os
import time
import xml.dom.minidom

from reduction_gui.reduction.scripter import BaseScriptElement

class AbsoluteUnitsScript(BaseScriptElement):
    
    do_absolute_units = False
    
    def __init__(self):
        super(AbsoluteUnitsScript, self).__init__()
        self.reset()
        
    def to_script(self):
        script =  "DoAbsoluteUnits=%s,\n" % self.do_absolute_units
        return script
    
    def to_xml(self):
        """
            Create XML from the current data.
        """
        xml =  "<AbsoluteUnits>\n"
        xml += "  <do_absolute_units>%s</do_absolute_units>\n" % self.do_absolute_units
        xml += "</AbsoluteUnits>\n"
        return xml
    
    def from_xml(self, xml_str):
        """
            Read in data from XML
            @param xml_str: text to read the data from
        """       
        dom = xml.dom.minidom.parseString(xml_str)
        element_list = dom.getElementsByTagName("AbsoluteUnits")
        if len(element_list)>0:
            instrument_dom = element_list[0]
            self.find_bad_detectors = BaseScriptElement.getBoolElement(instrument_dom,
                                                                       "do_absolute_units",
                                                                       default=AbsoluteUnitsScript.do_absolute_units)

    def reset(self):
        """
            Reset state
        """
        self.do_absolute_units = AbsoluteUnitsScript.do_absolute_units