# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
"""
 dns xml data dump presenter
"""

from mantidqtinterfaces.dns.data_structures.dns_observer import \
    DNSObserver


class DNSXMLDumpPresenter(DNSObserver):

    def __init__(self, name=None, parent=None, view=None, model=None):
        super().__init__(parent=parent, name=name, view=view, model=model)
        self.last_filename = None

    def load_xml(self):
        """Load DNS GUI options from xml file """
        xml_file_path = self._get_xml_file_path_for_loading()
        options = self.model.xml_file_to_dict(xml_file_path)
        return options

    def save_as_xml(self):
        """Save of DNS GUI options to xml file asking for name"""
        xml_file_path = self._get_xml_file_path_for_saving()
        if xml_file_path:
            xml_header = self.view.get_file_header()
            options = self.param_dict
            self.model.dic_to_xml_file(options, xml_file_path, xml_header)
            self.last_filename = xml_file_path
            self.view.show_statusmessage(f'Saved as {xml_file_path}', 30)
        return xml_file_path

    def save_xml(self):
        """Save of DNS GUI options to xml file """
        if self.last_filename is not None:
            xml_header = self.view.get_file_header()
            options = self.param_dict
            self.model.dic_to_xml_file(options, self.last_filename, xml_header)
            self.view.show_statusmessage(
                f'Saved as {self.last_filename}', 30)
        else:
            self.save_as_xml()

    def set_view_from_param(self):
        pass

    def _get_xml_file_path_for_loading(self):
        xml_filepath = self.view.open_load_filename()
        xml_filepath = xml_filepath[0]
        if xml_filepath and not xml_filepath.lower().endswith('.xml'):
            xml_filepath = ''.join([xml_filepath, '.xml'])
        return xml_filepath

    def _get_xml_file_path_for_saving(self):
        xml_filepath = self.view.open_save_filename()
        xml_filepath = xml_filepath[0]
        if xml_filepath and not xml_filepath.lower().endswith('.xml'):
            xml_filepath = ''.join([xml_filepath, '.xml'])
        return xml_filepath
