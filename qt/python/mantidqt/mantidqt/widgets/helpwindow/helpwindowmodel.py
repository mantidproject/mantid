# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import os

from mantid import logger
from qtpy.QtCore import QUrl
from qtpy.QtWebEngineCore import QWebEngineUrlRequestInterceptor, QWebEngineUrlRequestInfo


def getMantidVersionString():
    """
    Placeholder function to get Mantid version
    """
    try:
        import mantid

        if hasattr(mantid, "__version__"):
            versionParts = str(mantid.__version__).split(".")
            if len(versionParts) >= 2:
                return f"v{versionParts[0]}.{versionParts[1]}.0"
    except ImportError:
        pass
    logger.warning("Warning: Could not determine Mantid version for documentation URL.")
    return None


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
    MODE_LOCAL = "Local Docs"
    MODE_ONLINE = "Online Docs"

    def __init__(self, localDocsBase=None, onlineBase="https://docs.mantidproject.org/"):
        self._rawLocalDocsBase = localDocsBase
        self._rawOnlineBase = onlineBase.rstrip("/")

        self._isLocal = False
        self._modeString = self.MODE_ONLINE
        self._baseUrl = self._rawOnlineBase
        self._versionString = None

        self._determine_mode_and_base_url()

    def _determine_mode_and_base_url(self):
        """
        Sets the internal state (_isLocal, _modeString, _baseWrl, _versionString)
        based on the validity of localDocsBase and attempts to find a versioned URL.
        """
        if self._rawLocalDocsBase and os.path.isdir(self._rawLocalDocsBase):
            self._isLocal = True
            self._modeString = self.MODE_LOCAL
            absLocalPath = os.path.abspath(self._rawLocalDocsBase)
            self._baseUrl = QUrl.fromLocalFile(absLocalPath).toString()
            self._versionString = None
            logger.debug(f"Using {self._modeString} from {self._baseUrl}")
        else:
            if self._rawLocalDocsBase:
                logger.warning(f"Local docs path '{self._rawLocalDocsBase}' is invalid or not found. Falling back to online docs.")

            self._isLocal = False
            self._modeString = self.MODE_ONLINE

            isLikelyRelease = self._rawLocalDocsBase is None
            if isLikelyRelease:
                self._versionString = getMantidVersionString()

            if self._versionString:
                baseOnline = self._rawOnlineBase
                if baseOnline.endswith("/stable"):
                    baseOnline = baseOnline[: -len("/stable")]

                if self._versionString not in baseOnline:
                    self._baseUrl = f"{baseOnline.rstrip('/')}/{self._versionString}"
                    logger.debug(f"Using {self._modeString} (Version: {self._versionString}) from {self._baseUrl}")
                else:
                    self._baseUrl = self._rawOnlineBase
                    logger.debug(f"Using {self._modeString} (Using provided base URL, possibly stable/latest): {self._baseUrl}")

            else:
                self._baseUrl = self._rawOnlineBase
                logger.debug(f"Using {self._modeString} (Version: Unknown/Stable) from {self._baseUrl}")

    def is_local_docs_mode(self):
        """
        :return: True if using local docs, False otherwise. Based on initial check.
        """
        return self._isLocal

    def get_mode_string(self):
        """
        :return: User-friendly string indicating the mode ("Local Docs" or "Online Docs").
        """
        return self._modeString

    def get_base_url(self):
        """
        :return: The determined base URL (either file:///path or https://docs...[/version])
        """
        return self._baseUrl.rstrip("/") + "/"

    def build_help_url(self, relativeUrl):
        """
        Returns a QUrl pointing to the determined doc source for the given relative URL.
        """
        if not relativeUrl or not relativeUrl.lower().endswith((".html", ".htm")):
            relativeUrl = "index.html"

        relativeUrl = relativeUrl.lstrip("/")
        base = self.get_base_url()
        fullUrlStr = f"{base}{relativeUrl}"

        url = QUrl(fullUrlStr)
        if not url.isValid():
            logger.warning(f"Constructed invalid URL: {fullUrlStr} from base '{base}' and relative '{relativeUrl}'")
        return url

    def get_home_url(self):
        """
        Return the 'home' page URL:
          - local 'index.html' if local docs are enabled
          - online docs homepage otherwise
        """
        return self.build_help_url("index.html")

    def create_request_interceptor(self):
        """
        Return an appropriate request interceptor:
          - LocalRequestInterceptor if local docs are used (for mathjax CORS)
          - NoOpRequestInterceptor otherwise
        """
        if self._isLocal:
            logger.debug("Using LocalRequestInterceptor.")
            return LocalRequestInterceptor()
        else:
            logger.debug("Using NoOpRequestInterceptor.")
            return NoOpRequestInterceptor()
