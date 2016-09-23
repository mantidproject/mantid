"""
Produce output tables from fitting benchmarking results.
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
