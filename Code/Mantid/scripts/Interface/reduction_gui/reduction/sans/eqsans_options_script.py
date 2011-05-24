"""
    General options for EQSANS reduction
"""
import xml.dom.minidom
from reduction_gui.reduction.scripter import BaseScriptElement
from hfir_options_script import ReductionOptions as BaseOptions

class ReductionOptions(BaseOptions):
    instrument_name = "EQSANS"
    nx_pixels = None
    ny_pixels = None
    pixel_size = None
    detector_offset = 0.0
    
    # Use TOF cuts from configuration file
    use_config_cutoff = True
    low_TOF_cut = 0.0
    high_TOF_cut = 0.0
    # Correct for flight path at larger angle
    correct_for_flight_path = True

    # Use the mask defined in the configuration file
    use_config_mask = True


    def __init__(self):
        super(ReductionOptions, self).__init__()
        self.reset()
        
    def reset(self):
        super(ReductionOptions, self).reset()
        self.nx_pixels = ReductionOptions.nx_pixels
        self.ny_pixels = ReductionOptions.ny_pixels
        self.pixel_size = ReductionOptions.pixel_size
        self.detector_offset = ReductionOptions.detector_offset

        self.use_config_cutoff = ReductionOptions.use_config_cutoff
        self.low_TOF_cut = ReductionOptions.low_TOF_cut
        self.high_TOF_cut = ReductionOptions.high_TOF_cut
        self.correct_for_flight_path = ReductionOptions.correct_for_flight_path
        
        self.use_config_mask = ReductionOptions.use_config_mask

    def to_script(self):
        """
            Generate reduction script
            @param execute: if true, the script will be executed
        """
        script = super(ReductionOptions, self).to_script()
        
        # TOF cutoff
        if self.use_config_cutoff:
            script += "UseConfigTOFTailsCutoff(True)\n"
        else:
            script += "UseConfigTOFTailsCutoff(False)\n"
            if self.low_TOF_cut>0 or self.high_TOF_cut>0:
                script += "SetTOFTailsCutoff(%g, %g)\n" % (self.low_TOF_cut, self.high_TOF_cut)
                
        # Flight path correction
        script += "PerformFlightPathCorrection(%s)\n" % self.correct_for_flight_path
        
        return script
            
    def to_xml(self):
        """
            Create XML from the current data.
        """
        xml = super(ReductionOptions, self).to_xml()
        
        # TOF cutoff and correction
        xml += "<TOFcorr>\n"
        xml += "  <low_tof_cut>%g</low_tof_cut>\n" % self.low_TOF_cut
        xml += "  <high_tof_cut>%g</high_tof_cut>\n" % self.high_TOF_cut
        xml += "  <use_config_cutoff>%s</use_config_cutoff>\n" % str(self.use_config_cutoff)
        xml += "  <perform_flight_path_corr>%s</perform_flight_path_corr>\n" % str(self.correct_for_flight_path)
        xml += "</TOFcorr>\n"
        
        # Mask
        xml += "<UseConfigMask>%s</UseConfigMask>\n" % self.use_config_mask
        
        return xml
    
    def from_xml(self, xml_str):
        """
            Read in data from XML
            @param xml_str: text to read the data from
        """    
        self.reset()   
        super(ReductionOptions, self).from_xml(xml_str)
        
        dom = xml.dom.minidom.parseString(xml_str)
        
        # TOF cutoff and correction
        element_list = dom.getElementsByTagName("TOFcorr")
        if len(element_list)>0: 
            tof_dom = element_list[0]

            self.use_config_cutoff = BaseScriptElement.getBoolElement(tof_dom, "use_config_cutoff",
                                                                      default = ReductionOptions.use_config_cutoff)
            self.correct_for_flight_path = BaseScriptElement.getBoolElement(tof_dom, "perform_flight_path_corr",
                                                                            default = ReductionOptions.correct_for_flight_path)
            self.low_TOF_cut = BaseScriptElement.getFloatElement(tof_dom, "low_tof_cut", 
                                                                 default=ReductionOptions.low_TOF_cut)
            self.high_TOF_cut = BaseScriptElement.getFloatElement(tof_dom, "high_tof_cut", 
                                                                  default=ReductionOptions.high_TOF_cut)
        
        # Mask
        self.use_config_mask = BaseScriptElement.getBoolElement(dom, "UseConfigMask",
                                                                default = ReductionOptions.use_config_mask)
        
    
