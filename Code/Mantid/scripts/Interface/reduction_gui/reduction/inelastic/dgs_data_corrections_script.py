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

    INCIDENT_BEAM_NORM_TYPES = ("None", "ByCurrent", "ToMonitor")

    filter_bad_pulses = False
    incident_beam_norm = INCIDENT_BEAM_NORM_TYPES[0]
    
    def __init__(self):
        super(DataCorrectionsScript, self).__init__()
        self.reset()
        
    def to_script(self):
        script =  "FilterBadPulses=%s,\n" % self.filter_bad_pulses
        script += "IncidentBeamNormalisation=\"%s\"\n," % self.incident_beam_norm
        return script
    
    def to_xml(self):
        """
            Create XML from the current data.
        """
        xml =  "<DataCorrections>\n"
        xml += "  <filter_bad_pulses>%s</filter_bad_pulses>\n" % str(self.filter_bad_pulses)
        xml += "  <incident_beam_norm>%s</incident_beam_norm>\n" % self.incident_beam_norm
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
            self.incident_beam_norm = BaseScriptElement.getBoolElement(instrument_dom, 
                                                                      "incident_beam_norm",
                                                                      default=DataCorrectionsScript.incident_beam_norm)

    def reset(self):
        """
            Reset state
        """
        self.filter_bad_pulses = DataCorrectionsScript.filter_bad_pulses
        self.incident_beam_norm = DataCorrectionsScript.incident_beam_norm
        