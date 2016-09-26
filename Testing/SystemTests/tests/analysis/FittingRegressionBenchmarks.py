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
import results_output as fitout

def fitting_problems_all_nist_nlr():
    # 'reference/nist_nonlinear_regression/DanWood.dat'
    file_paths = ['']

    return file_paths

def fitting_problems_cutest_selected():
    # 'reference/cutest/PALMER1.dat'
    file_paths = ['']


def fitting_problem_test_files():
    file_paths = fitting_problems_all_nist_nlr()
    file_paths.extend(fitting_problems_cutest_selected())

    return file_paths;

#pylint: disable=too-many-public-methods
class FittingBenchmarkTests(unittest.TestCase):

    def setUp(self):
        self.use_errors = True
        # result = fitting.RunProblems.do_calc()
        # TO-DO related to this: expose API::FuncMinimizerFactory to Python
        # TOTHINK: use different interface as in the API::FunctionFactory?
        # But still, do we want to enforce a particular ordering for the tables?
        minimizers_pseudofactory = ['BFGS', 'Conjugate gradient (Fletcher-Reeves imp.)',
                                    'Conjugate gradient (Polak-Ribiere imp.)', 'Damping',
                                    #'FABADA', # hide FABADA
                                    'Levenberg-Marquardt', 'Levenberg-MarquardtMD',
                                    'Simplex', 'SteepestDescent', 'DTRS']
        self.minimizers = minimizers_pseudofactory
        self.group_names = ['NIST, "lower" difficulty', 'NIST, "average" difficulty', 'NIST, "higher" difficulty', "Neutron data", "CUTEst"]
        self.group_suffix_names = ['nist_lower', 'nist_average', 'nist_higher', 'cutest', 'neutron_data']
        self.color_scale = [(1.1, 'ranking-top-1'), (1.33, 'ranking-top-2'), (1.75, 'ranking-med-3'), (3, 'ranking-low-4'), (float('nan'), 'ranking-low-5')]

    # Rename this to what it does/can do
    def test_rank_by_chi2_and_runtime_with_error_weights(self):
        """
        Run all benchmark problems, with weights (observational errors)
        """

        problems, results_per_group = fitbk.do_regression_fitting_benchmark(include_nist=True, include_cutest=True,
                                                                            data_groups_dirs = ['/home/fedemp/mantid-repos/mantid-fitting-systest/scripts/Fitting/test_examples/FittingNeutronData'],
                                                                            minimizers=self.minimizers, use_errors=True)


        for idx, group_results in enumerate(results_per_group):
            print("\n\n")
            print("********************************************************".format(idx+1))
            print("**************** RESULTS FOR GROUP {0}, {1} ************".format(idx+1,
                                                                                    self.group_names[idx]))
            print("********************************************************".format(idx+1))
            fitout.print_group_results_tables(self.minimizers, group_results, problems[idx],
                                              group_name=self.group_suffix_names[idx],
                                              use_errors=self.use_errors,
                                              simple_text=True, rst=True, color_scale=self.color_scale)

        header = "\n\n"
        header += '**************** OVERALL SUMMARY - ALL GROUPS ******** \n\n'
        print(header)
        fitout.print_overall_results_table(self.minimizers, results_per_group, problems, self.group_names,
                                           use_errors=self.use_errors, rst=True)

        # Flush to prevent mix-up with system tests/runner output
        import sys
        sys.stdout.flush()

    def test_rank_by_chi2_and_runtime_without_error_weights(self):
        """
        Run all benchmark problems, without weights (observational errors)
        """
        problems, results_per_group = fitbk.do_regression_fitting_benchmark(include_nist=True, include_cutest=True,
                                                                            data_groups_dirs = ['/home/fedemp/mantid-repos/mantid-fitting-systest/scripts/Fitting/test_examples/FittingNeutronData'],
                                                                            minimizers=self.minimizers, use_errors=False)

        for idx, group_results in enumerate(results_per_group):
            print("\n\n")
            print("********************************************************".format(idx+1))
            print("**************** RESULTS FOR GROUP {0}, {1} ************".format(idx+1,
                                                                                    self.group_names[idx]))
            print("********************************************************".format(idx+1))
            fitout.print_group_results_tables(self.minimizers, group_results, problems[idx],
                                              group_name=self.group_suffix_names[idx],
                                              use_errors=False,
                                              simple_text=True, rst=True, color_scale=self.color_scale)

        sys.stdout.flush()

# Run the unittest tests defined above as a Mantid system test
class FittingRegressionBenchmars(stresstesting.MantidStressTest):

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
