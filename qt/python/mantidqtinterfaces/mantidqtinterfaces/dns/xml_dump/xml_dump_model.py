# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
"""
 dns xml data dump presenter
"""

import xml.etree.ElementTree as etree
from collections import OrderedDict
from xml.dom import minidom

from mantidqtinterfaces.dns.data_structures.dns_obs_model import \
    DNSObsModel
from mantidqtinterfaces.dns.helpers.file_processing import save_txt


class DNSXMLDumpModel(DNSObsModel):

    def _convert_type(self, value, mytype):
        # pylint: disable=too-many-return-statements
        """
        Return a variable of the type described by
        string :mytype from the string :value
        """
        if mytype == 'bool':
            return value == 'True'
        if mytype == 'int':
            return int(value)
        if mytype == 'float':
            return float(value)
        if mytype == 'emptylist':
            return []
        if mytype.endswith('list'):
            return [
                self._convert_type(x, mytype=mytype.split('list')[0])
                for x in value.strip('[]').split(',')
            ]
        if mytype == 'None':
            return None
        return value

    def _dict_elm_to_xml(self, dictionary, node=None):
        """
        Return an xml element for a given dictionary
        """
        if node is None:
            node = etree.Element('document')
        for key, value in dictionary.items():
            sub = etree.SubElement(node, key)
            if isinstance(value, dict):
                self._dict_elm_to_xml(value, node=sub)
            else:
                sub.text = str(value)
                sub.set('type', self._return_type(value))
        return node

    def dic_to_xml_file(self, param_dict, filename, xml_header):
        """
        Write :dictionary to a xml file :filename
        dictionary can contain bool, None, str, int, float and list of them,
        or dicionaries, other types are converted to str and not converted back
        if you try to read them
        """
        dictionary = OrderedDict()
        dictionary['xml_header'] = xml_header
        dictionary.update(param_dict)
        xmlstr = self._dictionary_to_xml_string(dictionary)
        if filename:
            save_txt(xmlstr, filename)

    def _dictionary_to_xml_string(self, dictionary):
        """returns an xmlstring for a given dicionary, parsed by minidom"""
        xmlstr = etree.tostring(self._dict_elm_to_xml(dictionary))
        xmlstr = minidom.parseString(xmlstr).toprettyxml(indent="  ")
        return xmlstr

    def _return_type(self, value):
        # pylint: disable=too-many-return-statements
        """
        Return a string describing the type of :value
        """
        if isinstance(value, bool):  # bool is subtype of int
            return 'bool'
        if isinstance(value, int):
            return 'int'
        if isinstance(value, float):
            return 'float'
        if isinstance(value, list):
            if value:
                return ''.join([self._return_type(value[0]), 'list'])
            return 'emptylist'
        if value is None:
            return 'None'
        if isinstance(value, str):
            return 'str'
        print(value)
        return 'str'

    @staticmethod
    def _load_file_to_xml_tree(filename):
        tree = None
        if filename:
            try:
                tree = etree.parse(filename)
            except IOError:
                print('Error reading file')
                return None
        return tree

    @staticmethod
    def _check_instrument_name(tree):
        instrument_name = tree.find('.//instrument_name').text
        return instrument_name == 'DNS'

    def xml_file_to_dict(self, filename):
        """
        Return a dictionary from a given :filename
        works only with structures written by dic_to_xml_file
        """
        tree = self._load_file_to_xml_tree(filename)
        if tree and self._check_instrument_name(tree):
            return self._xml_to_dict(tree.getroot(), {}).get('document', {})
        return None

    def _xml_to_dict(self, elm, dictionary):
        """
        Updates and returns the given dictionary with
        values of xml tree element
        """
        children = list(elm)
        if children:
            new_dict = OrderedDict()
            dictionary.update({elm.tag: new_dict})
            for child in children:
                self._xml_to_dict(child, new_dict)
        else:
            dictionary.update({
                elm.tag: self._convert_type(elm.text, elm.get('type', 'str'))
            })
        return dictionary
