# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
"""
helper module for dns xml data
"""
from __future__ import (absolute_import, division, print_function)

from collections import OrderedDict
import sys
import time
import platform

import xml.etree.ElementTree as etree
from xml.dom import minidom

from mantidqt.gui_helper import get_qapplication

#### fancy stuff to get mantid versions and workbench or mantidplot
try:
    from mantid.kernel import version_str
    mantid_version = version_str()
except (ImportError, ImportWarning):
    mantid_version = None

SCRIPT_EXEC_METHOD = 'exec'
try: ## we need import to check if we are in mantidplot
    import mantidplot  # noqa: F401
    SCRIPT_EXEC_METHOD = 'mantidplot'
except (ImportError, ImportWarning):
    # are we running workbench
    if 'workbench' in sys.modules:
        SCRIPT_EXEC_METHOD = 'async'

app, within_mantid = get_qapplication()

## header is written to xml files at creation,
## it is also read and files are checked for instrument name = DNS
## will not be updated during program execution
xml_header = OrderedDict([
    ('instrument_name', 'DNS'),  ## hardcoded if we are not in mantid
    ('facility_name', 'MLZ'),
    ('timestamp_xml_save', time.ctime()),
    ('python_version', sys.version),
    ('platform', platform.system()),
    ('architecture', str(platform.architecture())),
    ('script_exec_method', SCRIPT_EXEC_METHOD),
    ('mantid_version', mantid_version),
    ('within_mantid', within_mantid),
])


def convert_type(value, mytype):
    """
    Return a variable of the type described by string :mytype from the string :value
    """
    if mytype == 'bool':
        return value == 'True'
    if mytype == 'int':
        return int(value)
    if mytype == 'float':
        return float(value)
    if mytype.endswith('list'):
        return [
            convert_type(x, mytype=mytype.split('list')[0])
            for x in value.strip('[]').split(',')
        ]
    if mytype == 'emptylist':
        return []
    if mytype == 'None':
        return None
    return value


def dict_to_xml(dictionary, node=None):
    """
    Return an xml element for a given dictionary
    """
    if node is None:
        node = etree.Element('document')
        #dictionary = dictionary['document']
    for key, value in dictionary.items():
        if isinstance(value, dict):
            sub = etree.SubElement(parent=node, tag=key)
            dict_to_xml(value, node=sub)
        else:
            sub = etree.SubElement(parent=node, tag=key)
            sub.text = str(value)
            sub.set('type', return_type(value))
    return node


def dic_to_xml_file(param_dict, filename):
    """
    Write :dictionary to a xml file :filename
    dictionary can contain bool, None, str, int, float and list of them,
    or dicionaries, other types are converted to str and not converted back
    if you try to read them
    """
    dictionary = OrderedDict()
    dictionary['xml_header'] = xml_header
    dictionary.update(param_dict)
    xmlstr = etree.tostring(dict_to_xml(dictionary))
    xmlstr = minidom.parseString(xmlstr).toprettyxml(indent="  ")
    if filename:
        try:
            with open(filename, "w") as f:
                f.write(xmlstr)
        except IOError:
            print('Error writing file')


def return_type(value):
    """
    Return a string describing the type of :value
    """
    if isinstance(value,
                  bool):  ## bool is subtype of int so check first for bool
        return 'bool'
    if isinstance(value, int):
        return 'int'
    if isinstance(value, float):
        return 'float'
    if isinstance(value, list):
        if value:
            return ''.join([return_type(value[0]), 'list'])
        return 'emptylist'
    if value is None:
        return 'None'
    return 'str'


def xml_file_to_dict(filename):
    """
    Return a dictionary from a given :filename
    works only with structures written by dic_to_xml_file
    """
    if filename:
        try:
            tree = etree.parse(filename)
        except IOError:
            print('Error reading file')
            return None
        instrument_name = tree.find('.//instrument_name').text
        if instrument_name == 'DNS':
            dictionary = xml_to_dict(tree.getroot(), {}).get('document', {})
            return dictionary
    return None


def xml_to_dict(elm, dictionary):
    """
    Return a dictionary for given xml tree element,
    """
    children = list(elm)
    if children:
        new_dict = OrderedDict()
        dictionary.update({elm.tag: new_dict})
        for child in children:
            xml_to_dict(child, new_dict)
    else:
        dictionary.update(
            {elm.tag: convert_type(elm.text, elm.get('type', 'str'))})
    return dictionary
