#!/usr/bin/env python
"""Utility to find new files that have not been added in SVN
but that should be.
This is for in-source builds in eclipse, where there are a lot of junky files
crowding out the "svn st" output. """

import os, sys

def do_list_new_files():
    """Parameters:"""
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
                if filename.endswith("checkin_except.py") or filename.endswith("svn_new_files.py"):
                    good = False
                else:
                    good = True

            if filename.endswith(".cmake"):
                good = ("Build/CMake/" in filename)

            if filename.endswith("CMakeLists.txt"):
                good = True

            if good:
                good_files.append(filename)
                        
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
    do_list_new_files()
