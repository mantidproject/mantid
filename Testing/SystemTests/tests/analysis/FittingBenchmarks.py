"""
This systemtest tests that the fit executes with different minimizers for the fitting test problems used for benchmarking minimizers.

Note the complete benchmarking suite is not tested here since I find this takes too long, but enough to provide
an additional health test of Mantid fitting and to test that the scripts used for fit minimizer benchmarking are working.

Fitting problem are stored as follows in the FittingTestProblems directory:

        CUTEst/
        NIST_nonlinear_regression/
        Neutron_data/

For more information what these problems are see the Mantid FittingMinimizers concept page.
"""
from __future__ import (absolute_import, division, print_function)

import unittest
import stresstesting

import os

import mantid.simpleapi as msapi
import fitting_benchmarking as fitbk
import results_output as fitout


class FittingBenchmarkTests(unittest.TestCase):

    def setUp(self):
        self.color_scale = [(1.1, 'ranking-top-1'),
                            (1.33, 'ranking-top-2'),
                            (1.75, 'ranking-med-3'),
                            (3, 'ranking-low-4'),
                            (float('nan'), 'ranking-low-5')
                            ]

        # Create the path for the specific fitting test files location
        input_data_dir = msapi.config['datasearch.directories'].split(';')[0]
        self.base_problem_files_dir = os.path.join(input_data_dir, 'FittingTestProblems')

    def test_all_nist_problem(self):
        """
        Runs benchmark on all NIST problems on about half of the available Mantid minimizers
        and without using weights.
        """

        minimizers = ['BFGS', 'Conjugate gradient (Fletcher-Reeves imp.)',
                      'Conjugate gradient (Polak-Ribiere imp.)', 'Damped GaussNewton']
        group_names = ['NIST, "lower" difficulty', 'NIST, "average" difficulty', 'NIST, "higher" difficulty']
        group_suffix_names = ['nist_lower', 'nist_average', 'nist_higher']

        use_errors = False

        nist_group_dir = os.path.join(self.base_problem_files_dir, 'NIST_nonlinear_regression')

        # test that fit executes on all NIST problems against a number of different minimizers
        problems, results_per_group = fitbk.do_fitting_benchmark(nist_group_dir=nist_group_dir,
                                                                 minimizers=minimizers, use_errors=use_errors)

        # test that can print individual group tables
        for idx, group_results in enumerate(results_per_group):
            print("\n\n")
            print("********************************************************")
            print("**************** RESULTS FOR GROUP {0}, {1} ************".format(idx+1,
                                                                                    group_names[idx]))
            print("********************************************************")
            fitout.print_group_results_tables(minimizers, group_results, problems[idx],
                                              group_name=group_suffix_names[idx], use_errors=use_errors,
                                              simple_text=True, rst=True, save_to_file=False,
                                              color_scale=self.color_scale)

        # test that can print summary tables
        header = '\n\n**************** OVERALL SUMMARY - ALL GROUPS ******** \n\n'
        print(header)
        fitout.print_overall_results_table(minimizers, results_per_group, problems, group_names,
                                           use_errors=use_errors, save_to_file=False)

    def test_all_cutest_problem(self):
        """
        Runs benchmark on all CUTEst problems on about half of the available Mantid minimizers
        and with using weights.
        """
        minimizers = ['Levenberg-Marquardt', 'Levenberg-MarquardtMD',
                      'Simplex', 'SteepestDescent', 'Trust Region']
        group_suffix_names = ['cutest']

        use_errors = True

        cutest_group_dir = os.path.join(self.base_problem_files_dir, 'CUTEst')

        # test that fit executes on all NIST problems against a number of different minimizers
        problems, results_per_group = fitbk.do_fitting_benchmark(cutest_group_dir=cutest_group_dir,
                                                                 minimizers=minimizers, use_errors=use_errors)

        # test that can print individual group table
        for idx, group_results in enumerate(results_per_group):
            fitout.print_group_results_tables(minimizers, group_results, problems[idx],
                                              group_name=group_suffix_names[idx], use_errors=use_errors,
                                              simple_text=True, rst=True, save_to_file=False,
                                              color_scale=self.color_scale)

    def test_all_neutron_data_problem(self):
        """
        Runs benchmark on all neutron data problems on one minimizers
        and using weights.
        """
        minimizers = ['Trust Region']
        group_suffix_names = ['neutron_data']

        use_errors = True

        neutron_data_group_dirs = [os.path.join(self.base_problem_files_dir, 'Neutron_data')]

        # test that fit executes on all Neutron data problems
        problems, results_per_group = fitbk.do_fitting_benchmark(neutron_data_group_dirs=neutron_data_group_dirs,
                                                                 minimizers=minimizers, use_errors=use_errors)

        # test that can print individual group table
        for idx, group_results in enumerate(results_per_group):
            fitout.print_group_results_tables(minimizers, group_results, problems[idx],
                                              group_name=group_suffix_names[idx], use_errors=use_errors,
                                              simple_text=True, rst=True, save_to_file=False,
                                              color_scale=self.color_scale)


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
