"""
    runpylint
    ~~~~~~~~~

    Run pylint selected Python files. By default the output is sent to stdout
    but there is an option to save to a file
"""
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

# pylint: disable=R0903
class DirectorySwitcher(object):
    """
    RAII struct to change to a particular directory and then switch back to the
    original directory when the object is destroyed
    """

    def __init__(self, directory):
        self.orig_dir = os.getcwd()
        if directory != self.orig_dir:
            logging.debug("")
            os.chdir(directory)
        else:
            self.orig_dir = None # indicates no switch

    def __del__(self):
        if self.orig_dir is not None:
            os.chdir(self.orig_dir)

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

    if os.path.isdir(target):
        raise NotImplementedError("Cannot handle directories yet")
    else:
        exec_pylint_on_file(target, serializer, options)


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
        print "ERROR: Incorrect number of arguments '%s'" % args
        print
        parser.print_help()
        sys.exit(1)

    return options, args

#------------------------------------------------------------------------------

def exec_pylint_on_file(srcpath, serializer, options):
    """
    Runs the pylint executable on the given file path to produce output
    in the chose format

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
    change_dir = DirectorySwitcher(os.path.dirname(srcpath))
    subp.call(cmd, stdout=serializer)
    del change_dir

#------------------------------------------------------------------------------


if __name__ == '__main__':
    main(sys.argv)
