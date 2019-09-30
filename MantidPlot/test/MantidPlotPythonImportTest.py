# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
""" Test script to see if Mantid starts Python correctly

    This includes ensuring the Python algorithms are there.
"""
import unittest
import mantidplottests
from mantidplottests import *
import time
import sys
import types

class MantidPlotPythonImportTest(unittest.TestCase):

    def setUp(self):
        pass

    def tearDown(self):
        """Clean up by closing the created window """
        pass

    def test_NewAPIIsDefault(self):
        self.assertTrue('mantid' in sys.modules)

    def test_ConjoinFilesExistsAsAFunction(self):
        # This means the Python algs were loaded
        self.assertTrue('ConjoinFiles' in globals())
        self.assertEqual(type(ConjoinFiles),types.FunctionType)

    def test_V3D_is_New_API_Version(self):
        import mantid
        self.assertTrue(isinstance(V3D(), mantid.kernel.V3D))

# Run the unit tests
mantidplottests.runTests(MantidPlotPythonImportTest)
