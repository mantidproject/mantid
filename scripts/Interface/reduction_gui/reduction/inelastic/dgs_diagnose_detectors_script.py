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

class DiagnoseDetectorsScript(BaseScriptElement):

    high_counts = 1.0e+10
    low_counts = 1.0e-10
    median_test_out_high = 100
    median_test_out_low = 0.01
    median_test_high = 3
    median_test_low = 0.1
    errorbar_criterion = 0.0
    det_van2 = ''
    detvan_ratio_var = 1.1
    background_check = False
    sambkg_median_test_high = 5
    sambkg_median_test_low = 0.1
    sambkg_errorbar_criterion = 3.3
    tof_start = ''
    tof_end = ''
    reject_zero_bkg = False
    psd_bleed = False
    max_framerate = ''
    ignored_pixels = ''

    def __init__(self, inst_name):
        super(DiagnoseDetectorsScript, self).__init__()
        self.set_default_pars(inst_name)
        self.reset()

    def set_default_pars(self, inst_name):
        import dgs_utils
        ip = dgs_utils.InstrumentParameters(inst_name)
        DiagnoseDetectorsScript.high_counts = ip.get_parameter("diag_huge")
        DiagnoseDetectorsScript.low_counts = ip.get_parameter("diag_tiny")
        DiagnoseDetectorsScript.median_test_out_high = ip.get_parameter("diag_van_out_hi")
        DiagnoseDetectorsScript.median_test_out_low = ip.get_parameter("diag_van_out_lo")
        DiagnoseDetectorsScript.median_test_high = ip.get_parameter("diag_van_hi")
        DiagnoseDetectorsScript.median_test_low = ip.get_parameter("diag_van_lo")
        DiagnoseDetectorsScript.errorbar_criterion = ip.get_parameter("diag_van_sig")
        DiagnoseDetectorsScript.detvan_ratio_var = ip.get_parameter("diag_variation")
        DiagnoseDetectorsScript.background_check = ip.get_bool_param("check_background")
        DiagnoseDetectorsScript.sambkg_median_test_high = ip.get_parameter("diag_samp_hi")
        DiagnoseDetectorsScript.sambkg_median_test_low = ip.get_parameter("diag_samp_lo")
        DiagnoseDetectorsScript.sambkg_errorbar_criterion = ip.get_parameter("diag_samp_sig")
        DiagnoseDetectorsScript.tof_start = int(ip.get_parameter("bkgd-range-min"))
        DiagnoseDetectorsScript.tof_end = int(ip.get_parameter("bkgd-range-max"))
        DiagnoseDetectorsScript.reject_zero_bkg = ip.get_bool_param("diag_samp_zero")
        DiagnoseDetectorsScript.psd_bleed = ip.get_bool_param("diag_bleed_test")
        DiagnoseDetectorsScript.max_framerate = ip.get_parameter("diag_bleed_maxrate")
        DiagnoseDetectorsScript.ignored_pixels = ip.get_parameter("diag_bleed_pixels")

    def to_script(self):
        script = ""
        if self.high_counts != DiagnoseDetectorsScript.high_counts:
            script += "HighCounts=%s,\n" % str(self.high_counts)
        if self.low_counts != DiagnoseDetectorsScript.low_counts:
            script += "LowCounts=%s,\n" % str(self.low_counts)
        if self.median_test_out_low != DiagnoseDetectorsScript.median_test_out_low:
            script += "LowOutlier=%s,\n" % str(self.median_test_out_low)
        if self.median_test_out_high != DiagnoseDetectorsScript.median_test_out_high:
            script += "HighOutlier=%s,\n" % str(self.median_test_out_high)
        if self.median_test_high != DiagnoseDetectorsScript.median_test_high:
            script += "MedianTestHigh=%s,\n" % str(self.median_test_high)
        if self.median_test_low != DiagnoseDetectorsScript.median_test_low:
            script += "MedianTestLow=%s,\n" % str(self.median_test_low)
        if self.errorbar_criterion != DiagnoseDetectorsScript.errorbar_criterion:
            script += "ErrorBarCriterion=%s,\n" % str(self.errorbar_criterion)
        if self.det_van2 != DiagnoseDetectorsScript.det_van2:
            script += "DetectorVanadium2InputFile=\"%s\",\n" % self.det_van2
            if self.detvan_ratio_var!= DiagnoseDetectorsScript.detvan_ratio_var:
                script += "DetVanRatioVariation=%s,\n" % str(self.detvan_ratio_var)
        if self.background_check:
            script += "BackgroundCheck=%s,\n" % self.background_check
            if self.sambkg_median_test_high != DiagnoseDetectorsScript.sambkg_median_test_high:
                script += "SamBkgMedianTestHigh=%s,\n" % str(self.sambkg_median_test_high)
            if self.sambkg_median_test_low != DiagnoseDetectorsScript.sambkg_median_test_low:
                script += "SamBkgMedianTestLow=%s,\n" % str(self.sambkg_median_test_low)
            if self.sambkg_errorbar_criterion != DiagnoseDetectorsScript.sambkg_errorbar_criterion:
                script += "SamBkgErrorBarCriterion=%s,\n" % str(self.sambkg_errorbar_criterion)
            if self.tof_start != DiagnoseDetectorsScript.tof_start:
                script += "BackgroundTofStart=%s,\n" % self.tof_start
            if self.tof_end != DiagnoseDetectorsScript.tof_end:
                script += "BackgroundTofEnd=%s,\n" % self.tof_end
        if self.reject_zero_bkg:
            script += "RejectZeroBackground=%s,\n" % self.reject_zero_bkg
        if self.psd_bleed:
            script += "PsdBleed=%s,\n" % self.psd_bleed
            if self.max_framerate != DiagnoseDetectorsScript.max_framerate:
                script += "MaxFramerate=%s,\n" % str(self.max_framerate)
            if self.ignored_pixels != DiagnoseDetectorsScript.ignored_pixels:
                script += "IgnoredPixels=%s,\n" % str(self.ignored_pixels)
        return script

    def to_xml(self):
        """
            Create XML from the current data.
        """
        xml =  "<DiagnoseDetectors>\n"
        xml += "  <high_counts>%s</high_counts>\n" % str(self.high_counts)
        xml += "  <low_counts>%s</low_counts>\n" % str(self.low_counts)
        xml += "  <median_test_outlier_low>%s</median_test_outlier_low>\n" % str(self.median_test_out_low)
        xml += "  <median_test_outlier_high>%s</median_test_outlier_high>\n" % str(self.median_test_out_high)
        xml += "  <median_test_low>%s</median_test_low>\n" % str(self.median_test_low)
        xml += "  <median_test_high>%s</median_test_high>\n" % str(self.median_test_high)
        xml += "  <errorbar_criterion>%s</errorbar_criterion>\n" % str(self.errorbar_criterion)
        xml += "  <det_van2>%s</det_van2>\n" % self.det_van2
        xml += "  <detvan_ratio_var>%s</detvan_ratio_var>\n" % str(self.detvan_ratio_var)
        xml += "  <background_check>%s</background_check>\n" % self.background_check
        xml += "  <sambkg_median_test_low>%s</sambkg_median_test_low>\n" % str(self.sambkg_median_test_low)
        xml += "  <sambkg_median_test_high>%s</sambkg_median_test_high>\n" % str(self.sambkg_median_test_high)
        xml += "  <sambkg_errorbar_criterion>%s</sambkg_errorbar_criterion>\n" % str(self.sambkg_errorbar_criterion)
        xml += "  <background_tof_start>%d</background_tof_start>\n" % self.tof_start
        xml += "  <background_tof_end>%d</background_tof_end>\n" % self.tof_end
        xml += "  <reject_zero_bkg>%s</reject_zero_bkg>\n" % self.reject_zero_bkg
        xml += "  <psd_bleed>%s</psd_bleed>\n" % self.psd_bleed
        xml += "  <max_framerate>%s</max_framerate>\n" % self.max_framerate
        xml += "  <ignored_pixels>%s</ignored_pixels>\n" % self.ignored_pixels
        xml += "</DiagnoseDetectors>\n"
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
            self.high_counts = BaseScriptElement.getFloatElement(instrument_dom,
                                                                 "high_counts",
                                                                 default=DiagnoseDetectorsScript.high_counts)
            self.low_counts = BaseScriptElement.getFloatElement(instrument_dom,
                                                                "low_counts",
                                                                default=DiagnoseDetectorsScript.low_counts)
            self.median_test_out_low = BaseScriptElement.getFloatElement(instrument_dom,
                                                                         "median_test_out_low",
                                                                         default=DiagnoseDetectorsScript.median_test_out_low)
            self.median_test_out_high = BaseScriptElement.getFloatElement(instrument_dom,
                                                                          "median_test_out_high",
                                                                          default=DiagnoseDetectorsScript.median_test_out_high)
            self.median_test_low = BaseScriptElement.getFloatElement(instrument_dom,
                                                                     "median_test_low",
                                                                     default=DiagnoseDetectorsScript.median_test_low)
            self.median_test_high = BaseScriptElement.getFloatElement(instrument_dom,
                                                                      "median_test_high",
                                                                      default=DiagnoseDetectorsScript.median_test_high)
            self.errorbar_criterion = BaseScriptElement.getFloatElement(instrument_dom,
                                                                        "errorbar_criterion",
                                                                        default=DiagnoseDetectorsScript.errorbar_criterion)
            self.det_van2 = BaseScriptElement.getStringElement(instrument_dom,
                                                               "det_van2",
                                                               default=DiagnoseDetectorsScript.det_van2)
            self.detvan_ratio_var = BaseScriptElement.getFloatElement(instrument_dom,
                                                                      "detvan_ratio_var",
                                                                      default=DiagnoseDetectorsScript.detvan_ratio_var)
            self.background_check = BaseScriptElement.getBoolElement(instrument_dom,
                                                                     "background_check",
                                                                     default=DiagnoseDetectorsScript.background_check)
            self.sambkg_median_test_low = BaseScriptElement.getFloatElement(instrument_dom,
                                                                            "sambkg_median_test_low",
                                                                            default=DiagnoseDetectorsScript.sambkg_median_test_low)
            self.sambkg_median_test_high = BaseScriptElement.getFloatElement(instrument_dom,
                                                                             "sambkg_median_test_high",
                                                                             default=DiagnoseDetectorsScript.sambkg_median_test_high)
            self.sambkg_errorbar_criterion = BaseScriptElement.getFloatElement(instrument_dom,
                                                                               "sambkg_errorbar_criterion",
                                                                               default=DiagnoseDetectorsScript.sambkg_errorbar_criterion)
            self.tof_start = BaseScriptElement.getIntElement(instrument_dom,\
                                                              "background_tof_start",\
                                                              default=DiagnoseDetectorsScript.tof_start)
            self.tof_end = BaseScriptElement.getIntElement(instrument_dom,
                                                           "background_tof_end",
                                                           default=DiagnoseDetectorsScript.tof_end)
            self.reject_zero_bkg = BaseScriptElement.getBoolElement(instrument_dom,
                                                                    "reject_zero_bkg",
                                                                    default=DiagnoseDetectorsScript.reject_zero_bkg)
            self.psd_bleed = BaseScriptElement.getBoolElement(instrument_dom,
                                                              "psd_bleed",
                                                              default=DiagnoseDetectorsScript.psd_bleed)
            self.max_framerate = BaseScriptElement.getStringElement(instrument_dom,
                                                                    "max_framerate",
                                                                    default=DiagnoseDetectorsScript.max_framerate)
            self.ignored_pixels = BaseScriptElement.getStringElement(instrument_dom,
                                                                     "ignored_pixels",
                                                                     default=DiagnoseDetectorsScript.ignored_pixels)

    def reset(self):
        """
            Reset state
        """
        self.high_counts = DiagnoseDetectorsScript.high_counts
        self.low_counts = DiagnoseDetectorsScript.low_counts
        self.median_test_out_low = DiagnoseDetectorsScript.median_test_out_low
        self.median_test_out_high = DiagnoseDetectorsScript.median_test_out_high
        self.median_test_low = DiagnoseDetectorsScript.median_test_low
        self.median_test_high = DiagnoseDetectorsScript.median_test_high
        self.errorbar_criterion = DiagnoseDetectorsScript.errorbar_criterion
        self.det_van2 = DiagnoseDetectorsScript.det_van2
        self.detvan_ratio_var = DiagnoseDetectorsScript.detvan_ratio_var
        self.background_check = DiagnoseDetectorsScript.background_check
        self.sambkg_median_test_low = DiagnoseDetectorsScript.sambkg_median_test_low
        self.sambkg_median_test_high = DiagnoseDetectorsScript.sambkg_median_test_high
        self.sambkg_errorbar_criterion = DiagnoseDetectorsScript.sambkg_errorbar_criterion
        self.tof_start = DiagnoseDetectorsScript.tof_start
        self.tof_end = DiagnoseDetectorsScript.tof_end
        self.reject_zero_bkg = DiagnoseDetectorsScript.reject_zero_bkg
        self.psd_bleed = DiagnoseDetectorsScript.psd_bleed
        self.max_framerate = DiagnoseDetectorsScript.max_framerate
        self.ignored_pixels = DiagnoseDetectorsScript.ignored_pixels
