# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.
import os
import tempfile
import shutil
import unittest

from mantidqt.widgets.helpwindow.helpwindowmodel import HelpWindowModel


class TestHelpWindowModel(unittest.TestCase):
    def setUp(self):
        # We can create a temporary directory to simulate "local_docs_base" that actually exists
        self.temp_dir = tempfile.mkdtemp()

    def tearDown(self):
        # Clean up any temporary directories
        shutil.rmtree(self.temp_dir, ignore_errors=True)

    def test_init_with_local_docs_configures_for_local(self):
        model = HelpWindowModel(local_docs_base=self.temp_dir, online_base="https://example.org")

        # The model should see this as local
        self.assertTrue(model.is_local_docs_enabled(), "Expected is_local_docs_enabled() to be True for a valid folder")

        # build_help_url should produce a file:// URL
        test_url = model.build_help_url("algorithms/MyAlgorithm-v1.html")
        self.assertTrue(test_url.isLocalFile(), "Expected a local file:// URL")

        # home url also should be local
        home_url = model.get_home_url()
        self.assertTrue(home_url.isLocalFile(), "Expected a local file:// for home URL")

    def test_init_with_no_local_docs_uses_online(self):
        model = HelpWindowModel(local_docs_base=None, online_base="https://example.org")

        # The model should see this as NOT local
        self.assertFalse(model.is_local_docs_enabled(), "Expected is_local_docs_enabled() to be False")

        # build_help_url should produce an https://example.org/ URL
        test_url = model.build_help_url("algorithms/MyAlgorithm-v1.html")
        self.assertTrue(test_url.toString().startswith("https://example.org/algorithms/"), "Expected an online docs URL")

        # home url also should be online
        home_url = model.get_home_url()
        self.assertTrue("example.org/index.html" in home_url.toString(), "Expected an online docs home URL")

    def test_init_with_bad_local_raises_exception(self):
        # Create a path we know doesn't exist anymore
        invalid_path = os.path.join(self.temp_dir, "non_existent")
        # We do NOT create that subfolder

        with self.assertRaises(ValueError):
            HelpWindowModel(local_docs_base=invalid_path, online_base="https://example.org")


if __name__ == "__main__":
    unittest.main()
