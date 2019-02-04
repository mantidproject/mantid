# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#pylint: disable=no-init
from __future__ import (absolute_import, division, print_function)
from mantid.kernel import *
from mantid.api import *
from mantid.simpleapi import *


class ExampleRedStep(PythonAlgorithm):

    def name(self):
        return "ExampleRedStep"

    def PyInit(self):
        self.declareProperty(MatrixWorkspaceProperty("InputWorkspace", "", Direction.Input))
        self.declareProperty(MatrixWorkspaceProperty("OutputWorkspace", "", Direction.Output))
        self.declareProperty(AlgorithmProperty("Algorithm"))

    def PyExec(self):
        input_ws = self.getProperty("InputWorkspace").value
        output_ws = self.getProperty("OutputWorkspace").value

        alg = self.getProperty("Algorithm").value
        alg.setPropertyValue("InputWorkspace", str(input_ws))
        alg.setPropertyValue("OutputWorkspace", output_ws)
        alg.execute()

#AlgorithmFactory.subscribe(ExampleRedStep)


class ExampleLoader(PythonAlgorithm):

    def name(self):
        return "ExampleLoader"

    def PyInit(self):
        self.declareProperty("Filename", "")
        self.declareProperty(MatrixWorkspaceProperty("OutputWorkspace", "", Direction.Output))

    def PyExec(self):
        filename = self.getProperty("Filename").value
        output_ws = self.getProperty("OutputWorkspace").value
        LoadAscii(filename, output_ws)

        print(filename, output_ws)
