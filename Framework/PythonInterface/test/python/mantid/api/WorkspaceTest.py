# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from mantid.api import Workspace


class WorkspaceTest(unittest.TestCase):
    def test_that_one_cannot_be_instantiated(self):
        try:
            Workspace()
            error = False
        except RuntimeError:  # For some reason self.assertRaises doesn't catch this
            error = True
        self.assertTrue(error, True)


if __name__ == "__main__":
    unittest.main()
