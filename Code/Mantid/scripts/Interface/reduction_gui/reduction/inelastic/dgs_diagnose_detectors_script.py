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
    output_mask_file = ''
    errorbar_criterion = 3.3
    det_van1 = ''
    high_counts = 1.0e+10
    low_counts = 1.0e-10
    median_test_high = 3
    median_test_low = 0.1
    det_van2 = ''
    prop_change_criterion = 1.1
    background_check = True
    acceptance_factor = 5
    tof_start = 18000
    tof_end = 19500
    reject_zero_bkg = True
    psd_bleed = False
    max_framerate = ''
    ignored_pixels = ''

    def __init__(self):
        super(DiagnoseDetectorsScript, self).__init__()
        self.reset()
        
    def to_script(self):
        script =  "FindBadDetectors=%s,\n" % self.find_bad_detectors
        if self.find_bad_detectors:
            script += "OutputMaskFile=\"%s\",\n" % self.output_mask_file
            script += "ErrorBarCriterion=%s,\n" % str(self.errorbar_criterion)
            script += "DetectorVanadium1=\"%s\",\n" % self.det_van1
            script += "HighCounts=%s,\n" % str(self.high_counts)
            script += "LowCounts=%s,\n" % str(self.low_counts)
            script += "MedianTestHigh=%s,\n" % str(self.median_test_high)
            script += "MedianTestLow=%s,\n" % str(self.median_test_low)
            script += "DetectorVanadium2=\"%s\",\n" % self.det_van2
            script += "ProptionalChangeCriterion=%s,\n" % str(self.prop_change_criterion)
            script += "BackgroundCheck=%s,\n" % self.background_check
            if self.background_check:
                script += "AcceptanceFactor=%s,\n" % str(self.acceptance_factor)
                script += "BackgroundTofStart=%s,\n" % str(self.tof_start)
                script += "BackgroundTofEnd=%s,\n" % str(self.tof_end)
                script += "RejectZeroBackground=%s,\n" % self.reject_zero_bkg
            script += "PsdBleed=%s,\n" % self.psd_bleed
            if self.psd_bleed:
                script += "MaxFramerate=\"%s\",\n" % self.max_framerate
                script += "IgnoredPixels=\"%s\",\n" % self.ignored_pixels 
        return script
    
    def to_xml(self):
        """
            Create XML from the current data.
        """
        xml =  "<DiagnoseDetectors>\n"
        xml += "  <find_bad_detectors>%s</find_bad_detectors>\n" % self.find_bad_detectors
        xml += "  <output_mask_file>%s</output_mask_file>\n" % self.output_mask_file
        xml += "  <errorbar_criterion>%s</errorbar_criterion>\n" % str(self.errorbar_criterion)
        xml += "  <det_van1>%s</det_van1>\n" % self.det_van1
        xml += "  <high_counts>%s</high_counts>\n" % str(self.high_counts)
        xml += "  <low_counts>%s</low_counts>\n" % str(self.low_counts)
        xml += "  <median_test_low>%s</median_test_low>\n" % str(self.median_test_low)
        xml += "  <median_test_high>%s</median_test_high>\n" % str(self.median_test_high)
        xml += "  <det_van2>%s</det_van2>\n" % self.det_van2
        xml += "  <prop_change_criterion>%s</prop_change_criterion>\n" % str(self.prop_change_criterion)
        xml += "  <background_check>%s</backgound_check>\n" % self.background_check
        xml += "  <acceptance_factor>%s</acceptance_factor>\n" % str(self.acceptance_factor)
        xml += "  <background_tof_start>%s</background_tof_start>\n" % str(self.tof_start)
        xml += "  <background_tof_end>%s</background_tof_end>\n" % str(self.tof_end)
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
            self.find_bad_detectors = BaseScriptElement.getBoolElement(instrument_dom,
                                                                       "find_bad_detectors",
                                                                       default=DiagnoseDetectorsScript.find_bad_detectors)
            self.output_mask_file = BaseScriptElement.getStringElement(instrument_dom,
                                                                       "output_mask_file",
                                                                       default=DiagnoseDetectorsScript.output_mask_file)
            self.errorbar_criterion = BaseScriptElement.getFloatElement(instrument_dom,
                                                                        "errorbar_criterion",
                                                                        default=DiagnoseDetectorsScript.errorbar_criterion)
            self.det_van1 = BaseScriptElement.getStringElement(instrument_dom,
                                                               "det_van1",
                                                               default=DiagnoseDetectorsScript.det_van1)
            self.high_counts = BaseScriptElement.getFloatElement(instrument_dom,
                                                                 "high_counts",
                                                                 default=DiagnoseDetectorsScript.high_counts)
            self.low_counts = BaseScriptElement.getFloatElement(instrument_dom,
                                                                "low_counts",
                                                                default=DiagnoseDetectorsScript.low_counts)
            self.median_test_low = BaseScriptElement.getFloatElement(instrument_dom,
                                                                     "median_test_low",
                                                                     default=DiagnoseDetectorsScript.median_test_low)
            self.median_test_high = BaseScriptElement.getFloatElement(instrument_dom,
                                                                      "median_test_high",
                                                                      default=DiagnoseDetectorsScript.median_test_high)
            self.det_van2 = BaseScriptElement.getStringElement(instrument_dom,
                                                               "det_van2",
                                                               default=DiagnoseDetectorsScript.det_van2)
            self.prop_change_criterion = BaseScriptElement.getFloatElement(instrument_dom,
                                                                           "prop_change_criterion",
                                                                           default=DiagnoseDetectorsScript.prop_change_criterion)
            self.background_check = BaseScriptElement.getBoolElement(instrument_dom,
                                                                     "background_check",
                                                                     default=DiagnoseDetectorsScript.background_check)
            self.acceptance_factor = BaseScriptElement.getFloatElement(instrument_dom,
                                                                       "accpetance_factor",
                                                                       default=DiagnoseDetectorsScript.acceptance_factor)
            self.tof_start = BaseScriptElement.getIntElement(instrument_dom,
                                                             "background_tof_start",
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
        self.find_bad_detectors = DiagnoseDetectorsScript.find_bad_detectors
        self.output_mask_file = DiagnoseDetectorsScript.output_mask_file
        self.errorbar_criterion = DiagnoseDetectorsScript.errorbar_criterion
        self.det_van1 = DiagnoseDetectorsScript.det_van1
        self.high_counts = DiagnoseDetectorsScript.high_counts
        self.low_counts = DiagnoseDetectorsScript.low_counts
        self.median_test_low = DiagnoseDetectorsScript.median_test_low
        self.median_test_high = DiagnoseDetectorsScript.median_test_high
        self.det_van2 = DiagnoseDetectorsScript.det_van2
        self.prop_change_criterion = DiagnoseDetectorsScript.prop_change_criterion
        self.background_check = DiagnoseDetectorsScript.background_check
        self.acceptance_factor = DiagnoseDetectorsScript.acceptance_factor
        self.tof_start = DiagnoseDetectorsScript.tof_start
        self.tof_end = DiagnoseDetectorsScript.tof_end
        self.reject_zero_bkg = DiagnoseDetectorsScript.reject_zero_bkg
        self.psd_bleed = DiagnoseDetectorsScript.psd_bleed
        self.max_framerate = DiagnoseDetectorsScript.max_framerate
        self.ignored_pixels = DiagnoseDetectorsScript.ignored_pixels
        