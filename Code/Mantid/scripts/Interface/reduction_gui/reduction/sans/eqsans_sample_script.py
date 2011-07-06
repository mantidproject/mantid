"""
    Sample data options for EQSANS reduction
"""
import xml.dom.minidom
from reduction_gui.reduction.scripter import BaseScriptElement
from reduction_gui.reduction.sans.hfir_sample_script import SampleData as BaseSampleData

class SampleData(BaseSampleData):

    class BeamHole(BaseScriptElement):        
        def to_script(self):
            """
                Generate reduction script
            """
            return "BeamStopTransmission()\n"
            
        def to_xml(self):
            """
                Create XML from the current data.
            """
            return "  <BeamHole/>\n"

        def find(self, dom):
            """
                Return True if we find a tag belonging 
                to this class
            """
            element_list = dom.getElementsByTagName("BeamHole")
            return len(element_list)>0
        
    calculation_method = BaseSampleData.DirectBeam()
    # Option list
    option_list = [BaseSampleData.DirectBeam]
    # Option to fit the two frame separately when in frame-skipping mode
    combine_transmission_frames = True

    def __init__(self):
        super(SampleData, self).__init__()
        self.reset()

    def reset(self):
        """
            Reset state
        """
        super(SampleData, self).reset()
        self.calculation_method = SampleData.calculation_method
        self.combine_transmission_frames = SampleData.combine_transmission_frames
    
    def to_script(self):
        """
            Generate reduction script
            @param execute: if true, the script will be executed
        """
        script = super(SampleData, self).to_script()
        if self.calculate_transmission:
            script += "CombineTransmissionFits(%s)\n" % self.combine_transmission_frames
        return script
            
    def to_xml(self):
        """
            Create XML from the current data.
        """
        xml_str = super(SampleData, self).to_xml()
        return BaseScriptElement.addElementToSection(xml_str, "Transmission", "combine_transmission_frames", 
                                                     str(self.combine_transmission_frames))
    
    def from_xml(self, xml_str):
        """
            Read in data from XML
            @param xml_str: text to read the data from
        """    
        self.reset()   
        super(SampleData, self).from_xml(xml_str)
        
        dom = xml.dom.minidom.parseString(xml_str)
        element_list = dom.getElementsByTagName("Transmission")
        if len(element_list)>0:
            instrument_dom = element_list[0]      
            self.combine_transmission_frames = BaseScriptElement.getBoolElement(dom, "combine_transmission_frames",
                                                                                default = SampleData.combine_transmission_frames)

    