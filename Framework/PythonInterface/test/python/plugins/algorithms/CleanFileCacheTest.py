# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=invalid-name,too-many-public-methods,too-many-arguments,multiple-statements
import unittest
from mantid.kernel import *
from mantid.api import *
from mantid.simpleapi import CreateCacheFilename

import datetime, os, tempfile, glob, shutil

# A fixed time used for testing - 3:00pm today
now = datetime.datetime.now()
TIME_3PM = int(datetime.datetime(now.year, now.month, now.day, 15, 0, 0).timestamp())


class CleanFileCache(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        cls._cache_root = tempfile.mkdtemp()

        cls._non_cache_filepaths = [
            os.path.join(cls._cache_root, f)
            for f in [
                "a" * 39 + ".nxs",
                "0" * 41 + ".nxs",
                "alpha_" + "b" * 39 + ".nxs",
                "beta_" + "e" * 41 + ".nxs",
            ]
        ]
        for filepath in cls._non_cache_filepaths:
            touch(filepath)

    @classmethod
    def tearDownClass(cls) -> None:
        shutil.rmtree(cls._cache_root)

    def test2(self):
        """CleanFileCache: 'normal' files with 39 and 41-character filenames etc"""
        cache1, _ = CreateCacheFilename(CacheDir=self._cache_root, OtherProperties=["A=1"])
        cache2, _ = CreateCacheFilename(
            CacheDir=self._cache_root,
            OtherProperties=["B='silly'"],
        )

        execute_clean_cache(self._cache_root, 0)

        files_remained = glob.glob(os.path.join(self._cache_root, "*"))

        self.assertEqual(set(files_remained), set(self._non_cache_filepaths))

    def test3(self):
        """CleanFileCache: "age" parameter"""
        age = 10

        cache1, _ = CreateCacheFilename(CacheDir=self._cache_root, OtherProperties=["A=newer"])
        cache2, _ = CreateCacheFilename(
            CacheDir=self._cache_root,
            OtherProperties=["B=rightonedge"],
        )
        cache3, _ = CreateCacheFilename(
            CacheDir=self._cache_root,
            OtherProperties=["C=old"],
        )
        createFile(cache1, age - 1)
        createFile(cache2, age)
        createFile(cache3, age + 1)

        execute_clean_cache(self._cache_root, age)

        files_remained = glob.glob(os.path.join(self._cache_root, "*"))
        self.assertEqual(set(files_remained), set(self._non_cache_filepaths + [cache1]))


def execute_clean_cache(cache_root: str, age: int):
    from mantid.simpleapi import CleanFileCache

    CleanFileCache(CacheDir=cache_root, AgeInDays=age)


def createFile(f, daysbefore):
    "create a file and set modify time at n=daysbefore days before TIME_3PM"
    touch(f)
    t = computeTime(daysbefore)
    os.utime(f, (t, t))
    return


def computeTime(daysbefore):
    "compute time as float of the time at n=daysbefore days before today"
    return TIME_3PM - daysbefore * 24 * 60 * 60


def touch(f):
    with open(f, "w") as stream:
        stream.write("\n")
    return


if __name__ == "__main__":
    unittest.main()
