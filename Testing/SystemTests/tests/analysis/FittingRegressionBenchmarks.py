"""
Benchmarks for accuracy (and time / iterations).
"""
import unittest
import stresstesting


#### REORGANIZE
import os
import re
import time

import numpy as np


class FittingTestProblem:

    def __init__(self):
        name = None
        equation = None
        starting_values = None
        data_pattern_in = None
        data_pattern_out = None
        ref_residual_sum_sq = None


class FittingTestResult:
    def __init__(self):
        problem = None
        fit_status = None
        fit_chi2 = None
        fit_wks = None
        params = None
        errors = None
        sum_err_sq = None
        runtime = None
        
        

def parse_data_pattern(data_text):

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
        if 6 != len(comps):
            raise RuntimeError("Failed to parse this line as starting "
                               "values information: {0}".format(line))

        alt_values = [float(comps[2]), float(comps[3])]
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

        if line.startswith('Model'):
            print "Found model"

            # Would skip number of parameters, and empty line, but not
            # adequate for all test problems
            # idx += 3

            # Before 'y = ...' there can be lines like 'pi = 3.14159...'
            while (not re.match(r'\s*y\s*=(.+)', lines[idx])\
                   and not re.match(r'\s*log\[y\]\s*=(.+)', lines[idx]))\
                   and idx < len(lines): # [\s*\+\s*e]
                print "Didn't match: ", lines[idx]
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

        elif line.startswith('Residual Sum of Squares'):
            residual_sum_sq = float(line.split()[4])

        elif line.startswith('Data:'):
            if 0 == data_idx:
                print "First data"
                data_idx += 1
            elif 1 == data_idx:
                print "Found second data, idx: {0}".format(idx)
                data_pattern_text = lines[idx:]
                idx = len(lines)
            else:
                raise RuntimeError('Error')
        else:
            print "unknown line"


    # print "Data pattern, text: {0}".format(data_pattern_text)
    data_pattern = parse_data_pattern(data_pattern_text)
    #print "data_pattern: ", data_pattern
    print "data_pattern_shape: ", data_pattern.shape
    res_sum_sq = residual_sum_sq
    print "Residual sum of squares: {0}".format(res_sum_sq)
    parsed_eq = parse_equation(equation_text)
    print "Parsed equation: {0}".format(parsed_eq)
    print "Starting values: {0}".format(starting_values)
    prob = FittingTestProblem()
    prob.name = os.path.basename(spec_file.name)
    prob.equation = parsed_eq
    prob.starting_values = starting_values
    prob.data_pattern_in = data_pattern[:, 1:]
    prob.data_pattern_out = data_pattern[:, 0]
    prob.ref_residual_sum_sq = res_sum_sq

    return prob


def parse_nist_fitting_problem_file(problem_filename):
    with open(problem_filename) as spec_file:
        return parse_nist_file(spec_file)




import mantid.simpleapi as msapi

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
        print "*** with minimizer {0}, calculated: chi2: {1}".format(minimizer, calc_chi2)

    except RuntimeError as rerr:
        print "Warning, Fit probably failed. Going on. Error: {0}".format(str(rerr))

    #param_tbl = mtd['ws_Parameters']

    if param_tbl:
        params = param_tbl.column(1)[:-1]
        errors = param_tbl.column(2)[:-1]
    else:
        params = None
        errors = None

    return status, chi2, fit_wks, params, errors





data_search_dirs = msapi.ConfigService.Instance()["datasearch.directories"].split(';')
ref_dir = None
for data_dir in data_search_dirs:
    if 'reference' in data_dir:
        ref_dir = data_dir
        print "Found ref dir: {0}".format(ref_dir)
if not ref_dir:
    raise RuntimeError("Could not find the benchmark data directory")
        
##### TEMPORAL OVERWRITE
ref_dir = r'/home/fedemp/mantid-repos/mantid-fitting-benchmarking/Testing/SystemTests/tests/analysis/reference'



def get_nist_problem_files():
    """ Gets the problem files grouped in blocks """
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

    nist_lower_files = [os.path.join(ref_dir, nist_dir, fname) for fname in nist_lower]
    nist_average_files = [os.path.join(ref_dir, nist_dir, fname) for fname in nist_average]
    nist_higher_files = [os.path.join(ref_dir, nist_dir, fname) for fname in nist_higher]
    problem_files = [nist_lower_files, nist_average_files, nist_higher_files]

    return problem_files

