# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import os
from qtpy.QtCore import QUrl
from qtpy.QtWebEngineCore import QWebEngineUrlRequestInterceptor, QWebEngineUrlRequestInfo


class NoOpRequestInterceptor(QWebEngineUrlRequestInterceptor):
    """
    A no-op interceptor that does nothing. Used if we're not loading local docs.
    """

    def interceptRequest(self, info: QWebEngineUrlRequestInfo):
        pass


class LocalRequestInterceptor(QWebEngineUrlRequestInterceptor):
    """
    Intercepts requests so we can relax the CORS policy for loading MathJax fonts
    from cdn.jsdelivr.net when using local docs.
    """

    def interceptRequest(self, info: QWebEngineUrlRequestInfo):
        url = info.requestUrl()
        if url.host() == "cdn.jsdelivr.net":
            info.setHttpHeader(b"Access-Control-Allow-Origin", b"*")


class HelpWindowModel:
    def __init__(self, local_docs_base=None, online_base="https://docs.mantidproject.org/"):
        if local_docs_base and not os.path.isdir(local_docs_base):
            raise ValueError(f"Local docs directory '{local_docs_base}' does not exist or is invalid.")
        self.local_docs_base = local_docs_base
        self.online_base = online_base.rstrip("/")

    def is_local_docs_enabled(self):
        """
        :return: True if local_docs_base is set and is a valid directory
        """
        return bool(self.local_docs_base and os.path.isdir(self.local_docs_base))

    def build_help_url(self, relative_url):
        """
        Returns a QUrl pointing to either local or online docs for the given relative URL.
        """
        if not relative_url or not relative_url.endswith(".html"):
            relative_url = "index.html"

        if self.is_local_docs_enabled():
            full_path = os.path.join(self.local_docs_base, relative_url)
            return QUrl.fromLocalFile(full_path)
        else:
            return QUrl(self.online_base + "/" + relative_url)

    def get_home_url(self):
        """
        Return the 'home' page URL:
          - local 'index.html' if local docs are enabled
          - online docs homepage otherwise
        """
        if self.is_local_docs_enabled():
            full_path = os.path.join(self.local_docs_base, "index.html")
            return QUrl.fromLocalFile(full_path)
        else:
            return QUrl(self.online_base + "/index.html")

    def create_request_interceptor(self):
        """
        Return an appropriate request interceptor:
          - LocalRequestInterceptor if local docs are used (for mathjax CORS)
          - NoOpRequestInterceptor otherwise
        """
        if self.is_local_docs_enabled():
            return LocalRequestInterceptor()
        else:
            return NoOpRequestInterceptor()
