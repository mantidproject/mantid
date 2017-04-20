from __future__ import (absolute_import, division, print_function)

import unittest
from mantid.kernel import EnabledWhenProperty, PropertyCriterion, LogicOperator


class EnabledWhenPropertyTest(unittest.TestCase):

    def test_construction_with_name_criterion_only_succeeds(self):
        p = EnabledWhenProperty("OtherProperty", PropertyCriterion.IsDefault)
        self.assertIsNotNone(p)

    def test_construction_with_name_criterion_value_succeeds(self):
        p = EnabledWhenProperty("OtherProperty", PropertyCriterion.IsEqualTo, "value")
        self.assertIsNotNone(p)

    def test_Property_Criterion_Has_Expected_Attrs(self):
        attrs = ["IsNotDefault", "IsEqualTo", "IsNotEqualTo", "IsMoreOrEqual"]
        for att in attrs:
            self.assertTrue(hasattr(PropertyCriterion, att))

    def test_construction_with_multiple_OR_conditions_succeeds(self):
        # We cannot manipulate properties easily from the Python side compared to the C++
        # side. So we will check we can construct the object from Python correctly and rely
        # on the C++ unit tests to check all operators
        a = EnabledWhenProperty("PropA", PropertyCriterion.IsDefault)
        b = EnabledWhenProperty("PropB", PropertyCriterion.IsDefault)
        result = EnabledWhenProperty(a, b, LogicOperator.Or)
        self.assertIsNotNone(result)

    def test_logic_operators_has_expected_operators(self):
        expected = ["And", "Or", "Xor"]
        for val in expected:
            self.assertTrue(hasattr(LogicOperator, val))

    #  ------------ Failure cases ------------------

    def test_default_construction_raises_error(self):
        try:
            EnabledWhenProperty()
            self.fail("Expected default constructor to raise an error")
        except Exception as e:
            # boost.python.ArgumentError are not catchable
            if "Python argument types in" not in str(e):
                raise RuntimeError("Unexpected exception type raised")


if __name__ == '__main__':
    unittest.main()
