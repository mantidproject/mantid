# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
"""
Classes for each reduction step. Those are kept separately
from the interface class so that the HFIRReduction class could
be used independently of the interface implementation
"""

import xml.dom.minidom
from reduction_gui.reduction.scripter import BaseScriptElement
from reduction_gui.reduction.reflectometer.refl_data_script import DataSets as REFLDataSets


class DataSeries(BaseScriptElement):
    data_sets = []

    def __init__(self, data_class=REFLDataSets):
        super(DataSeries, self).__init__()
        self._data_class = data_class
        self.reset()

    def to_script(self):
        """
        Generate reduction script
        @param execute: if true, the script will be executed
        """
        raise RuntimeError("refl_data_series.DataSeries.to_script is deprecated")

    def update(self):
        """
        Update transmission from reduction output
        """
        pass

    def to_xml(self):
        """
        Create XML from the current data.
        """
        _xml = "<DataSeries>\n"
        for item in self.data_sets:
            _xml += item.to_xml()
        _xml += "</DataSeries>\n"

        return _xml

    def from_xml(self, xml_str):
        """
        Read in data from XML
        @param xml_str: text to read the data from
        """
        self.reset()
        self.data_sets = []
        dom = xml.dom.minidom.parseString(xml_str)

        #        # Get Mantid version
        #        mtd_version = BaseScriptElement.getMantidBuildVersion(dom)

        self._data_class = REFLDataSets
        element_list = dom.getElementsByTagName("Data")
        if len(element_list) == 0:
            element_list = dom.getElementsByTagName("RefLData")

        if len(element_list) > 0:
            for item in element_list:
                if item is not None:
                    data_set = self._data_class()
                    data_set.from_xml_element(item)
                    self.data_sets.append(data_set)

        if len(self.data_sets) == 0:
            self.data_sets = [self._data_class()]

    def reset(self):
        """
        Reset state
        """
        self.data_sets = [self._data_class()]
