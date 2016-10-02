"""
Classes and utility functions to support benchmarking of fitting in
Mantid or other packages useable from Python.  These benchmarks are
focused on comparing differnt minimizers in terms of accuracy and
computation time (or iterations of the minimizers).
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

import os
import time

import numpy as np

import mantid.simpleapi as msapi

import input_parsing as iparsing
import results_output as fitout
import test_result

def run_all_with_or_without_errors(problem_files_path, use_errors, minimizers,
                                   group_names, group_suffix_names, color_scale):
    """
    Run all benchmark problems available, with/without using weights in the cost
    function (observational errors). This is just a convenience
    function meant to be used by system/unit tests, or other scripts.

    Be warned this function ca be extremely verbose.

    @param minimizers :: list of minimizers to test
    @param group_names :: names for display purposes
    @param group_suffix_names :: group names to use as suffixes, for example in file names
    @param color_scale :: list with pairs of threshold value - color, to produce color
    """

    problems, results_per_group = do_fitting_benchmark(include_nist=True, include_cutest=True,
                                                       data_groups_dirs = problem_files_path,
                                                       minimizers=minimizers, use_errors=use_errors)

    # Results for every test problem in each group
    for idx, group_results in enumerate(results_per_group):
        print("\n\n")
        print("********************************************************")
        print("**************** RESULTS FOR GROUP {0}, {1} ************".format(idx+1,
                                                                                group_names[idx]))
        print("********************************************************")
        fitout.print_group_results_tables(minimizers, group_results, problems[idx],
                                          group_name=group_suffix_names[idx],
                                          use_errors=use_errors,
                                          simple_text=True, rst=True, color_scale=color_scale)

    # Results aggregated (median) by group (NIST, Neutron data, CUTEst, etc.)
    header = '\n\n**************** OVERALL SUMMARY - ALL GROUPS ******** \n\n'
    print(header)
    fitout.print_overall_results_table(minimizers, results_per_group, problems, group_names,
                                       use_errors=use_errors, rst=True)

    # Flush to reduce mix-up with system tests/runner output
    import sys
    sys.stdout.flush()

def do_fitting_benchmark(include_nist=True, include_cutest=True, data_groups_dirs=None,
                         minimizers=None, use_errors=True):
    """
    Run a fitting benchmark made of different groups (NIST, CUTEst,
    Neutron data, etc.)  This gets the groups of files that look like
    test problem definitions and runs tests for every group.

    @param include_nist :: whether to try to load NIST problems
    @param include_nist :: whether to try to load CUTEst problems
    @param data_groups_dirs :: list of directories where to load other / neutron data problems
    @param minimizers :: list of minimizers to test
    @param use_errors :: whether to use observational errors as weights in the cost function
    """
    if data_groups_dir:
        search_dir = os.path.split(data_groups_dirs[0])[0]
    else:
        search_dir = os.getcwd()

    # Several blocks of problems. Results for each block will be calculated sequentially, and
    # will go into a separate table
    problem_blocks = []

    if include_nist:
        problem_blocks.extend(get_nist_problem_files(search_dir))

    if include_cutest:
        problem_blocks.extend([get_cutest_problem_files(search_dir)])

    if data_groups_dirs:
        problem_blocks.extend(get_data_groups(data_groups_dirs))

    prob_results = [do_fitting_benchmark_group(block, minimizers, use_errors=use_errors) for
                    block in problem_blocks]

    probs, results = zip(*prob_results)

    if len(probs) != len(results):
        raise RuntimeError('probs : {0}, prob_results: {1}'.format(len(probs), len(results)))

    return probs, results

def do_fitting_benchmark_group(problem_files, minimizers, use_errors=True):
    """
    Applies one minmizer to a block/list of test problems

    @param problem_files :: a list of list of files that define a block of
    test problems, for example the "lower difficulty" NIST files . The first list level
    is for different blocks of test problems. The second level is for different individual
    problems/files.

    @param minimizers :: list of minimizers to test
    @param use_errors :: whether to use observational errors as weights in the cost function

    @returns :: problem definitions loaded from the files, and results of running them with
    the minimizers requested
    """

    problems = []
    results_per_problem = []
    for prob_file in problem_files:
        try:
            prob = iparsing.load_nist_fitting_problem_file(prob_file)
        except (AttributeError, RuntimeError):
            prob = iparsing.load_neutron_data_fitting_problem_file(prob_file)

        print("* Testing fitting for problem definition file {0}".format(prob_file))
        print("* Testing fitting of problem {0}".format(prob.name))

        results_prob = do_fitting_benchmark_one_problem(prob, minimizers, use_errors)
        results_per_problem.extend(results_prob)

    return problems, results_per_problem

def do_fitting_benchmark_one_problem(prob, minimizers, use_errors=True):
    """
    One problem with potentially several starting points, returns a list (start points) of
    lists (minimizers)

    @param prob :: fitting problem
    @param mnimizers :: list of minimizers to evaluate/compare
    @param use_errors :: whether to use observational errors when evaluating accuracy (in the
                         cost function)
    """

    wks, cost_function = prepare_wks_cost_function(prob, use_errors)

    # For NIST problems this will generate two results per file (two different starting points)
    results_fit_problem = []

    function_defs = get_function_definitions(prob)

    for user_func in function_defs:
        results_problem_start = []
        for minimizer_name in minimizers:
            t_start = time.clock()

            status, chi2, fit_wks, params, errors = run_fit(wks, prob, function=user_func,
                                                            minimizer=minimizer_name,
                                                            cost_function=cost_function)
            t_end = time.clock()
            print("*** with minimizer {0}, Status: {1}, chi2: {2}".format(minimizer_name, status, chi2))
            print("   params: {0}, errors: {1}".format(params, errors))

            def sum_of_squares(values):
                return np.sum(np.square(values))

            if fit_wks:
                sum_err_sq = sum_of_squares(fit_wks.readY(2))
                # print " output simulated values: {0}".format(fit_wks.readY(1))
            else:
                sum_err_sq = float("inf")
                print(" WARNING: no output fit workspace")

            print("   sum sq: {0}".format(sum_err_sq))

            result = test_result.FittingTestResult()
            result.problem = prob
            result.fit_status = status
            result.fit_chi2 = chi2
            result.params = params
            result.errors = errors
            result.sum_err_sq = sum_err_sq
            result.runtime = t_end - t_start
            print("Result object: {0}".format(result))
            results_problem_start.append(result)

        results_fit_problem.append(results_problem_start)

    return results_fit_problem

def run_fit(wks, prob, function, minimizer='Levenberg-Marquardt', cost_function='Least squares'):
    """
    Fits the data in a workspace with a function, using the algorithm Fit.
    Importantly, the option IgnoreInvalidData is enabled. Check the documentation of Fit for the
    implications of this.

    @param wks :: MatrixWorkspace with data to fit, in the format expected by the algorithm Fit
    @param prob :: Problem definition
    @param function :: function definition as used in the algorithm Fit
    @param minimizer :: minimizer to use in Fit
    @param cost_function :: cost function to use in Fit

    @returns the fitted parameter values and error estimates for these
    """
    status = None
    chi2 = None
    param_tbl = None
    fit_wks = None
    try:
        # When using 'Least squares' (weighted by errors), ignore nans and zero errors, but don't
        # ignore them when using 'Unweighted least squares' as that would ignore all values!
        ignore_invalid = cost_function == 'Least squares'

        # Note the ugly adhoc exception. We need to reconsider these WISH problems:
        if 'WISH17701' in prob.name:
            ignore_invalid = False

        status, chi2, covar_tbl, param_tbl, fit_wks = msapi.Fit(function, wks, Output='ws_fitting_test',
                                                                Minimizer=minimizer,
                                                                CostFunction=cost_function,
                                                                IgnoreInvalidData=ignore_invalid,
                                                                StartX=prob.start_x, EndX=prob.end_x)

        calc_chi2 = msapi.CalculateChiSquared(Function=function,
                                              InputWorkspace=wks, IgnoreInvalidData=ignore_invalid)
        print("*** with minimizer {0}, calculated: chi2: {1}".format(minimizer, calc_chi2))

    except RuntimeError as rerr:
        print("Warning, Fit probably failed. Going on. Error: {0}".format(str(rerr)))

    if param_tbl:
        params = param_tbl.column(1)[:-1]
        errors = param_tbl.column(2)[:-1]
    else:
        params = None
        errors = None

    return status, chi2, fit_wks, params, errors

def prepare_wks_cost_function(prob, use_errors):
    """
    Build a workspace ready for Fit() and a cost function string according to the problem
    definition.
    """
    data_e = None
    if use_errors:
        if not isinstance(prob.data_pattern_obs_errors, np.ndarray):
            # Fake observational errors
            data_e = np.sqrt(prob.data_pattern_in)
        else:
            data_e = prob.data_pattern_obs_errors

        wks = msapi.CreateWorkspace(DataX=prob.data_pattern_in, DataY=prob.data_pattern_out,
                                    DataE=data_e)
        cost_function = 'Least squares'
    else:
        wks = msapi.CreateWorkspace(DataX=prob.data_pattern_in, DataY=prob.data_pattern_out)
        cost_function = 'Unweighted least squares'

    return wks, cost_function

def get_function_definitions(prob):
    """
    Produces function definition strings (as a full definition in
    muparser format, including the function and the initial values for
    the parameters), one for every different starting point defined in
    the test problem.

    @param prob :: fitting test problem object

    @returns :: list of function strings ready for Fit()
    """
    function_defs = []
    if prob.starting_values:
        num_starts = len(prob.starting_values[0][1])
        for start_idx in range(0, num_starts):

            print("=================== starting values,: {0}, with idx: {1} ================".
                  format(prob.starting_values, start_idx))
            start_string = '' # like: 'b1=250, b2=0.0005'
            for param in prob.starting_values:
                start_string += ('{0}={1},'.format(param[0], param[1][start_idx]))

            if 'name' in prob.equation:
                function_defs.append(prob.equation)
            else:
                function_defs.append("name=UserFunction, Formula={0}, {1}".format(prob.equation, start_string))
    else:
        # Equation from a neutron data spec file. Ready to be used
        function_defs.append(prob.equation)

    return function_defs

def get_nist_problem_files(search_dir):
    """
       Gets the problem files grouped in blocks, where the blocks would
       normally be the different levels of difficulty (lower, average,
       higher).
    """
    # Grouped by "level of difficulty"
    nist_lower = [ 'Misra1a.dat', 'Chwirut2.dat', 'Chwirut1.dat', 'Lanczos3.dat',
                   'Gauss1.dat', 'Gauss2.dat', 'DanWood.dat', 'Misra1b.dat' ]

    nist_average = [ 'Kirby2.dat', 'Hahn1.dat',
                     # 'Nelson.dat' needs log[y] parsing / DONE, needs x1, x2
                     'MGH17.dat', 'Lanczos1.dat', 'Lanczos2.dat', 'Gauss3.dat',
                     'Misra1c.dat', 'Misra1d.dat',
                     # 'Roszman1.dat' <=== needs handling the  'pi = 3.1415...' / DOME
                     # And the 'arctan()'/ DONE, but generated lots of NaNs
                     'ENSO.dat' ]
    nist_higher = [ 'MGH09.dat','Thurber.dat', 'BoxBOD.dat', 'Rat42.dat',
                    'MGH10.dat', 'Eckerle4.dat', 'Rat43.dat', 'Bennett5.dat' ]

    nist_names = nist_lower + nist_average + nist_higher

    nist_subdir = 'NIST_nonlinear_regression'

    nist_lower_files = [os.path.join(search_dir, nist_subdir, fname) for fname in nist_lower]
    nist_average_files = [os.path.join(search_dir, nist_subdir, fname) for fname in nist_average]
    nist_higher_files = [os.path.join(search_dir, nist_subdir, fname) for fname in nist_higher]
    problem_files = [nist_lower_files, nist_average_files, nist_higher_files]

    return problem_files

def get_cutest_problem_files(search_dir):

    cutest_all = [ 'PALMER6C.dat', 'PALMER7C.dat', 'PALMER8C.dat', 'YFITU.dat', 'VESUVIOLS.dat', 'DMN15102LS.dat' ]
    cutest_subdir = 'CUTEst'
    cutest_files = [os.path.join(search_dir, cutest_subdir, fname) for fname in cutest_all]

    return cutest_files

def get_data_groups(data_groups_dirs):

    problem_groups = []
    for grp_dir in data_groups_dirs:
        problem_groups.append(get_data_group_problem_files(grp_dir))

    return problem_groups

def get_data_group_problem_files(grp_dir):
    import glob

    search_str = os.path.join(grp_dir, "*.txt")
    probs = glob.glob(search_str)

    probs.sort()
    print ("Found test problem files: ", probs)
    return probs
