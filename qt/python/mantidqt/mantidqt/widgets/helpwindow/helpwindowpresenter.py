# Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

try:
    from mantid.kernel import Logger
except ImportError:
    print("Warning: Mantid Logger not found, using basic print for logging.")

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


log = Logger("HelpWindowPresenter")

from mantidqt.widgets.helpwindow.helpwindowmodel import HelpWindowModel  # noqa: E402
from qtpy.QtGui import QDesktopServices  # noqa: E402
from qtpy.QtCore import QUrl  # noqa: E402


class HelpWindowPresenter:
    """
    Simplified help window presenter that opens all documentation in the system browser.
    Uses QDesktopServices to open both local and online documentation URLs.
    """

    def __init__(self, parentApp=None, onlineBaseUrl="https://docs.mantidproject.org/"):
        log.debug(f"Initializing with onlineBaseUrl='{onlineBaseUrl}'")
        try:
            self.model = HelpWindowModel(online_base=onlineBaseUrl)
        except Exception as e:
            log.error(f"Failed to initialize HelpWindowModel: {e}")
            self.model = None
        self.parentApp = parentApp

    def show_help_page(self, relativeUrl):
        """
        Opens the documentation page in the system browser.
        Builds the URL from the model (local file:// or online https://).
        """
        if not self.model:
            log.error("Cannot show help page, model is not available.")
            return

        log.debug(f"Opening help page in system browser: '{relativeUrl}'")
        try:
            docUrl = self.model.build_help_url(relativeUrl)
            if not QDesktopServices.openUrl(docUrl):
                log.error(f"Failed to open URL in system browser: {docUrl.toString()}")
        except FileNotFoundError as e:
            log.error(f"Documentation file not found: {e}")
            # Fallback to online docs if local file not found
            try:
                fallback_url = QUrl(f"{self.model._raw_online_base}/{relativeUrl}")
                log.debug(f"Attempting fallback to online docs: {fallback_url.toString()}")
                if not QDesktopServices.openUrl(fallback_url):
                    log.error(f"Failed to open fallback URL: {fallback_url.toString()}")
            except Exception as fallback_error:
                log.error(f"Fallback to online docs failed: {fallback_error}")
        except Exception as e:
            log.error(f"Error opening help page '{relativeUrl}': {e}")

    def show_home_page(self):
        """
        Opens the documentation home page in the system browser.
        """
        if not self.model:
            log.error("Cannot show home page, model is not available.")
            return

        log.debug("Opening home page in system browser.")
        try:
            homeUrl = self.model.get_home_url()
            if not QDesktopServices.openUrl(homeUrl):
                log.error(f"Failed to open home URL in system browser: {homeUrl.toString()}")
        except FileNotFoundError as e:
            log.error(f"Home page file not found: {e}")
            # Fallback to online docs
            try:
                fallback_url = QUrl(f"{self.model._raw_online_base}/index.html")
                log.debug(f"Attempting fallback to online home page: {fallback_url.toString()}")
                if not QDesktopServices.openUrl(fallback_url):
                    log.error(f"Failed to open fallback home URL: {fallback_url.toString()}")
            except Exception as fallback_error:
                log.error(f"Fallback to online home page failed: {fallback_error}")
        except Exception as e:
            log.error(f"Error opening home page: {e}")
