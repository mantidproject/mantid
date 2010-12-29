#!/usr/bin/env python

def getSourceDir():
    """Returns the location of the source code."""
    import os
    import sys
    script = os.path.abspath(sys.argv[0])
    if os.path.islink(script):
        script = os.path.realpath(script)
    return os.path.dirname(script)

def getTests(sourceDir, testName=None):
    import os
    CPP = ".cpp"
    # get all of the files in the directory
    files = os.listdir(sourceDir)
    # trim the list down to the cpp files
    files = [item for item in files if item.endswith(CPP)]
    # generate the executable names
    files = [os.path.join(sourceDir, item.replace(CPP,"")) for item in files]
    # confirm that there is an executable
    files = [item for item in files if os.path.exists(item)]
    # run only the test that was requested
    if testName is not None:
        files = [item for item in files if item.endswith(testName)]

    files.sort()
    return files

def runTest(cmd):
    print cmd
    import subprocess as sub
    proc = sub.Popen(cmd, stdout=sub.PIPE, stderr=sub.STDOUT, shell=True)
    (stdout, stderr) = proc.communicate()
    return_code = proc.wait()
    print stdout
    return return_code

def printSummary(success, failed_run, failed_check):
    if len(failed_run) == 0:
        print "ALL TESTS RAN"

    #if len(failed_check) == 0:
    #    print "ALL TESTS PASSED"
    #    return
    
    if len(success) > 0:
        print "SUCCESFULL TESTS:"
        for item in success:
            print item
    if len(failed_run) > 0:
        print "FAILED TO RUN:"
        for key in failed_run:
            print key, "returned", failed_run[key]
    if len(failed_check) > 0:
        print "DID NOT PASS CHECKS:"
        for key in failed_check:
            print key, "failed", failed_check[key], "checks"

if __name__ == "__main__":
    import os
    sourceDir = getSourceDir()
    tests = getTests(sourceDir)
    success = []
    failed = {}
    for test in tests:
        result = runTest(test)
        test = os.path.split(test)[-1]
        if result == 0:
            success.append(test)
        else:
            failed[test] = result
    printSummary(success, failed, {})
