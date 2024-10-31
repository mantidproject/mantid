# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=invalid-name,too-many-public-methods,too-many-arguments
import unittest
from pathlib import Path

from mantid.kernel import ConfigService, PropertyManager
from testhelpers import run_algorithm

import os
import mantid
import hashlib


class CreateCacheFilename(unittest.TestCase):
    def test1(self):
        """CreateCacheFilename: one prop"""
        pm = PropertyManager()
        pm.declareProperty("a", 0)
        pm.setProperty("a", 3)
        mantid.PropertyManagerDataService.add("pm", pm)
        # Execute
        alg_test = run_algorithm(
            "CreateCacheFilename",
            PropertyManager="pm",
            Properties=[],
            OtherProperties=[],
            Prefix="",
            CacheDir="",
        )
        # executed?
        self.assertTrue(alg_test.isExecuted())
        # Verify ....
        expected = os.path.join(ConfigService.getUserPropertiesDir(), "cache", "%s.nxs" % hashlib.sha1("a=3".encode("utf-8")).hexdigest())
        self.assertEqual(alg_test.getPropertyValue("OutputFilename"), expected)

        # Another test. don't specify the default values
        alg_test = run_algorithm(
            "CreateCacheFilename",
            PropertyManager="pm",
        )
        # executed?
        self.assertTrue(alg_test.isExecuted())
        # Verify ....
        expected = os.path.join(ConfigService.getUserPropertiesDir(), "cache", "%s.nxs" % hashlib.sha1("a=3".encode("utf-8")).hexdigest())
        self.assertEqual(alg_test.getPropertyValue("OutputFilename"), expected)
        return

    def test_wronginput(self):
        """CreateCacheFilename: wrong input"""
        # Execute
        with self.assertRaisesRegex(RuntimeError, "Either PropertyManager or OtherProperties should be supplied"):
            run_algorithm(
                "CreateCacheFilename",
            )

    def test_glob(self):
        """CreateCacheFilename: globbing"""
        # glob pattern search anything with 'a' in it
        # and leave other props out
        pm = PropertyManager()
        aprops = ["a", "alibaba", "taa", "sa", "a75"]
        props = aprops + ["b", "c", "d"]
        for p in props:
            pm.declareProperty(p, 0)
            pm.setProperty(p, 3)
            continue
        mantid.PropertyManagerDataService.add("test_glob", pm)
        # Execute
        alg_test = run_algorithm(
            "CreateCacheFilename",
            PropertyManager="test_glob",
            Properties=["*a*"],
        )
        # executed?
        self.assertTrue(alg_test.isExecuted())
        # Verify ....
        s = ",".join(sorted(["%s=3" % p for p in aprops]))
        expected = os.path.join(ConfigService.getUserPropertiesDir(), "cache", "%s.nxs" % hashlib.sha1(s.encode("utf-8")).hexdigest())
        self.assertEqual(alg_test.getPropertyValue("OutputFilename"), expected)
        return

    def test_otherprops_only(self):
        """CreateCacheFilename: other_props only"""
        # Execute
        alg_test = run_algorithm(
            "CreateCacheFilename",
            OtherProperties=["a=1", "b=2"],
        )
        # executed?
        self.assertTrue(alg_test.isExecuted())
        # Verify ....
        expected = os.path.join(
            ConfigService.getUserPropertiesDir(), "cache", "%s.nxs" % hashlib.sha1("a=1,b=2".encode("utf-8")).hexdigest()
        )
        self.assertEqual(alg_test.getPropertyValue("OutputFilename"), expected)
        return

    def test_bothprops(self):
        """CreateCacheFilename: use both PropertyManager and OtherProperties"""
        pm = PropertyManager()
        aprops = ["a", "alibaba", "taa", "sa", "a75"]
        props = aprops + ["b", "c", "d"]
        for p in props:
            pm.declareProperty(p, "")
            pm.setProperty(p, "fish")
            continue
        mantid.PropertyManagerDataService.add("test_bothprops", pm)
        other_props = ["A=1", "B=2"]
        # Execute
        alg_test = run_algorithm(
            "CreateCacheFilename",
            PropertyManager="test_bothprops",
            Properties=["*a*"],
            OtherProperties=other_props,
        )
        # executed?
        self.assertTrue(alg_test.isExecuted())
        # Verify ....
        s = ",".join(sorted(["%s=fish" % p for p in aprops] + other_props))
        expected = os.path.join(ConfigService.getUserPropertiesDir(), "cache", "%s.nxs" % hashlib.sha1(s.encode("utf-8")).hexdigest())
        self.assertEqual(alg_test.getPropertyValue("OutputFilename"), expected)
        return

    def test_prefix(self):
        """CreateCacheFilename: prefix"""
        # Execute
        alg_test = run_algorithm(
            "CreateCacheFilename",
            OtherProperties=["a=1", "b=2"],
            Prefix="vanadium",
        )
        # executed?
        self.assertTrue(alg_test.isExecuted())
        # Verify ....
        expected = os.path.join(
            ConfigService.getUserPropertiesDir(), "cache", "vanadium_%s.nxs" % hashlib.sha1("a=1,b=2".encode("utf-8")).hexdigest()
        )
        self.assertEqual(alg_test.getPropertyValue("OutputFilename"), expected)
        return

    def test_cache_dir(self):
        """CreateCacheFilename: cache_dir"""
        # Execute
        alg_test = run_algorithm(
            "CreateCacheFilename",
            OtherProperties=["a=1", "b=2"],
            CacheDir="my_cache",
        )
        # executed?
        self.assertTrue(alg_test.isExecuted())
        # Verify ....
        expected = f"{hashlib.sha1('a=1,b=2'.encode('utf-8')).hexdigest()}.nxs"
        self.assertEqual(Path(alg_test.getPropertyValue("OutputFilename")).name, expected)
        return


if __name__ == "__main__":
    unittest.main()
