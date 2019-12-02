# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
"""
 dns xml data dump presenter
"""

from __future__ import (absolute_import, division, print_function)

from DNSReduction.xml_dump import xml_helper
from DNSReduction.data_structures.dns_observer import DNSObserver
from DNSReduction.xml_dump.xml_dump_view import DNSXMLDump_view


class DNSXMLDump_presenter(DNSObserver):
    name = 'xml'

    def __init__(self, parent):
        super(DNSXMLDump_presenter, self).__init__(parent, 'xml_dump')
        self.name = 'xml_dump'
        self.view = DNSXMLDump_view(self.parent.view)
        self.last_filename = None
        
    def load_xml(self):
        """Load DNS GUI options from xml file """
        xml_file_path = self.view.get_load_filename()
        options = xml_helper.xml_file_to_dict(xml_file_path)
        if options is None:
            print('No DNS xml file')
        return options

    def save_as_xml(self):
        """Save of DNS GUI options to xml file """
        xml_file_path = self.view.get_save_filename()
        if xml_file_path:
            options = self.param_dict
            xml_helper.dic_to_xml_file(options, xml_file_path)
            self.last_filename = xml_file_path
            self.view.show_statusmessage(
                'Saved as {}'.format(xml_file_path) , 30)
        return xml_file_path

    def save_xml(self):
        """Save of DNS GUI options to xml file """
        if  self.last_filename is not None:
            options = self.param_dict
            xml_helper.dic_to_xml_file(options,  self.last_filename)
            self.view.show_statusmessage(
                'Saved as {}'.format(self.last_filename) , 30)
        else:
            self.save_as_xml()

    def set_view_from_param(self):
        pass
