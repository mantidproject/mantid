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

class AdvancedSets(BaseScriptElement):
    
    FilteringDataFlag = False
    DtOverTFlag = False
    AutoCleanupFlag = False
    PercentageQToRemove = ''
    
    OverwriteDataGeometryFlag = False
    DataGeometryFileName = ''
    OverwriteNormGeometryFlag = False
    NormGeometryFileName = ''
    
    DataCombinedBckFlag = False
    DataCombinedSpecularFlag = False
    DataCombinedSubtractedFlag = False
    NormCombinedBckFlag = False
    NormCombinedSpecularFlag = False
    NormCombinedSubtractedFlag = False
    RvsTOFFlag = False
    RvsTOFCombinedFlag = False
    RvsQFlag = False
    RvsQRebinningFlag = False

    def __init__(self):
        super(AdvancedSets, self).__init__()
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
        """
            Create XML from the current data.
        """

        xml  = "<Advanced>\n"
        xml += "<filtering_data_flag>%s</filtering_data_flag>\n" % self.FilteringDataFlag
        xml += "<dt_over_t_flag>%s</dt_over_t_flag>\n" % self.DtOverTFlag
        xml += "<auto_cleanup_flag>%s</auto_cleanup_flag>\n" % self.AutoCleanupFlag
        xml += "<percentage_q_to_remove>%s</percentage_q_to_remove>\n" % self.PercentageQToRemove
        xml += "<overwrite_data_geometry_flag>%s</overwrite_data_geometry_flag>\n" % self.OverwriteDataGeometryFlag
        xml += "<data_geometry_file_name>%s</data_geometry_file_name>\n" % self.DataGeometryFileName
        xml += "<overwrite_norm_geometry_flag>%s</overwrite_norm_geometry_flag>\n" % self.OverwriteNormGeometryFlag
        xml += "<data_combined_bck_flag>%s</data_combined_bck_flag>\n" % self.DataCombinedBckFlag
        xml += "<data_combined_specular_flag>%s</data_combined_specular_flag>\n" % self.DataCombinedSpecularFlag
        xml += "<data_combined_subtracted_flag>%s</data_combined_subtracted_flag>\n" % self.DataCombinedSubtractedFlag
        xml += "<norm_combined_bck_flag>%s</norm_combined_bck_flag>\n" % self.NormCombinedBckFlag
        xml += "<norm_combined_specular_flag>%s</norm_combined_specular_flag>\n" % self.NormCombinedSpecularFlag
        xml += "<norm_combined_subtracted_flag>%s</norm_combined_subtracted_flag>\n" % self.NormCombinedSubtractedFlag
        xml += "<r_vs_tof_flag>%s</r_vs_tof_flag>\n" % self.RvsTOFFlag
        xml += "<r_vs_tof_combined_flag>%s</r_vs_tof_combined_flag>\n" % self.RvsTOFCombinedFlag
        xml += "<r_vs_q_flag>%s</r_vs_q_flag>\n" % self.RvsQFlag
        xml += "<r_vs_q_rebinning_flag>%s</r_vs_q_rebinning_flag>\n" % self.RvsQRebinningFlag
        xml += "</Advanced>\n"
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

        element_list = dom.getElementsByTagName("Advanced")
        if len(element_list)>0:
            instrument_dom = element_list[0]
            
            #general
            self.FilteringDataFlag = BaseScriptElement.getBoolElement(instrument_dom, "filtering_data_flag")
            self.DtOverTFlag = BaseScriptElement.getBoolElement(instrument_dom, "dt_over_t_flag")
            self.AutoCleanupFlag = BaseScriptElement.getBoolElement(instrument_dom, "auto_cleanup_flag")
            self.PercentageQToRemove = BaseScriptElement.getStringElement(instrument_dom, "percentage_q_to_remove")
            self.OverwriteDataGeometryFlag = BaseScriptElement.getBoolElement(instrument_dom, "overwrite_data_geometry")
            self.DataGeometryFileName = BaseScriptElement.getStringElement(instrument_dom, "data_geometry_file_name")
            self.OverwriteNormGeometryFlag = BaseScriptElement.getBoolElement(instrument_dom, "overwrite_norm_geometry")
            self.DataCombinedBckFlag = BaseScriptElement.getBoolElement(instrument_dom, "data_combined_bck_flag")
            self.DataCombinedSpecularFlag = BaseScriptElement.getBoolElement(instrument_dom, "data_combined_specular_flag")
            self.DataCombinedSubtractedFlag = BaseScriptElement.getBoolElement(instrument_dom, "data_combined_subtracted_flag")
            self.NormCombinedBckFlag = BaseScriptElement.getBoolElement(instrument_dom, "norm_combined_bck_flag")
            self.NormCombinedSpecularFlag = BaseScriptElement.getBoolElement(instrument_dom, "norm_combined_specular_flag")
            self.NormCombinedSubtractedFlag = BaseScriptElement.getBoolElement(instrument_dom, "norm_combined_subtracted_flag")
            self.RvsTOFFlag = BaseScriptElement.getBoolElement(instrument_dom, "r_vs_tof_flag")
            self.RvsTOFCombinedFlag = BaseScriptElement.getBoolElement(instrument_dom, "r_vs_tof_combined_flag")
            self.RvsQFlag = BaseScriptElement.getBoolElement(instrument_dom, "r_vs_q_flag")
            self.RvsQRebinningFlag = BaseScriptElement.getBoolElement(instrument_dom, "r_vs_q_rebinning_flag")
            
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

    
