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
        out_path = "tempout_curve.json"
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
        return


if __name__ == '__main__':
    unittest.main()
