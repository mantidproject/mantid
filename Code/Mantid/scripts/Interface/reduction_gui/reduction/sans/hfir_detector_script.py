"""
    
"""
import xml.dom.minidom
import copy
import os
from reduction_gui.reduction.scripter import BaseScriptElement

# Check whether Mantid is available
try:
    from MantidFramework import *
    mtd.initialise(False)
    from reduction.instruments.sans.hfir_command_interface import *
    HAS_MANTID = True
except:
    HAS_MANTID = False  

# Check whether we are running in MantidPlot
IS_IN_MANTIDPLOT = False
try:
    import mantidplot
    IS_IN_MANTIDPLOT = True
except:
    pass

    
class Detector(BaseScriptElement):

    # Flag to perform sensitivity correction
    sensitivity_corr = False
    # Data file to be used to calculate sensitivity
    sensitivity_data = ''
    sensitivity_dark = ''
    # Minimum allowed relative sensitivity
    min_sensitivity = 0.5
    max_sensitivity = 1.5
    # Beam finder for flood data
    use_sample_beam_center = True
    flood_x_position = 0
    flood_y_position = 0
    flood_use_finder = False
    flood_beam_file = ''
    flood_beam_radius = 3.0
    flood_use_direct_beam = True
    
    # Beam finder
    x_position = 0
    y_position = 0
    use_finder = False
    beam_file = ''
    beam_radius = 3.0
    use_direct_beam = True
        
    def to_script(self):
        """
            Generate reduction script
            @param execute: if true, the script will be executed
        """
        script  = ""

        if not self.use_finder:
            script += "SetBeamCenter(%g, %g)\n" % (self.x_position, self.y_position) 
        else:
            if len(str(self.beam_file).strip())==0:
                raise RuntimeError, "Beam finder was selected but no data file was entered."

            if self.use_direct_beam:
                script += "DirectBeamCenter(\"%s\")\n" % self.beam_file
            else:
                script += "ScatteringBeamCenter(\"%s\", %g)\n" % (self.beam_file, self.beam_radius)
                
        if self.sensitivity_corr:
            if len(str(self.sensitivity_data).strip())==0:
                raise RuntimeError, "Sensitivity correction was selected but no sensitivity data file was entered."
            
            # Beam center
            if not self.use_sample_beam_center:
                if not self.flood_use_finder:
                    script += "SetSensitivityBeamCenter(%g, %g)\n" % (self.flood_x_position, self.flood_y_position) 
                else:
                    if len(str(self.flood_beam_file).strip())==0:
                        raise RuntimeError, "Sensitivity beam finder was selected but no data file was entered."
        
                    if self.flood_use_direct_beam:
                        script += "SensitivityDirectBeamCenter(\"%s\")\n" % self.flood_beam_file
                    else:
                        script += "SensitivityScatteringBeamCenter(\"%s\", %g)\n" % (self.flood_beam_file, self.flood_beam_radius)
                    
            if len(str(self.sensitivity_dark).strip())>0:
                script += "SensitivityCorrection('%s', min_sensitivity=%g, max_sensitivity=%g, dark_current='%s')\n" % \
                    (self.sensitivity_data, self.min_sensitivity, self.max_sensitivity, self.sensitivity_dark)
            else:
                script += "SensitivityCorrection('%s', min_sensitivity=%g, max_sensitivity=%g)\n" % \
                    (self.sensitivity_data, self.min_sensitivity, self.max_sensitivity)
        else:
            script += "NoSensitivityCorrection()\n"
                    
        return script           
    
    def to_xml(self):
        """
            Create XML from the current data.
        """
        xml  = "<Sensitivity>\n"
        xml += "  <sensitivity_corr>%s</sensitivity_corr>\n" % str(self.sensitivity_corr)
        xml += "  <sensitivity_data>%s</sensitivity_data>\n" % self.sensitivity_data
        xml += "  <sensitivity_dark>%s</sensitivity_dark>\n" % self.sensitivity_dark
        xml += "  <sensitivity_min>%s</sensitivity_min>\n" % self.min_sensitivity
        xml += "  <sensitivity_max>%s</sensitivity_max>\n" % self.max_sensitivity
        xml += "  <use_sample_beam_center>%s</use_sample_beam_center>\n" % str(self.use_sample_beam_center)
        xml += "  <FloodBeamFinder>\n"
        xml += "    <position>\n"
        xml += "      <x>%g</x>\n" % self.flood_x_position
        xml += "      <y>%g</y>\n" % self.flood_y_position
        xml += "    </position>\n"
        xml += "    <use_finder>%s</use_finder>\n" % str(self.flood_use_finder)
        xml += "    <beam_file>%s</beam_file>\n" % self.flood_beam_file
        xml += "    <use_direct_beam>%s</use_direct_beam>\n" % str(self.flood_use_direct_beam)
        xml += "    <beam_radius>%g</beam_radius>\n" % self.flood_beam_radius
        xml += "  </FloodBeamFinder>\n"
        xml += "</Sensitivity>\n"

        xml += "<BeamFinder>\n"
        xml += "  <position>\n"
        xml += "    <x>%g</x>\n" % self.x_position
        xml += "    <y>%g</y>\n" % self.y_position
        xml += "  </position>\n"
        xml += "  <use_finder>%s</use_finder>\n" % str(self.use_finder)
        xml += "  <beam_file>%s</beam_file>\n" % self.beam_file
        xml += "  <use_direct_beam>%s</use_direct_beam>\n" % str(self.use_direct_beam)
        xml += "  <beam_radius>%g</beam_radius>\n" % self.beam_radius
        xml += "</BeamFinder>\n"


        return xml
        
    def from_xml(self, xml_str):
        """
            Read in data from XML
            @param xml_str: text to read the data from
        """       
        dom = xml.dom.minidom.parseString(xml_str)
        element_list = dom.getElementsByTagName("Sensitivity")
        if len(element_list)>0:
            instrument_dom = element_list[0]
            self.sensitivity_corr = BaseScriptElement.getBoolElement(instrument_dom, "sensitivity_corr",
                                                                     default = Detector.sensitivity_corr)
            self.sensitivity_data = BaseScriptElement.getStringElement(instrument_dom, "sensitivity_data")
            self.sensitivity_dark = BaseScriptElement.getStringElement(instrument_dom, "sensitivity_dark")
            self.min_sensitivity = BaseScriptElement.getFloatElement(instrument_dom, "sensitivity_min",
                                                                default=Detector.min_sensitivity)
            self.max_sensitivity = BaseScriptElement.getFloatElement(instrument_dom, "sensitivity_max",
                                                                default=Detector.max_sensitivity)
            self.use_sample_beam_center = BaseScriptElement.getBoolElement(instrument_dom, "use_sample_beam_center",
                                                                     default = Detector.use_sample_beam_center)
            
            beam_center_list = instrument_dom.getElementsByTagName("FloodBeamFinder")
            if len(beam_center_list)>0:
                beam_finder_dom = beam_center_list[0]
                self.flood_x_position = BaseScriptElement.getFloatElement(beam_finder_dom, "x",
                                                                    default=Detector.flood_x_position) 
                self.flood_y_position = BaseScriptElement.getFloatElement(beam_finder_dom, "y",
                                                                    default=Detector.flood_y_position) 
                self.flood_use_finder = BaseScriptElement.getBoolElement(beam_finder_dom, "use_finder",
                                                                   default = Detector.flood_use_finder) 
                self.flood_beam_file = BaseScriptElement.getStringElement(beam_finder_dom, "beam_file")
                self.flood_beam_radius = BaseScriptElement.getFloatElement(beam_finder_dom, "beam_radius",
                                                                    default=Detector.flood_beam_radius) 
                self.flood_use_direct_beam = BaseScriptElement.getBoolElement(beam_finder_dom, "use_direct_beam",
                                                                   default = Detector.flood_use_direct_beam) 
            
        element_list = dom.getElementsByTagName("BeamFinder")
        if len(element_list)>0:
            beam_finder_dom = element_list[0]
            self.x_position = BaseScriptElement.getFloatElement(beam_finder_dom, "x",
                                                                default=Detector.x_position) 
            self.y_position = BaseScriptElement.getFloatElement(beam_finder_dom, "y",
                                                                default=Detector.y_position) 
            self.use_finder = BaseScriptElement.getBoolElement(beam_finder_dom, "use_finder",
                                                               default = Detector.use_finder) 
            self.beam_file = BaseScriptElement.getStringElement(beam_finder_dom, "beam_file")
            self.beam_radius = BaseScriptElement.getFloatElement(beam_finder_dom, "beam_radius",
                                                                default=Detector.beam_radius) 
            self.use_direct_beam = BaseScriptElement.getBoolElement(beam_finder_dom, "use_direct_beam",
                                                               default = Detector.use_direct_beam) 

    def reset(self):
        """
            Reset state
        """
        # Sensitivity
        self.sensitivity_corr = Detector.sensitivity_corr
        self.sensitivity_data = ''
        self.sensitivity_dark = ''
        self.min_sensitivity = Detector.min_sensitivity
        self.max_sensitivity = Detector.max_sensitivity
        self.flood_x_position = Detector.flood_x_position
        self.flood_y_position = Detector.flood_y_position
        self.flood_use_finder = Detector.flood_use_finder
        self.flood_beam_file = Detector.flood_beam_file
        self.flood_beam_radius = Detector.flood_beam_radius
        self.flood_use_direct_beam = Detector.flood_use_direct_beam
        self.use_sample_beam_center = Detector.use_sample_beam_center
        
        # Beam finder
        self.x_position = Detector.x_position
        self.y_position = Detector.y_position
        self.use_finder = Detector.use_finder
        self.beam_file = ''
        self.beam_radius = Detector.beam_radius
        self.use_direct_beam = Detector.use_direct_beam
        
