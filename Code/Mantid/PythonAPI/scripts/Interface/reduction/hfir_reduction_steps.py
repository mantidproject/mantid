"""
    Classes for each reduction step. Those are kept separately 
    from the the interface class so that the HFIRReduction class could 
    be used independently of the interface implementation
"""
import xml.dom.minidom
import copy
from hfir_reduction import BaseScriptElement

class Transmission(BaseScriptElement):
    def __str__(self):
        return ""
    
class InstrumentDescription(BaseScriptElement):
    instrument_name = "BIOSANS"
    nx_pixels = 192
    ny_pixels = 192
    pixel_size = 5.1
    
    # Sample-detector distance to force on the data set [mm]
    sample_detector_distance = 0
    # Detector distance offset [mm]
    detector_offset = 0
    # Wavelength value to force on the data set [Angstrom]
    wavelength = 0
    
    # Flag to perform the solid angle correction
    solid_angle_corr = True
    # Flag to perform sensitivity correction
    sensitivity_corr = True
    # Data file to be used to calculate sensitivity
    sensitivity_data = ''
    # Minimum allowed relative sensitivity
    min_sensitivity = 0.5
    max_sensitivity = 1.5
    
    NORMALIZATION_NONE = 0
    NORMALIZATION_TIME = 1
    NORMALIZATION_MONITOR = 2
    normalization = NORMALIZATION_MONITOR
        
    def __str__(self):
        """
            Representation as a Mantid script
        """
        script  = "HFIRSANS()\n"
        
        if self.sample_detector_distance != 0:
            script += "SetSampleDetectorDistance(%g)\n" % self.sample_detector_distance 
        if self.detector_offset != 0:
            script += "SetSampleDetectorOffset(%g)\n" % self.detector_offset 
        if self.wavelength != 0:
            script += "SetWavelength(%g)\n" % self.wavelength 
        
        if self.solid_angle_corr:
            script += "SolidAngle()\n"
        else:
            script += "NoSolidAngle()\n"
        
        if self.sensitivity_corr:
            script += "SensitivityCorrection('%s', min_sensitivity=%g, max_sensitivity=%g)\n" % \
                (self.sensitivity_data, self.min_sensitivity, self.max_sensitivity)
        else:
            script += "NoSensitivityCorrection()\n"
            
        if self.normalization==InstrumentDescription.NORMALIZATION_NONE:
            script += "NoNormalization()\n"
        elif self.normalization==InstrumentDescription.NORMALIZATION_TIME:
            script += "TimeNormalization()\n"
        elif self.normalization==InstrumentDescription.NORMALIZATION_MONITOR:
            script += "MonitorNormalization()\n"
        
        return script

    def apply(self, reducer):
        """
            The equivalent of the command line implementation, directly
            applied to a SANSReducer object
            @param reducer: SANSReducer object
        """
        return NotImplemeted    
    
    def to_xml(self):
        """
            Create XML from the current data.
        """
        xml  = "<Instrument>\n"
        xml += "  <name>%s</name>\n" % self.instrument_name
        xml += "  <nx_pixels>%g</nx_pixels>\n" % self.nx_pixels
        xml += "  <ny_pixels>%g</ny_pixels>\n" % self.ny_pixels
        xml += "  <pixel_size>%g</pixel_size>\n" % self.pixel_size

        xml += "  <sample_det_dist>%g</sample_det_dist>\n" % self.sample_detector_distance
        xml += "  <detector_offset>%g</detector_offset>\n" % self.detector_offset
        xml += "  <wavelength>%g</wavelength>\n" % self.wavelength
        
        xml += "  <solid_angle_corr>%s</solid_angle_corr>\n" % str(self.solid_angle_corr)
        xml += "  <sensitivity_corr>%s</sensitivity_corr>\n" % str(self.sensitivity_corr)
        xml += "  <sensitivity_data>%s</sensitivity_data>\n" % self.sensitivity_data
        xml += "  <sensitivity_min>%s</sensitivity_min>\n" % self.min_sensitivity
        xml += "  <sensitivity_max>%s</sensitivity_max>\n" % self.max_sensitivity

        xml += "  <normalization>%d</normalization>\n" % self.normalization
        
        xml += "</Instrument>\n"

        return xml
        
    def from_xml(self, xml_str):
        """
            Read in data from XML
            @param xml_str: text to read the data from
            
            TODO: improve the reader so that all the elements don't absolutely have to be present in the XML file.
        """       
        dom = xml.dom.minidom.parseString(xml_str)
        instrument_dom = dom.getElementsByTagName("Instrument")[0]
        self.nx_pixels = int(BaseScriptElement.getText(instrument_dom.getElementsByTagName("nx_pixels")[0].childNodes))
        self.ny_pixels = int(BaseScriptElement.getText(instrument_dom.getElementsByTagName("ny_pixels")[0].childNodes))
        self.name = BaseScriptElement.getText(instrument_dom.getElementsByTagName("name")[0].childNodes)
        self.pixel_size = float(BaseScriptElement.getText(instrument_dom.getElementsByTagName("pixel_size")[0].childNodes))
        
        self.sample_detector_distance = BaseScriptElement.getFloatElement(instrument_dom, "sample_det_dist", 
                                                                          default=InstrumentDescription.sample_detector_distance)
        self.detector_offset = BaseScriptElement.getFloatElement(instrument_dom, "detector_offset",
                                                                 default=InstrumentDescription.detector_offset)
        self.wavelength = BaseScriptElement.getFloatElement(instrument_dom, "wavelength",
                                                            default=InstrumentDescription.wavelength)
        
        self.solid_angle_corr = BaseScriptElement.getBoolElement(instrument_dom, "solid_angle_corr",
                                                                 default = InstrumentDescription.solid_angle_corr)
        self.sensitivity_corr = BaseScriptElement.getBoolElement(instrument_dom, "sensitivity_corr",
                                                                 default = InstrumentDescription.sensitivity_corr)
        self.sensitivity_data = BaseScriptElement.getStringElement(instrument_dom, "sensitivity_data")
        self.min_sensitivity = BaseScriptElement.getFloatElement(instrument_dom, "sensitivity_min",
                                                            default=InstrumentDescription.min_sensitivity)
        self.max_sensitivity = BaseScriptElement.getFloatElement(instrument_dom, "sensitivity_max",
                                                            default=InstrumentDescription.max_sensitivity)
        
        self.normalization = BaseScriptElement.getIntElement(instrument_dom, "normalization",
                                                             default=InstrumentDescription.normalization)

