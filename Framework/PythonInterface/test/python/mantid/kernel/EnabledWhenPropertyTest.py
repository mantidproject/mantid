# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
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

    def test_simple_dependsOn(self):
        # Verify that the single-property `dependsOn` works as expected.
        a = EnabledWhenProperty("PropA", PropertyCriterion.IsDefault)
        assert list(a.dependsOn("someOtherProp")) == ["PropA"]

    def test_multiple_dependsOn(self):
        # Verify that binary condition `dependsOn` case works as expected.
        a = EnabledWhenProperty("PropA", PropertyCriterion.IsDefault)
        b = EnabledWhenProperty("PropB", PropertyCriterion.IsDefault)
        c = EnabledWhenProperty(a, b, LogicOperator.Or)
        assert set(c.dependsOn("PropC")) == set(["PropA", "PropB"])

    def test_multiple_dependsOn_2X(self):
        # Verify that multiple binary condition `dependsOn` case works as expected.
        a = EnabledWhenProperty("PropA", PropertyCriterion.IsDefault)
        b = EnabledWhenProperty("PropB", PropertyCriterion.IsDefault)
        c = EnabledWhenProperty("PropC", PropertyCriterion.IsDefault)
        d = EnabledWhenProperty(c, EnabledWhenProperty(a, b, LogicOperator.Or), LogicOperator.And)
        assert set(d.dependsOn("PropD")) == set(["PropA", "PropB", "PropC"])

    def test_multiple_dependsOn_2X_no_duplicates(self):
        # Verify that multiple binary condition `dependsOn` case has no duplicates.
        a = EnabledWhenProperty("PropA", PropertyCriterion.IsDefault)
        b = EnabledWhenProperty("PropB", PropertyCriterion.IsDefault)
        d = EnabledWhenProperty(b, EnabledWhenProperty(a, b, LogicOperator.And), LogicOperator.Or)
        assert set(d.dependsOn("PropD")) == set(["PropA", "PropB"])
        assert len(set(d.dependsOn("PropD"))) == 2

    #  ------------ Failure cases ------------------

    def test_default_construction_raises_error(self):
        try:
            EnabledWhenProperty()
            self.fail("Expected default constructor to raise an error")
        except Exception as e:
            # boost.python.ArgumentError are not catchable
            if "Python argument types in" not in str(e):
                raise RuntimeError("Unexpected exception type raised")

    def test_simple_dependsOn_circular(self):
        # Verify that the single-property circular dependency raises exception.
        a = EnabledWhenProperty("PropA", PropertyCriterion.IsDefault)
        with self.assertRaises(RuntimeError) as context:
            a.dependsOn("PropA")
        assert "circular dependency" in str(context.exception)

    def test_multiple_dependsOn_circular(self):
        # Verify that binary condition circular dependency raises exception.
        a = EnabledWhenProperty("PropA", PropertyCriterion.IsDefault)
        b = EnabledWhenProperty("PropB", PropertyCriterion.IsDefault)
        ab = EnabledWhenProperty(a, b, LogicOperator.Or)
        with self.assertRaises(RuntimeError) as context:
            ab.dependsOn("PropA")
        assert "circular dependency" in str(context.exception)


if __name__ == "__main__":
    unittest.main()
