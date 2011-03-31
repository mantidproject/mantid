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
        xml += "</Sensitivity>\n"

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
        instrument_dom = dom.getElementsByTagName("Sensitivity")[0]
        self.sensitivity_corr = BaseScriptElement.getBoolElement(instrument_dom, "sensitivity_corr",
                                                                 default = InstrumentDescription.sensitivity_corr)
        self.sensitivity_data = BaseScriptElement.getStringElement(instrument_dom, "sensitivity_data")
        self.sensitivity_dark = BaseScriptElement.getStringElement(instrument_dom, "sensitivity_dark")
        self.min_sensitivity = BaseScriptElement.getFloatElement(instrument_dom, "sensitivity_min",
                                                            default=InstrumentDescription.min_sensitivity)
        self.max_sensitivity = BaseScriptElement.getFloatElement(instrument_dom, "sensitivity_max",
                                                            default=InstrumentDescription.max_sensitivity)
        
        element_list = dom.getElementsByTagName("BeamFinder")
        if len(element_list)>0:
            beam_finder_dom = element_list[0]
            self.x_position = BaseScriptElement.getFloatElement(beam_finder_dom, "x",
                                                                default=BeamFinder.x_position) 
            self.y_position = BaseScriptElement.getFloatElement(beam_finder_dom, "y",
                                                                default=BeamFinder.y_position) 
            self.use_finder = BaseScriptElement.getBoolElement(beam_finder_dom, "use_finder",
                                                               default = BeamFinder.use_finder) 
            self.beam_file = BaseScriptElement.getStringElement(beam_finder_dom, "beam_file")
            self.use_direct_beam = BaseScriptElement.getBoolElement(beam_finder_dom, "use_direct_beam",
                                                               default = BeamFinder.use_direct_beam) 

    def reset(self):
        """
            Reset state
        """
        self.sensitivity_corr = InstrumentDescription.sensitivity_corr
        self.sensitivity_data = ''
        self.sensitivity_dark = ''
        self.min_sensitivity = InstrumentDescription.min_sensitivity
        self.max_sensitivity = InstrumentDescription.max_sensitivity
        
        self.x_position = BeamFinder.x_position
        self.y_position = BeamFinder.y_position
        self.use_finder = BeamFinder.use_finder
        self.beam_file = ''
        self.use_direct_beam = BeamFinder.use_direct_beam
        
