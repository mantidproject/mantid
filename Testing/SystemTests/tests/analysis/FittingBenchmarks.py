"""
Benchmarks for accuracy (and time / iterations).
"""
from __future__ import (absolute_import, division, print_function)

import unittest
import stresstesting

import os
import time

import numpy as np

import mantid.simpleapi as msapi
import fitting_benchmarking as fitbk

#pylint: disable=too-many-public-methods
class FittingBenchmarkTests(unittest.TestCase):

    def setUp(self):
        # TO-DO related to this: expose API::FuncMinimizerFactory to Python
        # TOTHINK: use different interface as in the API::FunctionFactory?
        # But still, do we want to enforce a particular ordering for the tables?
        minimizers_pseudofactory = ['BFGS', 'Conjugate gradient (Fletcher-Reeves imp.)',
                                    'Conjugate gradient (Polak-Ribiere imp.)', 'Damping',
                                    #'FABADA', # hide FABADA
                                    'Levenberg-Marquardt', 'Levenberg-MarquardtMD',
                                    'Simplex', 'SteepestDescent', 'DTRS']
        self.minimizers = minimizers_pseudofactory
        self.group_names = ['NIST, "lower" difficulty', 'NIST, "average" difficulty', 'NIST, "higher" difficulty', "CUTEst", "Neutron data"]
        self.group_suffix_names = ['nist_lower', 'nist_average', 'nist_higher', 'cutest', 'neutron_data']
        self.color_scale = [(1.1, 'ranking-top-1'), (1.33, 'ranking-top-2'), (1.75, 'ranking-med-3'), (3, 'ranking-low-4'), (float('nan'), 'ranking-low-5')]


    def run_all_with_or_without_errors(self, use_errors):
        # pick data file from system tests path
        input_data_dir = msapi.config['datasearch.directories'].split(';')[0]
        # Look for the specific fitting test files location
        problem_files_path = os.path.join(input_data_dir, 'fitting_test_problems','Neutron_data')
        fitbk.run_all_with_or_without_errors([problem_files_path], use_errors, self.minimizers,
                                             self.group_names, self.group_suffix_names, self.color_scale)

    def test_rank_by_accuracy_and_runtime_with_error_weights(self):
        self.run_all_with_or_without_errors(True)

    def test_rank_by_accuracy_and_runtime_without_error_weights(self):
        self.run_all_with_or_without_errors(False)


# Run the unittest tests defined above as a Mantid system test
class FittingBenchmars(stresstesting.MantidStressTest):

    _success = False

    def __init__(self, *args, **kwargs):
        # super(FittingBenchmarkTests, self).__init__(*args, **kwargs)
        # old-style
        stresstesting.MantidStressTest.__init__(self, *args, **kwargs)
        self._success = False

    def requiredFile(self):
        return None

    def runTest(self):

        self._success = False
        # Custom code to create and run this single test suite
        suite = unittest.TestSuite()
        suite.addTest(unittest.makeSuite(FittingBenchmarkTests, "test") )
        runner = unittest.TextTestRunner()
        # Run and check success
        res = runner.run(suite)
        self._success = res.wasSuccessful()

    def validate(self):
        return self._success
