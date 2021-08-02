# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

from mantid.api import MatrixWorkspace
from testhelpers import create_algorithm


class ReflectometryISISLoadAndProcessTest(unittest.TestCase):
    ALG_NAME = "ReflectometryISISPreprocess"

    def test_algorithm_executes(self):
        alg = create_algorithm(self.ALG_NAME)
        alg.execute()
        self.assertTrue(alg.isExecuted())

    def test_input_run_is_loaded(self):
        args = {'InputRunList': '13460'}
        alg = create_algorithm(self.ALG_NAME, **args)
        alg.execute()
        output_ws = alg.getProperty("OutputWorkspace")
        self.assertIsInstance(MatrixWorkspace, output_ws)


if __name__ == '__main__':
    unittest.main()
