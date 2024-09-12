# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
"""
runpylint
~~~~~~~~~

Run pylint on selected Python files/directories.
"""

import logging
from optparse import OptionParser
import os.path
import subprocess as subp
import sys
import time

# Default output format
DEFAULT_PYLINT_FORMAT = "parseable"
# Exe to call
DEFAULT_PYLINT_EXE = "pylint"
# Default log level
DEFAULT_LOG_LEVEL = logging.WARNING
# Default config file
DEFAULT_RCFILE = os.path.join(os.path.dirname(__file__), "pylint.cfg")
# Default number of processes
DEFAULT_NPROCS = 8

# Prefix to label all result files
OUTPUT_PREFIX = "PYLINT-"

# ------------------------------------------------------------------------------


class Results(object):
    """
    Keep track of the check pass/failure status
    """

    def __init__(self):
        self.totalchecks = 0
        self.failures = []  # list of module names

    @property
    def success(self):
        """
        Return true if all files were clean
        """
        return len(self.failures) == 0

    def add(self, modulename, status):
        """
        Add either pass/fail depending on status

        Args:
          modulename (str): A string containing the name of the module
                            that failed
          status (bool): True/false status
        """
        if status:
            self.add_passed()
        else:
            self.add_failed(modulename)

    def add_passed(self):
        """
        Increment the number of checks but not the number of failures
        """
        self.totalchecks += 1

    def add_failed(self, modulename):
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
            percentpass = npassed * 100.0 / self.totalchecks
            msg = HEADER % (percentpass, nfailed, self.totalchecks)
            msg += "\nChecks of the following modules FAILED:\n\t"
            msg += "\n\t".join(self.failures)
        return msg

    def update(self, other):
        """
        Update this object with results from another
        """
        self.totalchecks += other.totalchecks
        self.failures.extend(other.failures)


# ------------------------------------------------------------------------------


def main(argv):
    """
    Main entry point

    Args:
      argv (list): List of strings giving command line arguments, including
                   the python exe name
    """
    logging.basicConfig(level=DEFAULT_LOG_LEVEL)

    options, args = parse_arguments(argv[1:])
    setup_environment(options.mantidpath)
    create_dir_if_required(options.outputdir)
    status = run_checks(args, options)

    if status or options.nofail:
        return 0
    else:
        return 1


# ------------------------------------------------------------------------------


def parse_arguments(argv):
    """
    Parse arguments for script input

    Args:
      argv (list): List of strings giving command line arguments
    """
    # Setup options
    parser = OptionParser(usage="%prog [options] TARGET")
    parser.add_option(
        "-b",
        "--basedir",
        dest="basedir",
        metavar="BASEDIR",
        help="If provided, use this as the base for all relative paths." "The default is the current working directory.",
    )
    parser.add_option(
        "-e",
        "--exe",
        dest="exe",
        metavar="EXEPATH",
        help="If provided, use this as the executable path." "Default is to simply call 'pylint'",
    )
    parser.add_option(
        "-f",
        "--format",
        dest="format",
        metavar="FORMAT",
        help="If provided, use the given format type " "[default=%s]." % DEFAULT_PYLINT_FORMAT,
    )
    parser.add_option(
        "-m",
        "--mantidpath",
        dest="mantidpath",
        metavar="MANTIDPATH",
        help="If provided, add this to the PYTHONPATH, overriding" "anything that is currently set.",
    )
    parser.add_option(
        "-n", "--nofail", action="store_true", dest="nofail", help="If specified, then script will always return an exit status of 0."
    )
    parser.add_option(
        "-r", "--rcfile", dest="rcfile", metavar="CFG_FILE", help="If provided, use this configuration file " "instead of the default one"
    )
    parser.add_option("-o", "--output", dest="outputdir", metavar="OUTDIR", help="If provided, store the output given directory")
    parser.add_option(
        "-x",
        "--exclude",
        dest="exclude",
        metavar="EXCLUDES",
        help="If provided, a space-separated list of " "files/directories to exclude. Relative paths are " "taken as relative to --basedir",
    )
    parser.add_option("-j", "--parallel", dest="parallel", metavar="PARALLEL", help="")

    parser.set_defaults(
        format=DEFAULT_PYLINT_FORMAT,
        exe=DEFAULT_PYLINT_EXE,
        nofail=False,
        basedir=os.getcwd(),
        rcfile=DEFAULT_RCFILE,
        exclude="",
        parallel=DEFAULT_NPROCS,
    )

    options, args = parser.parse_args(argv)
    if len(args) < 1:
        print("ERROR: Incorrect number of arguments '%s'" % args)
        print()
        parser.print_help()
        sys.exit(1)

    # rcfile needs to be absolute
    if options.rcfile is not None and not os.path.isabs(options.rcfile):
        options.rcfile = os.path.join(os.getcwd(), options.rcfile)
    # exclude option is more helpful as a list
    options.exclude = options.exclude.split()

    return options, args


