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
from docutils.core import publish_string
import post_processing as postproc
import os

# older version of numpy does not support nanmean and nanmedian
# and nanmean and nanmedian was removed in scipy 0.18 in favor of numpy
# so try numpy first then scipy.stats
try:
    from numpy import nanmean, nanmedian
except ImportError:
    from scipy.stats import nanmean, nanmedian

# Some naming conventions for the output files
BENCHMARK_VERSION_STR = 'v3.8'
FILENAME_SUFFIX_ACCURACY = 'acc'
FILENAME_SUFFIX_RUNTIME = 'runtime'
FILENAME_EXT_TXT = 'txt'
FILENAME_EXT_HTML = 'html'
# Directory of where the script is called from (e.g. MantidPlot dir)
WORKING_DIR = os.getcwd()
# Directory of this script (e.g. in source)
SCRIPT_DIR = os.path.dirname(__file__)


def print_group_results_tables(minimizers, results_per_test, problems_obj, group_name, use_errors,
                               simple_text=True, rst=False, save_to_file=False, color_scale=None):
    """
    Prints out results for a group of fit problems in accuracy and runtime tables, in a summary
    format and both as simple text, rst format and/or to file depending on input arguments

    @param minimizers :: list of minimizer names
    @param results_per_test :: result objects
    @param problems_obj :: definitions of the test problems
    @param group_name :: name of this group of problems (example 'NIST "lower difficulty"', or
                         'Neutron data')
    @param use_errors :: whether to use observational errors
    @param simple_text :: whether to print the tables in a simple text format
    @param rst :: whether to print the tables in rst format. They are printed to the standard outputs
                  and to files following specific naming conventions
    @param save_to_file :: If rst=True, whether to save the tables to files following specific naming conventions
    @param color_scale :: threshold-color pairs. This is used for RST tables. The number of levels
                          must be consistent with the style sheet used in the documentation pages (5
                          at the moment).
    """
    linked_problems = build_indiv_linked_problems(results_per_test, group_name)

    # Calculate summary tables
    accuracy_tbl, runtime_tbl = postproc.calc_accuracy_runtime_tbls(results_per_test, minimizers)
    norm_acc_rankings, norm_runtimes, summary_cells_acc, summary_cells_runtime = \
        postproc.calc_norm_summary_tables(accuracy_tbl, runtime_tbl)

    if simple_text:
        print_tables_simple_text(minimizers, results_per_test, accuracy_tbl, runtime_tbl, norm_acc_rankings)

    if rst:
        # print out accuracy table for this group of fit problems
        tbl_acc_indiv = build_rst_table(minimizers, linked_problems, norm_acc_rankings,
                                        comparison_type='accuracy', comparison_dim='',
                                        using_errors=use_errors, color_scale=color_scale)
        header = " ************* Comparison of sum of square errors (RST): *********\n"
        header += " *****************************************************************\n"
        header += "\n\n"
        print(header)
        print (tbl_acc_indiv)

        # optionally save the above table to a .txt file and a .html file
        if save_to_file:
            save_table_to_file(table_data=tbl_acc_indiv, errors=use_errors, group_name=group_name,
                               metric_type=FILENAME_SUFFIX_ACCURACY, file_extension=FILENAME_EXT_TXT)
            save_table_to_file(table_data=tbl_acc_indiv, errors=use_errors, group_name=group_name,
                               metric_type=FILENAME_SUFFIX_ACCURACY, file_extension=FILENAME_EXT_HTML)

        # print out accuracy summary table for this group of fit problems
        ext_summary_cols = minimizers
        ext_summary_rows = ['Best ranking', 'Worst ranking', 'Average', 'Median']
        tbl_acc_summary = build_rst_table(ext_summary_cols, ext_summary_rows, summary_cells_acc,
                                          comparison_type='accuracy', comparison_dim='',
                                          using_errors=use_errors, color_scale=color_scale)
        header = '**************** Statistics/Summary (accuracy): ******** \n\n'
        print(header)
        print(tbl_acc_summary)

        # print out runtime table for this group of fit problems
        tbl_runtime_indiv = build_rst_table(minimizers, linked_problems, norm_runtimes,
                                            comparison_type='runtime', comparison_dim='',
                                            using_errors=use_errors, color_scale=color_scale)
        header = " ************* Comparison of runtimes (RST): ****************\n"
        header += " *****************************************************************\n"
        header += "\n\n"
        print(header)
        print (tbl_runtime_indiv)

        # optionally save the above table to a .txt file and a .html file
        if save_to_file:
            save_table_to_file(table_data=tbl_runtime_indiv, errors=use_errors, group_name=group_name,
                               metric_type=FILENAME_SUFFIX_RUNTIME, file_extension=FILENAME_EXT_TXT)
            save_table_to_file(table_data=tbl_runtime_indiv, errors=use_errors, group_name=group_name,
                               metric_type=FILENAME_SUFFIX_RUNTIME, file_extension=FILENAME_EXT_HTML)

        # print out runtime summary table for this group of fit problems
        tbl_runtime_summary = build_rst_table(ext_summary_cols, ext_summary_rows, summary_cells_runtime,
                                              comparison_type='runtime', comparison_dim='',
                                              using_errors=use_errors, color_scale=color_scale)
        header = '**************** Statistics/Summary (runtime): ******** \n\n'
        print(header)
        print(tbl_runtime_summary)


