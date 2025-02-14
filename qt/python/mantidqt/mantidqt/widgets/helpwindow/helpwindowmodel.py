# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import os
from qtpy.QtCore import QUrl


class HelpWindowModel:
    def __init__(self, local_docs_base=None, online_base="https://docs.mantidproject.org/"):
        self.local_docs_base = local_docs_base
        self.online_base = online_base.rstrip("/")

    def build_help_url(self, relative_url):
        """
        Returns a QUrl pointing to either local or online docs for the given relative URL.
        """
        if not relative_url or not relative_url.endswith(".html"):
            relative_url = "index.html"

        if self.local_docs_base and os.path.isdir(self.local_docs_base):
            full_path = os.path.join(self.local_docs_base, relative_url)
            return QUrl.fromLocalFile(full_path)
        else:
            return QUrl(self.online_base + "/" + relative_url)

    def is_local_docs_enabled(self):
        """
        :return: True if local_docs_base is set and is a valid directory
        """
        return bool(self.local_docs_base and os.path.isdir(self.local_docs_base))

    def get_home_url(self):
        """
        Return the 'home' page URL:
          - Local 'index.html' if local docs are enabled
          - Online docs homepage otherwise
        """
        if self.is_local_docs_enabled():
            # local docs base + 'index.html'
            full_path = os.path.join(self.local_docs_base, "index.html")
            return QUrl.fromLocalFile(full_path)
        else:
            # online base
            return QUrl(self.online_base + "/index.html")