def do_regression_fitting_benchmark(include_nist=True, include_cutest=False, minimizers=None):
    problem_blocks = []

    if include_nist:
        problem_blocks.extend(get_nist_problem_files())

    if include_cutest:
        problem_blocks.extend([cutest_all])

    fit_results = [do_regression_fitting_benchmark_block(block, minimizers) for block in problem_blocks]
    return fit_results

def do_regression_fitting_benchmark_block(benchmark_problems, minimizers):
    """
    Applies one minmizer to a block/list of test problems

    @param problem_files :: a list of list of files that define a block of
    test problems, for example the "lower difficulty" NIST files . The first list level
    is for different blocks of test problems. The second level is for different individual
    problems/files.
    """

    results_per_problem = []
    for prob_file in benchmark_problems:
        prob = parse_nist_fitting_problem_file(prob_file)
        print "Testing fitting of problem {0} (file {1}".format(prob.name, prob_file)

        results_prob = do_regresion_fitting_benchmark_one_problem(prob, minimizers)
        results_per_problem.extend(results_prob)

    return results_per_problem

def do_regresion_fitting_benchmark_one_problem(prob, minimizers, use_errors = True):
    """
    One problem with potentially several starting points, returns a list (start points) of
    lists (minimizers)
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

        print ("=================== starting values,: {0}, with idx: {1} ================".
               format(prob.starting_values, start_idx))
        start_string = '' # like: 'b1=250, b2=0.0005'
        for param in prob.starting_values:
            start_string += ('{0}={1},'.format(param[0], param[1][start_idx]))
        user_func = "name=UserFunction, Formula={0}, {1}".format(prob.equation, start_string)

        results_problem_start = []
        for minimizer_name in minimizers:
            t_start = time.clock()

            status, chi2, fit_wks, params, errors = run_fit(wks, function=user_func,
                                                            minimizer=minimizer_name,
                                                            cost_function=cost_function)
            t_end = time.clock()
            print "*** with minimizer {0}, Status: {1}, chi2: {2}".format(minimizer_name, status, chi2)
            print "   params: {0}, errors: {1}".format(params, errors)

            def sum_of_squares(values):
                return np.sum(np.square(values))

            if fit_wks:
                sum_err_sq = sum_of_squares(fit_wks.readY(2))
                # print " output simulated values: {0}".format(fit_wks.readY(1))
            else:
                sum_err_sq = float("inf")
                print " WARNING: no output fit workspace"

            print "   sum sq: {0}".format(sum_err_sq)

            result = FittingTestResult()
            result.problem = prob
            result.fit_status = status
            result.fit_chi2 = chi2
            result.params = params
            result.errors = errors
            result.sum_err_sq = sum_err_sq
            result.runtime = t_end - t_start
            print "Result object: {0}".format(result)
            results_problem_start.append(result)
        # meaningless, because not scaled/divided by n
        #avg_sum_sq = np.average([res.sum_err_sq for res in result_fits_minimizer])
        #avg_sum_sq = np.nanmean([res.sum_err_sq for res in result_fits_minimizer])
        #print ("Results for this minimizer '{0}', avg sum errors sq: {1}".
        #       format(minimizer_name, avg_sum_sq))
        results_fit_problem.append(results_problem_start)

    return results_fit_problem



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
        minimizers_alpha = ['Levenberg-Marquardt', 'Simplex', 'FABADA',
                            'Conjugate gradient (Fletcher-Reeves imp.)',
                            'Conjugate gradient (Polak-Ribiere imp.)',
                            'BFGS', 'Levenberg-MarquardtMD', 'SteepestDescent']
        # TODO: expose API::FuncMinimizerFactory to Python
        # TOTHINK: use different interface as in the API::FunctionFactory?
        minimizers_factory = ['BFGS', 'Conjugate gradient (Fletcher-Reeves imp.)',
                              'Conjugate gradient (Polak-Ribiere imp.)', 'Damping',
                               #'FABADA', # hide FABADA
                              'Levenberg-Marquardt', 'Levenberg-MarquardtMD',
                              'Simplex', 'SteepestDescent']
        minimizers = minimizers_factory
        results_per_block = do_regression_fitting_benchmark(include_nist=True, minimizers=minimizers)
        # do_regression_fitting_benchmark()


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
