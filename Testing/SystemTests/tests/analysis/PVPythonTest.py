# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#pylint: disable=W0232,R0903
import systemtesting
from paraview.simple import *


class PVPythonTest(systemtesting.MantidSystemTest):

    def runTest(self):
        self.assertEqual(GetParaViewVersion().major, 5)