def save_table_to_file(table_data, errors, group_name, metric_type, file_extension):
    """
    Saves a group results table or overall results table to a given file type.

    @param table_data :: the results table
    @param errors :: whether to use observational errors
    @param group_name :: name of this group of problems (example 'NIST "lower difficulty"', or
                         'Neutron data')
    @param metric_type :: the test type of the table data (e.g. runtime, accuracy)
    @param file_extension :: the file type extension (e.g. html)
    """
    file_name = ('comparison_{weighted}_{version}_{metric_type}_{group_name}.'
                 .format(weighted=weighted_suffix_string(errors),
                         version=BENCHMARK_VERSION_STR, metric_type=metric_type, group_name=group_name))

    if file_extension == 'html':
        rst_content = '.. include:: ' + str(os.path.join(SCRIPT_DIR, 'color_definitions.txt'))
        rst_content += '\n' + table_data
        table_data = publish_string(rst_content, writer_name='html')

    with open(file_name + file_extension, 'w') as tbl_file:
        print(table_data, file=tbl_file)
    print('Saved {file_name}{extension} to {working_directory}'.
          format(file_name=file_name, extension=file_extension, working_directory=WORKING_DIR))


def build_indiv_linked_problems(results_per_test, group_name):
    """
    Makes a list of linked problem names which would be used for the
    rows of the first column of the tables of individual results.

    @param results_per_test :: results as produces by the fitting tests
    @param group_name :: name of the group (NIST, Neutron data, etc.) this problem is part of

    @returns :: list of problems with their description link tags
    """
    num_tests = len(results_per_test)
    prev_name = ''
    prob_count = 1
    linked_problems = []
    for test_idx in range(0, num_tests):
        raw_name = results_per_test[test_idx][0].problem.name
        name = raw_name.split('.')[0]
        if name == prev_name:
            prob_count += 1
        else:
            prob_count = 1

        prev_name = name
        name_index = name + ' ' + str(prob_count)

        # TO-DO: move this to the nist loader, not here!
        if 'nist_' in group_name:
            linked_problems.append("`{0} <http://www.itl.nist.gov/div898/strd/nls/data/{1}.shtml>`__".
                                   format(name_index, name.lower()))
        else:
            linked_problems.append(name)

    return linked_problems


def build_group_linked_names(group_names):
    """
    Add a link for RST tables if there is a website or similar describing the group.

    @param group_names :: names as plain text

    @returns :: names with RST links if available
    """
    linked_names = []
    for name in group_names:
        # This should be tidied up. We currently don't have links for groups other than NIST
        if 'NIST' in name:
            linked = "`{0} <http://www.itl.nist.gov/div898/strd/nls/nls_main.shtml>`__".format(name)
        else:
            linked = name

        linked_names.append(linked)

    return linked_names


def print_overall_results_table(minimizers, group_results, problems, group_names, use_errors,
                                simple_text=True, save_to_file=False):

    groups_norm_acc, groups_norm_runtime = postproc.calc_summary_table(minimizers, group_results)

    grp_linked_names = build_group_linked_names(group_names)

    header = '**************** Accuracy ******** \n\n'
    print(header)
    tbl_all_summary_acc = build_rst_table(minimizers, grp_linked_names, groups_norm_acc,
                                          comparison_type='summary', comparison_dim='accuracy',
                                          using_errors=use_errors)
    print(tbl_all_summary_acc)

    if save_to_file:
        save_table_to_file(tbl_all_summary_acc, use_errors, 'summary', FILENAME_SUFFIX_ACCURACY, FILENAME_EXT_TXT)
        save_table_to_file(tbl_all_summary_acc, use_errors, 'summary', FILENAME_SUFFIX_ACCURACY, FILENAME_EXT_HTML)

    header = '**************** Runtime ******** \n\n'
    print(header)
    tbl_all_summary_runtime = build_rst_table(minimizers, grp_linked_names, groups_norm_runtime,
                                              comparison_type='summary', comparison_dim='runtime',
                                              using_errors=use_errors)
    print(tbl_all_summary_runtime)

    if save_to_file:
        save_table_to_file(tbl_all_summary_runtime, use_errors, 'summary', FILENAME_SUFFIX_RUNTIME, FILENAME_EXT_TXT)
        save_table_to_file(tbl_all_summary_runtime, use_errors, 'summary', FILENAME_SUFFIX_RUNTIME, FILENAME_EXT_HTML)


