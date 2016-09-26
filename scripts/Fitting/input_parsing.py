"""
Parse input files describing fitting test examples and load the
information into problem objects

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

import os
import re

import numpy as np

import test_problem

def parse_data_pattern(data_text):
    if not data_text:
        return None

    first = data_text[0].strip()
    dim = len(first.split())
    data_points = np.zeros((len(data_text), dim))

    for idx, line in enumerate(data_text):
        line = line.strip()
        point_text = line.split()
        point = [float(val) for val in point_text]
        data_points[idx, :] = point

    return data_points

def parse_equation(eq_text):
    """
    Parses the equation text (possibly multi-line) and does a rough
    conversion from NIST equation format to muparser format

    @param eq_text :: equation formula as given in a NIST problem description
    @return formula ready to be used in the 'Formula=' of functions 'UserFunction' of
            the Fit algorithm
    """
    start_normal = r'\s*y\s*=(.+)'
    start_log = r'\s*log\[y\]\s*=(.+)'
    # try first the usual syntax
    if re.match(start_normal, eq_text):
        match = re.search(r'y\s*=(.+)\s*\+\s*e', eq_text)
        equation = match.group(1).strip()
    # log-syntax used in NIST/Nelson
    elif re.match(start_log, eq_text):
        match = re.search(r'log\[y\]\s*=(.+)\s*\+\s*e', eq_text)
        equation = "exp(" + match.group(1).strip() + ")"
    else:
        raise RuntimeError("Unrecognized equation syntax when trying to parse a NIST "
                           "equation: " + equation_text)

    print "groups captured: ", match.groups()
    print "group 0: ", match.group(0)

    # 'NIST equation syntax' => muparser syntax
    # brackets for muparser
    equation = equation.replace('[', '(')
    equation = equation.replace(']', ')')
    equation = equation.replace('arctan', 'atan')
    equation = equation.replace('**', '^')
    print "group 2: ", equation
    return equation

def parse_starting_values(lines):
    starting_vals = []
    print "parse starting values"
    for line in lines:
        if not line.strip() or line.startswith('Residual'):
            break

        comps = line.split()
        if 6 != len(comps) and 5 != len(comps):
            raise RuntimeError("Failed to parse this line as starting "
                               "values information: {0}".format(line))

        # A bit weak/lax parsing, if there is one less column, assume only one start point
        if 6 == len(comps):
            alt_values = [float(comps[2]), float(comps[3])]
        elif 5 == len(comps):
            alt_values = [float(comps[2])]

        starting_vals.append([comps[0], alt_values])

    return starting_vals

def parse_nist_file(spec_file):
    # spec_text = spec_file.read()
    # print problem_text

    #data_lines = re.search(r'Data.+\(lines[[:space:]]+(\d+).+to.+(\d+)\)', problem_text)
    #print "data_lines: " , data_lines.group(1)

    lines = spec_file.readlines()

    idx = 0
    data_idx = 0
    data_pattern_text = []
    residual_sum_sq = 0
    equation_text = None
    starting_values = None

    # The first line should be:
    # NIST/ITL StRD
    while idx < len(lines):
        line = lines[idx].strip()
        idx += 1

        if not line:
            continue

        if line.startswith('Model:'):
            print "Found model, first line: {0}".format(line)

            # Would skip number of parameters, and empty line, but not
            # adequate for all test problems
            # idx += 3

            # Before 'y = ...' there can be lines like 'pi = 3.14159...'
            while (not re.match(r'\s*y\s*=(.+)', lines[idx])\
                   and not re.match(r'\s*log\[y\]\s*=(.+)', lines[idx]))\
                   and idx < len(lines): # [\s*\+\s*e]
                print "Line didn't match: ", lines[idx]
                idx += 1
            # Next non-empty lines are assumed to continue the equation
            equation_text = ''
            while lines[idx].strip():
                equation_text += lines[idx].strip()
                idx += 1
            print "Equation lines found: {0}".format(equation_text)

        elif 'Starting values' in line or 'Starting Values' in line:
            # 1 empty line, and one heading line before values
            idx += 2
            starting_values = parse_starting_values(lines[idx:])
            idx += len(starting_values)
            print "Starting values, got: ", starting_values

        elif line.startswith('Residual Sum of Squares'):
            residual_sum_sq = float(line.split()[4])

        elif line.startswith('Data:'):
            if 0 == data_idx:
                print "First data, line: {0}".format(line)
                data_idx += 1
            elif 1 == data_idx:
                print "Found second data, idx: {0}".format(idx)
                data_pattern_text = lines[idx:]
                idx = len(lines)
            else:
                raise RuntimeError('Error parsing data line: {}'.format(line))
        else:
            print "unknown line: {0}".format(line)


    # print "Data pattern, text: {0}".format(data_pattern_text)
    data_pattern = parse_data_pattern(data_pattern_text)
    #print "data_pattern: ", data_pattern
    print "data_pattern_shape: ", data_pattern.shape
    res_sum_sq = residual_sum_sq
    print "Residual sum of squares: {0}".format(res_sum_sq)
    parsed_eq = parse_equation(equation_text)
    print "Parsed equation: {0}".format(parsed_eq)
    print "Starting values: {0}".format(starting_values)
    prob = test_problem.FittingTestProblem()
    prob.name = os.path.basename(spec_file.name)
    prob.linked_name = ("`{0} <http://www.itl.nist.gov/div898/strd/nls/data/{1}.shtml>`__".
                        format(prob.name, prob.name.lower()))
    prob.equation = parsed_eq
    prob.starting_values = starting_values
    prob.data_pattern_in = data_pattern[:, 1:]
    prob.data_pattern_out = data_pattern[:, 0]
    prob.ref_residual_sum_sq = res_sum_sq

    return prob

def parse_nist_fitting_problem_file(problem_filename):
    with open(problem_filename) as spec_file:
        return parse_nist_file(spec_file)

def parse_neutron_data_fitting_problem_file(fname):
    """
    Builds a FittingTestProblem object from a file. The file is expected to
    have a list of variables (input filename, name, equation, etc.)

    Other alternatives could be ConfigParser (ini format, parser not extermely
    good), or JSON.

    """
    prob = test_problem.FittingTestProblem()
    with open(fname) as probf:
        entries = get_neutron_data_problem_entries(probf)
        get_fitting_neutron_data(entries['input_file'], prob)
        prob.name = entries['name']
        prob.equation = entries['function']
        prob.starting_values = None
        if 'fit_parameters' in entries:
            prob.start_x = entries['fit_parameters']['StartX']
            prob.end_x = entries['fit_parameters']['EndX']

    return prob

def get_neutron_data_problem_entries(problem_file):
    """
    Get values from the lines of a "neutron fitting problem definition file",
    They are returned as a dictionary of key (lhs) - value (rhs).
    """
    entries = {}
    for line in problem_file:
        # discard comments
        line = line.partition('#')[0]
        line = line.rstrip()
        if not line:
            continue

        # take values (lhs = rhs)
        lhs, rhs = line.split("=", 1)
        # assumes it is safe to evaluate the rhs (it's a string for example)
        entries[lhs.strip()] = eval(rhs.strip())

    return entries

def get_fitting_neutron_data(fname, prob):
    """
    Load the X-Y data from a file and get the values into a fitting problem
    definition object.
    """
    import mantid.simpleapi as msapi

    wks = msapi.Load(fname)
    prob.data_pattern_in = wks.readX(0)
    prob.data_pattern_out = wks.readY(0)
    prob.data_pattern_obs_errors = wks.readE(0)
    prob.ref_residual_sum_sq = 0
