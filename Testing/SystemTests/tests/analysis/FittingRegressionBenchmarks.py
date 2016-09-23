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

BENCHMARK_VERSION_STR = 'v3.8'
FILENAME_SUFFIX_ACCURACY = '_acc'
FILENAME_SUFFIX_RUNTIME = '_runtime'

USE_ERRORS = True

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
        self.group_names = ['NIST, "lower" difficulty', 'NIST, "average" difficulty', 'NIST, "higher" difficulty', "CUTEst"]
        self.group_suffix_names = ['nist_lower', 'nist_average', 'nist_higher', 'cutest']

    # Rename this to what it does/can do
    def test_rank_by_chi2_and_runtime_with_error_weights(self):

        results_per_group = fitbk.do_regression_fitting_benchmark(include_nist=True, minimizers=self.minimizers, use_errors=USE_ERRORS)

        color_scale = [(1.1, 'ranking-top-1'), (1.33, 'ranking-top-2'), (1.75, 'ranking-med-3'), (3, 'ranking-low-4'), (float('nan'), 'ranking-low-5')]

        for idx, group_results in enumerate(results_per_group):
            print("\n\n")
            print("********************************************************".format(idx+1))
            print("**************** RESULTS FOR GROUP {0} *****************".format(idx+1))
            print("********************************************************".format(idx+1))
            self.print_group_results_tables(self.minimizers, group_results, group_name=self.group_suffix_names[idx],
                                            simple_text=True, rst=True, color_scale=color_scale)

        header = "\n\n"
        header += '**************** OVERALL SUMMARY - ALL GROUPS ******** \n\n'
        print(header)
        self.print_overall_results_table(self.minimizers, results_per_group, self.group_names, rst=True)

        # Flush to prevent mix-up with system tests/runner output
        import sys
        sys.stdout.flush()

    def disabled_test_rank_by_chi2_and_runtime_without_error_weights(self):
        pass


    # TODO: split into prepare + print
    def print_overall_results_table(self, minimizers, group_results, group_names, simple_text=True, rst=False):


        num_groups = len(group_results)
        num_minimizers = len(minimizers)

        groups_norm_acc = np.zeros((num_groups, num_minimizers))
        groups_norm_runtime = np.zeros((num_groups, num_minimizers))
        for group_idx, group_res in enumerate(group_results):
            results_per_test = group_res
            num_tests = len(results_per_test)

            accuracy_tbl = np.zeros((num_tests, num_minimizers))
            time_tbl = np.zeros((num_tests, num_minimizers))
            for test_idx in range(0, num_tests):
                for minimiz_idx in range(0, num_minimizers):
                    accuracy_tbl[test_idx, minimiz_idx] = results_per_test[test_idx][minimiz_idx].sum_err_sq
                    time_tbl[test_idx, minimiz_idx] = results_per_test[test_idx][minimiz_idx].runtime
            # Min across all minimizers
            min_sum_err_sq = np.nanmin(accuracy_tbl, 1)
            min_runtime = np.nanmin(time_tbl, 1)

            norm_acc_rankings = accuracy_tbl / min_sum_err_sq[:, None]
            norm_runtime_rankings = time_tbl / min_runtime[:, None]

            #acc_mean = np.nanmedian(accuracy_tbl, 0)
            groups_norm_acc[group_idx, :] = np.nanmedian(norm_acc_rankings, 0)
            #runtime_mean = np.nanmedian(time_tbl, 0)
            groups_norm_runtime[group_idx, :] = np.nanmedian(norm_runtime_rankings, 0)

        header = '**************** Accuracy ******** \n\n'
        print(header)

        linked_names = []
        for name in group_names:
            linked_names.append("`{0} <http://www.itl.nist.gov/div898/strd/nls/nls_main.shtml>`__".format(name))

        tbl_all_summary_acc = fitout.build_rst_table(minimizers, linked_names, groups_norm_acc, comparison_type='summary', using_errors=USE_ERRORS)
        print(tbl_all_summary_acc)

        header = '**************** Runtime ******** \n\n'
        print(header)
        tbl_all_summary_runtime = fitout.build_rst_table(minimizers, linked_names, groups_norm_runtime, comparison_type='summary', using_errors=USE_ERRORS)
        print(tbl_all_summary_runtime)

    # TODO: split into prepare + print
    def print_group_results_tables(self, minimizers, results_per_test, group_name, simple_text=True, rst=False, color_scale=None):
        """
        Prints in possibly several alternative formats.

        @param minimizers :: list of minimizer names
        @param results_per_test :: result objects
        @param simple_text :: whether to print the tables in a simple text format
        @param rst :: whether to print the tables in rst format. They are printed to the standard outputs
                      and to files following specific naming conventions
        """

        num_tests = len(results_per_test)
        num_minimizers = len(minimizers)

        problems = []
        linked_problems = []
        prev_name = ''
        prob_count = 1
        for test_idx in range(0, num_tests):
            raw_name = results_per_test[test_idx][0].problem.name
            name = raw_name.split('.')[0]
            if name == prev_name:
                prob_count += 1
            else:
                prob_count = 1
            prev_name = name
            name_index = name + ' ' + str(prob_count)
            problems.append(name_index)
            # TODO: move this to the nist loader, not here!
            linked_problems.append("`{0} <http://www.itl.nist.gov/div898/strd/nls/data/{1}.shtml>`__".format(name_index, name.lower()))

        # An np matrix for convenience
        # 1 row per problem+start, 1 column per minimizer
        accuracy_tbl = np.zeros((num_tests, num_minimizers))
        time_tbl = np.zeros((num_tests, num_minimizers))
        for test_idx in range(0, num_tests):
            for minimiz_idx in range(0, num_minimizers):
                accuracy_tbl[test_idx, minimiz_idx] = results_per_test[test_idx][minimiz_idx].sum_err_sq
                time_tbl[test_idx, minimiz_idx] = results_per_test[test_idx][minimiz_idx].runtime
        # Min across all minimizers
        min_sum_err_sq = np.nanmin(accuracy_tbl, 1)
        min_runtime = np.nanmin(time_tbl, 1)

        #prec_digits = 5
        #np.set_printoptions(precision=prec_digits)

        norm_acc_rankings = accuracy_tbl / min_sum_err_sq[:, None]
        norm_runtimes = time_tbl / min_runtime[:, None]

        # Calculate summary table
        summary_cols = minimizers
        summary_rows = ['Best ranking', 'Worst ranking', 'Average', 'Median', 'First quartile', 'Third quartile']
        summary_cells = np.array([np.nanmin(norm_acc_rankings, 0),
                                  np.nanmax(norm_acc_rankings, 0),
                                  np.nanmean(norm_acc_rankings, 0),
                                  np.nanmedian(norm_acc_rankings, 0),
                                  np.nanpercentile(norm_acc_rankings, 25, axis=0),
                                  np.nanpercentile(norm_acc_rankings, 75, axis=0)
                                 ])

        summary_cells_runtime =  np.array([np.nanmin(norm_runtimes, 0),
                                           np.nanmax(norm_runtimes, 0),
                                           np.nanmean(norm_runtimes, 0),
                                           np.nanmedian(norm_runtimes, 0),
                                           np.nanpercentile(norm_runtimes, 25, axis=0),
                                           np.nanpercentile(norm_runtimes, 75, axis=0)
                                          ])

        if simple_text:
            fitout.print_tables_simple_text(minimizers, results_per_test, accuracy_tbl, time_tbl, norm_acc_rankings)

        if rst:
            tbl_acc_indiv = fitout.build_rst_table(minimizers, linked_problems, norm_acc_rankings, comparison_type='accuracy', using_errors=USE_ERRORS, color_scale=color_scale)
            header = " ************* Comparison of sum of square errors (RST): *********\n"
            header += " *****************************************************************\n"
            header += "\n\n"
            print(header)
            print (tbl_acc_indiv)

            def weighted_suffix_string(use_errors):
                values = {True: 'weighted', False: 'unweighted'}
                return values[use_errors]

            fname = ('comparison_{weighted}_{version}_{metric_type}_{group_name}.txt'.
                     format(weighted=weighted_suffix_string(USE_ERRORS),
                            version=BENCHMARK_VERSION_STR, metric_type='acc', group_name=group_name))
            with open(fname, 'w') as tbl_file:
                print(tbl_acc_indiv, file=tbl_file)

            # extended summary
            tbl_acc_summary = fitout.build_rst_table(summary_cols, summary_rows, summary_cells, comparison_type='', using_errors=USE_ERRORS, color_scale=color_scale)
            header = '**************** And Summary (accuracy): ******** \n\n'
            print(header)
            print(tbl_acc_summary)
            fname = ('comparison_{weighted}_{version}_{metric_type}_{group_name}.txt'.
                     format(weighted=weighted_suffix_string(USE_ERRORS),
                            version=BENCHMARK_VERSION_STR, metric_type='acc', group_name='summary'))
            with open(fname, 'w') as tbl_file:
                print(tbl_acc_summary, file=tbl_file)

            tbl_runtime_indiv = fitout.build_rst_table(minimizers, linked_problems, norm_runtimes, comparison_type='runtime', using_errors=USE_ERRORS, color_scale=color_scale)
            header = " ************* Comparison of runtimes (RST): ****************\n"
            header += " *****************************************************************\n"
            header += "\n\n"
            print(header)
            print (tbl_runtime_indiv)
            fname = ('comparison_{weighted}_{version}_{metric_type}_{group_name}.txt'.
                     format(weighted=weighted_suffix_string(USE_ERRORS),
                            version=BENCHMARK_VERSION_STR, metric_type='runtime', group_name=group_name))
            with open(fname, 'w') as tbl_file:
                print(tbl_runtime_indiv, file=tbl_file)

            # extended summary
            tbl_runtime_summary = fitout.build_rst_table(summary_cols, summary_rows, summary_cells_runtime, comparison_type='', using_errors=USE_ERRORS, color_scale=color_scale)
            header = '**************** And Summary (runtime): ******** \n\n'
            print(header)
            print(tbl_runtime_summary)
            fname = ('comparison_{weighted}_{version}_{metric_type}_{group_name}.txt'.
                     format(weighted=weighted_suffix_string(USE_ERRORS),
                            version=BENCHMARK_VERSION_STR, metric_type='runtime', group_name='summary'))
            with open(fname, 'w') as tbl_file:
                print(tbl_runtime_summary, file=tbl_file)

    
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
