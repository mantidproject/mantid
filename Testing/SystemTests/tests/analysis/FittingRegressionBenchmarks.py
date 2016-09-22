"""
Benchmarks for accuracy (and time / iterations).
"""
import unittest
import stresstesting

import os
import time

import numpy as np

import mantid.simpleapi as msapi
import fitting_benchmarking as fitbk

#### REORGANIZE

#import fitting
#FittingProblem, FittingProblemLoader, RunProblems

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

    result = None

    def setUp(self):
        if not self.__class__.result:
            problem_set = None
            # result = fitting.RunProblems.do_calc()

    # Rename this to what it does/can do
    def test_rank_by_chi2(self):
        # TO-DO related to this: expose API::FuncMinimizerFactory to Python
        # TOTHINK: use different interface as in the API::FunctionFactory?
        # But still, do we want to enforce a particular ordering for the tables?
        minimizers_factory = ['BFGS', 'Conjugate gradient (Fletcher-Reeves imp.)',
                              'Conjugate gradient (Polak-Ribiere imp.)', 'Damping',
                               #'FABADA', # hide FABADA
                              'Levenberg-Marquardt', 'Levenberg-MarquardtMD',
                              'Simplex', 'SteepestDescent', 'DTRS']
        minimizers = minimizers_factory
        results_per_block = fitbk.do_regression_fitting_benchmark(include_nist=True, minimizers=minimizers)


        color_scale = [(1.1, 'ranking-top-1'), (1.33, 'ranking-top-2'), (1.75, 'ranking-med-3'), (3, 'ranking-low-4'), (float('nan'), 'ranking-low-5')]

        for idx, block_results in enumerate(results_per_block):
            print("\n\n")
            print("********************************************************").format(idx+1)
            print("**************** RESULTS FOR BLOCK {0} *****************").format(idx+1)
            print("********************************************************").format(idx+1)
            self.print_block_results_tables(minimizers, block_results, simple_text=True, rst=True, color_scale=color_scale)

        header = "\n\n"
        header += '**************** OVERALL SUMMARY - ALL BLOCKS ******** \n\n'
        print(header)
        self.print_overall_results_table(minimizers, results_per_block, rst=True)

        # Flush to prevent mix-up with system tests/runner output
        import sys
        sys.stdout.flush()

    # TODO: split into prepare + print
    def print_overall_results_table(self, minimizers, block_results, simple_text=True, rst=False):

        num_blocks = len(block_results)
        num_minimizers = len(minimizers)

        blocks_norm_acc = np.zeros((num_blocks, num_minimizers))
        blocks_norm_runtime = np.zeros((num_blocks, num_minimizers))
        for block_idx, block in enumerate(block_results):
            results_per_test = block
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
            blocks_norm_acc[block_idx, :] = np.nanmedian(norm_acc_rankings, 0)
            #runtime_mean = np.nanmedian(time_tbl, 0)
            blocks_norm_runtime[block_idx, :] = np.nanmedian(norm_runtime_rankings, 0)

        header = '**************** Accuracy ******** \n\n'
        print(header)

        block_names = ['NIST, "lower" difficulty', 'NIST, "average" difficulty', 'NIST, "higher" difficulty']
        linked_names = []
        for name in block_names:
            linked_names.append("`{0} <http://www.itl.nist.gov/div898/strd/nls/nls_main.shtml>`__".format(name))

        tbl_all_summary_acc = self.build_rst_table(minimizers, linked_names, blocks_norm_acc)
        print(tbl_all_summary_acc)

        header = '**************** Runtime ******** \n\n'
        print(header)
        tbl_all_summary_runtime = self.build_rst_table(minimizers, linked_names, blocks_norm_runtime)
        print(tbl_all_summary_runtime)

    # TODO: split into prepare + print
    def print_block_results_tables(self, minimizers, results_per_test, simple_text=True, rst=False, color_scale=None):
        """
        Prints in possibly several alternative formats.

        @param minimizers :: list of minimizer names
        @param results_per_test :: result objects
        @param simple_text :: whether to print the tables in a simple text format
        @param rst :: whether to print the tables in rst format
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
            self.print_tables_simple_text(minimizers, results_per_test, accuracy_tbl, time_tbl, norm_acc_rankings)

        if rst:
            #tbl_indiv = self.calc_individual_table_rst(minimizers, results_per_minimizer, norm_acc_rankings)
            tbl_indiv = self.build_rst_table(minimizers, linked_problems, norm_acc_rankings, color_scale)
            header = " ************* Comparison of sum of square errors (RST): *********\n"
            header += " *****************************************************************\n"
            header += "\n\n"
            print(header)
            print (tbl_indiv)

            tbl_summary = self.build_rst_table(summary_cols, summary_rows, summary_cells, color_scale)
            header = '**************** And Summary (accuracy): ******** \n\n'
            print(header)
            print(tbl_summary)

            tbl_runtime_indiv = self.build_rst_table(minimizers, linked_problems, norm_runtimes, color_scale)
            header = " ************* Comparison of runtimes (RST): ****************\n"
            header += " *****************************************************************\n"
            header += "\n\n"
            print(header)
            print (tbl_runtime_indiv)

            tbl_summary_runtime = self.build_rst_table(summary_cols, summary_rows, summary_cells_runtime, color_scale)
            header = '**************** And Summary (runtime): ******** \n\n'
            print(header)
            print(tbl_summary_runtime)


    def print_tables_simple_text(self, minimizers, results_per_test, accuracy_tbl, time_tbl, norm_acc_rankings):
        header = " ============= Comparison of sum of square errors: ===============\n"
        header += " =================================================================\n"
        header += "\n\n"

        for minimiz in minimizers:
            header += " {0} |".format(minimiz)
        header +="\n"
        print header

        min_sum_err_sq = np.amin(accuracy_tbl, 1)
        num_tests = len(results_per_test)
        results_text = ''
        for test_idx in range(0, num_tests):
            results_text += "{0}\t".format(results_per_test[test_idx][0].problem.name)
            for minimiz_idx, minimiz in enumerate(minimizers):
                # 'e' format is easier to read in raw text output than 'g'
                results_text += (" {0:.10g}".
                                 format(results_per_test[test_idx][minimiz_idx].sum_err_sq /
                                        min_sum_err_sq[test_idx]))
            results_text += "\n"

        # Beware for the statistics, if some of the fits fail badly, they'll produce 'nan'
        # values => 'nan' errors. Requires np.nanmedian() and the like
        
        # summary lines
        results_text += '---------------- Summary (accuracy): -------- \n'
        results_text += 'Best ranking: {0}\n'.format(np.nanmin(norm_acc_rankings, 0))
        results_text += 'Worst ranking: {0}\n'.format(np.nanmax(norm_acc_rankings, 0))
        results_text += 'Average: {0}\n'.format(np.nanmean(norm_acc_rankings, 0))
        results_text += 'Median: {0}\n'.format(np.nanmedian(norm_acc_rankings, 0))
        results_text += '\n'
        results_text += 'First quartile: {0}\n'.format(np.nanpercentile(norm_acc_rankings, 25, axis=0))
        results_text += 'Third quartile: {0}\n'.format(np.nanpercentile(norm_acc_rankings, 75, axis=0))

        print results_text

        print " ======== Time: ======="
        time_text = ''
        for test_idx in range(0, num_tests):
            time_text += "{0}\t".format(results_per_test[test_idx][0].problem.name)
            for minimiz_idx, minimiz in enumerate(minimizers):
                time_text += " {0}".format(results_per_test[test_idx][minimiz_idx].runtime)
            time_text += "\n"

        min_runtime = np.amin(time_tbl, 1)
        norm_runtimes = time_tbl / min_runtime[:, None]
        time_text += '---------------- Summary (run time): -------- \n'
        time_text += 'Best ranking: {0}\n'.format(np.nanmin(norm_runtimes, 0))
        time_text += 'Worst ranking: {0}\n'.format(np.nanmax(norm_runtimes, 0))
        time_text += 'Average: {0}\n'.format(np.average(norm_runtimes, 0))
        time_text += 'Median: {0}\n'.format(np.nanmedian(norm_runtimes, 0))
        time_text += '\n'
        time_text += 'First quartile: {0}\n'.format(np.nanpercentile(norm_runtimes, 25, axis=0))
        time_text += 'Third quartile: {0}\n'.format(np.nanpercentile(norm_runtimes, 75, axis=0))

        print time_text


    def build_rst_table(self, columns_txt, rows_txt, cells, color_scale=None):
        """"
        Builds an RST table, given the list of column and row headers,
        and a 2D numpy array with values for the cells

        See http://docutils.sourceforge.net/docs/dev/rst/problems.html

        @param colums_txt :: the text for the columns
        @param rows_txt :: the text for the rows (will go in the leftmost column)
        @param cells :: a 2D numpy array with as many rows as items have been given
        in rows_txt, and as many columns as items have been given in columns_txt
        """
        # One length for all cells
        cell_len = 50
        cell_len = 0
        for col in columns_txt:
            new_len = len(col) + 2
            if new_len > cell_len:
                cell_len = new_len

        # The first column tends to be disproportionately long if it has a link
        first_col_len = cell_len
        for row_name in rows_txt:
            name_len = len(row_name)
            if name_len > first_col_len:
                first_col_len = name_len

        tbl_header_top = '+'
        tbl_header_text = '|'
        tbl_header_bottom = '+'

        # First column in the header for the name of the test or statistics
        tbl_header_top += '-'.ljust(first_col_len, '-') + '+'
        tbl_header_text += ' '.ljust(first_col_len, ' ') + '|'
        tbl_header_bottom += '='.ljust(first_col_len,'=') + '+'
        for col_name in columns_txt:
            tbl_header_top += '-'.ljust(cell_len, '-') + '+'
            tbl_header_text += col_name.ljust(cell_len, ' ') + '|'
            tbl_header_bottom += '='.ljust(cell_len,'=') + '+'

        tbl_header = tbl_header_top + '\n' + tbl_header_text + '\n' + tbl_header_bottom + '\n'
        # the footer is in general the delimiter between rows, including the last one
        tbl_footer = tbl_header_top + '\n'


        tbl_body = ''
        for row in range(0, cells.shape[0]):
            tbl_body += '|' + rows_txt[row].ljust(first_col_len, ' ') + '|'
            for col in range(0, cells.shape[1]):
                tbl_body += self.format_cell_value_rst(cells[row,col], cell_len, color_scale) + '|'
                #tbl_body += ' {:.4g}'.format(cells[row, col]).ljust(cell_len, ' ') + '|'
            tbl_body += '\n'
            tbl_body += tbl_footer

        return tbl_header + tbl_body

    def format_cell_value_rst(self, value, width, color_scale=None):
        if not color_scale:
            value_text = ' {0:.4g}'.format(value).ljust(width, ' ')
        else:
            color = ''
            for color_descr in color_scale:
                if value <= color_descr[0]:
                    color = color_descr[1]
                    break
            if not color:
                color = color_scale[-1][1]
            value_text = " :{0}:`{1:.4g}`".format(color, value).ljust(width, ' ')

        return value_text

    def disabled_test_rank_by_time(self):
        pass

    
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
