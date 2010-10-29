#!/usr/bin/env python
# Monitor a file for changes, and runs tests as soon as it changes
# Run this from the ProjectName/test/ subfolder!
# You can also specify command line arguments. These will be fed to the
#   ./runTests.sh script that is being called.
#
# For example, in Geometry/test, to only do the QuatTest file:
#
# $../../monitorTests.py QuatTest.h
#

import os
import time
import sys
import glob
import subprocess
import select
import MantidBuild
from MantidBuild import COLOR


def get_all_times(filelist):
    """Return list of all file modified times."""
    out = []
    for filename in filelist:
        out.append(os.path.getmtime(filename))
    return out


def times_changed(new_times, old_times):
    """Return True if any of the file modification times is new."""
    if len(new_times)!=len(old_times):
        return True
    for i in xrange(len(new_times)):
        if new_times[i] != old_times[i]:
            return True
    #If you get here, nothing changed.
    return False

def getSourceDir():
    import sys
    import os
    script = os.path.abspath(sys.argv[0])
    return os.path.dirname(script)

def getSharedObjExt():
    return ".so"

def getTestDir(direc):
    import os
    if direc is None:
        direc = os.getcwd()
    
    direc = os.path.abspath(direc)
    direc = os.path.normpath(direc)

    source_dir = getSourceDir()
    if not direc.startswith(source_dir):
        raise RuntimeError("Trying to monitor test outside of the source " \
                           + "directory: " + direc + " is not in " \
                           + source_dir)

    # if the test directory was supplied, then just carry on
    if direc.endswith("test"):
        if os.path.exists(direc):
            return direc
        else:
            raise RuntimeError("test directory does not exist " + direc)

    # determine which subproject
    direc = os.path.join(direc, "test")
    if os.path.exists(direc):
        return direc

    # error out
    raise RuntimeError("Failed to determine subproject from directory " \
                       + direc)

def getkey():
    """Non-blocking wait for keypress. Linux only. Only works with return, for some reason."""
    result = select.select([sys.stdin], [], [], 0.05)
    if sys.stdin in result[0]:
        sys.stdin.read(1) #Flush the buffer
        return True
    return False

def argsToArgs(input_args):
    import os
    hpp = []
    subproj = None
    for arg in input_args:
        if arg.endswith(".h"):
            (direc, header) = os.path.split(arg)
            subproj = direc
            hpp.append(os.path.split(arg)[1])
        else:
            if subproj is None:
                subproj = arg
            elif subproj is arg:
                pass
            else:
                raise RuntimeError("Can only watch one subproject")
    return (subproj, hpp)

if __name__ == "__main__":
    # set up a real command line parser
    import optparse
    epilog = "This program will watch the directory started in by default " \
             + "or can have  a directory specified as an argument"
    parser = optparse.OptionParser("usage: %prog [options] <dir> <test>",
                                   epilog=epilog)
    parser.add_option("", "--release", dest="release_libs",
                      action="store_true",
                      help="Watch for libraries in the release directory " \
                           + "rather than the debug directory")
    (options, args) = parser.parse_args()

    # Convert the options into something more useful
    source_dir = getSourceDir()
    import os
    if options.release_libs:
        shared_objs = os.path.join(source_dir, "release")
    else:
        shared_objs = os.path.join(source_dir, "debug")
    shared_objs = os.path.join(shared_objs, "*" + getSharedObjExt())

    # get the test directory and hpp files
    (subproj, hpp_files) = argsToArgs(args)
    subproj = getTestDir(subproj)

    # create list of files to watch
    files_to_check = glob.glob(shared_objs)
    files_to_check += glob.glob(os.path.join(subproj, "*.h"))

    print "------ Monitoring tests in", \
          subproj.replace(source_dir, "$SRCDIR"), \
          "and", \
          shared_objs.replace(source_dir, "$SRCDIR"), " for changes -------"

    runtests_cmd = "./runTests.sh %s" % " ".join(hpp_files)
    print "Will run %s in %s upon changes" % \
          (runtests_cmd, subproj.replace(source_dir, "$SRCDIR"))

    last_modified = get_all_times(files_to_check)
    last_time = time.time()
    while True:
        #Loop until stopped by Ctrl+C
        #Inform the user
        if time.time() - last_time > 60:
            print "Still monitoring; or press return to test now ..."
            last_time = time.time()

        runTests = False

        #--- Wait one second or until key pressed ---
        for x in xrange(20):
            if getkey():
                runTests = True
                break

        #---- Check if a file changed ------
        current_times = get_all_times(files_to_check)
        if times_changed(current_times, last_modified):
            print "File(s) changed! Running tests..."
            runTests = True

        if runTests:
            print COLOR["blue"] + '-'*80 + COLOR["reset"]
            now = '='*20 + time.asctime()
            now += '='*(80-len(now))
            print COLOR["blue"] + now + COLOR["reset"]
            print COLOR["blue"] + '-'*80 + COLOR["reset"]
            print ""

            #Delete the mantid log file to clear its contents
            os.system("rm ~/.mantid/mantid.log")

            #Start runTests.sh and colorize output
            MantidBuild.color_output(cmdline=runtests_cmd, workingdir=subproj)

            #Now show the log
            if 1:
              print
              print "-------------- mantid.log output (ERRORS only) ------------------------------"
              print
              MantidBuild.color_output(cmdline="grep --max-count=100 Error ~/.mantid/mantid.log", workingdir=subproj)

            last_modified = current_times;
            print "\n\n--- continuing to monitor changes to file; or press return to test now ---"


