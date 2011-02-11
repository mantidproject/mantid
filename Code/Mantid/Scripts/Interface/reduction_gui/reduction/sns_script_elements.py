"""
    Classes that implement script elements specific to EQSANS
"""
import xml.dom.minidom
import copy
import os
from scripter import BaseScriptElement

# Check whether Mantid is available
try:
    from MantidFramework import *
    mtd.initialise(False)
    from reduction.instruments.sans.sns_command_interface import *
    from reduction.instruments.sans.sns_reduction_steps import LoadRun
    from reduction.instruments.sans.sns_reducer import EqSansReducer
    from reduction.instruments.sans.sns_instrument import EQSANS
    HAS_MANTID = True
except:
    HAS_MANTID = False  
    
# Check whether we have numpy
try:
    import numpy
    HAS_NUMPY = True
except:
    HAS_NUMPY = False    
    
class InstrumentDescription(BaseScriptElement):
    instrument_name = "EQSANS"
    
    # Data file
    data_file = ''
    
    # Flag to perform the solid angle correction
    solid_angle_corr = True
    # Flag to perform sensitivity correction
    sensitivity_corr = False
    # Data file to be used to calculate sensitivity
    sensitivity_data = ''
    # Minimum allowed relative sensitivity
    min_sensitivity = 0.5
    max_sensitivity = 1.5
    
    # Transmission
    calculate_transmission = True
    transmission = 1.0
    transmission_error = 0.0
    
    # Frame skipping option
    frame_skipping = False
    
    # Sample-detector distance
    sample_distance = 0.0
    
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
        script  = "EQSANS()\n"
                
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
            
        # Transmission correction
        if self.calculate_transmission:
            script += "\n" #TODO: need a command to enable transmission
        else:
            script += "SetTransmission(trans=%g, error=%g)\n" % (self.transmission, self.transmission_error)
            
        # Q binning
        script += "AzimuthalAverage(n_bins=%g, n_subpix=%g, log_binning=%s)\n" % (self.n_q_bins, self.n_sub_pix, str(self.log_binning))        
        
        # Data file
        if len(str(self.data_file).strip())>0:
            parts = os.path.split(str(self.data_file))
            script += "DataPath(\"%s\")\n" % parts[0]
            script += "AppendDataFile(\"%s\")\n" % self.data_file
        else:
            raise RuntimeError, "Trying to generate reduction script without a data file."
        
        # Frame skipping
        if self.frame_skipping:
            script += "FrameSkipping(True)\n"
        else:
            script += "FrameSkipping(False)\n"
        
        # Normalization
        if self.normalization == InstrumentDescription.NORMALIZATION_NONE:
            script += "NoNormalization()\n"
        elif self.normalization == InstrumentDescription.NORMALIZATION_MONITOR:
            script += "TotalChargeNormalization()\n"
        
        return script           
    
    def to_xml(self):
        """
            Create XML from the current data.
        """
        xml  = "<Instrument>\n"
        xml += "  <name>%s</name>\n" % self.instrument_name
        xml += "  <data_file>%s</data_file>\n" % self.data_file

        xml += "  <solid_angle_corr>%s</solid_angle_corr>\n" % str(self.solid_angle_corr)
        xml += "  <sensitivity_corr>%s</sensitivity_corr>\n" % str(self.sensitivity_corr)
        xml += "  <sensitivity_data>%s</sensitivity_data>\n" % self.sensitivity_data
        xml += "  <sensitivity_min>%s</sensitivity_min>\n" % self.min_sensitivity
        xml += "  <sensitivity_max>%s</sensitivity_max>\n" % self.max_sensitivity

        xml += "  <transmission>%s</transmission>\n" % str(self.transmission)
        xml += "  <transmission_error>%s</transmission_error>\n" % str(self.transmission_error)
        xml += "  <calculate_transmission>%s</calculate_transmission>\n" % str(self.calculate_transmission)

        xml += "  <n_q_bins>%g</n_q_bins>\n" % self.n_q_bins
        xml += "  <n_sub_pix>%g</n_sub_pix>\n" % self.n_sub_pix
        xml += "  <log_binning>%s</log_binning>\n" % str(self.log_binning)

        xml += "  <normalization>%d</normalization>\n" % self.normalization
        
        xml += "  <frame_skipping>%s</frame_skipping>\n" % str(self.frame_skipping)
        
        xml += "</Instrument>\n"

        return xml
        
    def from_xml(self, xml_str):
        """
            Read in data from XML
            @param xml_str: text to read the data from
        """       
        dom = xml.dom.minidom.parseString(xml_str)
        instrument_dom = dom.getElementsByTagName("Instrument")[0]
        
        self.data_file = BaseScriptElement.getStringElement(instrument_dom, "data_file")
                
        self.solid_angle_corr = BaseScriptElement.getBoolElement(instrument_dom, "solid_angle_corr",
                                                                 default = InstrumentDescription.solid_angle_corr)
        self.sensitivity_corr = BaseScriptElement.getBoolElement(instrument_dom, "sensitivity_corr",
                                                                 default = InstrumentDescription.sensitivity_corr)
        self.sensitivity_data = BaseScriptElement.getStringElement(instrument_dom, "sensitivity_data")
        self.min_sensitivity = BaseScriptElement.getFloatElement(instrument_dom, "sensitivity_min",
                                                            default=InstrumentDescription.min_sensitivity)
        self.max_sensitivity = BaseScriptElement.getFloatElement(instrument_dom, "sensitivity_max",
                                                            default=InstrumentDescription.max_sensitivity)

        self.transmission = BaseScriptElement.getFloatElement(instrument_dom, "transmission",
                                                              default=InstrumentDescription.transmission)
        self.transmission_error = BaseScriptElement.getFloatElement(instrument_dom, "transmission_error",
                                                                    default=InstrumentDescription.transmission_error)
        self.calculate_transmission = BaseScriptElement.getBoolElement(instrument_dom, "calculate_transmission",
                                                              default = InstrumentDescription.calculate_transmission)
        
        self.n_q_bins = BaseScriptElement.getIntElement(instrument_dom, "n_q_bins",
                                                       default=InstrumentDescription.n_q_bins)
        self.n_sub_pix = BaseScriptElement.getIntElement(instrument_dom, "n_sub_pix",
                                                       default=InstrumentDescription.n_sub_pix)
        self.log_binning = BaseScriptElement.getBoolElement(instrument_dom, "log_binning",
                                                              default = InstrumentDescription.log_binning)
        
        self.normalization = BaseScriptElement.getIntElement(instrument_dom, "normalization",
                                                             default=InstrumentDescription.normalization)
        
        self.frame_skipping = BaseScriptElement.getBoolElement(instrument_dom, "frame_skipping",
                                                              default = InstrumentDescription.frame_skipping)
        

    def reset(self):
        """
            Reset state
        """
        self.name = ''
        
        self.data_file = ''
        
        self.solid_angle_corr = InstrumentDescription.solid_angle_corr
        self.sensitivity_corr = InstrumentDescription.sensitivity_corr
        self.sensitivity_data = ''
        self.min_sensitivity = InstrumentDescription.min_sensitivity
        self.max_sensitivity = InstrumentDescription.max_sensitivity
        
        self.transmission = InstrumentDescription.transmission
        self.transmission_error = InstrumentDescription.transmission_error
        self.calculate_transmission = InstrumentDescription.calculate_transmission
        
        self.n_q_bins = InstrumentDescription.n_q_bins
        self.n_sub_pix = InstrumentDescription.n_sub_pix
        self.log_binning = InstrumentDescription.log_binning
        
        self.normalization = InstrumentDescription.normalization
        
        self.frame_skipping = InstrumentDescription.frame_skipping
        
        self.sample_distance = 0.0
        
    def update(self):
        """
            Update UI from reduction output
        """
        if HAS_MANTID:
            if ReductionSingleton()._transmission_calculator is not None:
                trans = ReductionSingleton()._transmission_calculator.get_transmission()
                self.transmission = trans[0]
                self.transmission_error = trans[1]
            self.sample_distance = mtd[ReductionSingleton()._data_files.keys()[0]].getRun().getProperty("sample_detector_distance").value
        
        
class DataFileProxy(object):
    sample_detector_distance = None
    data = None
    
    ## Error log
    errors = []
    
    def __init__(self, data_file):
        if HAS_MANTID:
            try:
                reducer = EqSansReducer()
                reducer.set_instrument(EQSANS())
                loader = LoadRun(str(data_file))
                loader.execute(reducer, "raw_data_file")
                self.sample_detector_distance = mtd["raw_data_file"].getRun().getProperty("sample_detector_distance").value
                
                if HAS_NUMPY:
                    raw_data = numpy.zeros(reducer.instrument.nx_pixels*reducer.instrument.ny_pixels)
                    for i in range(reducer.instrument.nx_pixels*reducer.instrument.ny_pixels):
                        raw_data[i-reducer.instrument.nMonitors] = mtd["raw_data_file"].readY(i)[0]
                        
                    self.data = numpy.reshape(raw_data, (reducer.instrument.nx_pixels, reducer.instrument.ny_pixels), order='F')
            except:
                self.errors.append("Error loading data file:\n%s" % sys.exc_value)
            
            
        