# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#pylint: disable=W0232,R0903
import sys
import systemtesting


class PVPythonTest(systemtesting.MantidSystemTest):

    def skipTests(self):
        return sys.platform == 'win32'

    def runTest(self):
        # Make Vates/ParaView a soft requirement rather than failing on module import
        # when determining what tests to run.
        from paraview.simple import GetParaViewVersion
        self.assertEqual(GetParaViewVersion().major, 5)