def weighted_suffix_string(use_errors):
    """
    Produces a suffix weighted/unweighted. Used to generate names of
    output files.
    """
    values = {True: 'weighted', False: 'unweighted'}
    return values[use_errors]


def display_name_for_minimizers(names):
    """
    Converts minimizer names into their "display names". For example
    to rename DTRS to "Trust region" or similar

    """
    display_names = names
    # Quick fix for DTRS name in version 3.8 - REMOVE
    for idx, minimizer in enumerate(names):
        if 'DTRS' == minimizer:
            display_names[idx] = 'Trust Region'

    return display_names


def calc_cell_len_rst_table(columns_txt, items_link, cells, color_scale=None):
    """
    Calculate ascii character width needed for an RST table, using the length of the longest table cell.

    @param columns_txt :: list of the contents of the column headers
    @param items_link :: the links from rst table cells to other pages/sections of pages
    @param cells :: the values of the results
    @param color_scale :: whether a color_scale is used or not
    @returns :: the length of the longest cell in a table
    """

    # The length of the longest header (minimizer name)
    max_header = len(max((col for col in columns_txt), key=len))
    # The value of the longest (once formatted) value in the table
    max_value = max(("%.4g" % cell for cell in np.nditer(cells)), key=len)
    # The length of the longest link reference (angular bracket content present in summary tables)
    max_item = max(items_link, key=len) if isinstance(items_link, list) else items_link
    # One space on each end of a cell
    padding = 2
    # Set cell length equal to the length of: the longest combination of value, test name, and colour (plus padding)
    cell_len = len(format_cell_value_rst(value=float(max_value),
                                         color_scale=color_scale,
                                         items_link=max_item).strip()) + padding
    # If the header is longer than any cell's contents, i.e. is a group results table, use that length instead
    if cell_len < max_header:
        cell_len = max_header

    return cell_len


def build_rst_table(columns_txt, rows_txt, cells, comparison_type, comparison_dim,
                    using_errors, color_scale=None):
    """"
    Builds an RST table as a string, given the list of column and row headers,
    and a 2D numpy array with values for the cells.
    This can be tricky/counterintuitive, see:
    http://docutils.sourceforge.net/docs/dev/rst/problems.html

    @param columns_txt :: the text for the columns, one item per column
    @param rows_txt :: the text for the rows (will go in the leftmost column)
    @param cells :: a 2D numpy array with as many rows as items have been given
    in rows_txt, and as many columns as items have been given in columns_txt

    @param comparison_type :: whether this is a 'summary', or a full 'accuracy', or 'runtime'
                              table.
    @param comparison_dim :: dimension (accuracy / runtime)
    @param using_errors :: whether this comparison uses errors in the cost function
    (weighted or unweighted), required to link the table properly

    @param color_scale :: list with pairs of threshold value - color, to produce color
    tags for the cells
    """
    columns_txt = display_name_for_minimizers(columns_txt)

    items_link = build_items_links(comparison_type, comparison_dim, using_errors)

    cell_len = calc_cell_len_rst_table(columns_txt, items_link, cells, color_scale)

    # The first column tends to be disproportionately long if it has a link
    first_col_len = calc_first_col_len(cell_len, rows_txt)

    tbl_header_top, tbl_header_text, tbl_header_bottom = build_rst_table_header_chunks(first_col_len, cell_len,
                                                                                       columns_txt)

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
            tbl_body += format_cell_value_rst(cells[row, col], cell_len, color_scale, link) + '|'

        tbl_body += '\n'
        tbl_body += tbl_footer

    return tbl_header + tbl_body


