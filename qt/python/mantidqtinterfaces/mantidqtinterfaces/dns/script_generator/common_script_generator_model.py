# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
"""
Common Presenter for DNS Script generators
"""

from mantidqtinterfaces.dns.data_structures.dns_error import DNSError
from mantidqtinterfaces.dns.data_structures.dns_obs_model import DNSObsModel
from mantidqtinterfaces.dns.helpers.file_processing import (create_dir,
                                                            save_txt)


class DNSScriptGeneratorModel(DNSObsModel):
    """
    Common Model for DNS Script generators
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
            return ''
        self._progress_is_canceled = False
        for i, line in enumerate(script):
            try:
                exec(line)  # pylint: disable=exec-used
            except DNSError as errormessage:
                return str(errormessage)
            self._update_progress(i)
            if self._progress_is_canceled:
                return 'Warning script execution stopped, no valid data.'
        return ''

    def cancel_progress(self):
        self._progress_is_canceled = True

    def script_maker(self, _options, _paths, _fselector):
        """
        Generation of a Mantid script to reduce DNS data
        """
        self._script = [""]
        error = ''
        return self._script, error

    # helper functions:

    @staticmethod
    def get_auto_scriptname(fulldata):
        samplename = fulldata[0]['samplename'].strip('_.')
        filenumbers = [x['filenumber'] for x in fulldata]
        return f"script_{samplename}_from_{min(filenumbers)}" \
               f"_to_{max(filenumbers)}.py"

    def get_filename(self, filename, fulldata, auto=False):
        if auto:
            return self.get_auto_scriptname(fulldata)
        if filename:
            if not filename.endswith('.py'):
                filename = ''.join((filename, '.py'))
        else:
            filename = 'script.py'
        return filename

    @staticmethod
    def save_script(script, filename, scriptdir):
        create_dir(scriptdir)
        return save_txt(script, filename, scriptdir)
