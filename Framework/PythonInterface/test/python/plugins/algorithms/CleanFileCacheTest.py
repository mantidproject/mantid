# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=invalid-name,too-many-public-methods,too-many-arguments,multiple-statements
import unittest

from datetime import datetime
from glob import glob
from os import path, utime
from shutil import rmtree
from tempfile import mkdtemp

from mantid.simpleapi import CreateCacheFilename

# A fixed time used for testing - 3:00pm today
now = datetime.now()
TIME_3PM = int(datetime(now.year, now.month, now.day, 15, 0, 0).timestamp())


class CleanFileCache(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        # Make a temporary directory
        cls._cache_root = mkdtemp()

        # Filepaths which are close to the cache file regex containing 40 characters
        cls._non_cache_filepaths = [
            path.join(cls._cache_root, f)
            for f in [
                "a" * 39 + ".nxs",
                "0" * 41 + ".nxs",
                "alpha_" + "b" * 39 + ".nxs",
                "beta_" + "e" * 41 + ".nxs",
            ]
        ]
        for filepath in cls._non_cache_filepaths:
            _write_file(filepath)

        cls._cache_file1, _ = CreateCacheFilename(CacheDir=cls._cache_root, OtherProperties=["A=first"])
        cls._cache_file2, _ = CreateCacheFilename(CacheDir=cls._cache_root, OtherProperties=["B=second"])
        cls._cache_file3, _ = CreateCacheFilename(CacheDir=cls._cache_root, OtherProperties=["C=third"])

    @classmethod
    def tearDownClass(cls) -> None:
        # Remove temporary directory
        rmtree(cls._cache_root)

    def test_clean_cache_will_not_clear_non_cache_files(self):
        _write_file(self._cache_file1)
        _write_file(self._cache_file2)

        _execute_clean_cache(self._cache_root, 0)

        files_remained = glob(path.join(self._cache_root, "*"))
        self.assertEqual(set(files_remained), set(self._non_cache_filepaths))

    def test_clean_cache_will_not_clear_a_file_older_than_an_age_limit(self):
        age = 10

        _write_file(self._cache_file1, age - 1)
        _write_file(self._cache_file2, age)
        _write_file(self._cache_file3, age + 1)

        _execute_clean_cache(self._cache_root, age)

        files_remained = glob(path.join(self._cache_root, "*"))
        self.assertEqual(set(files_remained), set(self._non_cache_filepaths + [self._cache_file1]))


def _execute_clean_cache(cache_root: str, age: int):
    from mantid.simpleapi import CleanFileCache

    CleanFileCache(CacheDir=cache_root, AgeInDays=age)


def _write_file(filepath: str, days_before: int = None) -> None:
    with open(filepath, "w") as stream:
        stream.write("\n")

    if days_before is not None:
        # Convert to seconds before 3pm today
        t = TIME_3PM - days_before * 24 * 60 * 60
        utime(filepath, (t, t))


if __name__ == "__main__":
    unittest.main()
