# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.
import argparse
import os

from mantid import __version__ as mtd_version
import warnings


def main(args=None):
    # setup command line arguments
    parser = argparse.ArgumentParser(description="Mantid Workbench")
    parser.add_argument("script", nargs="?")
    parser.add_argument("--version", action="version", version=mtd_version)
    parser.add_argument("-x", "--execute", action="store_true", help="execute the script file given as argument")
    parser.add_argument("-q", "--quit", action="store_true", help="execute the script file with '-x' given as argument and then exit")
    parser.add_argument("--profile", action="store", help="Run workbench with execution profiling. Specify a path for the output file.")
    parser.add_argument("--yappi", action="store_true", help="Profile using Yappi instead of cProfile to capture multi-threaded execution.")
    parser.add_argument("--error-on-warning", action="store_true", help="Convert python warnings to exceptions")
    parser.add_argument(
        "--single-process",
        action="store_true",
        help="Run workbench with a single process and not in dual-process mode. "
        "By default the initial process creates a second process to run the "
        "workbench that users see, allowing crashes of the second process "
        "to be detected. This flag disables this behaviour and runs everything "
        "in one process. This is mostly useful for debuggers. "
        "Be aware that this also implies --no-error-reporter.",
    )
    parser.add_argument(
        "--no-error-reporter", action="store_true", help="Stop the error reporter from opening if you suffer an exception or crash."
    )

    try:
        # set up bash completion as a soft dependency
        import argcomplete

        argcomplete.autocomplete(parser)
    except ImportError:
        pass  # silently skip this

    # parse the command line options
    options = parser.parse_args(args=args)

    if options.error_on_warning:
        warnings.simplefilter("error")  # Change the filter in this process
        os.environ["PYTHONWARNINGS"] = "error"  # Also affect subprocesses

    if options.profile:
        output_path = os.path.abspath(os.path.expanduser(options.profile))
        if not os.path.exists(os.path.dirname(output_path)):
            raise ValueError("Invalid path given for profile output. " "Please specify and existing directory and filename.")
        if options.yappi:
            _enable_yappi_profiling(output_path, options)
        else:
            _enable_cprofile_profiling(output_path, options)
    else:
        start(options)


def _enable_yappi_profiling(output_path, options):
    import yappi

    # Dont use wall time as Qt event loop runs for entire duration
    yappi.set_clock_type("cpu")
    yappi.start(builtins=False, profile_threads=True, profile_greenlets=True)
    try:
        start(options)
    finally:
        # Qt will try to sys.exit so wrap in finally block before we go down
        print(f"Saving kcachegrind to {output_path}")
        yappi.stop()
        prof_data = yappi.get_func_stats()
        prof_data.save(output_path, type="callgrind")


def _enable_cprofile_profiling(output_path, options):
    import cProfile

    cProfile.runctx("start(options)", globals(), locals(), filename=output_path)


def start(options):
    from workbench.app.start import start

    start(options)


if __name__ == "__main__":
    main()
