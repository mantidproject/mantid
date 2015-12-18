#pylint: disable=invalid-name,too-many-public-methods,too-many-arguments
import unittest
import numpy as np
import mantid.simpleapi as api
from mantid.kernel import *
from mantid.api import *
from testhelpers import run_algorithm
from mantid.api import AnalysisDataService

import os, mantid, hashlib

class CreateCacheFilename(unittest.TestCase):

    def test1(self):
        """CreateCacheFilename: one prop 
        """
        pm = PropertyManager()
        pm.declareProperty("a", 0)
        pm.setProperty("a", 3)
        mantid.PropertyManagerDataService.add("pm", pm)
        
        # Execute
        alg_test = run_algorithm(
            "CreateCacheFilename",
            PropertyManager = "pm",
            Properties = [],
            OtherProperties = [],
            Prefix = "",
            CacheDir = "",
            )
        # executed?
        self.assertTrue(alg_test.isExecuted())
        # Verify ....
        expected = os.path.join(
            ConfigService.getUserPropertiesDir(), "cache",
            "%s.nxs" % hashlib.sha1("a=3").hexdigest()
            )
        self.assertEqual(
            alg_test.getPropertyValue("OutputFilename"),
            expected)

        # Another test. don't specify the default values
        alg_test = run_algorithm(
            "CreateCacheFilename",
            PropertyManager = "pm",
            )
        # executed?
        self.assertTrue(alg_test.isExecuted())
        # Verify ....
        expected = os.path.join(
            ConfigService.getUserPropertiesDir(), "cache",
            "%s.nxs" % hashlib.sha1("a=3").hexdigest()
            )
        self.assertEqual(
            alg_test.getPropertyValue("OutputFilename"),
            expected)
        return

    def test_wronginput(self):
        """CreateCacheFilename: wrong input
        """
        # Execute
        alg_test = run_algorithm(
            "CreateCacheFilename",
            )
        # executed?
        self.assertFalse(alg_test.isExecuted())
        return

    def test_glob(self):
        """CreateCacheFilename: globbing
        """
        # glob pattern search anything with 'a' in it
        # and leave other props out
        pm = PropertyManager()
        aprops = ["a", "alibaba", "taa", "sa", "a75"]
        props = aprops + ['b', 'c', 'd']
        for p in props:
            pm.declareProperty(p, 0)
            pm.setProperty(p, 3)
            continue
        mantid.PropertyManagerDataService.add("test_glob", pm)
        
        # Execute
        alg_test = run_algorithm(
            "CreateCacheFilename",
            PropertyManager = "test_glob",
            Properties = ['*a*'],
            )
        # executed?
        self.assertTrue(alg_test.isExecuted())
        # Verify ....
        s = ','.join(sorted( ['%s=3' % p for p in aprops] ))
        expected = os.path.join(
            ConfigService.getUserPropertiesDir(), "cache",
            "%s.nxs" % hashlib.sha1(s).hexdigest()
            )
        self.assertEqual(
            alg_test.getPropertyValue("OutputFilename"),
            expected)
        return

    def test_otherprops_only(self):
        """CreateCacheFilename: other_props only
        """
        # Execute
        alg_test = run_algorithm(
            "CreateCacheFilename",
            OtherProperties = ["a=1", "b=2"],
            )
        # executed?
        self.assertTrue(alg_test.isExecuted())
        # Verify ....
        expected = os.path.join(
            ConfigService.getUserPropertiesDir(), "cache",
            "%s.nxs" % hashlib.sha1("a=1,b=2").hexdigest()
            )
        self.assertEqual(
            alg_test.getPropertyValue("OutputFilename"),
            expected)
        return

    def test_prefix(self):
        """CreateCacheFilename: prefix
        """
        # Execute
        alg_test = run_algorithm(
            "CreateCacheFilename",
            OtherProperties = ["a=1", "b=2"],
            Prefix = "vanadium",
            )
        # executed?
        self.assertTrue(alg_test.isExecuted())
        # Verify ....
        expected = os.path.join(
            ConfigService.getUserPropertiesDir(), "cache",
            "vanadium_%s.nxs" % hashlib.sha1("a=1,b=2").hexdigest()
            )
        self.assertEqual(
            alg_test.getPropertyValue("OutputFilename"),
            expected)
        return


if __name__ == '__main__':
    unittest.main()
