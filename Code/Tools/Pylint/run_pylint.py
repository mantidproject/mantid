"""
    runpylint
    ~~~~~~~~~

    Run pylint on selected Python files/directories. By default the output is sent to stdout
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
# Default config file
DEFAULT_RCFILE = os.path.join(os.path.dirname(__file__), "pylint.cfg")

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
            percentpass = npassed*100.0/self.totalchecks
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
    setup_environment(options.mantidpath)
    serializer = get_serializer(options.output)

    status = run_checks(args, serializer, options)
    if type(serializer) == file:
        serializer.close()

    if status or options.nofail:
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
    parser.add_option("-b", "--basedir", dest="basedir", metavar="BASEDIR",
                      help="If provided, use this as the base for all relative paths."
                           "The default is the current working directory.")
    parser.add_option("-e", "--exe", dest="exe", metavar="EXEPATH",
                      help="If provided, use this as the executable path."
                           "Default is to simply call 'pylint'")
    parser.add_option("-f", "--format", dest="format", metavar = "FORMAT",
                      help="If provided, use the given format type "\
                           "[default=%s]. Options are: text, html, msvs, "\
                           "parseable" % DEFAULT_PYLINT_FORMAT)
    parser.add_option("-m", "--mantidpath", dest="mantidpath", metavar="MANTIDPATH",
                      help="If provided, use this as the MANTIDPATH, overriding"
                           "anything that is currently set.")
    parser.add_option("-n", "--nofail", action="store_true",dest="nofail",
                      help="If specified, then script will always return an exit status of 0.")
    parser.add_option("-r", "--rcfile", dest="rcfile", metavar = "CFG_FILE",
                      help="If provided, use this configuration file "
                           "instead of the default one")
    parser.add_option("-o", "--output", dest="output", metavar="FILE",
                      help="If provided, store the output in the given file.")
    parser.add_option("-x", "--exclude", dest="exclude", metavar="EXCLUDES",
                      help="If provided, a space-separated list of "
                            "files/directories to exclude. Relative paths are "
                            "taken as relative to --basedir")

    parser.set_defaults(format=DEFAULT_PYLINT_FORMAT, exe=DEFAULT_PYLINT_EXE, nofail=False,
                        basedir=os.getcwd(), rcfile=DEFAULT_RCFILE, exclude="")

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

#------------------------------------------------------------------------------

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
    Inserts the given path at the front of the PYTHONPATH and
    sets the MANTIDPATH variable

    Args:
      mantidpath (str): A string that points to a directory containing
                        the 'mantid' module
    """
    # Check for mantid module
    if not os.path.isfile(os.path.join(mantidpath, "mantid","__init__.py")):
        raise ValueError("Unable to find mantid python module in '%s'"\
                           % mantidpath)

    os.environ["MANTIDPATH"] = mantidpath
    cur_pypath = os.environ.get("PYTHONPATH", "")
    # for subprocesses
    os.environ["PYTHONPATH"] = mantidpath + os.pathsep + cur_pypath
    sys.path.insert(0, mantidpath) # for current process

#------------------------------------------------------------------------------

def check_module_imports():
    """
    Returns an empty string if the environment variables
    are set so that the mantid module is importable else
    it returns an error string.

    Returns:
      str: String indicating success/failure
    """
    msg = ""
    try:
        import mantid
    except ImportError, exc:
        msg = "Unable to import mantid module: '%s'\n"\
              "Try passing the -m option along with the path to the module"\
                % str(exc)
    return msg

#------------------------------------------------------------------------------

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
        return open(filename, 'w')

#------------------------------------------------------------------------------

def run_checks(targets, serializer, options):
    """
    Run pylint on the chosen targets

    Args:
      targets (list): A list of relative directory/file targets relative
                      to options.basedir
      serializer (file-like): An object with a write method that will receive
                              the output
      options (object): Settings to use when running pylint
    Returns:
      bool: Success/failure
    """
    overall_stats = Results()
    # convert to excludes absolute paths
    def make_abs(path):
        return os.path.join(options.basedir, path)
    excludes = map(make_abs, options.exclude)

    for target in targets:
        # pylint will only check modules or packages so we need to do some additional work
        # for plain directories
        targetpath = os.path.join(options.basedir, target)
        pkg_init = os.path.join(targetpath, "__init__.py")
        if os.path.isfile(targetpath) or os.path.isfile(pkg_init):
            overall_stats.add(exec_pylint_on_importable(targetpath, serializer, options))
        else:
            overall_stats.update(exec_pylint_on_all(targetpath, serializer, options, excludes))
    ##
    print(overall_stats.summary())

    return overall_stats.success

#------------------------------------------------------------------------------

def exec_pylint_on_all(dirpath, serializer, options, excludes=[]):
    """
    Executes pylint on .py files and packages from the given starting directory

    Args:
      dirpath (str): A string giving a directory to parse
      serializer (file-like): An object with a write method that will receive
                              the output
      options (object): Settings to use when running pylint
      excludes (list): A list of absolute paths to exclude from the checks
    Returns:
      Results: Object detailing passes and failures
    """
    def skip_item(abspath):
        for excluded in excludes:
            if abspath.startswith(excluded):
                return True
        return False
    logging.debug("Discovering all importable python modules/packages"
                  " from '%s'" , dirpath)
    targets  = find_importable_targets(dirpath)
    stats = Results()
    for item in targets:
        if skip_item(item):
            logging.debug("Skipping item '%s' by request", item)
        else:
            stats.add(item, exec_pylint_on_importable(item, serializer, options))
    return stats

#------------------------------------------------------------------------------

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
            if (os.path.isfile(abspath) and item.endswith(".py")) or \
                os.path.isfile(pkg_init):
                 importables.append(abspath)
            elif os.path.isdir(abspath):
                importables.extend(package_walk(abspath))
        return importables
    #
    return package_walk(dirpath)

#------------------------------------------------------------------------------

def exec_pylint_on_importable(srcpath, serializer, options):
    """
    Runs the pylint executable on the given file/package path to produce output
    in the chosen format

    Args:
      srcpath (str): A string giving a path to a file or package to analyze
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
