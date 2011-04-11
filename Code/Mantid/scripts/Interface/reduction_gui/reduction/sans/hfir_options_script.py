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
    
    # Masking
    class RectangleMask(object):
        def __init__(self, x_min=0, x_max=0, y_min=0, y_max=0):
            self.x_min = x_min
            self.x_max = x_max
            self.y_min = y_min
            self.y_max = y_max
        
    # Masked edges
    top = 0
    bottom = 0
    left = 0
    right = 0
    
    # MAsked shapes
    shapes = []
    
    # Masked detector IDs
    detector_ids = ''

    
    NORMALIZATION_NONE = 0
    NORMALIZATION_TIME = 1
    NORMALIZATION_MONITOR = 2
    normalization = NORMALIZATION_MONITOR
        
    def __init__(self):
        super(ReductionOptions, self).__init__()
        self.reset()
        
    def to_script(self):
        """
            Generate reduction script
            @param execute: if true, the script will be executed
        """
        script  = "%s()\n" % self.instrument_name
        
        if self.scaling_factor != 1:
            script += "ScaleFactor(%g)\n" % self.scaling_factor
        
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
            script += "DarkCurrent(\"%s\")\n" % self.dark_current_data
            
        if self.normalization==ReductionOptions.NORMALIZATION_NONE:
            script += "NoNormalization()\n"
        elif self.normalization==ReductionOptions.NORMALIZATION_TIME:
            script += "TimeNormalization()\n"
        elif self.normalization==ReductionOptions.NORMALIZATION_MONITOR:
            script += "MonitorNormalization()\n"
        
        # Q binning
        script += "AzimuthalAverage(n_bins=%g, n_subpix=%g, log_binning=%s)\n" % (self.n_q_bins, self.n_sub_pix, str(self.log_binning))        
        script += "IQxQy(nbins=%g)\n" % self.n_q_bins
        
        # Mask 
        #   Edges
        if (self.top != 0 or self.bottom != 0 or self.left != 0 or self.right != 0):
            script += "Mask(nx_low=%d, nx_high=%d, ny_low=%d, ny_high=%d)\n" % (self.left, self.right, self.bottom, self.top)
        #   Rectangles
        for item in self.shapes:
            script += "MaskRectangle(x_min=%g, x_max=%g, y_min=%g, y_max=%g)\n" % (item.x_min, item.x_max, item.y_min, item.y_max)
        #   Detector IDs
        if len(self.detector_ids)>0:
            script += "MaskDetectors([%s])\n" % self.detector_ids

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
        
        xml += "<Mask>\n"
        xml += "  <mask_top>%g</mask_top>\n" % self.top
        xml += "  <mask_bottom>%g</mask_bottom>\n" % self.bottom
        xml += "  <mask_left>%g</mask_left>\n" % self.left
        xml += "  <mask_right>%g</mask_right>\n" % self.right
        
        xml += "  <Shapes>\n"
        for item in self.shapes:
            xml += "    <rect x_min='%g' x_max='%g' y_min='%g' y_max='%g' />\n" % (item.x_min, item.x_max, item.y_min, item.y_max)
        xml += "  </Shapes>\n"
        
        xml += "  <DetectorIDs>\n"
        xml += "    %s\n" % self.detector_ids
        xml += "  </DetectorIDs>\n"
        
        xml += "</Mask>\n"


        return xml
        
    def from_xml(self, xml_str):
        """
            Read in data from XML
            @param xml_str: text to read the data from
        """    
        self.reset()   
        dom = xml.dom.minidom.parseString(xml_str)
        
        # Get Mantid version
        mtd_version = BaseScriptElement.getMantidBuildVersion(dom)
        
        instrument_dom = dom.getElementsByTagName("Instrument")[0]
        self.nx_pixels = BaseScriptElement.getIntElement(instrument_dom, "nx_pixels",
                                                         default=ReductionOptions.nx_pixels) 
        self.ny_pixels = BaseScriptElement.getIntElement(instrument_dom, "ny_pixels",
                                                         default=ReductionOptions.ny_pixels) 
        self.instrument_name = BaseScriptElement.getStringElement(instrument_dom, "name")
        self.pixel_size = BaseScriptElement.getFloatElement(instrument_dom, "pixel_size",
                                                            default=ReductionOptions.pixel_size)
        
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
        
        # Dark current - take care of backward compatibility
        if mtd_version<BaseScriptElement.UPDATE_1_CHANGESET_CUTOFF:
            bck_entries = dom.getElementsByTagName("Background")
            if len(bck_entries)>0:
                self.dark_current_corr = BaseScriptElement.getBoolElement(bck_entries[0], "dark_current_corr",
                                                                          default = ReductionOptions.dark_current_corr)
                self.dark_current_data = BaseScriptElement.getStringElement(bck_entries[0], "dark_current_file")
        else:
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

        # Mask - take care of backward compatibility
        if mtd_version<BaseScriptElement.UPDATE_1_CHANGESET_CUTOFF:
            self.top = BaseScriptElement.getIntElement(instrument_dom, "mask_top", default=ReductionOptions.top)
            self.bottom = BaseScriptElement.getIntElement(instrument_dom, "mask_bottom", default=ReductionOptions.bottom)
            self.right = BaseScriptElement.getIntElement(instrument_dom, "mask_right", default=ReductionOptions.right)
            self.left = BaseScriptElement.getIntElement(instrument_dom, "mask_left", default=ReductionOptions.left)
        else:   
            element_list = dom.getElementsByTagName("Mask")
            if len(element_list)>0: 
                mask_dom = element_list[0]
                self.top = BaseScriptElement.getIntElement(mask_dom, "mask_top", default=ReductionOptions.top)
                self.bottom = BaseScriptElement.getIntElement(mask_dom, "mask_bottom", default=ReductionOptions.bottom)
                self.right = BaseScriptElement.getIntElement(mask_dom, "mask_right", default=ReductionOptions.right)
                self.left = BaseScriptElement.getIntElement(mask_dom, "mask_left", default=ReductionOptions.left)
                
                self.shapes = []
                shapes_dom_list = mask_dom.getElementsByTagName("Shapes")
                if len(shapes_dom_list)>0:
                    shapes_dom = shapes_dom_list[0]
                    for item in shapes_dom.getElementsByTagName("rect"):
                        x_min =  float(item.getAttribute("x_min"))
                        x_max =  float(item.getAttribute("x_max"))
                        y_min =  float(item.getAttribute("y_min"))
                        y_max =  float(item.getAttribute("y_max"))
                        self.shapes.append(ReductionOptions.RectangleMask(x_min, x_max, y_min, y_max))
                                
                self.detector_ids = ''
                self.detector_ids = BaseScriptElement.getStringElement(mask_dom, "DetectorIDs", default='').strip()

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
        
        self.top = ReductionOptions.top
        self.bottom = ReductionOptions.bottom
        self.left = ReductionOptions.left
        self.right = ReductionOptions.right
        
        self.shapes = []
        self.detector_ids = ''

