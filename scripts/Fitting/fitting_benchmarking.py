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
import test_result
import test_problem

def do_regression_fitting_benchmark(include_nist=True, include_cutest=True, minimizers=None, use_errors=True):
    # Several blocks of problems. Results for each block will go into a separate table
    problem_blocks = []

    if include_nist:
        problem_blocks.extend(get_nist_problem_files())

    if include_cutest:
        problem_blocks.extend([get_cutest_problem_files()])

    fit_results = [do_regression_fitting_benchmark_block(block, minimizers) for block in problem_blocks]
    return fit_results

def do_regression_fitting_benchmark_block(benchmark_problems, minimizers, use_errors = True):
    """
    Applies one minmizer to a block/list of test problems

    @param problem_files :: a list of list of files that define a block of
    test problems, for example the "lower difficulty" NIST files . The first list level
    is for different blocks of test problems. The second level is for different individual
    problems/files.
    """

    results_per_problem = []
    for prob_file in benchmark_problems:
        prob = iparsing.parse_nist_fitting_problem_file(prob_file)
        print("Testing fitting of problem {0} (file {1}".format(prob.name, prob_file))

        results_prob = do_regresion_fitting_benchmark_one_problem(prob, minimizers, use_errors)
        results_per_problem.extend(results_prob)

    return results_per_problem

def do_regresion_fitting_benchmark_one_problem(prob, minimizers, use_errors = True):
    """
    One problem with potentially several starting points, returns a list (start points) of
    lists (minimizers)

    @param prob :: fitting problem
    @param mnimizers :: list of minimizers to evaluate/compare
    @param use_errors :: whether to use observational errors when evaluating accuracy (in the
                         cost function)
    """

    if use_errors:
        wks = msapi.CreateWorkspace(DataX=prob.data_pattern_in, DataY=prob.data_pattern_out,
                                    DataE=np.sqrt(prob.data_pattern_in))
                                    # ERRORS. If all zeros, check Fit/IgnoreInvalidData
                                    # DataE=np.zeros((1, len(prob.data_pattern_in))))
        cost_function = 'Least squares'
    else:
        wks = msapi.CreateWorkspace(DataX=prob.data_pattern_in, DataY=prob.data_pattern_out)
        cost_function = 'Unweighted least squares'

    # For NIST problems this will generate two results per file (two different starting points)
    results_fit_problem = []
    num_starts = len(prob.starting_values[0][1])
    for start_idx in range(0, num_starts):

        print("=================== starting values,: {0}, with idx: {1} ================".
               format(prob.starting_values, start_idx))
        start_string = '' # like: 'b1=250, b2=0.0005'
        for param in prob.starting_values:
            start_string += ('{0}={1},'.format(param[0], param[1][start_idx]))


        if 'name' in prob.equation:
            user_func = prob.equation
        else:
            user_func = "name=UserFunction, Formula={0}, {1}".format(prob.equation, start_string)

        results_problem_start = []
        for minimizer_name in minimizers:
            t_start = time.clock()

            status, chi2, fit_wks, params, errors = run_fit(wks, function=user_func,
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
        # This would be meaningless, because not scaled/divided by n
        #avg_sum_sq = np.average([res.sum_err_sq for res in result_fits_minimizer])
        #avg_sum_sq = np.nanmean([res.sum_err_sq for res in result_fits_minimizer])
        #print ("Results for this minimizer '{0}', avg sum errors sq: {1}".
        #       format(minimizer_name, avg_sum_sq))
        results_fit_problem.append(results_problem_start)

    return results_fit_problem

def run_fit(wks, function, minimizer='Levenberg-Marquardt', cost_function='Least squares'):
    """
    Fits the data in a workspace with a function, using the algorithm Fit.
    Importantly, the option IgnoreInvalidData is enabled. Check the documentation of Fit for the
    implications of this.

    @param wks :: MatrixWorkspace with data to fit, in the format expected by the algorithm Fit
    @param function :: function definition as used in the algorithm Fit
    @param minimizer :: minimizer to use in Fit
    @param cost_function :: cost function to use in Fit

    @returns the fitted parameter values and error estimates for these
    """
    status = None
    chi2 = None
    covar_tbl = None
    param_tbl = None
    fit_wks = None
    try:
        ignore_invalid = True
        status, chi2, covar_tbl, param_tbl, fit_wks = msapi.Fit(function, wks, Output='ws',
                                                                Minimizer=minimizer,
                                                                CostFunction=cost_function,
                                                                IgnoreInvalidData=ignore_invalid)

        calc_chi2 = msapi.CalculateChiSquared(Function=function,
                                              InputWorkspace=wks, IgnoreInvalidData=ignore_invalid)
        print("*** with minimizer {0}, calculated: chi2: {1}".format(minimizer, calc_chi2))

    except RuntimeError as rerr:
        print("Warning, Fit probably failed. Going on. Error: {0}".format(str(rerr)))

    #param_tbl = mtd['ws_Parameters']

    if param_tbl:
        params = param_tbl.column(1)[:-1]
        errors = param_tbl.column(2)[:-1]
    else:
        params = None
        errors = None

    return status, chi2, fit_wks, params, errors

HARDCODED_REF_DIR = r'/home/fedemp/mantid-repos/mantid-fitting-benchmarking/Testing/SystemTests/tests/analysis/reference'
def get_nist_problem_files():
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

    nist_dir = os.path.join('fitting','NIST_nonlinear_regression') # 'reference/fitting/nist_nonlinear_regression/'
    # Could use this list to generate a table for all the problems at once
    # benchmark_problems = [os.path.join(ref_dir, nist_dir, fname) for fname in nist_names]

    # Find ref_dir
    data_search_dirs = msapi.ConfigService.Instance()["datasearch.directories"].split(';')
    ref_dir = None
    for data_dir in data_search_dirs:
        if 'reference' in data_dir:
            ref_dir = data_dir
            print("Found ref dir: {0}".format(ref_dir))
    if not ref_dir:
        raise RuntimeError("Could not find the benchmark data directory")

    ##### TEMPORAL OVERWRITE
    ref_dir = HARDCODED_REF_DIR

    nist_lower_files = [os.path.join(ref_dir, nist_dir, fname) for fname in nist_lower]
    nist_average_files = [os.path.join(ref_dir, nist_dir, fname) for fname in nist_average]
    nist_higher_files = [os.path.join(ref_dir, nist_dir, fname) for fname in nist_higher]
    problem_files = [nist_lower_files, nist_average_files, nist_higher_files]

    return problem_files

def get_cutest_problem_files():
    # TODO - fix this
    ref_dir = HARDCODED_REF_DIR

    cutest_all = [ 'PALMER6C.dat', 'PALMER7C.dat', 'PALMER8C.dat', 'YFITU.dat', 'VESUVIOLS.dat', 'DMN15102LS.dat' ]
    cutest_dir = os.path.join('fitting', 'CUTEst')
    cutest_files = [os.path.join(ref_dir, cutest_dir, fname) for fname in cutest_all]

    return cutest_files
