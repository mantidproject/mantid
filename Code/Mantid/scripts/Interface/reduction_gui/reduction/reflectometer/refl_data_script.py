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
    #from reduction.instruments.sans.hfir_command_interface import *
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

class DataSets(BaseScriptElement):

    DataPeakSelectionType = 'narrow'
    DataPeakPixels = ['','']
    DataPeakDiscreteSelection = 'N/A'
    DataBackgroundFlag = False
    DataBackgroundRoi = ['','','','']
    DataTofRange = ['','']

    # Data file
    data_files = []

    def __init__(self):
        super(DataSets, self).__init__()
        self.reset()

    def to_script(self):
        """
            Generate reduction script
            @param execute: if true, the script will be executed
        """
        script = ""

#        # Sample thickness
#        script += "DivideByThickness(%g)\n" % self.sample_thickness
#        
#        if not self.calculate_transmission:
#            script += "SetTransmission(%g, %g)\n" % (self.transmission, self.transmission_spread)
#        else:
#            script += str(self.calculation_method)
#            
#        script += "ThetaDependentTransmission(%s)\n" % str(self.theta_dependent)
#        if self.dark_current is not None and len(str(self.dark_current))>0:
#            script += "TransmissionDarkCurrent(\"%s\")\n" % str(self.dark_current)
#        
#        # Data files
#        if len(self.data_files)>0:
#            parts = os.path.split(str(self.data_files[0]).strip())
#            if len(parts[0])>0:
#                script += "DataPath(\"%s\")\n" % parts[0]
#            else:
#                script += "#Note: Data path was not found at script generation, will try at run time.\n"
#            script += "AppendDataFile([\"%s\"])\n" % '\",\"'.join(self.data_files)
#        else:
#            raise RuntimeError, "Trying to generate reduction script without a data file."

        return script

    def update(self):
        """
            Update transmission from reduction output
        """
        pass
#        if HAS_MANTID and ReductionSingleton()._transmission_calculator is not None:
#            trans = ReductionSingleton()._transmission_calculator.get_transmission()
#            self.transmission = trans[0]
#            self.transmission_spread = trans[1]
           

    def to_xml(self):
#        """
#            Create XML from the current data.
#        """
        xml  = "<Data>\n"
        xml += "<peak_selection_type>%s</peak_selection_type>\n" % self.DataPeakSelectionType
        xml += "<from_peak_pixels>%s</from_peak_pixels>\n" % self.DataPeakPixels[0]
        xml += "<to_peak_pixels>%s</to_peak_pixels>\n" % self.DataPeakPixels[1]
        xml += "<peak_discrete_selection>%s</peak_discrete_selection>\n" % self.DataPeakDiscreteSelection
        xml += "<background_flag>%s</background_flag>\n" % str(self.DataBackgroundFlag)
        xml += "<back_roi1_from>%s</back_roi1_from>\n" % self.DataBackgroundRoi[0]
        xml += "<back_roi1_to>%s</back_roi1_to>\n" % self.DataBackgroundRoi[1]
        xml += "<back_roi2_from>%s</back_roi2_from>\n" % self.DataBackgroundRoi[2]
        xml += "<back_roi2_to>%s</back_roi2_to>\n" % self.DataBackgroundRoi[3]
        xml += "<from_tof_range>%s</from_tof_range>\n" % self.DataTofRange[0]
        xml += "<to_tof_range>%s</to_tof_range>\n" % self.DataTofRange[1]
        xml += "</Data>\n"
        return xml

    def from_xml(self, xml_str):
        """
            Read in data from XML
            @param xml_str: text to read the data from
        """   

        self.reset()    
        dom = xml.dom.minidom.parseString(xml_str)
        
#        # Get Mantid version
#        mtd_version = BaseScriptElement.getMantidBuildVersion(dom)

        element_list = dom.getElementsByTagName("Data")
        if len(element_list)>0:
            instrument_dom = element_list[0]
            
            #Peak selection
            self.DataPeakSelectionType = BaseScriptElement.getStringElement(instrument_dom, "peak_selection_type")
            
            #Peak from/to pixels
            self.DataPeakPixels[0] = BaseScriptElement.getIntElement(instrument_dom, "from_peak_pixels")
            self.DataPeakPixels[1] = BaseScriptElement.getIntElement(instrument_dom, "to_peak_pixels")
            
            #discrete selection string
            self.DataBackgroundFlag = BaseScriptElement.getStringElement(instrument_dom, "peak_discrete_selection")
            
            #background flag
            self.DataBackgroundFlag = BaseScriptElement.getBoolElement(instrument_dom, "background_flag")

            #background from/to pixels
            self.DataBackgroundRoi[0] = BaseScriptElement.getStringElement(instrument_dom, "back_roi1_from")
            self.DataBackgroundRoi[1] = BaseScriptElement.getStringElement(instrument_dom, "back_roi1_to")
            self.DataBackgroundRoi[2] = BaseScriptElement.getStringElement(instrument_dom, "back_roi2_from")
            self.DataBackgroundRoi[3] = BaseScriptElement.getStringElement(instrument_dom, "back_roi2_to")

            #from TOF and to TOF
            self.DataTofRange[0] = BaseScriptElement.getStringElement(instrument_dom, "from_tof_range")
            self.DataTofRange[1] = BaseScriptElement.getStringElement(instrument_dom, "to_tof_range")

    def reset(self):
        """
            Reset state
        """
        pass
#        self.transmission = SampleData.transmission      
#        self.transmission_spread = SampleData.transmission_spread  
#        self.calculate_transmission = SampleData.calculate_transmission
#        self.calculation_method = SampleData.calculation_method
#        self.theta_dependent = SampleData.theta_dependent
#        self.dark_current = SampleData.dark_current
#        self.sample_thickness = SampleData.sample_thickness
#        self.data_files = []
#    

    