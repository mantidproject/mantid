# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#pylint: disable=no-init,invalid-name
from __future__ import (absolute_import, division, print_function)

from mantid.kernel import *
from mantid.api import *


class TestWorkspaceGroupProperty(PythonAlgorithm):
    """
    """

    def category(self):
        return "Workflow\\Testing"

    def name(self):
        return "WorkspaceGroupProperty"

    def summary(self):
        return "Use only for testing"

    def PyInit(self):
        self.declareProperty(WorkspaceGroupProperty("InputWorkspace", "", Direction.Input),
                             doc="Group workspace that automatically includes all members.")
        self.declareProperty(MatrixWorkspaceProperty("InputWorkspace2", "", Direction.Input),
                             doc="Another group workspace that automatically includes all members.")

    def PyExec(self):
        ws = self.getProperty("InputWorkspace").value
        logger.notice("Input type: %s" % str(type(ws)))
        ws2 = self.getProperty("InputWorkspace2").value
        logger.notice("Input type: %s" % str(type(ws2)))


AlgorithmFactory.subscribe(TestWorkspaceGroupProperty)
