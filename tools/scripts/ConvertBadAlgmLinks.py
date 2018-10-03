# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#pylint: disable=invalid-name
from __future__ import (absolute_import, division, print_function)
import re
import glob
import os
from mantid.api import AlgorithmFactory


def grep(patt,lines):
    """ finds patt in file - patt is a compiled regex
        returns all lines that match patt """
    matchlines = []
    for line in lines:
        match = patt.search(line)
        if match:
            matchline = match.group()
            matchlines.append(matchline)
    results = '\n '.join(matchlines)
    if results:
        return results
    else:
        return None


#get alg names
algs = AlgorithmFactory.getRegisteredAlgorithms(True)
regexs= {}
for alg in algs:
    regexs[alg] = re.compile(r'`%s\s+<[\w\:\/\.]+\/%s>`_' % (alg,alg))


# Example use
dir = r"C:\Mantid\Code\Mantid\docs\source\algorithms"
files = glob.glob(os.path.join(dir, '*.rst'))
for filename in files:

    #print os.path.basename(filename)[:-4]
    with open(filename) as file:
        lines = file.readlines()
        for alg in algs:
            expr = regexs[alg]
            results = grep(expr, lines)
            if results:
                print(filename)
                print(results)
