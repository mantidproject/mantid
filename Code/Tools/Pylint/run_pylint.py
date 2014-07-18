"""
    runpylint
    ~~~~~~~~~

    Run pylint selected Python files. By default the output is sent to stdout
    but there is an option to save to a file
"""
from __future__ import print_function

from contextlib import contextmanager
import logging
from optparse import OptionParser
import os.path
import subprocess as subp
import sys

# Default output format
DEFAULT_PYLINT_FORMAT = 'text'
# Exe to call
DEFAULT_PYLINT_EXE = 'pylint'
# Default log level
DEFAULT_LOG_LEVEL = logging.WARNING

#------------------------------------------------------------------------------

@contextmanager
def temp_dir_change(directory):
    """
    Change directory temporarily for a given context

    Args:
      directory (str): A string denoting the new directory
    """
    start_dir = os.getcwd()
    if directory == start_dir:
        yield
    else:
        os.chdir(directory)
        yield
        os.chdir(start_dir)

#------------------------------------------------------------------------------

class Results(object):
    """
    Keep track of the check pass/failure status
    """

    def __init__(self):
        self.totalchecks = 0
        self.failures = [] #list of module names

    @property
    def success(self):
        """
        Return true if all files were clean
        """
        return (len(self.failures) == 0)

    def track_passed(self):
        """
        Increment the number of checks but not the number of failures
        """
        self.totalchecks += 1

    def track_failed(self, modulename):
        """
        Increment the number of checks and track a failure in the
        given module.

        Args:
          modulename (str): A string containing the name of the module
                            that failed
        """
        self.totalchecks += 1
        self.failures.append(modulename)

    def summary(self):
        """
        Return a string summary of the check status
        """
        HEADER = "%d%% checks passed, %d checks failed out of %d"
        if self.success:
            msg = HEADER % (100, 0, self.totalchecks)
        else:
            nfailed = len(self.failures)
            npassed = self.totalchecks - nfailed
            percentpass = npassed*100.0/self.totalchecks
            msg = HEADER % (percentpass, nfailed, self.totalchecks)
            msg += "\nChecks of the following modules FAILED:\n\t"
            msg += "\n\t".join(self.failures)
        return msg

#------------------------------------------------------------------------------

def main(argv):
    """
    Main entry point

    Args:
      argv (list): List of strings giving command line arguments, including
                   the python exe name
    """
    logging.basicConfig(level=DEFAULT_LOG_LEVEL)

    options, args = parse_arguments(argv[1:])
    target = args[0]
    if options.output:
        serializer = open(options.output, 'w')
    else:
        serializer = sys.stdout

    target = __file__

    stats = Results()
    if os.path.isdir(target):
        stats = exec_pylint_on_dir(target, serializer, options)
    else:
        if exec_pylint_on_file(target, serializer, options):
            stats.track_passed()
        else:
            stats.track_failed(target)

    print(stats.summary())
    if stats.success:
        return 0
    else:
        return 1

#------------------------------------------------------------------------------

def parse_arguments(argv):
    """
    Parse arguments for script input
   
    Args:
      argv (list): List of strings giving command line arguments
    """
    # Setup options
    parser = OptionParser(usage="%prog [options] TARGET")
    parser.add_option("-r", "--rcfile", dest="rcfile", metavar = "CFG_FILE",
                      help="If provided, use this configuration file "
                      "instead of a default one")
    parser.add_option("-f", "--format", dest="format", metavar = "FORMAT",
                      help="If provided, use the given format type "\
                      "[default=%s]. Options are: text, html, msvs, "\
                      "parseable" % DEFAULT_PYLINT_FORMAT)
    parser.add_option("-o", "--output", dest="output", metavar="FILE",
                      help="If provided, store the output in the given file.")
    parser.add_option("-e", "--exe", dest="exe", metavar="EXEPATH",
                      help="If provided, use this as the executable path."
                      "Default is to simply call 'pylint'")
    parser.set_defaults(format=DEFAULT_PYLINT_FORMAT, exe=DEFAULT_PYLINT_EXE)

    options, args = parser.parse_args(argv)
    if len(args) < 1 or len(args) > 1:
        print("ERROR: Incorrect number of arguments '%s'" % args)
        print()
        parser.print_help()
        sys.exit(1)

    return options, args

#------------------------------------------------------------------------------

def exec_pylint_on_dir(startdir, serializer, options, recursive=True):
    """
    Executes pylint on .py files from the given starting directory

    Args:
      startdir (str): A string giving a directory to start the walk
      serializer (file-like): An object with a write method that will receive
                              the output
      options (object): Settings to use when running pylint
      recurse (bool): If true recurse into all sub-directories
    """
    raise NotImplementedError("Directory traversal not implmented yet")

#------------------------------------------------------------------------------

def exec_pylint_on_file(srcpath, serializer, options):
    """
    Runs the pylint executable on the given file path to produce output
    in the chosen format

    Args:
      srcpath (str): A string giving a path to a file to analyze
      serializer (file-like): An object with a write method that will receive
                              the output
      options (object): Settings to use when running pylint
    """
    logging.info("Running pylint on '%s'", srcpath)
    cmd = [options.exe]
    cmd.extend(["-f", options.format])
    if options.rcfile is not None:
        cmd.extend(["--rcfile=" + options.rcfile])
    # and finally, source module
    # pylint runs by importing the modules so we strip the filepath
    # and change directory to the containing folder
    cmd.append(os.path.basename(srcpath))

    logging.debug("Command '%s'" , " ".join(cmd))
    with temp_dir_change(os.path.dirname(srcpath)):
        status = subp.call(cmd, stdout=serializer)

    return (status == 0)

#------------------------------------------------------------------------------


if __name__ == '__main__':
    sys.exit(main(sys.argv))
