#!/usr/bin/python
import os
import sys
import VTKConvert

if( len(sys.argv) == 1 ):
    print "Usage: processISISData file-name1 file-name2 ...\n       processISISDATA dir-name"
    exit(1)

names=[]
is_dir = os.path.isdir(sys.argv[1])
if( is_dir ):
    names = os.listdir(sys.argv[1])
else:
    for i in range(1,len(sys.argv)):
        names.append(sys.argv[i])

prefix=""
if( is_dir ):
    prefix = sys.argv[1].split('/')[0] + "-VTU/"
    if( os.path.isdir(prefix) ):
        print "Directory " + prefix + " already exists, please move\n"
        exit(1)
    else:
        os.mkdir(prefix)
else:
    prefix = "./"

for file in names:
    if( is_dir ):
        filename = sys.argv[1] + file
    VTKConvert.convertToVTU(filename, prefix)

# Write parallel file
VTKConvert.writeParallelVTU(names, prefix)
