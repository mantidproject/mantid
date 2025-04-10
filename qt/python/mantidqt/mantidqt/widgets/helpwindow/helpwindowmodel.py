# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import os

import logging
from qtpy.QtCore import QUrl
from qtpy.QtWebEngineCore import QWebEngineUrlRequestInterceptor, QWebEngineUrlRequestInfo

# Module-level logger for functions outside of classes
_logger = logging.getLogger(__name__)


def _get_version_string_for_url():
    """
    Returns the Mantid version string formatted for use in documentation URLs.
    For example, "v6.12.0" from version "6.12.0.1"

    Returns:
        str: Formatted version string in the form "vX.Y.Z" or None if version cannot be determined
    """
    versionStr = None
    try:
        import mantid

        # Use the mantid version object (proper way)
        versionObj = mantid.version()
        # Retrieve the patch
        patch = versionObj.patch.split(".")[0]
        versionStr = f"v{versionObj.major}.{versionObj.minor}.{patch}"
    except ImportError:
        _logger.warning("Could not determine Mantid version for documentation URL.")
    except Exception as e:
        _logger.warning(f"Error determining Mantid version for documentation URL: {e}")

    return versionStr


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
    _logger = logging.getLogger(__name__)

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
            self._logger.debug(f"Using {self._modeString} from {self._baseUrl}")
        else:
            if self._rawLocalDocsBase:
                self._logger.warning(f"Local docs path '{self._rawLocalDocsBase}' is invalid or not found. Falling back to online docs.")

            self._isLocal = False
            self._modeString = self.MODE_ONLINE

            isLikelyRelease = self._rawLocalDocsBase is None
            if isLikelyRelease:
                self._versionString = _get_version_string_for_url()

            if self._versionString:
                baseOnline = self._rawOnlineBase
                if baseOnline.endswith("/stable"):
                    baseOnline = baseOnline[: -len("/stable")]

                if self._versionString not in baseOnline:
                    self._baseUrl = f"{baseOnline.rstrip('/')}/{self._versionString}"
                    self._logger.debug(f"Using {self._modeString} (Version: {self._versionString}) from {self._baseUrl}")
                else:
                    self._baseUrl = self._rawOnlineBase
                    self._logger.debug(f"Using {self._modeString} (Using provided base URL, possibly stable/latest): {self._baseUrl}")
            else:
                self._baseUrl = self._rawOnlineBase
                self._logger.debug(f"Using {self._modeString} (Version: Unknown/Stable) from {self._baseUrl}")

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
        """`
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
        fullUrlStr = f"{self.get_base_url()}{relativeUrl}"

        url = QUrl(fullUrlStr)
        if not url.isValid():
            self._logger.warning(f"Constructed invalid URL: {fullUrlStr} from base '{self.get_base_url()}' and relative '{relativeUrl}'")
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
            self._logger.debug("Using LocalRequestInterceptor.")
            return LocalRequestInterceptor()
        else:
            self._logger.debug("Using NoOpRequestInterceptor.")
            return NoOpRequestInterceptor()
