# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=invalid-name
"""
Classes for each reduction step. Those are kept separately
from the interface class so that the DgsReduction class could
be used independently of the interface implementation
"""

import xml.dom.minidom

from reduction_gui.reduction.scripter import BaseScriptElement


class DataCorrectionsScript(BaseScriptElement):
    INCIDENT_BEAM_NORM_TYPES = ("None", "ByCurrent", "ToMonitor")

    filter_bad_pulses = False
    incident_beam_norm = INCIDENT_BEAM_NORM_TYPES[0]
    monitor_int_low = ""
    monitor_int_high = ""
    tib_subtraction = False
    tib_tof_start = ""
    tib_tof_end = ""
    correct_kikf = True
    detector_vanadium = ""
    detvan_integration = False
    detvan_int_range_low = ""
    detvan_int_range_high = ""
    detvan_int_range_units = "Energy"
    save_proc_detvan = False
    save_proc_detvan_file = ""
    use_proc_detvan = False

    def __init__(self, inst_name):
        super(DataCorrectionsScript, self).__init__()
        self.set_default_pars(inst_name)
        self.reset()

    def set_default_pars(self, inst_name):
        from . import dgs_utils

        ip = dgs_utils.InstrumentParameters(inst_name)
        DataCorrectionsScript.monitor_int_low = int(ip.get_parameter("norm-mon1-min"))
        DataCorrectionsScript.monitor_int_high = int(ip.get_parameter("norm-mon1-max"))
        DataCorrectionsScript.tib_tof_start = int(ip.get_parameter("bkgd-range-min"))
        DataCorrectionsScript.tib_tof_end = int(ip.get_parameter("bkgd-range-max"))
        DataCorrectionsScript.detvan_int_range_low = ip.get_parameter("wb-integr-min")
        DataCorrectionsScript.detvan_int_range_high = ip.get_parameter("wb-integr-max")

    def to_script(self):
        script = ""
        if self.filter_bad_pulses:
            script += "FilterBadPulses=%s,\n" % self.filter_bad_pulses
        script += 'IncidentBeamNormalisation="%s",\n' % self.incident_beam_norm
        if self.incident_beam_norm == "ToMonitor":
            if self.monitor_int_low != DataCorrectionsScript.monitor_int_low:
                script += "MonitorIntRangeLow=%s,\n" % self.monitor_int_low
            if self.monitor_int_high != DataCorrectionsScript.monitor_int_high:
                script += "MonitorIntRangeHigh=%s,\n" % self.monitor_int_high
        if self.tib_subtraction:
            script += "TimeIndepBackgroundSub=%s,\n" % self.tib_subtraction
            if self.tib_tof_start != DataCorrectionsScript.tib_tof_start:
                script += "TibTofRangeStart=%s,\n" % self.tib_tof_start
            if self.tib_tof_end != DataCorrectionsScript.tib_tof_end:
                script += "TibTofRangeEnd=%s,\n" % self.tib_tof_end
        if not self.correct_kikf:
            script += "CorrectKiKf=%s,\n" % self.correct_kikf
        if self.detector_vanadium != "":
            script += 'DetectorVanadiumInputFile="%s",\n' % self.detector_vanadium
            if self.detvan_integration:
                if self.detvan_integration:
                    script += "UseBoundsForDetVan=%s,\n" % self.detvan_integration
                    if self.detvan_int_range_low != DataCorrectionsScript.detvan_int_range_low:
                        script += "DetVanIntRangeLow=%s,\n" % self.detvan_int_range_low
                    script += "DetVanIntRangeHigh=%s,\n" % self.detvan_int_range_high
                if self.detvan_int_range_units != DataCorrectionsScript.detvan_int_range_units:
                    script += 'DetVanIntRangeUnits="%s",\n' % self.detvan_int_range_units
            if self.save_proc_detvan:
                script += "SaveProcessedDetVan=%s,\n" % self.save_proc_detvan
                if self.save_proc_detvan_file != DataCorrectionsScript.save_proc_detvan_file:
                    script += 'SaveProcDetVanFilename="%s",\n' % self.save_proc_detvan_file
            if self.use_proc_detvan:
                script += "UseProcessedDetVan=%s,\n" % self.use_proc_detvan

        return script

    def to_xml(self):
        """
        Create XML from the current data.
        """
        xml = "<DataCorrections>\n"
        xml += "  <filter_bad_pulses>%s</filter_bad_pulses>\n" % str(self.filter_bad_pulses)
        xml += "  <incident_beam_norm>%s</incident_beam_norm>\n" % self.incident_beam_norm
        xml += "  <monint_range_low>%d</monint_range_low>\n" % self.monitor_int_low
        xml += "  <monint_range_high>%d</monint_range_high>\n" % self.monitor_int_high
        xml += "  <timeindepbkg_sub>%s</timeindepbkg_sub>,\n" % self.tib_subtraction
        xml += "  <tib_tof_range_start>%d</tib_tof_range_start>\n" % self.tib_tof_start
        xml += "  <tib_tof_range_end>%d</tib_tof_range_end>\n" % self.tib_tof_end
        xml += "  <correct_kikf>%s</correct_kikf>\n" % str(self.correct_kikf)
        xml += "  <detector_vanadium>%s</detector_vanadium>\n" % self.detector_vanadium
        xml += "  <use_bounds_detvan>%s</use_bounds_detvan>\n" % str(self.detvan_integration)
        xml += "  <detvan_range_low>%s</detvan_range_low>\n" % self.detvan_int_range_low
        xml += "  <detvan_range_high>%s</detvan_range_high>\n" % self.detvan_int_range_high
        xml += "  <detvan_range_units>%s</detvan_range_units>\n" % self.detvan_int_range_units
        xml += "  <save_proc_detvan>%s</save_proc_detvan>\n" % str(self.save_proc_detvan)
        xml += "  <save_proc_detvan_filename>%s</save_proc_detvan_filename>\n" % str(self.save_proc_detvan_file)
        xml += "  <use_proc_detvan>%s</use_proc_detvan>\n" % str(self.use_proc_detvan)
        xml += "</DataCorrections>\n"
        return xml

    def from_xml(self, xml_str):
        """
        Read in data from XML
        @param xml_str: text to read the data from
        """
        dom = xml.dom.minidom.parseString(xml_str)
        element_list = dom.getElementsByTagName("DataCorrections")
        if len(element_list) > 0:
            instrument_dom = element_list[0]
            self.filter_bad_pulses = BaseScriptElement.getBoolElement(
                instrument_dom, "filter_bad_pulses", default=DataCorrectionsScript.filter_bad_pulses
            )
            self.incident_beam_norm = BaseScriptElement.getStringElement(
                instrument_dom, "incident_beam_norm", default=DataCorrectionsScript.incident_beam_norm
            )
            self.monitor_int_low = BaseScriptElement.getIntElement(
                instrument_dom, "monint_range_low", default=DataCorrectionsScript.monitor_int_low
            )
            self.monitor_int_high = BaseScriptElement.getIntElement(
                instrument_dom, "monint_range_high", default=DataCorrectionsScript.monitor_int_high
            )
            self.tib_subtraction = BaseScriptElement.getBoolElement(
                instrument_dom, "timeindepbkg_sub", default=DataCorrectionsScript.tib_subtraction
            )
            self.tib_tof_start = BaseScriptElement.getIntElement(
                instrument_dom, "tib_tof_range_start", default=DataCorrectionsScript.tib_tof_start
            )
            self.tib_tof_end = BaseScriptElement.getIntElement(
                instrument_dom, "tib_tof_range_end", default=DataCorrectionsScript.tib_tof_end
            )
            self.correct_kikf = BaseScriptElement.getBoolElement(instrument_dom, "correct_kikf", default=DataCorrectionsScript.correct_kikf)
            self.detector_vanadium = BaseScriptElement.getStringElement(
                instrument_dom, "detector_vanadium", default=DataCorrectionsScript.detector_vanadium
            )
            self.detvan_integration = BaseScriptElement.getBoolElement(
                instrument_dom, "use_bounds_detvan", default=DataCorrectionsScript.detvan_integration
            )
            self.detvan_int_range_low = BaseScriptElement.getStringElement(
                instrument_dom, "detvan_range_low", default=DataCorrectionsScript.detvan_int_range_low
            )
            self.detvan_int_range_high = BaseScriptElement.getStringElement(
                instrument_dom, "detvan_range_high", default=DataCorrectionsScript.detvan_int_range_high
            )
            self.detvan_int_range_units = BaseScriptElement.getStringElement(
                instrument_dom, "detvan_range_units", default=DataCorrectionsScript.detvan_int_range_units
            )
            self.save_proc_detvan = BaseScriptElement.getBoolElement(
                instrument_dom, "save_proc_detvan", default=DataCorrectionsScript.save_proc_detvan
            )
            self.save_proc_detvan_file = BaseScriptElement.getStringElement(
                instrument_dom, "save_proc_detvan_filename", default=DataCorrectionsScript.save_proc_detvan_file
            )
            self.use_proc_detvan = BaseScriptElement.getBoolElement(
                instrument_dom, "use_proc_detvan", default=DataCorrectionsScript.use_proc_detvan
            )

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
        self.correct_kikf = DataCorrectionsScript.correct_kikf
        self.detector_vanadium = DataCorrectionsScript.detector_vanadium
        self.detvan_integration = DataCorrectionsScript.detvan_integration
        self.detvan_int_range_low = DataCorrectionsScript.detvan_int_range_low
        self.detvan_int_range_high = DataCorrectionsScript.detvan_int_range_high
        self.detvan_int_range_units = DataCorrectionsScript.detvan_int_range_units
        self.save_proc_detvan = DataCorrectionsScript.save_proc_detvan
        self.save_proc_detvan_file = DataCorrectionsScript.save_proc_detvan_file
        self.use_proc_detvan = DataCorrectionsScript.use_proc_detvan
