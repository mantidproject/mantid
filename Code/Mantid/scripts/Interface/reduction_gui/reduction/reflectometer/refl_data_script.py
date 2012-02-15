"""
    Classes for each reduction step. Those are kept separately 
    from the the interface class so that the HFIRReduction class could 
    be used independently of the interface implementation
"""
import xml.dom.minidom
import os
import time
from reduction_gui.reduction.scripter import BaseScriptElement

class DataSets(BaseScriptElement):

    DataPeakSelectionType = 'narrow'
    DataPeakPixels = [120, 130]
    DataPeakDiscreteSelection = 'N/A'
    DataBackgroundFlag = False
    DataBackgroundRoi = [115, 137,123, 137]
    DataTofRange = [9600., 21600.]
    
    data_x_range_flag = True
    data_x_range = [115,210]
    
    norm_x_range_flag = True
    norm_x_range = [115,210]

    NormFlag = True
    NormPeakPixels = [120, 130]
    NormBackgroundFlag = False
    NormBackgroundRoi = [115, 137]

    # Data files
    #data_files = [66421]
    #norm_file = 66196
    data_files = [0]
    norm_file = 0
    
    # Q range
    q_min = 0.001
    q_step = 0.001
    auto_q_binning = False
    
    # Angle offset
    angle_offset = 0.0
    angle_offset_error = 0.0

    def __init__(self):
        super(DataSets, self).__init__()
        self.reset()

    def to_script(self, for_automated_reduction=False):
        """
            Generate reduction script
            @param execute: if true, the script will be executed
        """

        script =  "RefLReduction(RunNumbers=[%s],\n" % ','.join([str(i) for i in self.data_files])
        script += "              NormalizationRunNumber=%d,\n" % self.norm_file
        script += "              SignalPeakPixelRange=%s,\n" % str(self.DataPeakPixels)
        script += "              SubtractSignalBackground=%s,\n" % str(self.DataBackgroundFlag)
        script += "              SignalBackgroundPixelRange=%s,\n" % str(self.DataBackgroundRoi[:2])
        script += "              NormFlag=%s,\n" % str(self.NormFlag)
        script += "              NormPeakPixelRange=%s,\n" % str(self.NormPeakPixels)
        script += "              NormBackgroundPixelRange=%s,\n" % str(self.NormBackgroundRoi)
        script += "              SubtractNormBackground=%s,\n" % str(self.NormBackgroundFlag)
        script += "              LowResDataAxisPixelRangeFlag=%s,\n" % str(self.data_x_range_flag)
        script += "              LowResDataAxisPixelRange=%s,\n" % str(self.data_x_range)
        script += "              LowResNormAxisPixelRangeFlag=%s,\n" % str(self.norm_x_range_flag)
        script += "              LowResNormAxisPixelRange=%s,\n" % str(self.norm_x_range)
        script += "              TOFRange=%s,\n" % str(self.DataTofRange)
        script += "              QMin=%s,\n" % str(self.q_min)
        script += "              QStep=%s,\n" % str(self.q_step)

        # Angle offset
        if self.angle_offset != 0.0:
            script += "              AngleOffset=%s,\n" % str(self.angle_offset)
            script += "              AngleOffsetError=%s,\n" % str(self.angle_offset_error)
            
        # The output should be slightly different if we are generating
        # a script for the automated reduction
        if for_automated_reduction:
            script += "              OutputWorkspace='reflectivity_'+%s)" % str(self.data_files[0])
        else:
            script += "              OutputWorkspace='reflectivity_%s')" % str(self.data_files[0])
        script += "\n"

        return script

    def update(self):
        """
            Update transmission from reduction output
        """
        pass

    def to_xml(self):
        """
            Create XML from the current data.
        """
        xml  = "<RefLData>\n"
        xml += "<peak_selection_type>%s</peak_selection_type>\n" % self.DataPeakSelectionType
        xml += "<from_peak_pixels>%s</from_peak_pixels>\n" % str(self.DataPeakPixels[0])
        xml += "<to_peak_pixels>%s</to_peak_pixels>\n" % str(self.DataPeakPixels[1])
        xml += "<peak_discrete_selection>%s</peak_discrete_selection>\n" % self.DataPeakDiscreteSelection
        xml += "<background_flag>%s</background_flag>\n" % str(self.DataBackgroundFlag)
        xml += "<back_roi1_from>%s</back_roi1_from>\n" % str(self.DataBackgroundRoi[0])
        xml += "<back_roi1_to>%s</back_roi1_to>\n" % str(self.DataBackgroundRoi[1])
        xml += "<back_roi2_from>%s</back_roi2_from>\n" % str(self.DataBackgroundRoi[2])
        xml += "<back_roi2_to>%s</back_roi2_to>\n" % str(self.DataBackgroundRoi[3])
        xml += "<from_tof_range>%s</from_tof_range>\n" % str(self.DataTofRange[0])
        xml += "<to_tof_range>%s</to_tof_range>\n" % str(self.DataTofRange[1])
        xml += "<data_sets>%s</data_sets>\n" % ','.join([str(i) for i in self.data_files])
        xml += "<x_min_pixel>%s</x_min_pixel>\n" % str(self.data_x_range[0])
        xml += "<x_max_pixel>%s</x_max_pixel>\n" % str(self.data_x_range[1])
        xml += "<x_range_flag>%s</x_range_flag>\n" % str(self.data_x_range_flag)

        xml += "<norm_flag>%s</norm_flag>\n" % str(self.NormFlag)
        xml += "<norm_x_range_flag>%s</norm_x_range_flag>\n" % str(self.norm_x_range_flag)
        xml += "<norm_x_max>%s</norm_x_max>\n" % str(self.norm_x_range[1])
        xml += "<norm_x_min>%s</norm_x_min>\n" % str(self.norm_x_range[0])
        
        xml += "<norm_from_peak_pixels>%s</norm_from_peak_pixels>\n" % str(self.NormPeakPixels[0])
        xml += "<norm_to_peak_pixels>%s</norm_to_peak_pixels>\n" % str(self.NormPeakPixels[1])
        xml += "<norm_background_flag>%s</norm_background_flag>\n" % str(self.NormBackgroundFlag)
        xml += "<norm_from_back_pixels>%s</norm_from_back_pixels>\n" % str(self.NormBackgroundRoi[0])
        xml += "<norm_to_back_pixels>%s</norm_to_back_pixels>\n" % str(self.NormBackgroundRoi[1])
        xml += "<norm_dataset>%s</norm_dataset>\n" % str(self.norm_file)
        
        # Q cut
        xml += "<q_min>%s</q_min>\n" % str(self.q_min)
        xml += "<q_step>%s</q_step>\n" % str(self.q_step)
        xml += "<auto_q_binning>%s</auto_q_binning>" % str(self.auto_q_binning)
        
        # Angle offset
        xml += "<angle_offset>%s</angle_offset>\n" % str(self.angle_offset)
        xml += "<angle_offset_error>%s</angle_offset_error>\n" % str(self.angle_offset_error)
        
        xml += "</RefLData>\n"

        return xml

    def from_xml(self, xml_str):
        self.reset()    
        dom = xml.dom.minidom.parseString(xml_str)
        self.from_xml_element(dom)
        element_list = dom.getElementsByTagName("RefLData")
        if len(element_list)>0:
            instrument_dom = element_list[0]

    def from_xml_element(self, instrument_dom):
        """
            Read in data from XML
            @param xml_str: text to read the data from
        """   
        #Peak selection
        self.DataPeakSelectionType = BaseScriptElement.getStringElement(instrument_dom, "peak_selection_type")
        
        #Peak from/to pixels
        self.DataPeakPixels = [BaseScriptElement.getIntElement(instrument_dom, "from_peak_pixels"),
                               BaseScriptElement.getIntElement(instrument_dom, "to_peak_pixels")]
        
        
        #low resolution range
        self.data_x_range_flag = BaseScriptElement.getBoolElement(instrument_dom, "data_x_range_flag",
                                                                  default=DataSets.data_x_range_flag)
        
        self.data_x_range = [BaseScriptElement.getIntElement(instrument_dom, "x_min_pixel"),
                             BaseScriptElement.getIntElement(instrument_dom, "x_max_pixel")]
        
        self.norm_x_range_flag = BaseScriptElement.getBoolElement(instrument_dom, "norm_x_range_flag",
                                                                  default=DataSets.norm_x_range_flag)

        self.norm_x_range = [BaseScriptElement.getIntElement(instrument_dom, "norm_x_min"),
                             BaseScriptElement.getIntElement(instrument_dom, "norm_x_max")]
        
        #discrete selection string
        self.DataPeakDiscreteSelection = BaseScriptElement.getStringElement(instrument_dom, "peak_discrete_selection")
        
        #background flag
        self.DataBackgroundFlag = BaseScriptElement.getBoolElement(instrument_dom,
                                                                   "background_flag",
                                                                   default=DataSets.DataBackgroundFlag)

        #background from/to pixels
        self.DataBackgroundRoi = [BaseScriptElement.getIntElement(instrument_dom, "back_roi1_from"),
                                  BaseScriptElement.getIntElement(instrument_dom, "back_roi1_to"),
                                  BaseScriptElement.getIntElement(instrument_dom, "back_roi2_from"),
                                  BaseScriptElement.getIntElement(instrument_dom, "back_roi2_to")]

        #from TOF and to TOF
        self.DataTofRange = [BaseScriptElement.getFloatElement(instrument_dom, "from_tof_range"),
                             BaseScriptElement.getFloatElement(instrument_dom, "to_tof_range")]

        self.data_files = BaseScriptElement.getIntList(instrument_dom, "data_sets")
            
        #with or without norm 
        self.NormFlag = BaseScriptElement.getBoolElement(instrument_dom, "norm_flag",
                                                         default=DataSets.NormFlag)
        
        #Peak from/to pixels
        self.NormPeakPixels = [BaseScriptElement.getIntElement(instrument_dom, "norm_from_peak_pixels"),
                               BaseScriptElement.getIntElement(instrument_dom, "norm_to_peak_pixels")]

        #background flag
        self.NormBackgroundFlag = BaseScriptElement.getBoolElement(instrument_dom, 
                                                                   "norm_background_flag", 
                                                                   default=DataSets.NormBackgroundFlag)
        
        #background from/to pixels
        self.NormBackgroundRoi = [BaseScriptElement.getIntElement(instrument_dom, "norm_from_back_pixels"),
                                  BaseScriptElement.getIntElement(instrument_dom, "norm_to_back_pixels")]
        
        self.norm_file = BaseScriptElement.getIntElement(instrument_dom, "norm_dataset")
    
        # Q cut
        self.q_min = BaseScriptElement.getFloatElement(instrument_dom, "q_min", default=DataSets.q_min)    
        self.q_step = BaseScriptElement.getFloatElement(instrument_dom, "q_step", default=DataSets.q_step)
        self.auto_q_binning = BaseScriptElement.getBoolElement(instrument_dom, "auto_q_binning", default=False)
    
        # Angle offset
        self.angle_offset = BaseScriptElement.getFloatElement(instrument_dom, "angle_offset", default=DataSets.angle_offset)
        self.angle_offset_error = BaseScriptElement.getFloatElement(instrument_dom, "angle_offset_error", default=DataSets.angle_offset_error)        
        
    def reset(self):
        """
            Reset state
        """
        self.DataPeakSelectionType = DataSets.DataPeakSelectionType
        self.DataBackgroundFlag = DataSets.DataBackgroundFlag
        self.DataBackgroundRoi = DataSets.DataBackgroundRoi
        self.DataPeakDiscreteSelection = DataSets.DataPeakDiscreteSelection
        self.DataPeakPixels = DataSets.DataPeakPixels
        self.DataTofRange = DataSets.DataTofRange
        self.data_files = DataSets.data_files
        
        self.NormFlag = DataSets.NormFlag
        self.NormBackgroundFlag = DataSets.NormBackgroundFlag
        self.NormBackgroundRoi = DataSets.NormBackgroundRoi
        self.NormPeakPixels = DataSets.NormPeakPixels
        self.norm_file = DataSets.norm_file
        self.data_x_range_flag = DataSets.data_x_range_flag
        self.data_x_range = DataSets.data_x_range
        self.norm_x_range_flag = DataSets.norm_x_range_flag
        self.norm_x_range = DataSets.norm_x_range
        
        # Q range
        self.q_min = DataSets.q_min
        self.q_step = DataSets.q_step
        self.auto_q_binning = DataSets.auto_q_binning
        
        # Angle offset
        self.angle_offset = DataSets.angle_offset
        self.angle_offset_error = DataSets.angle_offset_error
