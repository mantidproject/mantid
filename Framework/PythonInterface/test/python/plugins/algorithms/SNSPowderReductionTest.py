# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2023 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from mantid.simpleapi import SNSPowderReduction

# tests run x10 slower with this on, but it may be useful to track down issues refactoring
CHECK_CONSISTENCY = False


class SNSPowderReductionTest(unittest.TestCase):
    def testValidateInputs(self):
        # PushDataPositive and OffsetData cannot be specified together
        self.assertRaises(
            RuntimeError, SNSPowderReduction, Filename="PG3_46577", OutputDirectory="/tmp/", PushDataPositive="AddMinimum", OffsetData=42.0
        )

    def testValidateInputsCompressLoad(self):
        # PushDataPositive and OffsetData cannot be specified together
        self.assertRaises(
            RuntimeError,
            SNSPowderReduction,
            Filename="PG3_46577",
            OutputDirectory="/tmp/",
            PushDataPositive="AddMinimum",
            OffsetData=42.0,
            MinSizeCompressOnLoad=1e-14,
        )


if __name__ == "__main__":
    unittest.main()
