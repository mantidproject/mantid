"""
    Classes for each reduction step. Those are kept separately 
    from the the interface class so that the HFIRReduction class could 
    be used independently of the interface implementation
"""
import xml.dom.minidom
import copy
import os
from scripter import BaseScriptElement

# Check whether Mantid is available
try:
    from MantidFramework import *
    mtd.initialise(False)
    from reduction.instruments.sans.hfir_command_interface import *
    HAS_MANTID = True
except:
    HAS_MANTID = False  

class Transmission(BaseScriptElement):

    class DirectBeam(BaseScriptElement):
        sample_file = ''
        direct_beam = ''
        # Beam radius in pixels
        beam_radius = 3.0
        
        def to_script(self):
            """
                Generate reduction script
                @param execute: if true, the script will be executed
            """
            if len(str(self.sample_file).strip())==0 \
                or len(str(self.direct_beam).strip())==0:
                raise RuntimeError, "Transmission with direct beam was selected but data files were not entered."

            return "DirectBeamTransmission(\"%s\", \"%s\", beam_radius=%g)\n" % \
            (self.sample_file, self.direct_beam, self.beam_radius)
            
        def to_xml(self):
            """
                Create XML from the current data.
            """
            xml  = "<DirectBeam>\n"
            xml += "  <sample_file>%s</sample_file>\n" % self.sample_file
            xml += "  <direct_beam>%s</direct_beam>\n" % self.direct_beam
            xml += "  <beam_radius>%g</beam_radius>\n" % self.beam_radius
            xml += "</DirectBeam>\n"
            return xml
        
        def find(self, dom):
            element_list = dom.getElementsByTagName("DirectBeam")
            return len(element_list)>0
        
        def from_xml(self, dom):
            """
                Read in data from XML
                @param dom: text to read the data from
            """
            element_list = dom.getElementsByTagName("DirectBeam")
            if len(element_list)>0:
                instrument_dom = element_list[0]       
                self.sample_file = BaseScriptElement.getStringElement(instrument_dom, "sample_file")
                self.direct_beam = BaseScriptElement.getStringElement(instrument_dom, "direct_beam")
                self.beam_radius = BaseScriptElement.getFloatElement(instrument_dom, "beam_radius",
                                                                     default=Transmission.DirectBeam.beam_radius)           
        
        def reset(self):
            """
                Reset state
            """
            self.sample_file = ''
            self.direct_beam = ''
            self.beam_radius = Transmission.DirectBeam.beam_radius   
            
    class BeamSpreader(BaseScriptElement):
        sample_scatt = ''
        sample_spreader = ''
        direct_scatt = ''
        direct_spreader = ''
        spreader_trans = 1.0
        spreader_trans_spread = 0.0
        
        def to_script(self):
            """
                Generate reduction script
                @param execute: if true, the script will be executed
            """
            if len(str(self.sample_scatt).strip())==0 \
                or len(str(self.sample_spreader).strip())==0 \
                or len(str(self.direct_scatt).strip())==0 \
                or len(str(self.direct_spreader).strip())==0:
                raise RuntimeError, "Transmission with beam spreader was selected but data files were not entered."
                        
            return "BeamSpreaderTransmission(\"%s\",\n \"%s\",\n \"%s\",\n \"%s\", %g, %g)\n" % \
            (self.sample_spreader, self.direct_spreader, 
             self.sample_scatt, self.direct_scatt, 
             self.spreader_trans, self.spreader_trans_spread) 
            
        def to_xml(self):
            """
                Create XML from the current data.
            """
            xml  = "<BeamSpreader>\n"
            xml += "  <sample_scatt>%s</sample_scatt>\n" % self.sample_scatt
            xml += "  <sample_spreader>%s</sample_spreader>\n" % self.sample_spreader
            xml += "  <direct_scatt>%s</direct_scatt>\n" % self.direct_scatt
            xml += "  <direct_spreader>%s</direct_spreader>\n" % self.direct_spreader

            xml += "  <spreader_trans>%g</spreader_trans>\n" % self.spreader_trans
            xml += "  <spreader_trans_spread>%g</spreader_trans_spread>\n" % self.spreader_trans_spread
            xml += "</BeamSpreader>\n"
            return xml
            
        def find(self, dom):
            element_list = dom.getElementsByTagName("BeamSpreader")
            return len(element_list)>0
        
        def from_xml(self, dom):
            """
                Read in data from XML
                @param dom: text to read the data from
            """       
            element_list = dom.getElementsByTagName("BeamSpreader")
            if len(element_list)>0:
                instrument_dom = element_list[0]      
                self.sample_scatt = BaseScriptElement.getStringElement(instrument_dom, "sample_scatt")
                self.sample_spreader = BaseScriptElement.getStringElement(instrument_dom, "sample_spreader")
                self.direct_scatt = BaseScriptElement.getStringElement(instrument_dom, "direct_scatt")
                self.direct_spreader = BaseScriptElement.getStringElement(instrument_dom, "direct_spreader")
                self.spreader_trans = BaseScriptElement.getFloatElement(instrument_dom, "spreader_trans",
                                                                     default=Transmission.BeamSpreader.spreader_trans)           
                self.spreader_trans_spread = BaseScriptElement.getFloatElement(instrument_dom, "spreader_trans_spread",
                                                                     default=Transmission.BeamSpreader.spreader_trans_spread)           

        def reset(self):
            """
                Reset state
            """
            self.sample_scatt = ''
            self.sample_spreader = ''
            self.direct_scatt = ''
            self.direct_spreader = ''
            self.spreader_trans = Transmission.BeamSpreader.spreader_trans           
            self.spreader_trans_spread = Transmission.BeamSpreader.spreader_trans_spread     

    transmission = 1.0
    transmission_spread = 0.0
    calculate_transmission = False
    calculation_method = DirectBeam()
    theta_dependent = True
            
    def to_script(self):
        """
            Generate reduction script
            @param execute: if true, the script will be executed
        """
        script = ""
        if not self.calculate_transmission:
            script += "SetTransmission(%g, %g)\n" % (self.transmission, self.transmission_spread)
        else:
            script += str(self.calculation_method)
            
        script += "ThetaDependentTransmission(%s)\n" % str(self.theta_dependent)
        return script

    def update(self):
        """
            Update transmission from reduction output
        """
        if HAS_MANTID and ReductionSingleton()._transmission_calculator is not None:
            trans = ReductionSingleton()._transmission_calculator.get_transmission()
            self.transmission = trans[0]
            self.transmission_spread = trans[1]
            

    def to_xml(self):
        """
            Create XML from the current data.
        """
        xml  = "<Transmission>\n"
        xml += "  <trans>%g</trans>\n" % self.transmission
        xml += "  <trans_spread>%g</trans_spread>\n" % self.transmission_spread
        xml += "  <calculate_trans>%s</calculate_trans>\n" % str(self.calculate_transmission)
        xml += "  <theta_dependent>%s</theta_dependent>\n" % str(self.theta_dependent)
        if self.calculate_transmission:
            xml += self.calculation_method.to_xml()
        xml += "</Transmission>\n"
        return xml

    def from_xml(self, xml_str):
        """
            Read in data from XML
            @param xml_str: text to read the data from
        """       
        dom = xml.dom.minidom.parseString(xml_str)
        element_list = dom.getElementsByTagName("Transmission")
        if len(element_list)>0:
            instrument_dom = element_list[0]      
            self.transmission = BaseScriptElement.getFloatElement(instrument_dom, "trans",
                                                                  default=Transmission.transmission)      
            self.transmission_spread = BaseScriptElement.getFloatElement(instrument_dom, "trans_spread",
                                                                  default=Transmission.transmission_spread)  
            self.calculate_transmission = BaseScriptElement.getBoolElement(instrument_dom, "calculate_trans",
                                                                           default = Transmission.calculate_transmission)
            self.theta_dependent = BaseScriptElement.getBoolElement(instrument_dom, "theta_dependent",
                                                                           default = Transmission.theta_dependent)
            
            if self.calculate_transmission:
                for m in [Transmission.DirectBeam, Transmission.BeamSpreader]:
                    method = m()
                    if method.find(dom):
                        method.from_xml(dom)
                        self.calculation_method = method
                        break
    
    def reset(self):
        """
            Reset state
        """
        self.transmission = Transmission.transmission      
        self.transmission_spread = Transmission.transmission_spread  
        self.calculate_transmission = Transmission.calculate_transmission
        self.calculation_method = Transmission.calculation_method
        self.theta_dependent = Transmission.theta_dependent
    
