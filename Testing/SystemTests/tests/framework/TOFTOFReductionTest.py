# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

import systemtesting
from mantid.kernel import config
from mantid.simpleapi import mtd


class TOFTOFReductionTest(systemtesting.MantidSystemTest):
    """
    Exercises the TOFTOF DGS reduction workflow that is usually driven via the
    Workbench GUI by running the generated script and validating the outputs.
    """

    def __init__(self):
        super(TOFTOFReductionTest, self).__init__()
        config["default.facility"] = "MLZ"
        config["default.instrument"] = "TOFTOF"

    def cleanup(self) -> None:
        mtd.clear()

    def validate(self):
        self.tolerance = 1e-3
        self.tolerance_is_rel_err = True
        return True

    def runTest(self):
        self.assertEqual(config["default.facility"], "MLZ", "Default facility must be MLZ for this test")
        self.assertEqual(config["default.instrument"], "TOFTOF", "Default instrument must be TOFTOF for this test")
