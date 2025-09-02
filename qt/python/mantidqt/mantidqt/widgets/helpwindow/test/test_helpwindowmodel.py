# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import os
import sys
import tempfile
import shutil
import unittest
from unittest.mock import patch

from mantidqt.widgets.helpwindow.helpwindowmodel import HelpWindowModel, LocalRequestInterceptor, NoOpRequestInterceptor

# --- Define Constants ---
CONFIG_SERVICE_LOOKUP_PATH = "mantidqt.widgets.helpwindow.helpwindowmodel.ConfigService"
VERSION_FUNC_LOOKUP_PATH = "mantidqt.widgets.helpwindow.helpwindowmodel.getMantidVersionString"
ONLINE_BASE_EXAMPLE = "https://example.org"
# ------------------------


class TestHelpWindowModelConfigService(unittest.TestCase):
    def setUp(self):
        self.temp_dir = tempfile.mkdtemp()
        self.props_dir = os.path.join(self.temp_dir, "bin", "")
        os.makedirs(self.props_dir)
        self.invalid_path = os.path.join(self.temp_dir, "non_existent_subfolder")
        if os.path.exists(self.invalid_path):
            os.rmdir(self.invalid_path)

    def tearDown(self):
        shutil.rmtree(self.temp_dir, ignore_errors=True)

    @patch(CONFIG_SERVICE_LOOKUP_PATH)
    def test_init_when_config_returns_valid_path_configures_for_local(self, mock_ConfigService):
        """Test local mode when ConfigService returns a valid directory path."""

        mantid_root = os.path.dirname(os.path.dirname(self.props_dir))
        docs_path = os.path.join(mantid_root, "share", "doc", "html")
        os.makedirs(docs_path)

        mock_ConfigService.getPropertiesDir.return_value = self.props_dir

        model = HelpWindowModel(online_base=ONLINE_BASE_EXAMPLE)

        self.assertTrue(model.is_local_docs_mode(), "Expected is_local_docs_mode() True")
        self.assertEqual(model.MODE_OFFLINE, model.get_mode_string(), "Expected mode string 'Offline Docs'")

        dummy_home_path = os.path.join(docs_path, "index.html")
        with open(dummy_home_path, "w") as f:
            f.write("<html></html>")

        algo_subdir = os.path.join(docs_path, "algorithms")
        os.makedirs(algo_subdir, exist_ok=True)
        algo_file_rel_path = "algorithms/MyAlgorithm-v1.html"
        dummy_algo_path = os.path.join(docs_path, algo_file_rel_path)
        with open(dummy_algo_path, "w") as f:
            f.write("<html></html>")

        test_url = model.build_help_url(algo_file_rel_path)
        self.assertTrue(test_url.isLocalFile(), "Expected a local file:// URL")

        local_file_path = test_url.toLocalFile()
        norm_local_file_path = os.path.normpath(local_file_path)
        norm_expected_file_path = os.path.normpath(dummy_algo_path)
        self.assertEqual(
            norm_local_file_path,
            norm_expected_file_path,
            f"URL path '{norm_local_file_path}' should match dummy file path '{norm_expected_file_path}'",
        )

        norm_expected_prefix = os.path.normpath(os.path.abspath(self.temp_dir))
        self.assertTrue(
            norm_local_file_path.startswith(norm_expected_prefix + os.sep),
            f"URL path '{norm_local_file_path}' should start with temp dir path '{norm_expected_prefix}{os.sep}'",
        )

        home_url = model.get_home_url()
        self.assertTrue(home_url.isLocalFile(), "Expected a local file:// for home URL")
        norm_home_path = os.path.normpath(home_url.toLocalFile())
        norm_expected_home_path = os.path.normpath(dummy_home_path)
        self.assertEqual(norm_home_path, norm_expected_home_path, "Home URL should point to dummy index.html")

        mock_ConfigService.getPropertiesDir.assert_called_once_with()

    @patch(CONFIG_SERVICE_LOOKUP_PATH)
    def test_init_when_config_returns_empty_props_path_uses_online(self, mock_ConfigService):
        """Test online mode when ConfigService returns emtpy path."""
        mock_ConfigService.getPropertiesDir.return_value = ""

        model = HelpWindowModel(online_base=ONLINE_BASE_EXAMPLE)

        self.assertFalse(model.is_local_docs_mode(), "Expected is_local_docs_mode() False")
        self.assertEqual(model.MODE_ONLINE, model.get_mode_string(), "Expected mode string 'Online Docs'")
        test_url = model.build_help_url("algorithms/MyAlgorithm-v1.html")
        self.assertFalse(test_url.isLocalFile(), "Expected a non-local (http/https) URL")
        self.assertEqual(test_url.scheme(), "https")
        self.assertTrue(ONLINE_BASE_EXAMPLE in test_url.toString())
        self.assertTrue("algorithms/MyAlgorithm-v1.html" in test_url.toString())
        home_url = model.get_home_url()
        self.assertFalse(home_url.isLocalFile(), "Expected an online docs home URL")
        self.assertTrue(ONLINE_BASE_EXAMPLE in home_url.toString())

        mock_ConfigService.getPropertiesDir.assert_called_once_with()

    @patch(CONFIG_SERVICE_LOOKUP_PATH)
    def test_init_when_config_returns_invalid_path_falls_back_to_online(self, mock_ConfigService):
        """Test fallback to online mode when ConfigService returns an invalid/non-existent directory path."""
        mock_ConfigService.getPropertiesDir.return_value = self.invalid_path

        model = HelpWindowModel(online_base=ONLINE_BASE_EXAMPLE)

        self.assertFalse(model.is_local_docs_mode(), "Expected is_local_docs_mode() False")
        self.assertEqual(model.MODE_ONLINE, model.get_mode_string(), "Expected mode string 'Online Docs' on fallback")

        mock_ConfigService.getPropertiesDir.assert_called_once_with()

    @patch(CONFIG_SERVICE_LOOKUP_PATH)
    def test_create_request_interceptor_based_on_mode(self, mock_ConfigService):
        """Test that the correct interceptor is created based on the mode determined from ConfigService."""
        mantid_root = os.path.dirname(os.path.dirname(self.props_dir))
        docs_path = os.path.join(mantid_root, "share", "doc", "html")
        os.makedirs(docs_path)

        mock_ConfigService.getPropertiesDir.return_value = self.props_dir
        local_model = HelpWindowModel(online_base=ONLINE_BASE_EXAMPLE)
        local_interceptor = local_model.create_request_interceptor()
        self.assertIsInstance(local_interceptor, LocalRequestInterceptor, "Expected LocalRequestInterceptor for local mode")
        mock_ConfigService.getPropertiesDir.assert_called_once_with()
        mock_ConfigService.reset_mock()

        mock_ConfigService.getPropertiesDir.return_value = ""
        online_model = HelpWindowModel(online_base=ONLINE_BASE_EXAMPLE)
        online_interceptor = online_model.create_request_interceptor()
        self.assertIsInstance(online_interceptor, NoOpRequestInterceptor, "Expected NoOpRequestInterceptor for online mode")
        mock_ConfigService.getPropertiesDir.assert_called_once_with()

    @patch(CONFIG_SERVICE_LOOKUP_PATH)
    @patch(VERSION_FUNC_LOOKUP_PATH, return_value=None)
    def test_online_url_construction_with_trailing_slashes(self, mock_version_func, mock_ConfigService):
        """Test online base URL construction handles trailing slashes correctly (forced online)."""
        mock_ConfigService.getPropertiesDir.return_value = ""

        model1 = HelpWindowModel(online_base="https://example.org")
        model2 = HelpWindowModel(online_base="https://example.org/")

        self.assertFalse(model1.is_local_docs_mode(), "Model1 should be in online mode")
        self.assertFalse(model2.is_local_docs_mode(), "Model2 should be in online mode")
        self.assertTrue(model1.get_base_url().endswith("/"), "get_base_url() should add trailing slash")
        self.assertTrue(model2.get_base_url().endswith("/"), "get_base_url() should keep trailing slash")
        self.assertEqual(model1.get_base_url(), model2.get_base_url(), "Base URLs should be identical")

        url1 = model1.build_help_url("test.html")
        url2 = model2.build_help_url("test.html")

        self.assertEqual(url1.toString(), url2.toString(), "Generated URLs should be identical")
        self.assertEqual(url1.toString(), "https://example.org/test.html", "Generated URL should be correct (no version)")
        self.assertTrue(mock_version_func.called)
        mock_ConfigService.getPropertiesDir.assert_called()


