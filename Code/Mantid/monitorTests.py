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


def getkey():
    """Non-blocking wait for keypress. Linux only"""
    result = select.select([sys.stdin], [], [], 0.05)
    if sys.stdin in result[0]:
        sys.stdin.read(1) #Flush the buffer
        return True
    return False

#==============================================================================
if __name__ == "__main__":
    # Get the list of files we want to check
    files_to_check = glob.glob("../../debug/*.so")
    files_to_check += glob.glob("./*.h")

    print "\n------ Monitoring current working directory, and the ../../debug/ folder for changes -------"
    commandline = "./runTests.sh " + " ".join(sys.argv[1:])
    print "Will run the following command upon changes:\n%s\n" % commandline

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
            print '\033[1;32m' + '-'*80 + '\033[1;m'
            print '\033[1;32m' + '='*80 + '\033[1;m'
            print '\033[1;32m' + '-'*80 + '\033[1;m'
            print ""

            #Start the subprocess (runTests.sh)
            p = subprocess.Popen(commandline, shell=True, bufsize=10000,
                                 stdin=subprocess.PIPE,
                                 stdout=subprocess.PIPE, close_fds=True)
            (put, get) = (p.stdin, p.stdout)

            line=get.readline()
            while line != "":
                if len(line)>1:
                    line = line[:-1] #Remove trailing /n
                if ("Error:" in line) or ("error:" in line):
                    #An error line!
                    #Print in red
                    print '\033[1;31m' + line  + '\033[1;m'
                else:
                    #Print normally
                    print line
                #Keep reading output.
                line=get.readline()

            #os.system(commandline)
            last_modified = current_times;
            print "\n\n--- continuing to monitor changes to file; or press return to test now ---"


