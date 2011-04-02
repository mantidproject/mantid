"""
    Classes for each reduction step. Those are kept separately 
    from the the interface class so that the HFIRReduction class could 
    be used independently of the interface implementation
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
   
class ReductionOptions(BaseScriptElement):
    instrument_name = "BIOSANS"
    nx_pixels = 192
    ny_pixels = 192
    pixel_size = 5.1
    
    # Absoulte scale
    scaling_factor = 1.0
    
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
    # Dark current
    dark_current_corr = False
    dark_current_data = ''
    
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
        script  = "%s()\n" % self.instrument_name
        
        if self.scaling_factor != 1:
            script += "#ScaleFactor(%g)\n" % self.scaling_factor
        
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
        
        if self.dark_current_corr:
            if len(str(self.dark_current_data).strip())==0:
                raise RuntimeError, "Dark current subtraction was selected but no sensitivity data file was entered." 
            script += "DarkCurrent(\"%s\")\n" % self.dark_current_datas
            
        if self.normalization==ReductionOptions.NORMALIZATION_NONE:
            script += "NoNormalization()\n"
        elif self.normalization==ReductionOptions.NORMALIZATION_TIME:
            script += "TimeNormalization()\n"
        elif self.normalization==ReductionOptions.NORMALIZATION_MONITOR:
            script += "MonitorNormalization()\n"
        
        # Mask
        script += "Mask(nx_low=%d, nx_high=%d, ny_low=%d, ny_high=%d)\n" % (self.mask_left, self.mask_right, self.mask_bottom, self.mask_top)
        
        # Q binning
        script += "AzimuthalAverage(n_bins=%g, n_subpix=%g, log_binning=%s)\n" % (self.n_q_bins, self.n_sub_pix, str(self.log_binning))        
        script += "IQxQy(nbins=%g)\n" % self.n_q_bins
        
        # Data files
        if len(self.data_files)>0:
            parts = os.path.split(str(self.data_files[0]).strip())
            script += "DataPath(\"%s\")\n" % parts[0]
            script += "AppendDataFile([\"%s\"])\n" % '\",\"'.join(self.data_files)
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

        #for item in self.data_files:
        #    xml += "  <data_file>%s</data_file>\n" % item
        #xml += "  <mask_top>%g</mask_top>\n" % self.mask_top
        #xml += "  <mask_bottom>%g</mask_bottom>\n" % self.mask_bottom
        #xml += "  <mask_left>%g</mask_left>\n" % self.mask_left
        #xml += "  <mask_right>%g</mask_right>\n" % self.mask_right
        
        xml += "  <scaling_factor>%g</scaling_factor>\n" % self.scaling_factor

        xml += "  <sample_det_dist>%g</sample_det_dist>\n" % self.sample_detector_distance
        xml += "  <detector_offset>%g</detector_offset>\n" % self.detector_offset
        xml += "  <wavelength>%g</wavelength>\n" % self.wavelength
        xml += "  <wavelength_spread>%g</wavelength_spread>\n" % self.wavelength_spread
        
        xml += "  <solid_angle_corr>%s</solid_angle_corr>\n" % str(self.solid_angle_corr)
        xml += "  <dark_current_corr>%s</dark_current_corr>\n" % str(self.dark_current_corr)
        xml += "  <dark_current_data>%s</dark_current_data>\n" % self.dark_current_data

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
                                                         default=ReductionOptions.nx_pixels) 
        self.ny_pixels = BaseScriptElement.getIntElement(instrument_dom, "ny_pixels",
                                                         default=ReductionOptions.ny_pixels) 
        self.instrument_name = BaseScriptElement.getStringElement(instrument_dom, "name")
        self.pixel_size = BaseScriptElement.getFloatElement(instrument_dom, "pixel_size",
                                                            default=ReductionOptions.pixel_size)
        
        #self.data_files = BaseScriptElement.getStringList(instrument_dom, "data_file")
        #self.mask_top = BaseScriptElement.getIntElement(instrument_dom, "mask_top",
        #                                                default=ReductionOptions.mask_top)
        #self.mask_bottom = BaseScriptElement.getIntElement(instrument_dom, "mask_bottom",
        #                                                   default=ReductionOptions.mask_bottom)
        #self.mask_right = BaseScriptElement.getIntElement(instrument_dom, "mask_right",
        #                                                  default=ReductionOptions.mask_right)
        #self.mask_left = BaseScriptElement.getIntElement(instrument_dom, "mask_left",
        #                                                 default=ReductionOptions.mask_left)
        
        self.scaling_factor = BaseScriptElement.getFloatElement(instrument_dom, "scaling_factor", 
                                                                          default=ReductionOptions.scaling_factor)

        self.sample_detector_distance = BaseScriptElement.getFloatElement(instrument_dom, "sample_det_dist", 
                                                                          default=ReductionOptions.sample_detector_distance)
        self.detector_offset = BaseScriptElement.getFloatElement(instrument_dom, "detector_offset",
                                                                 default=ReductionOptions.detector_offset)
        self.wavelength = BaseScriptElement.getFloatElement(instrument_dom, "wavelength",
                                                            default=ReductionOptions.wavelength)
        self.wavelength_spread = BaseScriptElement.getFloatElement(instrument_dom, "wavelength_spread",
                                                            default=ReductionOptions.wavelength_spread)
        
        self.solid_angle_corr = BaseScriptElement.getBoolElement(instrument_dom, "solid_angle_corr",
                                                                 default = ReductionOptions.solid_angle_corr)
        self.dark_current_corr = BaseScriptElement.getBoolElement(instrument_dom, "dark_current_corr",
                                                                 default = ReductionOptions.dark_current_corr)
        self.dark_current_data = BaseScriptElement.getStringElement(instrument_dom, "dark_current_data")        
        self.n_q_bins = BaseScriptElement.getIntElement(instrument_dom, "n_q_bins",
                                                       default=ReductionOptions.n_q_bins)
        self.n_sub_pix = BaseScriptElement.getIntElement(instrument_dom, "n_sub_pix",
                                                       default=ReductionOptions.n_sub_pix)
        self.log_binning = BaseScriptElement.getBoolElement(instrument_dom, "log_binning",
                                                                 default = ReductionOptions.log_binning)
        
        self.normalization = BaseScriptElement.getIntElement(instrument_dom, "normalization",
                                                             default=ReductionOptions.normalization)

    def reset(self):
        """
            Reset state
        """
        self.nx_pixels = ReductionOptions.nx_pixels
        self.ny_pixels = ReductionOptions.ny_pixels
        #self.instrument_name = ''
        self.pixel_size = ReductionOptions.pixel_size
        
        self.scaling_factor = ReductionOptions.scaling_factor
        
        self.sample_detector_distance = ReductionOptions.sample_detector_distance
        self.detector_offset = ReductionOptions.detector_offset
        self.wavelength = ReductionOptions.wavelength
        self.wavelength_spread = ReductionOptions.wavelength_spread
        
        self.solid_angle_corr = ReductionOptions.solid_angle_corr
        self.dark_current_corr = ReductionOptions.dark_current_corr
        self.dark_current_data = ''
        
        self.n_q_bins = ReductionOptions.n_q_bins
        self.n_sub_pix = ReductionOptions.n_sub_pix
        self.log_binning = ReductionOptions.log_binning
        
        self.normalization = ReductionOptions.normalization
