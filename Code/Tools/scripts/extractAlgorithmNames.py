# simply just print out all algorithm names in a directory which can be piped
# to a file

import string, os, re

import glob
import mmap


os.chdir("PythonInterface/plugins/algorithms/WorkflowAlgorithms")
for file in glob.glob("*.py"):
    #print file
    if 'PythonAlgorithm' in open(file).read():
        print file


#os.chdir("LiveData/src")
#for file in glob.glob("*.cpp"):
#    #print file
#    if 'DECLARE_ALGORITHM' in open(file).read():
#        print file

