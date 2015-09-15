"""
    Data set options for EQSANS reduction
"""
import xml.dom.minidom
from reduction_gui.reduction.scripter import BaseScriptElement
from reduction_gui.reduction.sans.eqsans_sample_script import SampleData as BaseSampleData
from reduction_gui.reduction.sans.eqsans_background_script import Background

class DataSets(BaseSampleData):

    background = Background()

    def __init__(self):
        super(DataSets, self).__init__()
        self.reset()

    def reset(self):
        """
            Reset state
        """
        super(DataSets, self).reset()
        self.background.reset()

    def to_script(self, data_file=None):
        """
            Generate reduction script
            @param execute: if true, the script will be executed
        """
        script_sample = super(DataSets, self).to_script(data_file)
        script_bck = self.background.to_script()
        return "%s\n%s" % (script_sample, script_bck)

    def to_xml(self):
        """
            Create XML from the current data.
        """
        xml_sample = super(DataSets, self).to_xml()
        xml_bck = self.background.to_xml()
        return "%s\n%s" % (xml_sample, xml_bck)

    def from_xml(self, xml_str):
        """
            Read in data from XML
            @param xml_str: text to read the data from
        """
        self.reset()
        super(DataSets, self).from_xml(xml_str)
        self.background.from_xml(xml_str)

    def from_setup_info(self, xml_str):
        """
            Read in data from XML using the string representation of the setup algorithm used
            to prepare the reduction properties.
            @param xml_str: text to read the data from
        """
        self.reset()
        super(DataSets, self).from_setup_info(xml_str)
        self.background.from_setup_info(xml_str)
