from __future__ import (absolute_import, division, print_function)

import unittest
from mantid.kernel import VisibleWhenProperty, PropertyCriterion, LogicOperator


class VisibleWhenPropertyTest(unittest.TestCase):

    def test_construction_with_name_criterion_only_succeeds(self):
        p = VisibleWhenProperty("OtherProperty", PropertyCriterion.IsDefault)
        self.assertIsNotNone(p)

    def test_construction_with_name_criterion_value_succeeds(self):
        p = VisibleWhenProperty("OtherProperty", PropertyCriterion.IsEqualTo, "value")
        self.assertIsNotNone(p)

    def test_multiple_condition_construction(self):
        # We cannot manipulate properties easily from the Python side compared to the C++
        # side. So we will check we can construct the object from Python correctly and rely
        # on the C++ unit tests to check all operators
        a = VisibleWhenProperty("PropA", PropertyCriterion.IsDefault)
        b = VisibleWhenProperty("PropB", PropertyCriterion.IsDefault)
        result = VisibleWhenProperty(a, b, LogicOperator.And)
        self.assertIsNotNone(result)

    # ------------ Failure cases ------------------

    def test_default_construction_raises_error(self):
        try:
            VisibleWhenProperty()
            self.fail("Expected default constructor to raise an error")
        except Exception as e:
            # boost.python.ArgumentError are not catchable
            if "Python argument types in" not in str(e):
                raise RuntimeError("Unexpected exception type raised")

if __name__ == '__main__':
    unittest.main()