# ------------------------------------------------------------------------------


def setup_environment(mantidpath):
    """
    Setup and check environment can import mantid.

    Args:
      mantidpath (str): A string that points to a directory containing
                        the 'mantid' module
    """
    if mantidpath is not None:
        setup_mantidpath(mantidpath)
    errors = check_module_imports()
    if errors != "":
        raise ValueError(errors)


def setup_mantidpath(mantidpath):
    """
    Setup the environment ready for the subprocess call.
    Inserts the given path at the front of the PYTHONPATH

    Args:
      mantidpath (str): A string that points to a directory containing
                        the 'mantid' module
    """
    # Check for mantid module
    if not os.path.isfile(os.path.join(mantidpath, "mantid", "__init__.py")):
        raise ValueError("Unable to find mantid python module in '%s'" % mantidpath)

    cur_pypath = os.environ.get("PYTHONPATH", "")
    # for subprocesses
    os.environ["PYTHONPATH"] = mantidpath + os.pathsep + cur_pypath
    sys.path.insert(0, mantidpath)  # for current process


# ------------------------------------------------------------------------------


def create_dir_if_required(path):
    """
    Create the given directory if it doesn't exist

    Arguments:
      path (str): Absolute path to a directory
    """
    if path and not os.path.exists(path):
        os.makedirs(path)


# ------------------------------------------------------------------------------


def check_module_imports():
    """
    Returns an empty string if the environment variables
    are set so that the mantid module is importable else
    it returns an error string.

    Returns:
      str: String indicating success/failure
    """
    msg = ""
    # pylint: disable=unused-variable
    try:
        import mantid  # noqa
    except ImportError as exc:
        msg = "Unable to import mantid module: '%s'\n" "Try passing the -m option along with the path to the module" % str(exc)
    return msg


# ------------------------------------------------------------------------------


def get_serializer(filename):
    """
    If no file name is provided then returns sys.stdout, else
    it creates the file and returns the handle

    Args:
      filename (str): Path to the filename for the output
    """
    if filename is None:
        return sys.stdout
    else:
        return open(filename, "w")


# ------------------------------------------------------------------------------


def cleanup_serializer(serializer):
    """
    Close a file handle if the serializer points to one.

    Arguments:
      serializer: File-like object with a write method, can be stdout
    """
    if serializer != sys.stdout:
        serializer.close()


# ------------------------------------------------------------------------------


def run_checks(relpaths, options):
    """
    Run pylint on the chosen files/directories

    Args:
      relpaths (list): A list of relative directory/file targets relative
                      to options.basedir
      options (object): Settings to use when running pylint
    Returns:
      bool: Success/failure
    """
    # convert to excludes absolute paths
    excludes = [os.path.join(options.basedir, path) for path in options.exclude]
    # Gather targets first
    target_paths = gather_targets(options.basedir, relpaths, excludes)
    logging.debug("Found {} targets".format(len(target_paths)))
    max_jobs = int(options.parallel)
    processes = []
    results = Results()
    logging.debug("Parallezing over {0} processes".format(options.parallel))
    for index, target in enumerate(target_paths):
        if options.outputdir:
            results_path = get_results_path(options.outputdir, os.path.relpath(target, options.basedir))
        else:
            # indicates sys.stdout
            results_path = None
        if len(processes) == max_jobs:
            logging.debug("Full processor load hit. Waiting for available slot.")
            while True:
                processes, child_results = get_proc_results(processes)
                if child_results.totalchecks > 0:
                    # Implies there is a slot available
                    results.update(child_results)
                    break
                else:
                    time.sleep(0.2)
        # endif
        logging.debug("Slot available. Running next check.")
        serializer = get_serializer(results_path)
        processes.append(start_pylint(target, serializer, options))
    # endfor
    # There will be a last set of processes in the list
    _, child_results = get_proc_results(processes, wait=True)
    results.update(child_results)

    # Get a summary of the failures
    print(results.summary())
    return results.success


