# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

# package imports
from mantid.api import PythonAlgorithm
from mantid.kernel import ConfigService, logger
from mantid.utils.deprecator import deprecated_algorithm
from mantid.utils.logging import capture_logs

# standard imports
from contextlib import contextmanager
import os
import sys
import tempfile
import unittest


class deprecatorTest(unittest.TestCase):

    def test_deprecated_algorithm(self):

        @deprecated_algorithm('MyNewAlg', '2020-12-25')
        class MyOldAlg(PythonAlgorithm):

            def category(self):
                return 'Inelastic\\Reduction'

            def PyInit(self):
                self.declareProperty('Meaning', 42, 'Assign a meaning to the Universe')

            def PyExec(self):
                logger.notice(f'The meaning of the Universe is {self.getPropertyValue("Meaning")}')

        with capture_logs(level="notice") as logs:
            alg = MyOldAlg()
            alg.initialize()
            alg.setProperty("Meaning", 42)

            config = ConfigService.Instance()
            config['algorithms.deprecated'] = 'Raise'
            alg.execute()
            self.assertTrue('Error in execution of algorithm MyOldAlg' in logs.getvalue())

            config['algorithms.deprecated'] = 'Log'
            alg.execute()  # use alias, log error message
            self.assertTrue(' Algorithm "MyOldAlg" is deprecated' in logs.getvalue())


if __name__ == "__main__":
    unittest.main()
