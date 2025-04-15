# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import os
import tempfile
import shutil
import unittest
from unittest.mock import patch

from mantidqt.widgets.helpwindow.helpwindowmodel import HelpWindowModel, LocalRequestInterceptor, NoOpRequestInterceptor

# --- Define Constants ---
CONFIG_SERVICE_LOOKUP_PATH = "mantidqt.widgets.helpwindow.helpwindowmodel.ConfigService"
VERSION_FUNC_LOOKUP_PATH = "mantidqt.widgets.helpwindow.helpwindowmodel.getMantidVersionString"
DOCS_ROOT_KEY = "docs.html.root"
ONLINE_BASE_EXAMPLE = "https://example.org"
# ------------------------


class TestHelpWindowModelConfigService(unittest.TestCase):
    def setUp(self):
        self.temp_dir = tempfile.mkdtemp()
        self.invalid_path = os.path.join(self.temp_dir, "non_existent_subfolder")
        if os.path.exists(self.invalid_path):
            os.rmdir(self.invalid_path)

    def tearDown(self):
        shutil.rmtree(self.temp_dir, ignore_errors=True)

    @patch(CONFIG_SERVICE_LOOKUP_PATH)
    def test_init_when_config_returns_valid_path_configures_for_local(self, mock_ConfigService):
        """Test local mode when ConfigService returns a valid directory path."""
        mock_instance = mock_ConfigService.Instance.return_value
        mock_instance.getString.return_value = self.temp_dir

        model = HelpWindowModel(online_base=ONLINE_BASE_EXAMPLE)

        self.assertTrue(model.is_local_docs_mode(), "Expected is_local_docs_mode() True")
        self.assertEqual(model.MODE_OFFLINE, model.get_mode_string(), "Expected mode string 'Offline Docs'")
        test_url = model.build_help_url("algorithms/MyAlgorithm-v1.html")
        self.assertTrue(test_url.isLocalFile(), "Expected a local file:// URL")

        local_file_path = test_url.toLocalFile()
        norm_local_file_path = os.path.normpath(local_file_path)
        norm_expected_prefix = os.path.normpath(os.path.abspath(self.temp_dir))

        self.assertTrue(
            norm_local_file_path.startswith(norm_expected_prefix + os.sep),
            f"URL path '{norm_local_file_path}' should start with temp dir path '{norm_expected_prefix}{os.sep}'",
        )

        home_url = model.get_home_url()
        self.assertTrue(home_url.isLocalFile(), "Expected a local file:// for home URL")

        mock_instance.getString.assert_called_once_with(DOCS_ROOT_KEY, True)
        mock_ConfigService.Instance.assert_called_once()

    @patch(CONFIG_SERVICE_LOOKUP_PATH)
    def test_init_when_config_returns_none_uses_online(self, mock_ConfigService):
        """Test online mode when ConfigService returns None (or empty string) for the path."""
        mock_instance = mock_ConfigService.Instance.return_value
        mock_instance.getString.return_value = None

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

        mock_instance.getString.assert_called_once_with(DOCS_ROOT_KEY, True)
        mock_ConfigService.Instance.assert_called_once()

    @patch(CONFIG_SERVICE_LOOKUP_PATH)
    def test_init_when_config_returns_invalid_path_falls_back_to_online(self, mock_ConfigService):
        """Test fallback to online mode when ConfigService returns an invalid/non-existent directory path."""
        mock_instance = mock_ConfigService.Instance.return_value
        mock_instance.getString.return_value = self.invalid_path

        model = HelpWindowModel(online_base=ONLINE_BASE_EXAMPLE)

        self.assertFalse(model.is_local_docs_mode(), "Expected is_local_docs_mode() False")
        self.assertEqual(model.MODE_ONLINE, model.get_mode_string(), "Expected mode string 'Online Docs' on fallback")

        mock_instance.getString.assert_called_once_with(DOCS_ROOT_KEY, True)
        mock_ConfigService.Instance.assert_called_once()

    @patch(CONFIG_SERVICE_LOOKUP_PATH)
    def test_create_request_interceptor_based_on_mode(self, mock_ConfigService):
        """Test that the correct interceptor is created based on the mode determined from ConfigService."""
        mock_instance_local = mock_ConfigService.Instance.return_value
        mock_instance_local.getString.return_value = self.temp_dir
        local_model = HelpWindowModel(online_base=ONLINE_BASE_EXAMPLE)
        local_interceptor = local_model.create_request_interceptor()
        self.assertIsInstance(local_interceptor, LocalRequestInterceptor, "Expected LocalRequestInterceptor for local mode")
        mock_instance_local.getString.assert_called_once_with(DOCS_ROOT_KEY, True)
        mock_ConfigService.Instance.reset_mock()

        mock_instance_online = mock_ConfigService.Instance.return_value
        mock_instance_online.getString.return_value = None
        online_model = HelpWindowModel(online_base=ONLINE_BASE_EXAMPLE)
        online_interceptor = online_model.create_request_interceptor()
        self.assertIsInstance(online_interceptor, NoOpRequestInterceptor, "Expected NoOpRequestInterceptor for online mode")
        mock_instance_online.getString.assert_called_once_with(DOCS_ROOT_KEY, True)
        mock_ConfigService.Instance.assert_called_once()

    @patch(CONFIG_SERVICE_LOOKUP_PATH)
    @patch(VERSION_FUNC_LOOKUP_PATH, return_value=None)
    def test_online_url_construction_with_trailing_slashes(self, mock_version_func, mock_ConfigService):
        """Test online base URL construction handles trailing slashes correctly (forced online)."""
        mock_instance = mock_ConfigService.Instance.return_value
        mock_instance.getString.return_value = None

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
        mock_ConfigService.Instance.assert_called()
        mock_instance.getString.assert_called()


if __name__ == "__main__":
    unittest.main()
