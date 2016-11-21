"""
Benchmarks for accuracy and time.

This systemtest tests that the fitting executes with different minimizers for the fitting tests used for comparing 
minimizers, and tests the scripts used for this are working
"""
from __future__ import (absolute_import, division, print_function)

import unittest
import stresstesting

import os

import mantid.simpleapi as msapi
import fitting_benchmarking as fitbk

#pylint: disable=too-many-public-methods
class FittingBenchmarkTests(unittest.TestCase):

    def setUp(self):
        self.minimizers = ['BFGS', 'Conjugate gradient (Fletcher-Reeves imp.)',
                                    'Conjugate gradient (Polak-Ribiere imp.)', 'Damping',
                                    #'FABADA', # Note the mantid FABADA Bayesian minimizer is not local minimization mimimizer   
                                    'Levenberg-Marquardt', 'Levenberg-MarquardtMD',
                                    'Simplex', 'SteepestDescent', 'Trust Region']
        self.group_names = ['NIST, "lower" difficulty', 'NIST, "average" difficulty', 'NIST, "higher" difficulty', "CUTEst", "Neutron data"]
        self.group_suffix_names = ['nist_lower', 'nist_average', 'nist_higher', 'cutest', 'neutron_data']
        self.color_scale = [(1.1, 'ranking-top-1'),
                            (1.33, 'ranking-top-2'),
                            (1.75, 'ranking-med-3'),
                            (3, 'ranking-low-4'),
                            (float('nan'), 'ranking-low-5')
                           ]
        self.save_to_file = False  # print to standard out but don't save rst tables to files 

    def run_all_with_or_without_errors(self, use_errors):
        """
        Runs benchmark on all the available tes problems.
        
        @param use_errors : whether to use errors as weights in the cost funtion
                            (weighted least squares)
        """
        # Create the path for the specific fitting test files location
        input_data_dir = msapi.config['datasearch.directories'].split(';')[0]        
        base_problem_files_dir = os.path.join(input_data_dir, 'FittingTestProblems')
        
        # run fit minimizer benchmarking tests
        fitbk.run_all_with_or_without_errors(base_problem_files_dir, use_errors, self.minimizers,
                                             self.group_names, self.group_suffix_names, self.color_scale, 
                                             self.save_to_file)

    def test_rank_accuracy_runtime_with_errors(self):
        self.run_all_with_or_without_errors(use_errors=True)

    def test_rank_accuracy_runtime_without_errors(self):
        self.run_all_with_or_without_errors(use_errors=False)


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