class BeamFinder(BaseScriptElement):
    """
        Small class to hold the state of the interface
    """
    # Beam finder
    x_position = 0
    y_position = 0
    use_finder = False
    beam_file = ''
    beam_radius = 3.0
    use_direct_beam = True
    
    def __str__(self):
        """
            Representation as a Mantid script
        """
        script = ""
        if not self.use_finder:
            script += "SetBeamCenter(%g, %g)\n" % (self.x_position, self.y_position) 
        else:
            if self.use_direct_beam:
                script += "DirectBeamCenter(\"%s\")\n" % self.beam_file
            else:
                script += "ScatteringBeamCenter(\"%s\", %g)\n" % (self.beam_file, self.beam_radius)
        return script

    def apply(self, reducer):
        """
            The equivalent of the command line implementation, directly
            applied to a SANSReducer object
            @param reducer: SANSReducer object
        """
        return NotImplemeted
    
    def to_xml(self):
        """
            Create XML from the current data.
        """
        xml  = "<BeamFinder>\n"
        xml += "  <position>\n"
        xml += "    <x>%g</x>\n" % self.x_position
        xml += "    <y>%g</y>\n" % self.y_position
        xml += "  </position>\n"
        xml += "  <use_finder>%s</use_finder>\n" % str(self.use_finder)
        xml += "  <beam_file>%s</beam_file>\n" % self.beam_file
        xml += "  <use_direct_beam>%s</use_direct_beam>\n" % str(self.use_direct_beam)
        xml += "</BeamFinder>\n"

        return xml
        
    def from_xml(self, xml_str):
        """
            Read in data from XML
            @param xml_str: text to read the data from
        """
        dom = xml.dom.minidom.parseString(xml_str)
        beam_finder_dom = dom.getElementsByTagName("BeamFinder")[0]
        self.x_position = BaseScriptElement.getFloatElement(beam_finder_dom, "x",
                                                            default=BeamFinder.x_position) 
        self.y_position = BaseScriptElement.getFloatElement(beam_finder_dom, "y",
                                                            default=BeamFinder.y_position) 
        self.use_finder = BaseScriptElement.getBoolElement(beam_finder_dom, "use_finder",
                                                           default = BeamFinder.use_finder) 
        self.beam_file = BaseScriptElement.getStringElement(beam_finder_dom, "beam_file")
        self.use_direct_beam = BaseScriptElement.getBoolElement(beam_finder_dom, "use_direct_beam",
                                                           default = BeamFinder.use_direct_beam) 
        
        
        