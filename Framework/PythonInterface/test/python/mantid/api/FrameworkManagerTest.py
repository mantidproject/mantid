# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)

import unittest
import testhelpers
from mantid.api import FrameworkManager, FrameworkManagerImpl, IAlgorithm, AlgorithmProxy

class FrameworkManagerTest(unittest.TestCase):

    def test_clear_functions_do_not_throw(self):
        # Test they don't throw for now
        testhelpers.assertRaisesNothing(self, FrameworkManager.clear)
        testhelpers.assertRaisesNothing(self, FrameworkManager.clearData)
        testhelpers.assertRaisesNothing(self, FrameworkManager.clearAlgorithms)
        testhelpers.assertRaisesNothing(self, FrameworkManager.clearInstruments)
        testhelpers.assertRaisesNothing(self, FrameworkManager.clearPropertyManagers)

if __name__ == '__main__':
    unittest.main()