class Background(BaseScriptElement):
    
    class DirectBeam(Transmission.DirectBeam):
        def __init__(self, state=None):
            Transmission.DirectBeam.__init__(self)
            if state is not None:
                self.sample_file = state.sample_file
                self.direct_beam = state.direct_beam
                self.beam_radius = state.beam_radius

        def to_script(self):
            """
                Generate reduction script
                @param execute: if true, the script will be executed
            """
            if len(str(self.sample_file).strip())==0 or len(str(self.direct_beam).strip())==0:
                raise RuntimeError, "Direct beam method for background transmission was selected but was selected but all the appropriate data files were not entered."    
            
            return "BckDirectBeamTransmission(\"%s\", \"%s\", beam_radius=%g)\n" % \
            (self.sample_file, self.direct_beam, self.beam_radius)
        
    class BeamSpreader(Transmission.BeamSpreader):
        def __init__(self, state=None):
            Transmission.BeamSpreader.__init__(self)
            if state is not None:
                self.sample_scatt = state.sample_scatt
                self.sample_spreader = state.sample_spreader
                self.direct_scatt = state.direct_scatt
                self.direct_spreader = state.direct_spreader
                self.spreader_trans = state.spreader_trans
                self.spreader_trans_spread = state.spreader_trans_spread

        def to_script(self):
            """
                Generate reduction script
                @param execute: if true, the script will be executed
            """
            if len(str(self.sample_scatt).strip())==0\
                or len(str(self.sample_spreader).strip())==0\
                or len(str(self.direct_scatt).strip())==0\
                or len(str(self.direct_spreader).strip())==0:
                raise RuntimeError, "Beam spreader method for background transmission was selected but all the appropriate data files were not entered."    
                    
            return "BckBeamSpreaderTransmission(\"%s\",\n \"%s\",\n \"%s\",\n \"%s\", %g, %g)\n" % \
            (self.sample_spreader, self.direct_spreader, 
             self.sample_scatt, self.direct_scatt, 
             self.spreader_trans, self.spreader_trans_spread) 
        
    dark_current_corr = False
    dark_current_file = ''
    
    background_corr = False
    background_file = ''
        
    bck_transmission_enabled = True
    bck_transmission = 1.0
    bck_transmission_spread = 0.0
    calculate_transmission = False 
    theta_dependent = True 
    trans_calculation_method = DirectBeam()
        
    def to_script(self):
        """
            Generate reduction script
        """
        script = ""
        
        # Dark current
        if self.dark_current_corr:
            if len(str(self.dark_current_file).strip())==0:
                raise RuntimeError, "Dark current subtraction was selected but no dark current data file was entered."            
            script += "DarkCurrent(\"%s\")\n" % self.dark_current_file
        
        # Background
        if self.background_corr:
            if len(str(self.background_file).strip())==0:
                raise RuntimeError, "Background subtraction was selected but no background data file was entered."            
            script += "Background(\"%s\")\n" % self.background_file
            
            # Background transmission
            if self.bck_transmission_enabled:
                if not self.calculate_transmission:
                    script += "SetBckTransmission(%g, %g)\n" % (self.bck_transmission, self.bck_transmission_spread)
                else:
                    script += str(self.trans_calculation_method)

        script += "BckThetaDependentTransmission(%s)\n" % str(self.theta_dependent)
            
        return script           
    
    def update(self):
        """
            Update data member from reduction output
        """
        if HAS_MANTID and ReductionSingleton()._background_subtracter is not None:
            trans = ReductionSingleton()._background_subtracter.get_transmission()
            if trans is not None:
                self.bck_transmission = trans[0]
                self.bck_transmission_spread = trans[1]
            
    def to_xml(self):
        """
            Create XML from the current data.
        """
        xml  = "<Background>\n"
        xml += "  <dark_current_corr>%s</dark_current_corr>\n" % str(self.dark_current_corr)
        if self.dark_current_corr:
            xml += "  <dark_current_file>%s</dark_current_file>\n" % self.dark_current_file

        xml += "  <background_corr>%s</background_corr>\n" % str(self.background_corr)
        if self.background_corr:
            xml += "  <background_file>%s</background_file>\n" % self.background_file
            xml += "  <bck_trans_enabled>%s</bck_trans_enabled>\n" % str(self.bck_transmission_enabled)
            xml += "  <bck_trans>%g</bck_trans>\n" % self.bck_transmission
            xml += "  <bck_trans_spread>%g</bck_trans_spread>\n" % self.bck_transmission_spread
            xml += "  <calculate_trans>%s</calculate_trans>\n" % str(self.calculate_transmission)
            xml += "  <theta_dependent>%s</theta_dependent>\n" % str(self.theta_dependent)
            if self.calculate_transmission:
                xml += self.trans_calculation_method.to_xml()
        xml += "</Background>\n"
        return xml

    def from_xml(self, xml_str):
        """
            Read in data from XML
            @param xml_str: text to read the data from
        """       
        dom = xml.dom.minidom.parseString(xml_str)
        element_list = dom.getElementsByTagName("Background")
        if len(element_list)>0:
            instrument_dom = element_list[0]   
            
            self.dark_current_corr = BaseScriptElement.getBoolElement(instrument_dom, "dark_current_corr",
                                                                      default = Background.dark_current_corr)
            self.dark_current_file = BaseScriptElement.getStringElement(instrument_dom, "dark_current_file")

            self.background_corr = BaseScriptElement.getBoolElement(instrument_dom, "background_corr",
                                                                      default = Background.background_corr)
            self.background_file = BaseScriptElement.getStringElement(instrument_dom, "background_file")
               
            self.bck_transmission_enabled = BaseScriptElement.getBoolElement(instrument_dom, "bck_trans_enabled",
                                                                           default = Background.bck_transmission_enabled)

            self.bck_transmission = BaseScriptElement.getFloatElement(instrument_dom, "bck_trans",
                                                                  default=Background.bck_transmission)      
            self.bck_transmission_spread = BaseScriptElement.getFloatElement(instrument_dom, "bck_trans_spread",
                                                                  default=Background.bck_transmission_spread)  
            self.calculate_transmission = BaseScriptElement.getBoolElement(instrument_dom, "calculate_trans",
                                                                           default = Background.calculate_transmission)
            self.theta_dependent = BaseScriptElement.getBoolElement(instrument_dom, "theta_dependent",
                                                                           default = Background.theta_dependent)

            if self.calculate_transmission:
                for m in [Background.DirectBeam, Background.BeamSpreader]:
                    method = m()
                    if method.find(dom):
                        method.from_xml(dom)
                        self.trans_calculation_method = method
                        break

    def reset(self):
        """
            Reset state
        """
        self.dark_current_corr = Background.dark_current_corr
        self.dark_current_file = ''
        self.background_corr = Background.background_corr
        self.background_file = ''
        self.bck_transmission_enabled = Background.bck_transmission_enabled      
        self.bck_transmission = Background.bck_transmission      
        self.bck_transmission_spread = Background.bck_transmission_spread  
        self.calculate_transmission = Background.calculate_transmission
        self.theta_dependent = Background.theta_dependent
        self.trans_calculation_method = Background.trans_calculation_method
    
