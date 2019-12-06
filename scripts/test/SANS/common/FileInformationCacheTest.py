# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from mock import patch

from sans.common.FileInformationCache import FileInformationCache, CachedRequest


class FileInformationCacheTest(unittest.TestCase):

    def test_returns_none_when_not_found(self):
        instance = FileInformationCache(max_len=2)
        self.assertIsNone(instance.get_element("FileNotFound.nxs"))

    @patch("os.path")
    def test_returns_cached_when_still_there(self, mocked_path):
        instance = FileInformationCache(max_len=1)

        mocked_path.exists.return_value = True

        requested_name = "test.nxs"
        expected_path = "fake_path/test.nxs"
        found_run = CachedRequest(requested_name=requested_name, found_path=expected_path)

        instance.add_new_element(cached_request=found_run)

        self.assertEqual(instance.get_element(requested_name), found_run)

    @patch("os.path")
    def test_returns_cached_when_not_there(self, mocked_path):
        instance = FileInformationCache(max_len=1)

        mocked_path.exists.return_value = False

        requested_name = "test.nxs"
        expected_path = "fake_path/test.nxs"
        found_run = CachedRequest(requested_name=requested_name, found_path=expected_path)

        instance.add_new_element(cached_request=found_run)

        self.assertIsNone(instance.get_element(requested_name))

    @patch("os.path")
    def test_rotates_new_element_in(self, mocked_path):
        instance = FileInformationCache(max_len=1)

        mocked_path.exists.return_value = True

        requested_name = "test.nxs"
        expected_path = "fake_path/test.nxs"
        original_run = CachedRequest(requested_name="a", found_path="a")
        found_run = CachedRequest(requested_name=requested_name, found_path=expected_path)

        instance.add_new_element(cached_request=original_run)
        instance.add_new_element(cached_request=found_run)

        self.assertIsNone(instance.get_element("a"))  # Should rotate out
        self.assertEqual(instance.get_element(requested_name), found_run)

    @patch("os.path")
    def test_multiple_cache_entries_one_not_found(self, mocked_path):
        instance = FileInformationCache(max_len=2)
        mocked_path.exists.return_value = False

        requested_name = "test.nxs"
        expected_path = "fake_path/test.nxs"
        original_run = CachedRequest(requested_name="a", found_path="a")
        found_run = CachedRequest(requested_name=requested_name, found_path=expected_path)

        instance.add_new_element(cached_request=original_run)
        instance.add_new_element(cached_request=found_run)

        self.assertIsNone(instance.get_element("a"))  # Should not be found

        mocked_path.exists.return_value = True
        self.assertIsNone(instance.get_element("a"))
        self.assertEqual(instance.get_element(requested_name), found_run)

    @patch("os.path")
    def test_multiple_cache_entries_first_rotates_out(self, mocked_path):
        instance = FileInformationCache(max_len=2)
        mocked_path.exists.return_value = True

        requested_name = "test.nxs"
        expected_path = "fake_path/test.nxs"
        original_run = CachedRequest(requested_name="a", found_path="a")
        second_run = CachedRequest(requested_name="b", found_path="b")
        found_run = CachedRequest(requested_name=requested_name, found_path=expected_path)

        instance.add_new_element(cached_request=original_run)
        instance.add_new_element(cached_request=second_run)
        instance.add_new_element(cached_request=found_run)

        self.assertIsNone(instance.get_element("a"))  # Should not be found
        self.assertEqual(instance.get_element("b"), second_run)
        self.assertEqual(instance.get_element(requested_name), found_run)

    @patch("os.path")
    def test_multiple_cache_entries_second_rotated_out(self, mocked_path):
        instance = FileInformationCache(max_len=2)
        mocked_path.exists.return_value = True

        requested_name = "test.nxs"
        expected_path = "fake_path/test.nxs"
        original_run = CachedRequest(requested_name="a", found_path="a")
        second_run = CachedRequest(requested_name="b", found_path="b")
        found_run = CachedRequest(requested_name=requested_name, found_path=expected_path)

        instance.add_new_element(cached_request=original_run)
        instance.add_new_element(cached_request=second_run)

        instance.get_element("a")  # This should bump it to top of cache

        instance.add_new_element(cached_request=found_run)  # Thus this should knock off the second run

        self.assertIsNone(instance.get_element("b"))  # Should not be found
        self.assertEqual(instance.get_element("a"), original_run)
        self.assertEqual(instance.get_element(requested_name), found_run)

    @patch("os.path")
    @patch("sans.common.FileInformationCache.config")
    def test_changed_data_search_resets_cache(self, mocked_config, mocked_path):
        mocked_config.getDataSearchDirs.return_value = ["dir1", "dir2"]
        instance = FileInformationCache(max_len=2)
        mocked_path.exists.return_value = True

        original_run = CachedRequest(requested_name="a", found_path="a")
        second_run = CachedRequest(requested_name="b", found_path="b")

        instance.add_new_element(cached_request=original_run)
        instance.add_new_element(cached_request=second_run)

        mocked_config.getDataSearchDirs.return_value = ["dir1", "dir3"]

        self.assertIsNone(instance.get_element("a"))
        self.assertIsNone(instance.get_element("b"))


if __name__ == '__main__':
    unittest.main()
