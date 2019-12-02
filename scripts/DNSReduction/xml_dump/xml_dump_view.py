# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
"""
DNS XML dump view
"""
from __future__ import (absolute_import, division, print_function)
from os.path import expanduser

from qtpy.QtWidgets import QFileDialog

from DNSReduction.data_structures.dns_view import DNSView


class DNSXMLDump_view(DNSView):
    """
        Widget that lets user save or load data reduction xml files
    """
    ## Widget name
    name = "Paths"

    def __init__(self, parent):
        super(DNSXMLDump_view, self).__init__(parent)
        self.name = 'xml_dump'
        self.xml_filepath = None
        self.main_view = parent
        self.has_tab = False
        self.setVisible(False)

    def get_load_filename(self, startpath=expanduser("~")):
        """
        Open a file dialog to load xml file
        """
        xml_filepath = QFileDialog.getOpenFileName(
            parent=self.main_view,
            caption="Select XML file for loading",
            directory=startpath,
            filter="XML files (*.xml)",
            options=QFileDialog.DontUseNativeDialog)
        if xml_filepath:
            xml_filepath = xml_filepath[0]
        self.xml_filepath = xml_filepath
        return self.xml_filepath

    def get_save_filename(self, startpath=expanduser("~")):
        """
        Open a file dialog save xml file
        """
        xml_filepath = QFileDialog.getSaveFileName(
            parent=self.main_view,
            caption="Select XML file for saving",
            directory=startpath,
            filter="XML files (*.xml)",
            options=QFileDialog.DontUseNativeDialog)
        if xml_filepath:
            xml_filepath = xml_filepath[0]
        if not xml_filepath.lower().endswith('.xml'):
            xml_filepath = ''.join([xml_filepath, '.xml'])
        self.xml_filepath = xml_filepath
        return self.xml_filepath

    def get_xml_filepath(self):
        return self.xml_filepath[0]
