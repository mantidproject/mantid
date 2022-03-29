# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
"""
DNS XML dump view
"""

import platform
import sys
import time
from collections import OrderedDict
from os.path import expanduser
from qtpy.QtWidgets import QFileDialog
from mantidqtinterfaces.dns.data_structures.dns_view import DNSView


# to get mantid version
try:
    from mantid.kernel import version_str
    mantid_version = version_str()
except (ImportError, ImportWarning):
    mantid_version = None


class DNSXMLDumpView(DNSView):
    """
        Widget that lets user save or load data reduction xml files
    """
    # Widget name
    name = "XmlDump"
    HAS_TAB = False

    def __init__(self, parent):
        super().__init__(parent)
        self.name = 'xml_dump'
        self.main_view = parent
        self.setVisible(False)

    def open_load_filename(self, startpath=expanduser("~")):
        """
        Open a file dialog to load xml file
        """
        xml_filepath = QFileDialog.getOpenFileName(
            parent=self.main_view,
            caption="Select XML file for loading",
            directory=startpath,
            filter="XML files (*.xml)",
            options=QFileDialog.DontUseNativeDialog)
        return xml_filepath

    def open_save_filename(self, startpath=expanduser("~")):
        """
        Open a file dialog save xml file
        """
        xml_filepath = QFileDialog.getSaveFileName(
            parent=self.main_view,
            caption="Select XML file for saving",
            directory=startpath,
            filter="XML files (*.xml)",
            options=QFileDialog.DontUseNativeDialog)
        return xml_filepath

    @staticmethod
    def get_file_header():
        return OrderedDict([
            ('instrument_name', 'DNS'),
            ('facility_name', 'MLZ'),
            ('timestamp_xml_save', time.ctime()),
            ('python_version', sys.version),
            ('platform', platform.system()),
            ('architecture', str(platform.architecture())),
            ('mantid_version', mantid_version),
        ])
