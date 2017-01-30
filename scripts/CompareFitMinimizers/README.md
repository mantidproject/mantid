## Usage

The benchmarking scripts can be run via the `FittingBenchmarks.py` system test.

## Code Structure

The following psuedocode demonstrates how the benchmarking scripts work:

> Fitting settings (data directories, minimizers, etc.) passed into 
`fitting_benchmarking.do_fitting_benchmark`:

> > Nested list of problems problem per group created

> > Each group passed into `fitting_benchmarking.do_fitting_benchmark_group`

> > > For each file in the group:

> > > > File parsed for relevant data, and a `FittingTestProblem` created from this

> > > > Problem object passed into `fitting_benchmarking.do_fitting_benchmark_one_problem`

> > > > > Fitting done via `fitting_benchmarking.run_fit`

> > > > > > Return a list containing (starting points * minimizers) `FittingTestResult`s per file

> > > > Return a list of `FittingTestResult`s for the group

> > Return a nested list of results per group

> The list of groups (or files within) can be passed into a chosen `results_output.py` function to be printed out and optionally saved to file
