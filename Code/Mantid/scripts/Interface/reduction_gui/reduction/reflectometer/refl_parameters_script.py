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

class ParametersSets(BaseScriptElement):

    AutomaticQRangeFlag = True
    Qrange = ['','']
    NbrOfQBinsFlag = False
    NbrOfBins_BinSize = ['','']
    BinSize_NbrOfBins = ['','']
    LinearBinningFlag = True
    OutputFolder = ''

    def __init__(self):
        super(ParametersSets, self).__init__()
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
        xml  = "<Parameters>\n"
        xml += "<auto_q_range_flag>%s</auto_q_range_flag>\n" % str(self.AutomaticQRangeFlag)
        xml += "<from_q>%s</from_q>\n" % str(self.Qrange[0])
        xml += "<to_q>%s</to_q>\n" % str(self.Qrange[1])
        xml += "<nbr_bins_flag>%s</nbr_bins_flag>\n" % str(self.NbrOfQBinsFlag)
        xml += "<nbr_q_mode_nbr_bins>%s</nbr_q_mode_nbr_bins>\n" % str(self.NbrOfBins_BinSize[0])
        xml += "<nbr_q_mode_bin_size>%s</nbr_q_mode_bin_size>\n" % str(self.NbrOfBins_BinSize[1])
        xml += "<q_bin_mode_bin_size>%s</q_bin_mode_bin_size>\n" % str(self.BinSize_NbrOfBins[0])
        xml += "<q_bin_mode_nbr_bin>%s</q_bin_mode_nbr_bin>\n" % str(self.BinSize_NbrOfBins[1])
        xml += "<linear_binning_flag>%s</linear_binning_flag>\n" % str(self.LinearBinningFlag)
        xml += "<output_folder>%s</output_folder>\n" % str(self.OutputFolder)
        xml += "</Parameters>\n"
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

        element_list = dom.getElementsByTagName("Parameters")
        if len(element_list)>0:
            instrument_dom = element_list[0]
            
            #automatic Q range flag
            self.AutomaticQRangeFlag = BaseScriptElement.getBoolElement(instrument_dom, "auto_q_range_flag")
            
            #from/to Q values
            self.Qrange[0] = BaseScriptElement.getStringElement(instrument_dom, "from_q")
            self.Qrange[1] = BaseScriptElement.getStringElement(instrument_dom, "to_q")
            
            #Number of Q bins flag and fields
            self.NbrOfQBinsFlag = BaseScriptElement.getBoolElement(instrument_dom, "nbr_bins_flag")
            self.NbrOfBins_BinSize[0] = BaseScriptElement.getStringElement(instrument_dom, "nbr_q_mode_nbr_bins")
            self.NbrOfBins_BinSize[1] = BaseScriptElement.getStringElement(instrument_dom, "nbr_q_mode_bin_size")
            
            #Q bin size
            self.BinSize_NbrOfBins[0] = BaseScriptElement.getStringElement(instrument_dom, "q_bin_mode_bin_size")
            self.BinSize_NbrOfBins[1] = BaseScriptElement.getStringElement(instrument_dom, "q_bin_mode_nbr_bin")
            
            #linear/log binning
            self.LinearBinningFlag = BaseScriptElement.getBoolElement(instrument_dom, "linear_binning_flag")
            
            #Output directory
            self.OutputFolder = BaseScriptElement.getStringElement(instrument_dom, "output_folder")

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

    