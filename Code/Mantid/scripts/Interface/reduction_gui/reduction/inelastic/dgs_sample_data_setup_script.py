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
    
    sample_file = ""
    incident_energy_guess = ""
    use_ei_guess = False
    rebin_et = False
    et_range_low = ""
    et_range_width = ""
    et_range_high = ""
    et_is_distribution = True
    hardmask_file = ""
    grouping_file = ""
    
    def __init__(self):
        super(SampleSetupScript, self).__init__()
        self.reset()
        
    def to_script(self):
        script =  "SampleInputFile=\"%s\",\n" % self.sample_file
        script += "IncidentEnergyGuess=\"%s\",\n" % self.incident_energy_guess
        script += "UseIncidentEnergyGuess=%s,\n" % self.use_ei_guess
        if self.rebin_et:
            script += "EnergyTransferRange=\"%s,%s,%s\",\n" % (self.et_range_low, 
                                                               self.et_range_width, 
                                                               self.et_range_high)
            script += "SofPhiEIsDistribution=%s,\n" % self.et_is_distribution
        script += "HardMaskFile=\"%s\",\n" % self.hardmask_file
        script += "GroupingFile=\"%s\",\n" % self.grouping_file
        return script
        
    def to_xml(self):
        """
            Create XML from the current data.
        """
        xml = "<SampleSetup>\n"
        xml += "  <sample_input_file>%s</sample_input_file>\n" % self.sample_file
        xml += "  <incident_energy_guess>%s</incident_energy_guess>\n" % self.incident_energy_guess
        xml += "  <use_ei_guess>%s</use_ei_guess>\n" % str(self.use_ei_guess)
        xml += "  <et_range>\n"
        xml += "    <low>%s</low>\n" % self.et_range_low
        xml += "    <width>%s</width>\n"  % self.et_range_width
        xml += "    <high>%s</high>\n" % self.et_range_high
        xml += "  </et_range>\n"
        xml += "  <sofphie_is_distribution>%s</sofphie_is_distribution>" % str(self.et_is_distribution)
        xml += "  <hardmask_file>%s</hardmask_file>\n" % self.hardmask_file
        xml += "  <grouping_file>%s</grouping_file>\n" % self.grouping_file
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
            self.sample_file = BaseScriptElement.getStringElement(instrument_dom, 
                                                                  "sample_input_file",
                                                                  default=SampleSetupScript.sample_file)
            self.incident_energy_guess = BaseScriptElement.getStringElement(instrument_dom,
                                                                            "incident_energy_guess",
                                                                            default=SampleSetupScript.incident_energy_guess)
            self.use_ei_guess = BaseScriptElement.getBoolElement(instrument_dom,
                                                                 "use_ei_guess",
                                                                 default=SampleSetupScript.use_ei_guess)
            self.et_range_low = BaseScriptElement.getStringElement(instrument_dom,
                                                                   "et_range/low",
                                                                   default=SampleSetupScript.et_range_low)
            self.et_range_width = BaseScriptElement.getStringElement(instrument_dom,
                                                                     "et_range/width",
                                                                     default=SampleSetupScript.et_range_width)
            self.et_range_high = BaseScriptElement.getStringElement(instrument_dom,
                                                                    "et_range/high",
                                                                    default=SampleSetupScript.et_range_high)
            self.et_is_distribution = BaseScriptElement.getBoolElement(instrument_dom,
                                                                       "sofphie_is_distribution",
                                                                       default=SampleSetupScript.et_is_distribution)
            self.hardmask_file = BaseScriptElement.getStringElement(instrument_dom,
                                                                    "hardmask_file",
                                                                    default=SampleSetupScript.hardmask_file)
            self.grouping_file = BaseScriptElement.getStringElement(instrument_dom,
                                                                    "grouping_file",
                                                                    default=SampleSetupScript.grouping_file)

    def reset(self):
        """
            Reset state
        """
        self.sample_file = SampleSetupScript.sample_file
        self.incident_energy_guess = SampleSetupScript.incident_energy_guess
        self.use_ei_guess = SampleSetupScript.use_ei_guess
        self.rebin_et = SampleSetupScript.rebin_et
        self.et_range_low = SampleSetupScript.et_range_low
        self.et_range_width = SampleSetupScript.et_range_width
        self.et_range_high = SampleSetupScript.et_range_high
        self.et_is_distribution = SampleSetupScript.et_is_distribution
        self.hardmask_file = SampleSetupScript.hardmask_file
        self.grouping_file = SampleSetupScript.grouping_file
        