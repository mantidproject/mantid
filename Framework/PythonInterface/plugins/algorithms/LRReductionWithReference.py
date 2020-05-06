# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#pylint: disable=no-init,invalid-name
import time
import math
import os
from mantid.api import *
from mantid.simpleapi import *
from mantid.kernel import *
from functools import reduce #pylint: disable=redefined-builtin


class LRReductionWithReference(PythonAlgorithm):
    def category(self):
        return "Reflectometry\\SNS"

    def name(self):
        return "LRReductionWithReference"

    def version(self):
        return 1

    def summary(self):
        return "REFL reduction using a reference measurement for normalization"

    def PyInit(self):
        alg = AlgorithmManager.create("LiquidsReflectometryReduction")
        props_to_copy = [prop.name for prop in alg.getProperties()]

    def PyExec(self):
            pass

AlgorithmFactory.subscribe(LRReductionWithReference)