# ------------------------------------------------------------------------------


def get_proc_results(processes, wait=False):
    """
    Return a list of processed results if any are available and pops
    the finished ones of the list

    Args:
      processes: A list of running processes
      wait (bool): If true, wait until all processes have finished
    Returns:
      (Updated processes, results)
    """
    results = Results()
    running = []
    for index, proc_info in enumerate(processes):
        if wait:
            proc_info[0].wait()
        else:
            proc_info[0].poll()
        exitcode = proc_info[0].returncode
        if exitcode is None:
            running.append(proc_info)
        else:
            results.add(proc_info[1], (exitcode == 0))
            cleanup_serializer(proc_info[2])

    if wait:
        assert len(running) == 0
        return None, results
    else:
        return running, results


# ------------------------------------------------------------------------------


def gather_targets(basedir, relpaths, excludes=[]):
    """
    Walks the given directories and finds all importable entities.

    Args:
      basedir (str): A string giving a directory to which all relative
                     paths are based
      relpaths (list): A list of relative paths to search for importable
                       entities
      excludes (list): A list of absolute paths to exclude
    Returns:
      Results: Object detailing passes and failures
    """
    logging.debug("Discovering all importable python modules/packages")
    targets = []
    for relpath in relpaths:
        # pylint will only check modules or packages
        targetpath = os.path.join(basedir, relpath)
        pkg_init = os.path.join(targetpath, "__init__.py")
        if os.path.isfile(targetpath) or os.path.isfile(pkg_init):
            targets.append(targetpath)
        else:
            targets.extend(find_importable_targets(targetpath))
    # endfor

    def include_target(path):
        for exclude_path in excludes:
            if path.startswith(exclude_path):
                return False
        return True

    return list(filter(include_target, targets))


# ------------------------------------------------------------------------------


def find_importable_targets(dirpath):
    """
    Returns a list of modules and packages that can be found, using the given
    directory as a starting point. Files that are part of a package are not
    included.

    Args:
      dirpath (str): Starts the search from this directory
    Returns:
      list: A list of module and package targets that were found
    """

    def package_walk(path):
        contents = os.listdir(path)
        importables = []
        for item in contents:
            abspath = os.path.join(path, item)
            pkg_init = os.path.join(abspath, "__init__.py")
            if (os.path.isfile(abspath) and item.endswith(".py")) or os.path.isfile(pkg_init):
                importables.append(abspath)
            elif os.path.isdir(abspath):
                importables.extend(package_walk(abspath))
        return importables

    #
    return package_walk(dirpath)


# ------------------------------------------------------------------------------


def get_results_path(dirname, target):
    """
    Return the path to a results file for the given target.

    Args:
      dirname (str): An output directory to store the file
      target (str): An absolute path to a target
    Returns:
      An absolute path to a results file
    """
    return os.path.join(dirname, OUTPUT_PREFIX + target.replace("/", "-") + ".log")


# ------------------------------------------------------------------------------


def start_pylint(srcpath, serializer, options):
    """
    Runs pylint on the given file/package

    Args:
      srcpath (str): A string giving a path to a file or package to analyze
      serializer (file-like): An object with a write method that will receive
                              the output
      options (object): Settings to use when running pylint
    Returns:
      The (proc, srcpath, serializer)
    """
    logging.info("Running pylint on '%s'", srcpath)
    proc = subp.Popen(build_pylint_cmd(srcpath, options), stdout=serializer, stderr=serializer, cwd=os.path.dirname(srcpath))
    return proc, srcpath, serializer


# ------------------------------------------------------------------------------


def build_pylint_cmd(srcpath, options):
    """
    Build a list defining the command to run pylint for the given target

    Args:
      srcpath (str): A string giving a path to a file or package to analyze
      options (object): Settings to use when running pylint
    Returns:
      A list of args to pass to subprocess.Popen
    """
    cmd = [options.exe]
    cmd.extend(["--output-format=" + options.format])
    if options.rcfile is not None:
        cmd.extend(["--rcfile=" + options.rcfile])
    # and finally, source module
    # pylint runs by importing the modules so we strip the filepath
    # and change directory to the containing folder
    cmd.append(os.path.basename(srcpath))
    return cmd


# ------------------------------------------------------------------------------


if __name__ == "__main__":
    sys.exit(main(sys.argv))
