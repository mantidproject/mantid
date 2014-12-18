"""
    Sample data options for EQSANS reduction
"""
import xml.dom.minidom
from reduction_gui.reduction.scripter import BaseScriptElement
from reduction_gui.reduction.sans.hfir_sample_script import SampleData as BaseSampleData

class SampleData(BaseSampleData):

    calculation_method = BaseSampleData.DirectBeam()
    # Option list
    option_list = [BaseSampleData.DirectBeam]
    # Option to fit the two frame separately when in frame-skipping mode
    combine_transmission_frames = False
    calculate_transmission = True

    def __init__(self):
        super(SampleData, self).__init__()
        self.reset()

    def reset(self):
        """
            Reset state
        """
        super(SampleData, self).reset()
        self.calculation_method.reset()
        self.combine_transmission_frames = SampleData.combine_transmission_frames
        self.calculate_transmission = SampleData.calculate_transmission

    def to_script(self, data_file=None):
        """
            Generate reduction script
            @param execute: if true, the script will be executed
        """
        script = super(SampleData, self).to_script(data_file)
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
            self.combine_transmission_frames = BaseScriptElement.getBoolElement(instrument_dom, "combine_transmission_frames",
                                                                                default = SampleData.combine_transmission_frames)

    def from_setup_info(self, xml_str):
        """
            Read in data from XML using the string representation of the setup algorithm used
            to prepare the reduction properties.
            @param xml_str: text to read the data from
        """
        self.reset()
        super(SampleData, self).from_setup_info(xml_str)

        from mantid.api import Algorithm
        dom = xml.dom.minidom.parseString(xml_str)

        process_dom = dom.getElementsByTagName("SASProcess")[0]
        setup_alg_str = BaseScriptElement.getStringElement(process_dom, 'SetupInfo')
        alg=Algorithm.fromString(str(setup_alg_str))
        self.combine_transmission_frames = BaseScriptElement.getPropertyValue(alg, "FitFramesTogether",
                                                                              default=SampleData.combine_transmission_frames)
