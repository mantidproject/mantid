# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantid import FileFinder
from mantidqtinterfaces.sans_isis.gui_logic.models.run_file import SummableRunFile


class SummableRunFinder(object):
    def __init__(self, file_info_source):
        self._file_info_source = file_info_source

    def find_all_from_query(self, query_string):
        try:
            results = FileFinder.findRuns(query_string)
            return ("", [self.find_from_file_path(file_path) for file_path in results])
        except RuntimeError:
            return ("", [])
        except ValueError as ex:
            return (str(ex), [])

    def find_from_file_path(self, file_path):
        file = self._file_info_for_path(file_path)
        return SummableRunFile(file_path, self._display_name(file), self._is_event_mode(file))

    def _file_info_for_path(self, file_path):
        return self._file_info_source.create_sans_file_information(file_path)

    def _display_name(self, file):
        return str(file.get_run_number())

    def _is_event_mode(self, file):
        return file.is_event_mode()
