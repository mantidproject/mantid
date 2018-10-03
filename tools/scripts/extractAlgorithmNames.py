# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#pylint: disable=invalid-name
# simply just print out all algorithm names in a directory which can be piped
# to a file
from __future__ import (absolute_import, division, print_function)
import os
import glob

os.chdir("PythonInterface/plugins/algorithms/WorkflowAlgorithms")
for filename in glob.glob("*.py"):
    #print file
    if 'PythonAlgorithm' in open(filename).read():
        print(filename)


#os.chdir("LiveData/src")
#for file in glob.glob("*.cpp"):
#    #print file
#    if 'DECLARE_ALGORITHM' in open(file).read():
#        print file
