#pylint: disable=invalid-name
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
    vanadium_mass = 32.58
    sample_mass = 1.0
    sample_rmm = 1.0
    absunits_median_test_high = 1.2
    absunits_median_test_low = 0.8
    absunits_median_test_out_high = 100
    absunits_median_test_out_low = 0.01
    absunits_errorbar_criterion = 0.0
    find_bad_detectors = None

    def __init__(self, inst_name):
        super(AbsoluteUnitsScript, self).__init__()
        self.set_default_pars(inst_name)
        self.reset()

    def set_default_pars(self, inst_name):
        import dgs_utils
        ip = dgs_utils.InstrumentParameters(inst_name)
        AbsoluteUnitsScript.emin = ip.get_parameter("monovan-integr-min")
        AbsoluteUnitsScript.emax = ip.get_parameter("monovan-integr-max")
        AbsoluteUnitsScript.vanadium_mass = ip.get_parameter("vanadium-mass")
        AbsoluteUnitsScript.absunits_median_test_out_high = ip.get_parameter("monovan_hi_bound")
        AbsoluteUnitsScript.absunits_median_test_out_low = ip.get_parameter("monovan_lo_bound")
        AbsoluteUnitsScript.absunits_median_test_high = ip.get_parameter("monovan_hi_frac")
        AbsoluteUnitsScript.absunits_median_test_low = ip.get_parameter("monovan_lo_frac")
        AbsoluteUnitsScript.absunits_errorbar_criterion = ip.get_parameter("diag_samp_sig")

    def to_script(self):
        script = ""
        if self.do_absolute_units:
            script += "DoAbsoluteUnits=%s,\n" % self.do_absolute_units
            script += "AbsUnitsSampleInputFile=\"%s\",\n" % self.absunits_vanadium
            if self.grouping_file != AbsoluteUnitsScript.grouping_file:
                script += "AbsUnitsGroupingFile=\"%s\",\n" % self.grouping_file
            if self.absunits_detector_vanadium != AbsoluteUnitsScript.absunits_detector_vanadium:
                script += "AbsUnitsDetectorVanadiumInputFile=\"%s\",\n" % self.absunits_detector_vanadium
            if self.incident_energy != AbsoluteUnitsScript.incident_energy:
                script += "AbsUnitsIncidentEnergy=%s,\n" % str(self.incident_energy)
            if self.emin != AbsoluteUnitsScript.emin:
                script += "AbsUnitsMinimumEnergy=%s,\n" % str(self.emin)
            if self.emax != AbsoluteUnitsScript.emax:
                script += "AbsUnitsMaximumEnergy=%s,\n" % str(self.emax)
            if self.vanadium_mass != AbsoluteUnitsScript.vanadium_mass:
                script += "VanadiumMass=%s,\n" % str(self.vanadium_mass)
            if self.sample_mass != AbsoluteUnitsScript.sample_mass:
                script += "SampleMass=%s,\n" % str(self.sample_mass)
            if self.sample_rmm != AbsoluteUnitsScript.sample_rmm:
                script += "SampleRmm=%s,\n" % str(self.sample_rmm)
            if self.absunits_median_test_out_low != AbsoluteUnitsScript.absunits_median_test_out_low:
                script += "AbsUnitsLowOutlier=%s,\n" % str(self.absunits_median_test_out_low)
            if self.absunits_median_test_out_high != AbsoluteUnitsScript.absunits_median_test_out_high:
                script += "AbsUnitsHighOutlier=%s,\n" % str(self.absunits_median_test_out_high)
            if self.absunits_median_test_high != AbsoluteUnitsScript.absunits_median_test_high:
                script += "AbsUnitsMedianTestHigh=%s,\n" % str(self.absunits_median_test_high)
            if self.absunits_median_test_low != AbsoluteUnitsScript.absunits_median_test_low:
                script += "AbsUnitsMedianTestLow=%s,\n" % str(self.absunits_median_test_low)
            if self.absunits_errorbar_criterion != AbsoluteUnitsScript.absunits_errorbar_criterion:
                script += "AbsUnitsErrorBarCriterion=%s,\n" % str(self.absunits_errorbar_criterion)
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
        xml += "  <vanadium_mass>%s</vanadium_mass>\n" % str(self.vanadium_mass)
        xml += "  <sample_mass>%s</sample_mass>\n" % str(self.sample_mass)
        xml += "  <sample_rmm>%s</sample_rmm>\n" % str(self.sample_rmm)
        xml += "  <median_test_outlier_low>%s</median_test_outlier_low>\n" % str(self.absunits_median_test_out_low)
        xml += "  <median_test_outlier_high>%s</median_test_outlier_high>\n" % str(self.absunits_median_test_out_high)
        xml += "  <median_test_low>%s</median_test_low>\n" % str(self.absunits_median_test_low)
        xml += "  <median_test_high>%s</median_test_high>\n" % str(self.absunits_median_test_high)
        xml += "  <errorbar_criterion>%s</errorbar_criterion>\n" % str(self.absunits_errorbar_criterion)
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
            self.vanadium_mass = BaseScriptElement.getFloatElement(instrument_dom,
                                                                   "vanadium_mass",
                                                                   default=AbsoluteUnitsScript.vanadium_mass)
            self.sample_mass = BaseScriptElement.getFloatElement(instrument_dom,
                                                                 "sample_mass",
                                                                 default=AbsoluteUnitsScript.sample_mass)
            self.sample_rmm = BaseScriptElement.getFloatElement(instrument_dom,
                                                                "sample_rmm",
                                                                default=AbsoluteUnitsScript.sample_rmm)
            self.absunits_median_test_out_low = BaseScriptElement.getFloatElement(instrument_dom,
                                                                                  "median_test_out_low",
                                                                                  default=AbsoluteUnitsScript.absunits_median_test_out_low)
            self.absunits_median_test_out_high = BaseScriptElement.getFloatElement(instrument_dom,
                                                                                   "median_test_out_high",
                                                                                   default=AbsoluteUnitsScript.absunits_median_test_out_high)
            self.absunits_median_test_low = BaseScriptElement.getFloatElement(instrument_dom,
                                                                              "median_test_low",
                                                                              default=AbsoluteUnitsScript.absunits_median_test_low)
            self.absunits_median_test_high = BaseScriptElement.getFloatElement(instrument_dom,
                                                                               "median_test_high",
                                                                               default=AbsoluteUnitsScript.absunits_median_test_high)
            self.absunits_errorbar_criterion = BaseScriptElement.getFloatElement(instrument_dom,
                                                                                 "errorbar_criterion",
                                                                                 default=AbsoluteUnitsScript.absunits_errorbar_criterion)

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
        self.vanadium_mass = AbsoluteUnitsScript.vanadium_mass
        self.sample_mass = AbsoluteUnitsScript.sample_mass
        self.sample_rmm = AbsoluteUnitsScript.sample_rmm
        self.absunits_median_test_out_low = AbsoluteUnitsScript.absunits_median_test_out_low
        self.absunits_median_test_out_high = AbsoluteUnitsScript.absunits_median_test_out_high
        self.absunits_median_test_low = AbsoluteUnitsScript.absunits_median_test_low
        self.absunits_median_test_high = AbsoluteUnitsScript.absunits_median_test_high
        self.absunits_errorbar_criterion = AbsoluteUnitsScript.absunits_errorbar_criterion
