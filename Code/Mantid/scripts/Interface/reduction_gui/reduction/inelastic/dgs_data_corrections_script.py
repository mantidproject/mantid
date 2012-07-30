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
    monitor_int_low = ''
    monitor_int_high = ''
    tib_subtraction = False
    tib_tof_start = ''
    tib_tof_end = ''
    detector_vanadium = ''
    det_van_integration = False
    det_van_int_range_low = ''
    det_van_int_range_high = ''
    det_van_int_range_units = 'DeltaE'
    
    def __init__(self):
        super(DataCorrectionsScript, self).__init__()
        self.reset()
        
    def to_script(self):
        script =  "FilterBadPulses=%s,\n" % self.filter_bad_pulses
        script += "IncidentBeamNormalisation=\"%s\",\n" % self.incident_beam_norm
        if self.incident_beam_norm == "ToMonitor":
            script += "MonitorIntRangeLow=\"%s\",\n" % self.monitor_int_low
            script += "MonitorIntRangeHigh=\"%s\",\n" % self.monitor_int_high
        script += "TimeIndepBackgroundSub=%s,\n" % self.tib_subtraction
        if self.tib_subtraction:
            script += "TibTofRangeStart=\"%s\",\n" % self.tib_tof_start
            script += "TibTofRangeEnd=\"%s\",\n" % self.tib_tof_end
        script += "DetectorVanadiumInputFile=\"%s\",\n" % self.detector_vanadium
        script += "UseBoundsForDetVan=%s,\n" % self.det_van_integration
        if self.det_van_integration:
            script += "DetVanIntRangeLow=\"%s\",\n" % self.det_van_int_range_low
            script += "DetVanIntRangeHigh=\"%s\",\n" % self.det_van_int_range_high
            script += "DetVanIntRangeUnits=\"%s\",\n" % self.det_van_int_range_units
        return script
    
    def to_xml(self):
        """
            Create XML from the current data.
        """
        xml =  "<DataCorrections>\n"
        xml += "  <filter_bad_pulses>%s</filter_bad_pulses>\n" % str(self.filter_bad_pulses)
        xml += "  <incident_beam_norm>%s</incident_beam_norm>\n" % self.incident_beam_norm
        xml += "  <monint_range_low>%s</monint_range_low>\n" % self.monitor_int_low
        xml += "  <monint_range_high>%s</monint_range_high>\n" % self.monitor_int_high
        xml += "  <timeindepbkg_sub>%s</timeindepbkg_sub>,\n" % self.tib_subtraction
        xml += "  <tib_tof_range_start>%s</tib_tof_range_start>\n" % self.tib_tof_start
        xml += "  <tib_tof_range_end>%s</tib_tof_range_end>\n" % self.tib_tof_end
        xml += "  <detector_vanadium>%s</detector_vanadium>\n" % self.detector_vanadium
        xml += "  <use_bounds_detvan>%s</use_bounds_detvan>\n" % str(self.det_van_integration)
        xml += "  <detvan_range_low>%s</detvan_range_low>\n" % self.det_van_int_range_low
        xml += "  <detvan_range_high>%s</detvan_range_high>\n" % self.det_van_int_range_high
        xml += "  <detvan_range_units>%s</detvan_range_units>\n" % self.det_van_int_range_units
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
            self.incident_beam_norm = BaseScriptElement.getStringElement(instrument_dom, 
                                                                         "incident_beam_norm",
                                                                         default=DataCorrectionsScript.incident_beam_norm)
            self.monitor_int_low = BaseScriptElement.getStringElement(instrument_dom,
                                                                      "",
                                                                      default=DataCorrectionsScript.monitor_int_low)
            self.monitor_int_high = BaseScriptElement.getStringElement(instrument_dom,
                                                                       "",
                                                                       default=DataCorrectionsScript.monitor_int_high)
            self.tib_subtraction = BaseScriptElement.getBoolElement(instrument_dom,
                                                                    "timeindepbkg_sub",
                                                                    default=DataCorrectionsScript.tib_subtraction)
            self.tib_tof_start = BaseScriptElement.getStringElement(instrument_dom,
                                                                    "tib_tof_range_start",
                                                                    default=DataCorrectionsScript.tib_tof_start)
            self.tib_tof_end = BaseScriptElement.getStringElement(instrument_dom,
                                                                  "tib_tof_range_end",
                                                                  default=DataCorrectionsScript.tib_tof_end)
            self.detector_vanadium = BaseScriptElement.getStringElement(instrument_dom,
                                                                        "detector_vanadium",
                                                                        default=DataCorrectionsScript.detector_vanadium)
            self.det_van_integration = BaseScriptElement.getBoolElement(instrument_dom,
                                                                        "use_bounds_detvan",
                                                                        default=DataCorrectionsScript.det_van_integration)
            self.det_van_int_range_low = BaseScriptElement.getStringElement(instrument_dom,
                                                                            "detvan_range_low",
                                                                            default=DataCorrectionsScript.det_van_int_range_low)
            self.det_van_int_range_high = BaseScriptElement.getStringElement(instrument_dom,
                                                                             "detvan_range_high",
                                                                             default=DataCorrectionsScript.det_van_int_range_high)
            self.det_van_int_range_units = BaseScriptElement.getStringElement(instrument_dom,
                                                                              "detvan_range_units",
                                                                              default=DataCorrectionsScript.det_van_int_range_units)

    def reset(self):
        """
            Reset state
        """
        self.filter_bad_pulses = DataCorrectionsScript.filter_bad_pulses
        self.incident_beam_norm = DataCorrectionsScript.incident_beam_norm
        self.monitor_int_low = DataCorrectionsScript.monitor_int_low
        self.monitor_int_high = DataCorrectionsScript.monitor_int_high
        self.tib_subtraction = DataCorrectionsScript.tib_subtraction
        self.tib_tof_start = DataCorrectionsScript.tib_tof_start
        self.tib_tof_end = DataCorrectionsScript.tib_tof_end
        self.detector_vanadium = DataCorrectionsScript.detector_vanadium
        self.det_van_integration = DataCorrectionsScript.det_van_integration
        self.det_van_int_range_low = DataCorrectionsScript.det_van_int_range_low
        self.det_van_int_range_high = DataCorrectionsScript.det_van_int_range_high
        self.det_van_int_range_units = DataCorrectionsScript.det_van_int_range_units
        