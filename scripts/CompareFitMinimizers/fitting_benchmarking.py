"""
Classes and utility functions to support benchmarking of fitting minimizers in
Mantid or other packages useable from Python.  These benchmarks are
focused on comparing different minimizers in terms of accuracy and
computation time.
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


def run_all_with_or_without_errors(base_problem_files_dir, use_errors, minimizers,
                                   group_names, group_suffix_names, color_scale, save_to_file=False):
    """
    Run all benchmark problems available, with/without using weights in the cost
    function. This is just a convenience function meant to be used by system/unit tests, or other scripts

    ALL means: NIST + CUTEST + any fitting problems against observed Neutron data (+ any other
    which may be added in the future)

    At this point in time it is assumed that the problem files are in store relative to
    a base_problem_files_dir as follows:

        CUTEst/
        NIST_nonlinear_regression/
        Neutron_data/

    Be warned this function can be verbose.

    @param base_problem_files_dir :: base directory path of all the benchmark problems
    @param use_errors :: whether to use observational errors as weights in the cost function
    @param minimizers :: list of minimizers to test
    @param group_names :: names for display purposes
    @param group_suffix_names :: group names to use as suffixes, for example in file names
    @param color_scale :: list with pairs of threshold value - color, to produce color
    @param save_to_file :: whether to save the table outputs to files following specific naming conventions
    """

    # Assume the benchmark problems are stores as follows
    nist_group_dir = os.path.join(base_problem_files_dir, 'NIST_nonlinear_regression')
    cutest_group_dir = os.path.join(base_problem_files_dir, 'CUTEst')
    neutron_data_group_dirs = [os.path.join(base_problem_files_dir, 'Neutron_data')]

    problems, results_per_group = do_fitting_benchmark(nist_group_dir=nist_group_dir,
                                                       cutest_group_dir=cutest_group_dir,
                                                       neutron_data_group_dirs=neutron_data_group_dirs,
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
                                          simple_text=True, rst=True, save_to_file=save_to_file,
                                          color_scale=color_scale)

    # Results aggregated (median) by group (NIST, Neutron data, CUTEst, etc.)
    header = '\n\n**************** OVERALL SUMMARY - ALL GROUPS ******** \n\n'
    print(header)
    fitout.print_overall_results_table(minimizers, results_per_group, problems, group_names,
                                       use_errors=use_errors, save_to_file=save_to_file)

    # Flush to reduce mix-up with system tests/runner output
    import sys
    sys.stdout.flush()


def do_fitting_benchmark(nist_group_dir=None, cutest_group_dir=None, neutron_data_group_dirs=None,
                         minimizers=None, use_errors=True):
    """
    Run a fit minimizer benchmark against groups of fitting problems.

    Unless group directories of fitting problems are specified no fitting benchmarking is done.

    NIST and CUTEst refer to the NIST and CUTEst fitting test problem sets, where
    for example CUTEst is used for fit tests in the mathematical numerical literature.

    The Neutron_data group contain fit tests against real noisy experimental neutron data.
    This latter group may grow to contain fitting example from multiple directories.

    @param nist_group_dir :: whether to try to load NIST problems
    @param cutest_group_dir :: whether to try to load CUTEst problems
    @param neutron_data_group_dirs :: base directory where fitting problems are located including NIST+CUTEst
    @param minimizers :: list of minimizers to test
    @param use_errors :: whether to use observational errors as weights in the cost function
    """

    # Several blocks of problems. Results for each block will be calculated sequentially, and
    # will go into a separate table
    problem_blocks = []

    if nist_group_dir:
        # get NIST problems grouped into blocks of problems, in blocks of increasing difficulty
        problem_blocks.extend(get_nist_problem_files(nist_group_dir))

    if cutest_group_dir:
        # get CUTEet problems - just treated as one block of problems
        problem_blocks.extend([get_cutest_problem_files(cutest_group_dir)])

    if neutron_data_group_dirs:
        problem_blocks.extend(get_data_groups(neutron_data_group_dirs))

    prob_results = [do_fitting_benchmark_group(block, minimizers, use_errors=use_errors) for
                    block in problem_blocks]

    probs, results = list(zip(*prob_results))

    if len(probs) != len(results):
        raise RuntimeError('probs : {0}, prob_results: {1}'.format(len(probs), len(results)))

    return probs, results


def do_fitting_benchmark_group(problem_files, minimizers, use_errors=True):
    """
    Applies minimizers to a group (collection) of test problems. For example the
    collection of all NIST problems

    @param problem_files :: a list of list of files that define a group of
    test problems. For example all the NIST files sub-groupped according to level
    of fitting difficulty, where the lowest list level list the file names

    @param minimizers :: list of minimizers to test
    @param use_errors :: whether to use observational errors as weights in the cost function

    @returns :: problem definitions loaded from the files, and results of running them with
    the minimizers requested
    """

    problems = []
    results_per_problem = []
    for prob_file in problem_files:
        try:
            # Note the CUTEst problem are assumed to be expressed in NIST format
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
    lists (minimizers).

    @param prob :: fitting problem
    @param minimizers :: list of minimizers to evaluate/compare
    @param use_errors :: whether to use observational errors when evaluating accuracy (in the
                         cost function)
    """

    wks, cost_function = prepare_wks_cost_function(prob, use_errors)

    # Each NIST problem generate two results per file - from two different starting points
    results_fit_problem = []

    # Get function definitions for the problem - one for each starting point
    function_defs = get_function_definitions(prob)

    # Loop over the different starting points
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
            # If the fit has failed, also set the runtime to NaN
            result.runtime = t_end - t_start if not np.isnan(chi2) else np.nan
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
    if use_errors:
        data_e = None
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
            start_string = ''  # like: 'b1=250, b2=0.0005'
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
    Group the NIST problem files into separeate blocks according
    to assumed fitting different levels: lower, average,
    higher.

    @returns :: list of list of problem files
    """
    # Grouped by "level of difficulty"
    nist_lower = ['Misra1a.dat', 'Chwirut2.dat', 'Chwirut1.dat', 'Lanczos3.dat',
                  'Gauss1.dat', 'Gauss2.dat', 'DanWood.dat', 'Misra1b.dat']

    nist_average = ['Kirby2.dat', 'Hahn1.dat',
                    # 'Nelson.dat' needs log[y] parsing / DONE, needs x1, x2
                    'MGH17.dat', 'Lanczos1.dat', 'Lanczos2.dat', 'Gauss3.dat',
                    'Misra1c.dat', 'Misra1d.dat',
                    # 'Roszman1.dat' <=== needs handling the  'pi = 3.1415...' / DOME
                    # And the 'arctan()'/ DONE, but generated lots of NaNs
                    'ENSO.dat']
    nist_higher = ['MGH09.dat', 'Thurber.dat', 'BoxBOD.dat', 'Rat42.dat',
                   'MGH10.dat', 'Eckerle4.dat', 'Rat43.dat', 'Bennett5.dat']

    nist_lower_files = [os.path.join(search_dir, fname) for fname in nist_lower]
    nist_average_files = [os.path.join(search_dir, fname) for fname in nist_average]
    nist_higher_files = [os.path.join(search_dir, fname) for fname in nist_higher]
    problem_files = [nist_lower_files, nist_average_files, nist_higher_files]

    return problem_files


def get_cutest_problem_files(search_dir):

    cutest_all = ['PALMER6C.dat', 'PALMER7C.dat', 'PALMER8C.dat', 'YFITU.dat', 'VESUVIOLS.dat', 'DMN15102LS.dat']

    cutest_files = [os.path.join(search_dir, fname) for fname in cutest_all]

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
