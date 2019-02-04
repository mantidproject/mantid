# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)

import unittest
from mantid.kernel import ProgressBase

class ProgressBaseTest(unittest.TestCase):

    def test_class_has_expected_attributes(self):
        self.assertTrue(hasattr(ProgressBase,"report"))
        self.assertTrue(hasattr(ProgressBase,"reportIncrement"))
        self.assertTrue(hasattr(ProgressBase,"setNumSteps"))
        self.assertTrue(hasattr(ProgressBase,"resetNumSteps"))
        self.assertTrue(hasattr(ProgressBase,"setNotifyStep"))
        self.assertTrue(hasattr(ProgressBase,"getEstimatedTime"))

if __name__ == '__main__':
    unittest.main()
