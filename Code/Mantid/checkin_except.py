#!/usr/bin/env python
"""Do a svn checkin of all files except .project files and some
others that may be user-modified.
Un-checked-in files are listed.
Prompts the user for the checkin message."""

import os

def getSVNRevision():
    import subprocess
    p = subprocess.Popen("svn status", shell=True, bufsize=10000,
          stdin=subprocess.PIPE, stdout=subprocess.PIPE, close_fds=True)
    (put, get) = (p.stdin, p.stdout)

    good_files = []

    line=get.readline()
    while line != "":
        status = line[0]
        props = line[1]
        if status=="A" or status=="D" or status=="M" or status=="R": #or props!=" ":
            #Change to file or stuff.
            filename = line[8:].strip()
            if (".cproject" in filename) or\
                (".project" in filename) or\
                ("Mantid.properties" in filename) or\
                ("Scons_Linux.conf" in filename) or\
                (filename == ".") or\
                ("test/runTests.sh" in filename) :
                    print "NOT CHECKING IN:", filename
            else:
                good_files.append(filename)
        line=get.readline()

    print ""
    print "-------- FILES TO CHECK IN -------------"
    print ""
    if len(good_files)==0:
        print "Nothing to check in! Exiting...\n\n"
        return
    
    print "\n".join(good_files)
    res = raw_input("\n\nEnter check-in log message; Use empty string to cancel: ")
    message = res.strip()
    if len(res)==0:
        print "Canceling check-in"
    else:
        message = res
        cmd = "svn ci " + " ".join(good_files) + ' -m "%s"' % message
        print "..."
        print cmd
        os.system(cmd)


#end def


getSVNRevision()
