# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantidqt package
import unittest
from unittest.mock import MagicMock

from mantidqt.utils.lab_frame_sample import lab_frame_sample


class LabFrameSampleTest(unittest.TestCase):
    def test_falls_back_to_the_sample_as_defined_when_it_cannot_be_turned(self):
        workspace = MagicMock()
        workspace.getLabFrameSample.side_effect = ValueError("no XML definition")

        self.assertIs(lab_frame_sample(workspace), workspace.sample())


if __name__ == "__main__":
    unittest.main()
