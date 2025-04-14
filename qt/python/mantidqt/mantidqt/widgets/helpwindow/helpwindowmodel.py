# Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import os


try:
    from mantid.kernel import Logger
    from mantid.kernel import ConfigService

except ImportError:
    print("Warning: Mantid Kernel (Logger/ConfigService) not found, using basic print/dummy.")

    class Logger:
        def __init__(self, name):
            self._name = name

        def warning(self, msg):
            print(f"WARNING [{self._name}]: {msg}")

        def debug(self, msg):
            print(f"DEBUG [{self._name}]: {msg}")

        def information(self, msg):
            print(f"INFO [{self._name}]: {msg}")

        def error(self, msg):
            print(f"ERROR [{self._name}]: {msg}")

    class ConfigService:
        @staticmethod
        def Instance():
            class DummyInstance:
                def getString(self, key, pathAbsolute=True):
                    return None  # Default to None

            return DummyInstance()


log = Logger("HelpWindowModel")

from qtpy.QtCore import QUrl  # noqa: E402
from qtpy.QtWebEngineCore import QWebEngineUrlRequestInterceptor, QWebEngineUrlRequestInfo  # noqa: E402


def getMantidVersionString():
    """Placeholder function to get Mantid version (e.g., 'v6.13.0')."""
    try:
        import mantid

        if hasattr(mantid, "__version__"):
            versionParts = str(mantid.__version__).split(".")
            if len(versionParts) >= 2:
                return f"v{versionParts[0]}.{versionParts[1]}.0"
    except ImportError:
        pass
    log.warning("Could not determine Mantid version for documentation URL.")
    return None


class NoOpRequestInterceptor(QWebEngineUrlRequestInterceptor):
    """A no-op interceptor used when loading online docs."""

    def interceptRequest(self, info: QWebEngineUrlRequestInfo):
        pass


class LocalRequestInterceptor(QWebEngineUrlRequestInterceptor):
    """Intercepts requests for local docs (e.g., handle CORS)."""

    def interceptRequest(self, info: QWebEngineUrlRequestInfo):
        url = info.requestUrl()
        if url.host() == "cdn.jsdelivr.net":  # Allow MathJax CDN
            info.setHttpHeader(b"Access-Control-Allow-Origin", b"*")


class HelpWindowModel:
    MODE_OFFLINE = "Offline Docs"
    MODE_ONLINE = "Online Docs"

    def __init__(self, online_base="https://docs.mantidproject.org/"):
        local_docs_path_from_config = None
        try:
            config_service = ConfigService.Instance()
            raw_path = config_service.getString("docs.html.root", True)
            if raw_path:
                local_docs_path_from_config = raw_path
                log.debug(f"Retrieved 'docs.html.root' from ConfigService: '{local_docs_path_from_config}'")
            else:
                log.debug("'docs.html.root' property is empty or not found in ConfigService.")

        except Exception as e:
            log.error(f"Error retrieving 'docs.html.root' from ConfigService: {e}")

        self._raw_online_base = online_base.rstrip("/")
        self._is_local = False
        self._mode_string = self.MODE_ONLINE
        self._base_url = self._raw_online_base
        self._version_string = None
        self._determine_mode_and_base_url(local_docs_path_from_config)

    def _determine_mode_and_base_url(self, local_docs_path):
        """
        Sets the internal state (_is_local, _mode_string, _base_url, _version_string)
        based on the validity of the provided local_docs_path.
        """
        log.debug(f"Determining mode with local_docs_path='{local_docs_path}'")
        if local_docs_path and os.path.isdir(local_docs_path):
            self._is_local = True
            self._mode_string = self.MODE_OFFLINE
            abs_local_path = os.path.abspath(local_docs_path)
            self._base_url = QUrl.fromLocalFile(abs_local_path).toString()
            self._version_string = None
            log.debug(f"Using {self._mode_string} from {self._base_url}")
        else:
            if local_docs_path:
                log.warning(
                    f"Local docs path '{local_docs_path}' from ConfigService ('docs.html.root') is invalid or not found. Falling back to online docs."  # noqa: E501
                )
            else:
                log.debug("No valid local docs path found from ConfigService. Using online docs.")

            self._is_local = False
            self._mode_string = self.MODE_ONLINE
            self._version_string = getMantidVersionString()

            if self._version_string:
                base_online = self._raw_online_base
                if base_online.endswith("/stable"):
                    base_online = base_online[: -len("/stable")]
                if self._version_string not in base_online:
                    self._base_url = f"{base_online.rstrip('/')}/{self._version_string}"
                    log.debug(f"Using {self._mode_string} (Version: {self._version_string}) from {self._base_url}")
                else:
                    self._base_url = self._raw_online_base
                    log.debug(f"Using {self._mode_string} (Using provided base URL, possibly stable/latest): {self._base_url}")
            else:
                self._base_url = self._raw_online_base
                log.debug(f"Using {self._mode_string} (Version: Unknown/Stable) from {self._base_url}")

    def is_local_docs_mode(self):
        return self._is_local

    def get_mode_string(self):
        return self._mode_string

    def get_base_url(self):
        return self._base_url.rstrip("/") + "/"

    def build_help_url(self, relative_url):
        if not relative_url or not relative_url.lower().endswith((".html", ".htm")):
            relative_url = "index.html"
        relative_url = relative_url.lstrip("/")
        base = self.get_base_url()
        full_url_str = f"{base}{relative_url}"
        url = QUrl(full_url_str)
        if not url.isValid():
            log.warning(f"Constructed invalid URL: {full_url_str} from base '{base}' and relative '{relative_url}'")
        return url

    def get_home_url(self):
        return self.build_help_url("index.html")

    def create_request_interceptor(self):
        if self._is_local:
            log.debug("Using LocalRequestInterceptor.")
            return LocalRequestInterceptor()
        else:
            log.debug("Using NoOpRequestInterceptor.")
            return NoOpRequestInterceptor()
