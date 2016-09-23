"""
Produce output tables from fitting benchmarking results, in different
formats such as RST and plain text.
"""
# Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory, NScD
# Oak Ridge National Laboratory & European Spallation Source
#
# This file is part of Mantid.
# Mantid is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3 of the License, or
# (at your option) any later version.
#
# Mantid is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#
# File change history is stored at: <https://github.com/mantidproject/mantid>.
# Code Documentation is available at: <http://doxygen.mantidproject.org>

from __future__ import (absolute_import, division, print_function)

import numpy as np

BENCHMARK_VERSION_STR = 'v3.8'
FILENAME_SUFFIX_ACCURACY = 'acc'
FILENAME_SUFFIX_RUNTIME = 'runtime'

# TO-DO: split into prepare + print
def print_group_results_tables(minimizers, results_per_test, group_name, use_errors, simple_text=True, rst=False, color_scale=None):
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
        print_tables_simple_text(minimizers, results_per_test, accuracy_tbl, time_tbl, norm_acc_rankings)

    if rst:
        tbl_acc_indiv = build_rst_table(minimizers, linked_problems, norm_acc_rankings, comparison_type='accuracy', using_errors=use_errors, color_scale=color_scale)
        header = " ************* Comparison of sum of square errors (RST): *********\n"
        header += " *****************************************************************\n"
        header += "\n\n"
        print(header)
        print (tbl_acc_indiv)

        def weighted_suffix_string(use_errors):
            values = {True: 'weighted', False: 'unweighted'}
            return values[use_errors]

        fname = ('comparison_{weighted}_{version}_{metric_type}_{group_name}.txt'.
                 format(weighted=weighted_suffix_string(use_errors),
                        version=BENCHMARK_VERSION_STR, metric_type=FILENAME_SUFFIX_ACCURACY, group_name=group_name))
        with open(fname, 'w') as tbl_file:
            print(tbl_acc_indiv, file=tbl_file)

        # extended summary
        tbl_acc_summary = build_rst_table(summary_cols, summary_rows, summary_cells, comparison_type='', using_errors=use_errors, color_scale=color_scale)
        header = '**************** And Summary (accuracy): ******** \n\n'
        print(header)
        print(tbl_acc_summary)
        fname = ('comparison_{weighted}_{version}_{metric_type}_{group_name}.txt'.
                 format(weighted=weighted_suffix_string(use_errors),
                        version=BENCHMARK_VERSION_STR, metric_type=FILENAME_SUFFIX_ACCURACY, group_name='summary'))
        with open(fname, 'w') as tbl_file:
            print(tbl_acc_summary, file=tbl_file)

        tbl_runtime_indiv = build_rst_table(minimizers, linked_problems, norm_runtimes, comparison_type='runtime', using_errors=use_errors, color_scale=color_scale)
        header = " ************* Comparison of runtimes (RST): ****************\n"
        header += " *****************************************************************\n"
        header += "\n\n"
        print(header)
        print (tbl_runtime_indiv)
        fname = ('comparison_{weighted}_{version}_{metric_type}_{group_name}.txt'.
                 format(weighted=weighted_suffix_string(use_errors),
                        version=BENCHMARK_VERSION_STR, metric_type=FILENAME_SUFFIX_RUNTIME, group_name=group_name))
        with open(fname, 'w') as tbl_file:
            print(tbl_runtime_indiv, file=tbl_file)

        # extended summary
        tbl_runtime_summary = build_rst_table(summary_cols, summary_rows, summary_cells_runtime, comparison_type='', using_errors=use_errors, color_scale=color_scale)
        header = '**************** And Summary (runtime): ******** \n\n'
        print(header)
        print(tbl_runtime_summary)
        fname = ('comparison_{weighted}_{version}_{metric_type}_{group_name}.txt'.
                 format(weighted=weighted_suffix_string(use_errors),
                        version=BENCHMARK_VERSION_STR, metric_type=FILENAME_SUFFIX_RUNTIME, group_name='summary'))
        with open(fname, 'w') as tbl_file:
            print(tbl_runtime_summary, file=tbl_file)

