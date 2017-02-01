## Usage

The benchmarking scripts can be run via the `FittingBenchmarks.py` system test.

## Code Structure

The following psuedocode demonstrates how the benchmarking scripts work:

> Fitting settings (data directories, minimizers, etc.) passed into 
`fitting_benchmarking.do_fitting_benchmark`:

> > Nested list of groups containing fit problems created

> > Each group passed into `fitting_benchmarking.do_fitting_benchmark_group`

> > > For each file in the group:

> > > > File parsed for relevant data, and a `FittingTestProblem` object created from this

> > > > `FittingTestProblem` object passed into `fitting_benchmarking.do_fitting_benchmark_one_problem`

> > > > > For each starting point:

> > > > > >  Fit the data via `fitting_benchmarking.run_fit` 

> > > > > Return a list of (starting points * minimizers) `FittingTestResult` objects (per file)

> > > Return a list of every file's individual results for the group

> > Return a nested list of results per group

> An entire list of groups (or an individual group) can be passed into a chosen `results_output.py` function to be printed out and optionally saved to file