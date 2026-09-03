# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import os

# --- Logger and ConfigService Setup ---
from mantid.kernel import Logger
from mantid.kernel import ConfigService
from mantid.kernel import version

log = Logger("HelpWindowModel")
# --------------------------------------

# Imports moved below logger/config setup to ensure log is defined
from qtpy.QtCore import QUrl  # noqa: E402


def getMantidVersionString():
    # Assume it's a nightly build if patch > 100, (i.e. patch == YYYYMMDD.TIME)
    if version.patch > 100:
        return "nightly"

    return f"v{version.major}.{version.minor}.{version.patch}"


class HelpWindowModel:
    MODE_OFFLINE = "Offline Docs"
    MODE_ONLINE = "Online Docs"

    ONLINE_BASE_URL = "https://docs.mantidproject.org"

    def __init__(self):
        # --- Step 1: Attempt to get local path ---
        local_docs_path_from_config = self._get_doc_path()

        # --- Step 2: Determine final mode and set ALL related state variables ---
        # This method now sets _is_local, _mode_string, _base_url, _version_string
        self._determine_mode_and_set_state(local_docs_path_from_config)

    def _get_doc_path(self):
        """
        Locate the documentation directory for Mantid installations.

        Searches for docs in various installation patterns:
        - Standalone installations: {installation_dir}/share/doc/html/
        - Conda installations: {env_dir}/share/doc/html/
        - Debug builds: {build_dir}/docs/html/

        Returns:
            str: Path to the documentation directory, or empty string if not found
        """

        # Get the bin directory from the properties dir, we move one level up because the return strings ends with /
        bin_dir = os.path.dirname(ConfigService.getPropertiesDir())
        if not bin_dir:
            return ""

        doc_paths_to_try = []
        if os.name == "posix":
            # On Unix-like systems the parent of the bin dir will always be either installation/env/build dir
            project_dir = os.path.dirname(bin_dir)

            doc_paths_to_try = [
                # Standard standalone/conda installation path
                os.path.join(project_dir, "share", "doc", "html"),
                # Debug build installation path
                os.path.join(project_dir, "docs", "html"),
            ]
        else:
            # On Windows the parent of the bin dir will be the installation dir
            installation_dir = os.path.dirname(bin_dir)
            # On Windows the second parent of the bin dir will be the conda env dir
            # because there is additional /Library/ in windows conda envs path structure
            env_dir = os.path.dirname(os.path.dirname(bin_dir))
            # On Windows (with Visual Studio) the second parent of the bin dir will be the debug build dir
            # because there is additional level for the configuration (i.e. /DebugWithRelRuntime)
            build_dir = os.path.dirname(os.path.dirname(bin_dir))
            # On Windows with ninja the first parent of the bin dir will be the build dir
            # because ninja does not have additional configuration level
            build_dir_ninja = os.path.dirname(bin_dir)

            doc_paths_to_try = [
                # Standard Windows installation path
                os.path.join(installation_dir, "share", "doc", "html"),
                # Windows debug build
                os.path.join(build_dir, "docs", "html"),
                # Windows ninja build
                os.path.join(build_dir_ninja, "docs", "html"),
                # Windows conda installation
                os.path.join(env_dir, "share", "doc", "html"),
            ]

        # Try each potential path until we find one that exists
        for docs_path in doc_paths_to_try:
            if os.path.exists(docs_path):
                return docs_path

        return ""

    def _determine_mode_and_set_state(self, local_docs_path):
        """
        Sets the final operational state (_is_local, _mode_string, _base_url, _version_string)
        based *only* on the validity of the provided local_docs_path argument, which is the
        result of the ConfigService lookup (can be a path string or None).
        """
        log.debug(f"Determining final mode and state with local_docs_path='{local_docs_path}'")

        # Check if the path from config is valid and points to an existing directory
        if local_docs_path and os.path.isdir(os.path.normpath(local_docs_path)):
            # --- Configure for LOCAL/OFFLINE Mode ---
            log.debug("Valid local docs path found. Configuring for Offline Mode.")
            self._is_local = True
            self._mode_string = self.MODE_OFFLINE
            abs_local_path = os.path.abspath(local_docs_path)  # Ensure absolute
            # Base URL for local files needs 'file:///' prefix and correct path format
            self._base_url = QUrl.fromLocalFile(abs_local_path).toString()

        else:
            # --- Configure for ONLINE Mode ---
            # Log reason if applicable
            if local_docs_path:  # Path was provided but invalid
                log.warning(f"Local docs path '{local_docs_path}' is invalid or not found. Falling back to Online Mode.")
            else:  # Path was None (not found in config or error during lookup)
                log.debug("No valid local docs path found. Configuring for Online Mode.")

            self._is_local = False
            self._mode_string = self.MODE_ONLINE

            # Use version string to link to either versioned or nightly docs.
            version_string = getMantidVersionString()

            # Set final base URL based on online path and version string
            self._base_url = f"{self.ONLINE_BASE_URL}/{version_string}"

        log.debug(f"Final state: Mode='{self._mode_string}', Base URL='{self._base_url}'")

    # --- Getter methods remain the same ---
    def is_local_docs_mode(self):
        """
        :return: True if using local docs, False otherwise. Based on state set during init.
        """
        return self._is_local

    def get_mode_string(self):
        """
        :return: User-friendly string indicating the mode ("Offline Docs" or "Online Docs").
        """
        return self._mode_string

    def get_base_url(self):
        """
        :return: The determined base URL (either file:///path/ or https://docs...[/version]/) with trailing slash.
        """
        # Ensure trailing slash for correct relative URL joining
        return self._base_url.rstrip("/") + "/"

    # --- URL building methods use the state set during init ---
    def build_help_url(self, relative_url):
        """
        Returns a QUrl pointing to the determined doc source for the given relative URL.
        Raises FileNotFoundError if building a URL for local mode and the target file doesn't exist.
        """
        # Some help urls may end with a paragraph section denoted by a hash (called a fragment).
        # We need to remove this before executing the logic for checking whether the file exists etc.
        # It is then re-added to the QUrl at the end.
        fragment = ""
        if "#" in relative_url:
            relative_url, fragment = relative_url.split("#", 1)

        # Default page logic
        if not relative_url or not relative_url.lower().endswith((".html", ".htm")):
            relative_url = "index.html"
        # Ensure relative path format
        relative_url = relative_url.lstrip("/")

        base = self.get_base_url()  # Get base URL (file:/// or https://)

        # --- Check local file existence if in local mode ---
        if self._is_local:
            # Convert base file:/// URL back to a filesystem path
            # Note: QUrl().toLocalFile() handles platform specifics
            base_path = QUrl(base).toLocalFile()
            if not base_path:  # Defensive check
                err_msg = f"Cannot determine local base path from URL: {base}"
                log.error(err_msg)
                # Raise a different error as this indicates a problem with the base URL itself
                raise ValueError(err_msg)

            # Construct the full potential path to the target HTML file
            full_path = os.path.join(base_path, relative_url)
            # Normalize for consistent checking and clearer error messages
            norm_full_path = os.path.normpath(full_path)

            log.debug(f"Checking local file existence: {norm_full_path}")
            # Check if it exists AND is a file (not a directory)
            if not os.path.isfile(norm_full_path):
                err_msg = f"Local help file not found: {norm_full_path}"
                log.warning(err_msg)
                # Raise FileNotFoundError as requested by reviewer suggestion
                raise FileNotFoundError(err_msg)
            else:
                # File exists, return the QUrl for the local file
                log.debug(f"Local file found. Returning URL: file:///{norm_full_path}")

                url = QUrl.fromLocalFile(norm_full_path)
                if fragment:
                    url.setFragment(fragment)
                return url
        # -------------------------------------------------
        else:  # Online mode
            # Construct the full online URL string
            full_url_str = f"{base}{relative_url}"
            url = QUrl(full_url_str)
            if fragment:
                url.setFragment(fragment)
            # Basic validation check
            if not url.isValid():
                log.warning(f"Constructed invalid Online URL: {full_url_str} from base '{base}' and relative '{relative_url}'")
            log.debug(f"Returning online URL: {url.toString()}")
            return url

    def get_home_url(self):
        """
        Return the 'home' page URL (index.html) based on the determined mode/base URL.
        May raise FileNotFoundError if in local mode and index.html does not exist.
        """
        # This call now incorporates the existence check from build_help_url
        return self.build_help_url("index.html")
