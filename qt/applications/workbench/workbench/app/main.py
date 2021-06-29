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


def main():
    # setup command line arguments
    parser = argparse.ArgumentParser(description='Mantid Workbench')
    parser.add_argument('script', nargs='?')
    parser.add_argument('--version', action='version', version=mtd_version)
    parser.add_argument('-x',
                        '--execute',
                        action='store_true',
                        help='execute the script file given as argument')
    parser.add_argument('-q',
                        '--quit',
                        action='store_true',
                        help='execute the script file with \'-x\' given as argument and then exit')
    parser.add_argument('--profile',
                        action='store',
                        help='Run workbench with execution profiling. Specify a path for the output file.')
    parser.add_argument('--error-on-warning',
                        action='store_true',
                        help='Convert python warnings to exceptions')

    try:
        # set up bash completion as a soft dependency
        import argcomplete
        argcomplete.autocomplete(parser)
    except ImportError:
        pass  # silently skip this

    # parse the command line options
    options = parser.parse_args()

    if options.error_on_warning:
        warnings.simplefilter("error") # Change the filter in this process
        os.environ["PYTHONWARNINGS"] = "error" # Also affect subprocesses

    if options.profile:
        import cProfile
        output_path = os.path.abspath(os.path.expanduser(options.profile))
        if os.path.exists(os.path.dirname(output_path)):
            cProfile.runctx('start(options)', globals(), locals(), filename=output_path)
        else:
            raise ValueError("Invalid path given for profile output. "
                             "Please specify and existing directory and filename.")
    else:
        start(options)


def start(options):
    from workbench.app.start import start
    start(options)


if __name__ == '__main__':
    main()
