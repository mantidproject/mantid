#!/usr/bin/env python
"""Do a svn checkin of all files except .project files and some
others that may be user-modified.
Un-checked-in files are listed.
Prompts the user for the checkin message."""

import os, sys


def do_checkin(only_this, exclude_this):
    """Parameters:
        only_this: a list of strings. The string must be in the filename
            for the file to be checked in. Leave None to check in
            all matching files.
        exclude_this: a list of strings to EXCLUDE from the list
                    """
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
                ("LoadDAETest.h" in filename) or\
                ("Mantid.properties" in filename) or\
                ("Scons_Linux.conf" in filename) or\
                (filename == ".") :
#                or ("test/runTests.sh" in filename) :
                    print "NOT CHECKING IN:", filename
            else:
                includeit = False
                
                if len(only_this) == 0:
                    includeit = True
                else:
                    #Check for any entry in the list only_this
                    for only_this_one in only_this:
                        if only_this_one in filename:
                            includeit = True
                            break

                # Remove any excluded ones
                if len(exclude_this) > 0:
                    for exclude_this_one in exclude_this:
                        if exclude_this_one in filename:
                            includeit = False
                              
                if includeit:                  
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


if __name__=="__main__":
    if len(sys.argv)>1:
        only_this = []
        exclude_this = []
        for i in xrange(1, len(sys.argv)):
            arg = sys.argv[i]
            if arg.startswith("-"):
                if arg == "-h" or arg == "--help":
                    print "checkin_except.py: Script to check in selected files.\n"
                    print "USAGE: "
                    print "    checkin_except.py = Do all files."
                    print "    checkin_except.py String1 String2 = Only match the given strings"
                    print "    checkin_except.py -String3 -String4 = Exclude any filename matching any of the given strings"
                    print "\n"
                    sys.exit()
                    
                if len(arg)>1: arg = arg[1:]
                exclude_this.append(arg)
            else:
                only_this.append(arg)
    else:
        only_this = []
        exclude_this = []
        

	# Also list the new svn files
	#try:
    import svn_new_files
    svn_new_files.do_list_new_files()
	#except:
#		pass
	
	# Now the checkin part	
    do_checkin(only_this, exclude_this)	