class DataSets(BaseScriptElement):
    def __str__(self):
        return ""
    
class InstrumentDescription(BaseScriptElement):
    instrument_name = "BIOSANS"
    nx_pixels = 192
    ny_pixels = 192
    pixel_size = 5.1
    
    # Data file
    data_file = ''
    
    # Mask
    mask_top = 0
    mask_bottom = 0
    mask_left = 0
    mask_right = 0
    
    # Sample-detector distance to force on the data set [mm]
    sample_detector_distance = 0
    # Detector distance offset [mm]
    detector_offset = 837.9
    # Wavelength value to force on the data set [Angstrom]
    wavelength = 0
    wavelength_spread = 0.1
    
    # Flag to perform the solid angle correction
    solid_angle_corr = True
    # Flag to perform sensitivity correction
    sensitivity_corr = False
    # Data file to be used to calculate sensitivity
    sensitivity_data = ''
    # Minimum allowed relative sensitivity
    min_sensitivity = 0.5
    max_sensitivity = 1.5
    
    # Q range
    n_q_bins = 100
    n_sub_pix = 1
    log_binning = False
    
    
    NORMALIZATION_NONE = 0
    NORMALIZATION_TIME = 1
    NORMALIZATION_MONITOR = 2
    normalization = NORMALIZATION_MONITOR
        
    def to_script(self):
        """
            Generate reduction script
            @param execute: if true, the script will be executed
        """
        script  = "HFIRSANS()\n"
        
        if self.sample_detector_distance != 0:
            script += "SetSampleDetectorDistance(%g)\n" % self.sample_detector_distance 
        if self.detector_offset != 0:
            script += "SetSampleDetectorOffset(%g)\n" % self.detector_offset 
        if self.wavelength != 0:
            script += "SetWavelength(%g, %g)\n" % (self.wavelength, self.wavelength_spread)
        
        if self.solid_angle_corr:
            script += "SolidAngle()\n"
        else:
            script += "NoSolidAngle()\n"
        
        if self.sensitivity_corr:
            if len(str(self.sensitivity_data).strip())==0:
                raise RuntimeError, "Sensitivity correction was selected but no sensitivity data file was entered."
                
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
        
        # Mask
        script += "Mask(nx_low=%d, nx_high=%d, ny_low=%d, ny_high=%d)\n" % (self.mask_left, self.mask_right, self.mask_bottom, self.mask_top)
        
        # Q binning
        script += "AzimuthalAverage(n_bins=%g, n_subpix=%g, log_binning=%s)\n" % (self.n_q_bins, self.n_sub_pix, str(self.log_binning))        
        
        # Data file
        if len(str(self.data_file).strip())>0:
            parts = os.path.split(str(self.data_file))
            script += "DataPath(\"%s\")\n" % parts[0]
            script += "AppendDataFile(\"%s\")\n" % self.data_file
        else:
            raise RuntimeError, "Trying to generate reduction script without a data file."
        
        return script           
    
    def to_xml(self):
        """
            Create XML from the current data.
        """
        xml  = "<Instrument>\n"
        xml += "  <name>%s</name>\n" % self.instrument_name
        xml += "  <nx_pixels>%g</nx_pixels>\n" % self.nx_pixels
        xml += "  <ny_pixels>%g</ny_pixels>\n" % self.ny_pixels
        xml += "  <pixel_size>%g</pixel_size>\n" % self.pixel_size

        xml += "  <data_file>%s</data_file>\n" % self.data_file
        xml += "  <mask_top>%g</mask_top>\n" % self.mask_top
        xml += "  <mask_bottom>%g</mask_bottom>\n" % self.mask_bottom
        xml += "  <mask_left>%g</mask_left>\n" % self.mask_left
        xml += "  <mask_right>%g</mask_right>\n" % self.mask_right

        xml += "  <sample_det_dist>%g</sample_det_dist>\n" % self.sample_detector_distance
        xml += "  <detector_offset>%g</detector_offset>\n" % self.detector_offset
        xml += "  <wavelength>%g</wavelength>\n" % self.wavelength
        xml += "  <wavelength_spread>%g</wavelength_spread>\n" % self.wavelength_spread
        
        xml += "  <solid_angle_corr>%s</solid_angle_corr>\n" % str(self.solid_angle_corr)
        xml += "  <sensitivity_corr>%s</sensitivity_corr>\n" % str(self.sensitivity_corr)
        xml += "  <sensitivity_data>%s</sensitivity_data>\n" % self.sensitivity_data
        xml += "  <sensitivity_min>%s</sensitivity_min>\n" % self.min_sensitivity
        xml += "  <sensitivity_max>%s</sensitivity_max>\n" % self.max_sensitivity

        xml += "  <n_q_bins>%g</n_q_bins>\n" % self.n_q_bins
        xml += "  <n_sub_pix>%g</n_sub_pix>\n" % self.n_sub_pix
        xml += "  <log_binning>%s</log_binning>\n" % str(self.log_binning)

        xml += "  <normalization>%d</normalization>\n" % self.normalization
        
        xml += "</Instrument>\n"

        return xml
        
    def from_xml(self, xml_str):
        """
            Read in data from XML
            @param xml_str: text to read the data from
        """       
        dom = xml.dom.minidom.parseString(xml_str)
        instrument_dom = dom.getElementsByTagName("Instrument")[0]
        self.nx_pixels = BaseScriptElement.getIntElement(instrument_dom, "nx_pixels",
                                                         default=InstrumentDescription.nx_pixels) 
        self.ny_pixels = BaseScriptElement.getIntElement(instrument_dom, "ny_pixels",
                                                         default=InstrumentDescription.ny_pixels) 
        self.name = BaseScriptElement.getStringElement(instrument_dom, "name")
        self.pixel_size = BaseScriptElement.getFloatElement(instrument_dom, "pixel_size",
                                                            default=InstrumentDescription.pixel_size)
        
        self.data_file = BaseScriptElement.getStringElement(instrument_dom, "data_file")
        self.mask_top = BaseScriptElement.getIntElement(instrument_dom, "mask_top",
                                                        default=InstrumentDescription.mask_top)
        self.mask_bottom = BaseScriptElement.getIntElement(instrument_dom, "mask_bottom",
                                                           default=InstrumentDescription.mask_bottom)
        self.mask_right = BaseScriptElement.getIntElement(instrument_dom, "mask_right",
                                                          default=InstrumentDescription.mask_right)
        self.mask_left = BaseScriptElement.getIntElement(instrument_dom, "mask_left",
                                                         default=InstrumentDescription.mask_left)
        
        self.sample_detector_distance = BaseScriptElement.getFloatElement(instrument_dom, "sample_det_dist", 
                                                                          default=InstrumentDescription.sample_detector_distance)
        self.detector_offset = BaseScriptElement.getFloatElement(instrument_dom, "detector_offset",
                                                                 default=InstrumentDescription.detector_offset)
        self.wavelength = BaseScriptElement.getFloatElement(instrument_dom, "wavelength",
                                                            default=InstrumentDescription.wavelength)
        self.wavelength_spread = BaseScriptElement.getFloatElement(instrument_dom, "wavelength_spread",
                                                            default=InstrumentDescription.wavelength_spread)
        
        self.solid_angle_corr = BaseScriptElement.getBoolElement(instrument_dom, "solid_angle_corr",
                                                                 default = InstrumentDescription.solid_angle_corr)
        self.sensitivity_corr = BaseScriptElement.getBoolElement(instrument_dom, "sensitivity_corr",
                                                                 default = InstrumentDescription.sensitivity_corr)
        self.sensitivity_data = BaseScriptElement.getStringElement(instrument_dom, "sensitivity_data")
        self.min_sensitivity = BaseScriptElement.getFloatElement(instrument_dom, "sensitivity_min",
                                                            default=InstrumentDescription.min_sensitivity)
        self.max_sensitivity = BaseScriptElement.getFloatElement(instrument_dom, "sensitivity_max",
                                                            default=InstrumentDescription.max_sensitivity)
        
        self.n_q_bins = BaseScriptElement.getIntElement(instrument_dom, "n_q_bins",
                                                       default=InstrumentDescription.n_q_bins)
        self.n_sub_pix = BaseScriptElement.getIntElement(instrument_dom, "n_sub_pix",
                                                       default=InstrumentDescription.n_sub_pix)
        self.log_binning = BaseScriptElement.getBoolElement(instrument_dom, "log_binning",
                                                                 default = InstrumentDescription.log_binning)
        
        self.normalization = BaseScriptElement.getIntElement(instrument_dom, "normalization",
                                                             default=InstrumentDescription.normalization)

    def reset(self):
        """
            Reset state
        """
        self.nx_pixels = InstrumentDescription.nx_pixels
        self.ny_pixels = InstrumentDescription.ny_pixels
        self.name = ''
        self.pixel_size = InstrumentDescription.pixel_size
        
        self.data_file = ''
        self.mask_top = InstrumentDescription.mask_top
        self.mask_bottom = InstrumentDescription.mask_bottom
        self.mask_right = InstrumentDescription.mask_right
        self.mask_left = InstrumentDescription.mask_left
        
        self.sample_detector_distance = InstrumentDescription.sample_detector_distance
        self.detector_offset = InstrumentDescription.detector_offset
        self.wavelength = InstrumentDescription.wavelength
        self.wavelength_spread = InstrumentDescription.wavelength_spread
        
        self.solid_angle_corr = InstrumentDescription.solid_angle_corr
        self.sensitivity_corr = InstrumentDescription.sensitivity_corr
        self.sensitivity_data = ''
        self.min_sensitivity = InstrumentDescription.min_sensitivity
        self.max_sensitivity = InstrumentDescription.max_sensitivity
        
        self.n_q_bins = InstrumentDescription.n_q_bins
        self.n_sub_pix = InstrumentDescription.n_sub_pix
        self.log_binning = InstrumentDescription.log_binning
        
        self.normalization = InstrumentDescription.normalization

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
    
    def to_script(self):
        """
            Generate reduction script
            @param execute: if true, the script will be executed
        """
        script = ""
        if not self.use_finder:
            script += "SetBeamCenter(%g, %g)\n" % (self.x_position, self.y_position) 
        else:
            if len(str(self.beam_file).strip())==0:
                raise RuntimeError, "Beam finder was selected but no data file was entered."

            if self.use_direct_beam:
                script += "DirectBeamCenter(\"%s\")\n" % self.beam_file
            else:
                script += "ScatteringBeamCenter(\"%s\", %g)\n" % (self.beam_file, self.beam_radius)
        return script
        
    def update(self):
        """
            Update data members according to reduction results
        """
        if HAS_MANTID and ReductionSingleton()._beam_finder is not None:
            pos = ReductionSingleton()._beam_finder.get_beam_center()
            self.x_position = pos[0]
            self.y_position = pos[1]
    
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
        self.x_position = BeamFinder.x_position
        self.y_position = BeamFinder.y_position
        self.use_finder = BeamFinder.use_finder
        self.beam_file = ''
        self.use_direct_beam = BeamFinder.use_direct_beam
        
class Output(BaseScriptElement):
    log_text = ''
    data = None
    
    def update(self):
        """
            Update data members according to reduction results
        """
        self.log_text = ReductionSingleton().log_text
        if HAS_MANTID and ReductionSingleton()._azimuthal_averager is not None:
            self.data = ReductionSingleton()._azimuthal_averager.get_data(ReductionSingleton()._data_files.keys()[0])
    
       