# TO-DO: split into prepare + print
def print_overall_results_table(minimizers, group_results, group_names, use_errors, simple_text=True, rst=False):

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

    tbl_all_summary_acc = build_rst_table(minimizers, linked_names, groups_norm_acc, comparison_type='summary', using_errors=use_errors)
    print(tbl_all_summary_acc)

    header = '**************** Runtime ******** \n\n'
    print(header)
    tbl_all_summary_runtime = build_rst_table(minimizers, linked_names, groups_norm_runtime, comparison_type='summary', using_errors=use_errors)
    print(tbl_all_summary_runtime)

def build_rst_table(columns_txt, rows_txt, cells, comparison_type, using_errors, color_scale=None):
    """"
    Builds an RST table, given the list of column and row headers,
    and a 2D numpy array with values for the cells.
    This can be tricky/counterintuitive, see:
    http://docutils.sourceforge.net/docs/dev/rst/problems.html

    @param colums_txt :: the text for the columns, one item per column
    @param rows_txt :: the text for the rows (will go in the leftmost column)
    @param cells :: a 2D numpy array with as many rows as items have been given
    in rows_txt, and as many columns as items have been given in columns_txt

    @param using_errors :: whether this comparison uses errors in the cost function
    (weighted or unweighted), required to link the table properly

    @param color_scale :: list with pairs of threshold value - color, to produce color
    tags for the cells
    """
    # One length for all cells
    cell_len = 50
    cell_len = 0
    for col in columns_txt:
        new_len = len(col) + 2
        if new_len > cell_len:
            cell_len = new_len

    # links for the cells of the summary tables (to the detailed results
    if 'summary' == comparison_type and 'using_errors':
        items_link = [ 'Minimizers_unweighted_comparison_in_terms_of_runtime_nist_lower',
                       'Minimizers_unweighted_comparison_in_terms_of_runtime_nist_average',
                       'Minimizers_unweighted_comparison_in_terms_of_runtime_nist_higher',
                       'Minimizers_unweighted_comparison_in_terms_of_runtime_cutest' ]
    elif 'summary' == comparison_type and not 'using_errors':
        items_link = [ 'Minimizers_unweighted_comparison_in_terms_of_accuracy_nist_lower',
                       'Minimizers_unweighted_comparison_in_terms_of_accuracy_nist_average',
                       'Minimizers_unweighted_comparison_in_terms_of_accuracy_nist_higher',
                       'Minimizers_unweighted_comparison_in_terms_of_accuracy_cutest' ]
    elif 'accuracy' == comparison_type or 'runtime' == comparison_type:
        if using_errors:
            items_link = 'FittingMinimizersComparisonDetailedWithWeights'
        else:
            items_link = 'FittingMinimizersComparisonDetailed'
    else:
        items_link = ''


    links_len = 0
    if items_link and isinstance(items_link, list):
        links_len = max([len(item) for item in items_link])
    elif items_link:
        links_len = len(items_link)

    additional_len = 0
    if items_link:
        additional_len = links_len
    cell_len += int(additional_len/1.2)


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
        # Pick either individual or group link
        if isinstance(items_link, list):
            link = items_link[row]
        else:
            link = items_link

        tbl_body += '|' + rows_txt[row].ljust(first_col_len, ' ') + '|'
        for col in range(0, cells.shape[1]):
            tbl_body += format_cell_value_rst(cells[row,col], cell_len, color_scale, link) + '|'

        tbl_body += '\n'
        tbl_body += tbl_footer

    return tbl_header + tbl_body

def format_cell_value_rst(value, width, color_scale=None, items_link=None):
    """
    Build the content string for a table cell, adding style/color tags
    if required.

    """
    if not color_scale:
        if not items_link:
            value_text = ' {0:.4g}'.format(value).ljust(width, ' ')
        else:
            value_text = ' :ref:`{0:.4g} <{1}>`'.format(value, items_link).ljust(width, ' ')
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

def print_tables_simple_text(minimizers, results_per_test, accuracy_tbl, time_tbl, norm_acc_rankings):
    header = " ============= Comparison of sum of square errors: ===============\n"
    header += " =================================================================\n"
    header += "\n\n"

    for minimiz in minimizers:
        header += " {0} |".format(minimiz)
    header +="\n"
    print(header)

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

    print(results_text)

    print(" ======== Time: =======")
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

    print(time_text)