class TestHelpWindowModelDocPathDiscovery(unittest.TestCase):
    """Tests for different documentation path discovery scenarios in _get_doc_path()"""

    def setUp(self):
        self.temp_dir = tempfile.mkdtemp()
        self.original_sys_prefix = sys.prefix

    def tearDown(self):
        shutil.rmtree(self.temp_dir, ignore_errors=True)
        sys.prefix = self.original_sys_prefix

    def _create_docs_structure(self, base_path, subpath="share/doc/html"):
        """Helper to create a documentation directory structure"""
        docs_path = os.path.join(base_path, subpath)
        os.makedirs(docs_path, exist_ok=True)
        # Create a dummy index.html to make it look like real docs
        with open(os.path.join(docs_path, "index.html"), "w") as f:
            f.write("<html><body>Test Docs</body></html>")
        return docs_path

    @patch("sys.prefix")
    @patch(CONFIG_SERVICE_LOOKUP_PATH)
    def test_conda_environment_docs_discovery(self, mock_ConfigService, mock_sys_prefix):
        """Test discovery of docs in conda environment installation"""
        # Set up conda-like sys.prefix
        conda_prefix = os.path.join(self.temp_dir, "conda_env")
        mock_sys_prefix.__str__ = lambda: conda_prefix  # For string operations
        mock_sys_prefix.__contains__ = lambda self, item: "conda" in conda_prefix
        mock_ConfigService.getPropertiesDir.return_value = ""

        # Create the expected conda docs structure
        docs_path = self._create_docs_structure(conda_prefix, "share/doc/html")

        with patch("sys.prefix", conda_prefix):
            model_path = HelpWindowModel._get_doc_path(HelpWindowModel())

        print(model_path)
        print(docs_path)

        self.assertEqual(os.path.normpath(model_path), os.path.normpath(docs_path), "Should find conda docs path")

    @patch(CONFIG_SERVICE_LOOKUP_PATH)
    def test_standalone_installation_docs_discovery(self, mock_ConfigService):
        """Test discovery of docs in standalone installation"""
        # Create a mock properties directory structure
        props_dir = os.path.join(self.temp_dir, "mantid", "bin", "")
        os.makedirs(props_dir, exist_ok=True)

        # The mantid root should be two levels up from properties
        mantid_root = os.path.join(self.temp_dir, "mantid")

        # Create the expected standalone docs structure
        docs_path = self._create_docs_structure(mantid_root, "share/doc/html")

        mock_ConfigService.getPropertiesDir.return_value = props_dir

        model = HelpWindowModel()
        discovered_path = model._get_doc_path()

        self.assertEqual(os.path.normpath(discovered_path), os.path.normpath(docs_path), "Should find standalone installation docs path")

    @patch(CONFIG_SERVICE_LOOKUP_PATH)
    def test_linux_debug_build_docs_discovery(self, mock_ConfigService):
        """Test discovery of docs in Linux debug build"""
        # Create a mock properties directory structure
        props_dir = os.path.join(self.temp_dir, "mantid_build", "bin", "properties")
        os.makedirs(props_dir, exist_ok=True)

        mantid_root = os.path.join(self.temp_dir, "mantid_build")

        # Create the expected debug build docs structure (docs/html instead of share/doc/html)
        docs_path = self._create_docs_structure(mantid_root, "docs/html")

        mock_ConfigService.getPropertiesDir.return_value = props_dir

        model = HelpWindowModel()
        discovered_path = model._get_doc_path()

        self.assertEqual(os.path.normpath(discovered_path), os.path.normpath(docs_path), "Should find Linux debug build docs path")

    @patch(CONFIG_SERVICE_LOOKUP_PATH)
    def test_windows_debug_build_docs_discovery(self, mock_ConfigService):
        """Test discovery of docs in Windows debug build (double parent lookup)"""
        # Create a more nested properties directory structure for Windows
        props_dir = os.path.join(self.temp_dir, "mantid_build", "bin", "Debug", "properties")
        os.makedirs(props_dir, exist_ok=True)

        # For Windows debug, mantid root is two parents up from props folder parent
        mantid_root = os.path.join(self.temp_dir, "mantid_build")

        # Create the expected Windows debug docs structure
        docs_path = self._create_docs_structure(mantid_root, "docs/html")

        mock_ConfigService.getPropertiesDir.return_value = props_dir

        model = HelpWindowModel()
        discovered_path = model._get_doc_path()

        self.assertEqual(os.path.normpath(discovered_path), os.path.normpath(docs_path), "Should find Windows debug build docs path")

    @patch(CONFIG_SERVICE_LOOKUP_PATH)
    def test_no_docs_found_returns_empty_string(self, mock_ConfigService):
        """Test that _get_doc_path returns empty string when no docs are found"""
        # Create properties dir but no docs
        props_dir = os.path.join(self.temp_dir, "mantid", "bin", "properties")
        os.makedirs(props_dir, exist_ok=True)

        mock_ConfigService.getPropertiesDir.return_value = props_dir

        model = HelpWindowModel()
        discovered_path = model._get_doc_path()

        self.assertEqual(discovered_path, "", "Should return empty string when no docs found")


if __name__ == "__main__":
    unittest.main()
