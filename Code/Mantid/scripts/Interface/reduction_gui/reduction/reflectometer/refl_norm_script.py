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
    import mantidplotpy
    IS_IN_MANTIDPLOT = True
except:
    pass

class NormSets(BaseScriptElement):

    NormPeakPixels = ['','']
    NormBackgroundFlag = False
    NormBackgroundRoi = ['','']

    # Data file
    norm_files = []

    def __init__(self):
        super(NormSets, self).__init__()
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
        xml  = "<Normalization>\n"
        xml += "<from_peak_pixels>%s</from_peak_pixels>\n" % self.NormPeakPixels[0]
        xml += "<to_peak_pixels>%s</to_peak_pixels>\n" % self.NormPeakPixels[1]
        xml += "<background_flag>%s</background_flag>\n" % self.NormBackgroundFlag
        xml += "<from_back_pixels>%s</from_back_pixels>\n" % self.NormBackgroundRoi[0]
        xml += "<to_back_pixels>%s</to_back_pixels>\n" % self.NormBackgroundRoi[1]
        xml += "</Normalization>\n"
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

        element_list = dom.getElementsByTagName("Normalization")
        if len(element_list)>0:
            instrument_dom = element_list[0]

            #Peak from/to pixels
            self.NormPeakPixels[0] = BaseScriptElement.getStringElement(instrument_dom, "from_peak_pixels")
            self.NormPeakPixels[1] = BaseScriptElement.getStringElement(instrument_dom, "to_peak_pixels")

            #background flag
            self.NormBackgroundFlag = BaseScriptElement.getBoolElement(instrument_dom, "background_flag")
            
            #background from/to pixels
            self.NormBackgroundRoi[0] = BaseScriptElement.getStringElement(instrument_dom, "from_back_pixels")
            self.NormBackgroundRoi[1] = BaseScriptElement.getStringElement(instrument_dom, "to_back_pixels")

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

    