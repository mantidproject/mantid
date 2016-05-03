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

                   

def do_regression_fitting_benchmark_minimizer(include_nist, include_cutest, minimizer_name):
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

    nist_dir = 'nist_nonlinear_regression' # 'reference/nist_nonlinear_regression/'
    benchmark_problems = [os.path.join(ref_dir, nist_dir, fname) for fname in nist_names]


    result_fits_minimizer = []
    for prob_file in benchmark_problems:
        prob = parse_nist_fitting_problem_file(prob_file)
        print "Testing fitting of problem {0} (file {1}".format(prob.name, prob_file)

        use_errors = False

        if use_errors:
            wks = msapi.CreateWorkspace(DataX=prob.data_pattern_in, DataY=prob.data_pattern_out,
                                        DataE=np.sqrt(prob.data_pattern_in))
                                        # ERRORS. If all zeros, check Fit/IgnoreInvalidData
                                        # DataE=np.zeros((1, len(prob.data_pattern_in))))
            cost_function = 'Least squares'
        else:
            wks = msapi.CreateWorkspace(DataX=prob.data_pattern_in, DataY=prob.data_pattern_out)
            cost_function = 'Unweighted least squares'
            
        num_starts = len(prob.starting_values[0][1])
        for start_idx in range(0, num_starts):
            
            print ("=================== starting values,: {0}, with idx: {1} ================".
                   format(prob.starting_values, start_idx))
            start_string = '' # like: 'b1=250, b2=0.0005'
            for param in prob.starting_values:
                start_string += ('{0}={1},'.format(param[0], param[1][start_idx]))
            user_func = "name=UserFunction, Formula={0}, {1}".format(prob.equation, start_string)
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
            print "Resul: {0}".format(result)
            result_fits_minimizer.append(result)

    # meaningless, because not scaled/divided by n
    avg_sum_sq = np.average([res.sum_err_sq for res in result_fits_minimizer])
    print ("Results for this minimizer '{0}', avg sum errors sq: {1}".
           format(minimizer_name, avg_sum_sq))

    return result_fits_minimizer



def do_regression_fitting_benchmark(include_nist=True, include_cutest=False, minimizers=None):

    results = []
    for minim in minimizers:
        result_minim = do_regression_fitting_benchmark_minimizer(include_nist, include_cutest, minim)
        results.append(result_minim)
    return results
        


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
        
    def test_rank_by_chi2(self):
        minimizers = ['Levenberg-Marquardt', 'Simplex', 'FABADA',
                      'Conjugate gradient (Fletcher-Reeves imp.)', 'Conjugate gradient (Polak-Ribiere imp.)',
                      'BFGS', 'Levenberg-MarquardtMD']
        results_per_minimizer = do_regression_fitting_benchmark(include_nist=True, minimizers=minimizers)
        # do_regression_fitting_benchmark()
        header = " ============= Comparison of sum of square errors: ===============\n"
        header = "\t\t"
        for minimiz in minimizers:
            header += " {0} |".format(minimiz)
        header +="\n"
        print header

        num_tests = len(results_per_minimizer[0])
        num_minimizers = len(minimizers)

        # An np matrix for convenience
        # 1 row per problem+start, 1 column per minimizer
        results_tbl = np.zeros((num_tests, num_minimizers))
        time_tbl = np.zeros((num_tests, num_minimizers))
        for test_idx in range(0, num_tests):
            for minimiz_idx in range(0, num_minimizers):
                results_tbl[test_idx, minimiz_idx] = results_per_minimizer[minimiz_idx][test_idx].sum_err_sq
                time_tbl[test_idx, minimiz_idx] = results_per_minimizer[minimiz_idx][test_idx].runtime
        # Min across all minimizers
        min_sum_err_sq = np.amin(results_tbl, 1)
        min_runtime = np.amin(time_tbl, 1)

        results_text = ''
        for test_idx in range(0, num_tests):
            results_text += "{0}\t".format(results_per_minimizer[0][test_idx].problem.name)
            for minimiz_idx, minimiz in enumerate(minimizers):
                # 'e' format is easier to read in raw text output than 'g'
                results_text += (" {0:.10g}".
                                 format(results_per_minimizer[minimiz_idx][test_idx].sum_err_sq /
                                        min_sum_err_sq[test_idx]))
            results_text += "\n"

        prec_digits = 5
        np.set_printoptions(precision=prec_digits)
        
        norm_rankings = results_tbl / min_sum_err_sq[:, None]
        # summary lines
        results_text += '---------------- Summary (accuracy): -------- \n'
        results_text += 'Best ranking: {0}\n'.format(np.amin(norm_rankings, 0))
        results_text += 'Worst ranking: {0}\n'.format(np.amax(norm_rankings, 0))
        results_text += 'Average: {0}\n'.format(np.average(norm_rankings, 0))
        results_text += 'Median: {0}\n'.format(np.median(norm_rankings, 0))
        results_text += '\n'
        results_text += 'First quartile: {0}\n'.format(np.percentile(norm_rankings, 25, axis=0))
        results_text += 'Third quartile: {0}\n'.format(np.percentile(norm_rankings, 75, axis=0))

        print results_text

        print " ======== Time: ======="
        time_text = ''
        for test_idx in range(0, num_tests):
            time_text += "{0}\t".format(results_per_minimizer[0][test_idx].problem.name)
            for minimiz_idx, minimiz in enumerate(minimizers):
                time_text += " {0}".format(results_per_minimizer[minimiz_idx][test_idx].runtime)
            time_text += "\n"

        norm_runtimes = time_tbl / min_runtime[:, None]
        time_text += '---------------- Summary (run time): -------- \n'
        time_text += 'Best ranking: {0}\n'.format(np.amin(norm_runtimes, 0))
        time_text += 'Worst ranking: {0}\n'.format(np.amax(norm_runtimes, 0))
        time_text += 'Average: {0}\n'.format(np.average(norm_runtimes, 0))
        time_text += 'Median: {0}\n'.format(np.median(norm_runtimes, 0))
        time_text += '\n'
        time_text += 'First quartile: {0}\n'.format(np.percentile(norm_runtimes, 25, axis=0))
        time_text += 'Third quartile: {0}\n'.format(np.percentile(norm_runtimes, 75, axis=0))

        print time_text

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