def build_rst_table_header_chunks(first_col_len, cell_len, columns_txt):
    """
    Prepare the horizontal and vertical lines in the RST headers.

    @param first_col_len :: length (in characters) of the first column
    @param cell_len :: length of all other cells
    """
    tbl_header_top = '+'
    tbl_header_text = '|'
    tbl_header_bottom = '+'

    # First column in the header for the name of the test or statistics
    tbl_header_top += '-'.ljust(first_col_len, '-') + '+'
    tbl_header_text += ' '.ljust(first_col_len, ' ') + '|'
    tbl_header_bottom += '='.ljust(first_col_len, '=') + '+'
    for col_name in columns_txt:
        tbl_header_top += '-'.ljust(cell_len, '-') + '+'
        tbl_header_text += col_name.ljust(cell_len, ' ') + '|'
        tbl_header_bottom += '='.ljust(cell_len, '=') + '+'

    return tbl_header_top, tbl_header_text, tbl_header_bottom


def calc_first_col_len(cell_len, rows_txt):
    first_col_len = cell_len
    for row_name in rows_txt:
        name_len = len(row_name)
        if name_len > first_col_len:
            first_col_len = name_len

    return first_col_len


def build_items_links(comparison_type, comparison_dim, using_errors):
    """
    Figure out the links from rst table cells to other pages/sections of pages.

    @param comparison_type :: whether this is a 'summary', or a full 'accuracy', or 'runtime' table.
    @param comparison_dim :: dimension (accuracy / runtime)
    @param using_errors :: whether using observational errors in cost functions

    @returns :: link or links to use from table cells.
    """
    if 'summary' == comparison_type:
        items_link = ['Minimizers_{0}_comparison_in_terms_of_{1}_nist_lower'.
                      format(weighted_suffix_string(using_errors), comparison_dim),
                      'Minimizers_{0}_comparison_in_terms_of_{1}_nist_average'.
                      format(weighted_suffix_string(using_errors), comparison_dim),
                      'Minimizers_{0}_comparison_in_terms_of_{1}_nist_higher'.
                      format(weighted_suffix_string(using_errors), comparison_dim),
                      'Minimizers_{0}_comparison_in_terms_of_{1}_cutest'.
                      format(weighted_suffix_string(using_errors), comparison_dim),
                      'Minimizers_{0}_comparison_in_terms_of_{1}_neutron_data'.
                      format(weighted_suffix_string(using_errors), comparison_dim),
                      ]
    elif 'accuracy' == comparison_type or 'runtime' == comparison_type:
        if using_errors:
            items_link = 'FittingMinimizersComparisonDetailedWithWeights'
        else:
            items_link = 'FittingMinimizersComparisonDetailed'
    else:
        items_link = ''

    return items_link


def format_cell_value_rst(value, width=None, color_scale=None, items_link=None):
    """
    Build the content string for a table cell, adding style/color tags
    if required.

    @param value :: the value of the result
    @param width :: the width of the longest table cell
    @param color_scale :: the colour scale used
    @param items_link :: the links from rst table cells to other pages/sections of pages
    @returns :: the (formatted) contents of a cell

    """
    if not color_scale:
        if not items_link:
            value_text = ' {0:.4g}'.format(value)
        else:
            value_text = ' :ref:`{0:.4g} <{1}>`'.format(value, items_link)
    else:
        color = ''
        for color_descr in color_scale:
            if value <= color_descr[0]:
                color = color_descr[1]
                break
        if not color:
            color = color_scale[-1][1]
        value_text = " :{0}:`{1:.4g}`".format(color, value)

    if width is not None:
        value_text = value_text.ljust(width, ' ')

    return value_text


def print_tables_simple_text(minimizers, results_per_test, accuracy_tbl, time_tbl, norm_acc_rankings):
    """
    Produces tables in plain ascii, without any markup.  This is much
    easier to read than RST and useful when developing or just
    checking the output of the runs from the system test logs.

    """

    header = " ============= Comparison of sum of square errors: ===============\n"
    header += " =================================================================\n"
    header += "\n\n"

    for minimiz in minimizers:
        header += " {0} |".format(minimiz)
    header += "\n"
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

    # If some of the fits fail badly, they'll produce 'nan' values =>
    # 'nan' errors. Requires np.nanmedian() and the like nan-safe
    # functions.

    # summary lines
    results_text += '---------------- Summary (accuracy): -------- \n'
    results_text += 'Best ranking: {0}\n'.format(np.nanmin(norm_acc_rankings, 0))
    results_text += 'Worst ranking: {0}\n'.format(np.nanmax(norm_acc_rankings, 0))
    results_text += 'Mean: {0}\n'.format(nanmean(norm_acc_rankings, 0))
    results_text += 'Median: {0}\n'.format(nanmedian(norm_acc_rankings, 0))
    results_text += '\n'

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
    time_text += 'Mean: {0}\n'.format(nanmean(norm_runtimes, 0))
    time_text += 'Median: {0}\n'.format(nanmedian(norm_runtimes, 0))
    time_text += '\n'

    print(time_text)
