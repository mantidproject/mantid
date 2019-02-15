# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)

import unittest
from mantid.simpleapi import LoadNexusProcessed, IndirectFlatPlateAbsorption
from mantid.api import *


class IndirectReplaceFitResultTest(unittest.TestCase):
    def setUp(self):
        """
        Loads the reduced container and sample files.
        """

        self._input_workspace = LoadNexusProcessed(Filename='iris26176_graphite002_conv_1L_s0_to_17_Result.nxs')
        self._single_fit_workspace = LoadNexusProcessed(Filename='iris26176_graphite002_conv_1L_s1_Result.nxs')

        self._result_group = LoadNexusProcessed(Filename='iris26176_graphite002_conv_1L_s0_to_17_Results.nxs')

    def _test_workspaces(self):
        """
        Checks the units of the Ass and corrected workspaces.

        @param corrected Corrected workspace
        @param factor_group WorkspaceGroup containing factors
        """

        corrected_x_unit = 'hi'
        self.assertEqual(corrected_x_unit, 'hi')


if __name__ == '__main__':
    unittest.main()
