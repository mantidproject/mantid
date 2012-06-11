"""
    Classes for each reduction step. Those are kept separately 
    from the the interface class so that the DgsReduction class could 
    be used independently of the interface implementation
"""
import os
import time
import xml.dom.minidom

from reduction_gui.reduction.scripter import BaseScriptElement

class SampleSetupScript(BaseScriptElement):
    
    sample_data = ""
    incident_energy = ""
    fixed_ei = False
    et_range_low = ""
    et_range_width = ""
    et_range_high = ""
    
    def __init__(self):
        super(SampleSetupScript, self).__init__()
        self.reset()
        
    def to_script(self):
        script =  "SampleData=\"%s\"\n" % self.sample_data
        script += "IncidentEnergy=\"%s\"\n" % self.incident_energy
        script += "FixedIncidentEnergy=\"%s\"\n" % self.fixed_ei
        script += "EnergyTransferRange=\"%s,%s,%s\"" % (self.et_range_low, 
                                                        self.et_range_width, 
                                                        self.et_range_high)
        return script
        
    def to_xml(self):
        """
            Create XML from the current data.
        """
        xml = "<SampleSetup>\n"
        xml += "  <sample_data>%s</sample_data>\n" % self.sample_data
        xml += "  <incident_energy>%s</incident_energy>\n" % self.incident_energy
        xml += "  <fixed_ei>%s</fixed_ei>\n" % str(self.fixed_ei)
        xml += "  <et_range>\n"
        xml += "    <low>%s</low>\n" % self.et_range_low
        xml += "    <width>%s</width>\n"  % self.et_range_width
        xml += "    <high>%s</high>\n" % self.et_range_high
        xml += "  </et_range>\n" 
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
                                                                  default=SampleSetupScript.sample_data)
            self.incident_energy = BaseScriptElement.getStringElement(instrument_dom,
                                                                     "incident_energy",
                                                                     default=SampleSetupScript.incident_energy)
            self.fixed_ei = BaseScriptElement.getBoolElement(instrument_dom,
                                                             "fixed_ei",
                                                             default=SampleSetupScript.fixed_ei)
            self.et_range_low = BaseScriptElement.getStringElement(instrument_dom,
                                                                   "et_range/low",
                                                                   default=SampleSetupScript.et_range_low)
            self.et_range_width = BaseScriptElement.getStringElement(instrument_dom,
                                                                     "et_range/width",
                                                                     default=SampleSetupScript.et_range_width)

            self.et_range_high = BaseScriptElement.getStringElement(instrument_dom,
                                                                    "et_range/high",
                                                                    default=SampleSetupScript.et_range_high)

    def reset(self):
        """
            Reset state
        """
        self.sample_data = SampleSetupScript.sample_data
        self.incident_energy = SampleSetupScript.incident_energy
        self.fixed_ei = SampleSetupScript.fixed_ei
        self.et_range_low = SampleSetupScript.et_range_low
        self.et_range_width = SampleSetupScript.et_range_width
        self.et_range_high = SampleSetupScript.et_range_high
