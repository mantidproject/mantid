#!/usr/bin/env python
"""Utility to find new files that have not been added in SVN
but that should be.
This is for in-source builds in eclipse, where there are a lot of junky files
crowding out the "svn st" output. """

import os, sys

def do_list_new_files(only_this):
    """Parameters:
        only_this: a list of strings. The string must be in the filename
            for the file to be matched."""
    import subprocess
    p = subprocess.Popen("svn status", shell=True, bufsize=10000,
          stdin=subprocess.PIPE, stdout=subprocess.PIPE, close_fds=True)
    (put, get) = (p.stdin, p.stdout)

    good_files = []

    line=get.readline()
    while line != "":
        status = line[0]
        props = line[1]
        if status=="?": #or props!=" ":
            #Change to file or stuff.
            filename = line[8:].strip()
            
            good = False
            if filename.endswith(".cpp"):
                good = ("/src/" in filename)
                
            if filename.endswith(".h"):
                good = ("/test/" in filename) or ("/inc/" in filename)
                
            if filename.endswith(".py"):
                good = not filename.endswith("checkin_except.py")

            if filename.endswith(".cmake"):
                good = ("Build/CMake/" in filename)

            if filename.endswith("CMakeLists.txt"):
                good = True

            if good:
                # Are we filtering any more?
                if only_this is None:
                    good_files.append(filename)
                else:
                    #Check for any entry in the list only_this
                    for only_this_one in only_this:
                        if only_this_one in filename:
                            good_files.append(filename)
                            break
                        
        line=get.readline()

    print ""
    print "-------- New files that have not bee 'svn add'ed: -------------"
    print ""
    if len(good_files)==0:
        print "Nothing new! \n"
        return
    
    print "\n".join(good_files)
    print ""
    print "svn add ", " ".join(good_files)


#end def

if __name__=="__main__":
    if len(sys.argv)>1:
        only_this = sys.argv[1:]
    else:
        only_this = None
    do_list_new_files(only_this)
