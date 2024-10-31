# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
"""
Test construction of the WorkspaceValidators
"""

import unittest
import testhelpers
from mantid.api import (
    WorkspaceUnitValidator,
    HistogramValidator,
    RawCountValidator,
    CommonBinsValidator,
    SpectraAxisValidator,
    NumericAxisValidator,
    InstrumentValidator,
    MDFrameValidator,
    OrientedLatticeValidator,
)


class WorkspaceValidatorsTest(unittest.TestCase):
    def test_WorkspaceUnitValidator_construction(self):
        """
        Test that the WorkspaceUnitValidator can be constructed
        with a single string
        """
        testhelpers.assertRaisesNothing(self, WorkspaceUnitValidator, "DeltaE")
        self.assertRaises(Exception, WorkspaceUnitValidator)

    def test_CommonBinsValidator_construction(self):
        """
        Test that the CommonBinsValidator can be constructed
        with no args
        """
        testhelpers.assertRaisesNothing(self, CommonBinsValidator)

    def test_HistogramValidator_construction(self):
        """
        Test that the HistogramValidator can be constructed
        with no args or a single bool
        """
        testhelpers.assertRaisesNothing(self, HistogramValidator)
        testhelpers.assertRaisesNothing(self, HistogramValidator, False)

    def test_RawCountValidator_construction(self):
        """
        Test that the HistogramValidator can be constructed
        with no args or a single bool
        """
        testhelpers.assertRaisesNothing(self, RawCountValidator)
        testhelpers.assertRaisesNothing(self, RawCountValidator, False)

    def test_SpectraAxisValidator_construction(self):
        """
        Test that the SpectraAxis can be constructed
        with no args or a single integer
        """
        testhelpers.assertRaisesNothing(self, SpectraAxisValidator)
        testhelpers.assertRaisesNothing(self, SpectraAxisValidator, 0)

    def test_NumericAxisValidator_construction(self):
        """
        Test that the NumericAxis can be constructed
        with no args or a single integer
        """
        testhelpers.assertRaisesNothing(self, NumericAxisValidator)
        testhelpers.assertRaisesNothing(self, NumericAxisValidator, 0)

    def test_InstrumentValidator_construction(self):
        """
        Test that the InstrumentValidator can be constructed
        with no args
        """
        testhelpers.assertRaisesNothing(self, InstrumentValidator)

    def test_MDFrameValidator_construction(self):
        """
        Test that the MDFrameValidator can be constructed
        with a single string
        """
        testhelpers.assertRaisesNothing(self, MDFrameValidator, "HKL")
        self.assertRaises(Exception, MDFrameValidator)

    def test_OrientedLatticeValidator_construction(self):
        """
        Test that the OrientedLatticeValidator can be constructed
        with no args
        """
        testhelpers.assertRaisesNothing(self, OrientedLatticeValidator)


if __name__ == "__main__":
    unittest.main()
