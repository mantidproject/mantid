# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

"""
Common model for DNS script generators.
"""

from mantidqtinterfaces.dns_powder_tof.data_structures.dns_error import DNSError
from mantidqtinterfaces.dns_powder_tof.data_structures.dns_obs_model import DNSObsModel
from mantidqtinterfaces.dns_powder_tof.helpers.file_processing import create_dir, save_txt


class DNSScriptGeneratorModel(DNSObsModel):
    """
    Common model for DNS script generators.
    """

    def __init__(self, parent):
        super().__init__(parent)
        self.data = None
        self._update_progress = parent.update_progress
        self._script = []
        self._progress_is_canceled = False

    def _add_lines_to_script(self, lines):
        self._script.extend(lines)

    def run_script(self, script):
        if not script:
            return ""
        self._progress_is_canceled = False
        for i, line in enumerate(script):
            try:
                exec(line)  # pylint: disable=exec-used
            except DNSError as error_message:
                return str(error_message)
            self._update_progress(i)
            if self._progress_is_canceled:
                return "Warning script execution stopped, no valid data."
        return ""

    def cancel_progress(self):
        self._progress_is_canceled = True

    def script_maker(self, _options, _paths, _file_selector):
        """
        Generation of a Mantid script to reduce DNS data.
        """
        self._script = [""]
        error = ""
        return self._script, error

    # helper functions:
    @staticmethod
    def get_auto_script_name(full_data):
        sample_name = full_data[0]["sample_name"].strip("_.")
        file_numbers = [x["file_number"] for x in full_data]
        return f"script_{sample_name}_from_{min(file_numbers)}_to_{max(file_numbers)}.py"

    def get_filename(self, filename, full_data, auto=False):
        if auto:
            return self.get_auto_script_name(full_data)
        if filename:
            if not filename.endswith(".py"):
                filename = "".join((filename, ".py"))
        else:
            filename = "script.py"
        return filename

    @staticmethod
    def save_script(script, filename, script_dir):
        create_dir(script_dir)
        return save_txt(script, filename, script_dir)
