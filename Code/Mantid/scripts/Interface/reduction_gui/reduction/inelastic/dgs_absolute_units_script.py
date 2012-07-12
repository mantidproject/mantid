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
    absunits_vanadium = ''
    grouping_file = ''
    absunits_detector_vanadium = ''
    incident_energy = ''
    emin = -1.0
    emax = 1.0
    vandium_mass = 32.58
    sample_mass = 1.0
    sample_rmm = 1.0
    
    def __init__(self):
        super(AbsoluteUnitsScript, self).__init__()
        self.reset()
        
    def to_script(self):
        script =  "DoAbsoluteUnits=%s,\n" % self.do_absolute_units
        if self.do_absolute_units:
            script += "AbsUnitsVanadium=\"%s\",\n" % self.absunits_vanadium
            script += "AbsUnitsGroupingFile=\"%s\",\n" % self.grouping_file
            script += "AbsUnitsDetectorVanadium=\"%s\",\n" % self.absunits_detector_vanadium
            script += "AbsUnitsIncidentEnergy=\"%s\",\n" % self.incident_energy
            script += "AbsUnitsMinimumEnergy=%s,\n" % str(self.emin)
            script += "AbsUnitsMaximumEnergy=%s,\n" % str(self.emax)
            script += "VanadiumMass=%s,\n" % str(self.vandium_mass)
            script += "SampleMass=%s,\n" % str(self.sample_mass)
            script += "SampleRmm=%s,\n" % str(self.sample_rmm)
        return script
    
    def to_xml(self):
        """
            Create XML from the current data.
        """
        xml =  "<AbsoluteUnits>\n"
        xml += "  <do_absolute_units>%s</do_absolute_units>\n" % self.do_absolute_units
        xml += "  <absunits_vanadium>%s</absunits_vanadium>\n" % self.absunits_vanadium
        xml += "  <grouping_file>%s</grouping_file>\n" % self.grouping_file
        xml += "  <absunits_detector_vanadium>%s</absunits_detector_vanadium>\n" % self.absunits_detector_vanadium
        xml += "  <incident_energy>%s</incident_energy>\n" % self.incident_energy
        xml += "  <minimum_energy>%s</minimum_energy>\n" % str(self.emin)
        xml += "  <maximum_energy>%s</maximum_energy>\n" % str(self.emax)
        xml += "  <vanadium_mass>%s</vanadium_mass>\n" % str(self.vandium_mass)
        xml += "  <sample_mass>%s</sample_mass>\n" % str(self.sample_mass)
        xml += "  <sample_rmm>%s</sample_rmm>\n" % str(self.sample_rmm)
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
            self.absunits_vanadium = BaseScriptElement.getStringElement(instrument_dom,
                                                                        "absunits_vanadium",
                                                                        default=AbsoluteUnitsScript.absunits_vanadium)
            self.grouping_file = BaseScriptElement.getStringElement(instrument_dom,
                                                                    "grouping_file",
                                                                    default=AbsoluteUnitsScript.grouping_file)
            self.absunits_detector_vanadium = BaseScriptElement.getStringElement(instrument_dom,
                                                                                 "absunits_detector_vanadium",
                                                                                 default=AbsoluteUnitsScript.absunits_detector_vanadium)
            self.incident_energy = BaseScriptElement.getStringElement(instrument_dom,
                                                                      "incident_energy",
                                                                      default=AbsoluteUnitsScript.incident_energy)
            self.emin = BaseScriptElement.getFloatElement(instrument_dom,
                                                          "minimum_energy",
                                                          default=AbsoluteUnitsScript.emin)
            self.emax = BaseScriptElement.getFloatElement(instrument_dom,
                                                          "maximum_energy",
                                                          default=AbsoluteUnitsScript.emax)
            self.vandium_mass = BaseScriptElement.getFloatElement(instrument_dom,
                                                                  "vanadium_mass",
                                                                  default=AbsoluteUnitsScript.vandium_mass)
            self.sample_mass = BaseScriptElement.getFloatElement(instrument_dom,
                                                                 "sample_mass",
                                                                 default=AbsoluteUnitsScript.sample_mass)
            self.sample_rmm = BaseScriptElement.getFloatElement(instrument_dom,
                                                                "sample_rmm",
                                                                default=AbsoluteUnitsScript.sample_rmm)            

    def reset(self):
        """
            Reset state
        """
        self.do_absolute_units = AbsoluteUnitsScript.do_absolute_units
        self.absunits_vanadium = AbsoluteUnitsScript.absunits_vanadium
        self.grouping_file = AbsoluteUnitsScript.grouping_file
        self.absunits_detector_vanadium = AbsoluteUnitsScript.absunits_detector_vanadium
        self.incident_energy = AbsoluteUnitsScript.incident_energy
        self.emin = AbsoluteUnitsScript.emin
        self.emax = AbsoluteUnitsScript.emax
        self.vandium_mass = AbsoluteUnitsScript.vandium_mass
        self.sample_mass = AbsoluteUnitsScript.sample_mass
        self.sample_rmm = AbsoluteUnitsScript.sample_rmm