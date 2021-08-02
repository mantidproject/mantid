# -*- coding: utf-8 -*-# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

from mantid.api import (AlgorithmFactory, DataProcessorAlgorithm)


class _Prop:
    RUNS = 'InputRunList'
    GROUP_TOF = 'GroupTOFWorkspaces'
    OUTPUT_WS = 'OutputWorkspace'


class ReflectometryISISPreprocess(DataProcessorAlgorithm):

    def __init__(self):
        """Initialize an instance of the algorithm."""
        DataProcessorAlgorithm.__init__(self)

    def category(self):
        """Return the categories of the algorithm."""
        return 'ISIS\\Reflectometry;Workflow\\Reflectometry'

    def name(self):
        """Return the name of the algorithm."""
        return 'ReflectometryISISPreprocess'

    def summary(self):
        """Return a summary of the algorithm."""
        return "Preprocess ISIS reflectometry data, including optional loading and summing of the input runs."

    def seeAlso(self):
        """Return a list of related algorithm names."""
        return ['ReflectometryISISLoadAndProcess', 'ReflectometryReductionOneAuto']

    def version(self):
        """Return the version of the algorithm."""
        return 1

    def PyInit(self):
        pass

    def PyExec(self):
        pass


AlgorithmFactory.subscribe(ReflectometryISISPreprocess)
