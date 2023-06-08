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

import datetime, os, sys, hashlib, tempfile, glob, shutil

# A fixed time used for testing - 3:00pm today
now = datetime.datetime.now()
TIME_3PM = int(datetime.datetime(now.year, now.month, now.day, 15, 0, 0).timestamp())


class CleanFileCache(unittest.TestCase):
    def test1(self):
        """CleanFileCache: simple test with two cache files and two normal files"""
        # create a temporary directory with fake cache files
        # and other files
        cache_root = tempfile.mkdtemp()
        _hash = lambda s: hashlib.sha1(s).hexdigest()
        cache1, _ = CreateCacheFilename(CacheDir=cache_root, OtherProperties=["A=1", "B=2"])
        cache2, _ = CreateCacheFilename(
            CacheDir=cache_root,
            OtherProperties=["C=3"],
        )
        touch(cache1)
        touch(cache2)
        non_cache = [os.path.join(cache_root, f) for f in ["normal1.txt", "normal2.dat"]]
        for p in non_cache:
            touch(p)
        # print glob.glob(os.path.join(cache_root, '*'))
        # Execute
        code = "CleanFileCache(CacheDir = %r, AgeInDays = 0)" % cache_root
        code = "from mantid.simpleapi import CleanFileCache; %s" % code
        cmd = '%s -c "%s"' % (sys.executable, code)
        if os.system(cmd):
            raise RuntimeError("Failed to excute %s" % cmd)

        files_remained = glob.glob(os.path.join(cache_root, "*"))

        self.assertEqual(set(files_remained), set(non_cache))

    def test2(self):
        """CleanFileCache: 'normal' files with 39 and 41-character filenames etc"""
        # create a temporary directory with fake cache files
        # and other files
        cache_root = tempfile.mkdtemp()
        _hash = lambda s: hashlib.sha1(s).hexdigest()
        cache1, _ = CreateCacheFilename(CacheDir=cache_root, OtherProperties=["A=1"])
        cache2, _ = CreateCacheFilename(
            CacheDir=cache_root,
            OtherProperties=["B='silly'"],
        )
        touch(cache1)
        touch(cache2)
        non_cache = [
            os.path.join(cache_root, f)
            for f in [
                "a" * 39 + ".nxs",
                "0" * 41 + ".nxs",
                "alpha_" + "b" * 39 + ".nxs",
                "beta_" + "e" * 41 + ".nxs",
            ]
        ]
        for p in non_cache:
            touch(p)
        # print glob.glob(os.path.join(cache_root, '*'))
        # Execute
        code = "CleanFileCache(CacheDir = %r, AgeInDays = 0)" % cache_root
        code = "from mantid.simpleapi import CleanFileCache; %s" % code
        cmd = '%s -c "%s"' % (sys.executable, code)
        if os.system(cmd):
            raise RuntimeError("Failed to excute %s" % cmd)

        files_remained = glob.glob(os.path.join(cache_root, "*"))

        self.assertEqual(set(files_remained), set(non_cache))

    def test3(self):
        """CleanFileCache: "age" parameter"""
        age = 10
        # create a temporary directory with fake cache files
        # and other files
        cache_root = tempfile.mkdtemp()
        _hash = lambda s: hashlib.sha1(s).hexdigest()
        cache1, _ = CreateCacheFilename(CacheDir=cache_root, OtherProperties=["A=newer"])
        cache2, _ = CreateCacheFilename(
            CacheDir=cache_root,
            OtherProperties=["B=rightonedge"],
        )
        cache3, _ = CreateCacheFilename(
            CacheDir=cache_root,
            OtherProperties=["C=old"],
        )
        print(f"cache1={cache1}", flush=True)
        createFile(cache1, age - 1, display=True)
        print(f"cache2={cache2}", flush=True)
        createFile(cache2, age, display=True)
        print(f"cache3={cache3}", flush=True)
        createFile(cache3, age + 1, display=True)
        non_cache = [
            os.path.join(cache_root, f)
            for f in [
                "a" * 39 + ".nxs",
                "0" * 41 + ".nxs",
                "alpha_" + "b" * 39 + ".nxs",
                "beta_" + "e" * 41 + ".nxs",
            ]
        ]
        for p in non_cache:
            touch(p)
        print("Cache files:", flush=True)
        print(glob.glob(os.path.join(cache_root, "*")), flush=True)

        # Execute
        code = "CleanFileCache(CacheDir = %r, AgeInDays = %s)" % (cache_root, age)
        code = "from mantid.simpleapi import CleanFileCache; %s" % code
        cmd = '%s -c "%s"' % (sys.executable, code)
        if os.system(cmd):
            raise RuntimeError("Failed to excute %s" % cmd)

        files_remained = glob.glob(os.path.join(cache_root, "*"))
        self.assertEqual(set(files_remained), set(non_cache + [cache1]))


def createFile(f, daysbefore, display=False):
    "create a file and set modify time at n=daysbefore days before TIME_3PM"
    touch(f)
    t = computeTime(daysbefore)
    if display:
        print(f"  Computed modification time: {t}")
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
