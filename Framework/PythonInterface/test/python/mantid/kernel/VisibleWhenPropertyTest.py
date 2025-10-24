# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
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

    def test_simple_dependsOn(self):
        # Verify that the single-property `dependsOn` works as expected.
        a = VisibleWhenProperty("PropA", PropertyCriterion.IsDefault)
        assert list(a.dependsOn("someOtherProp")) == ["PropA"]

    def test_multiple_dependsOn(self):
        # Verify that binary condition `dependsOn` case works as expected.
        a = VisibleWhenProperty("PropA", PropertyCriterion.IsDefault)
        b = VisibleWhenProperty("PropB", PropertyCriterion.IsDefault)
        c = VisibleWhenProperty(a, b, LogicOperator.Or)
        assert set(c.dependsOn("PropC")) == set(["PropA", "PropB"])

    def test_multiple_dependsOn_2X(self):
        # Verify that multiple binary condition `dependsOn` case works as expected.
        a = VisibleWhenProperty("PropA", PropertyCriterion.IsDefault)
        b = VisibleWhenProperty("PropB", PropertyCriterion.IsDefault)
        c = VisibleWhenProperty("PropC", PropertyCriterion.IsDefault)
        d = VisibleWhenProperty(c, VisibleWhenProperty(a, b, LogicOperator.Or), LogicOperator.And)
        assert set(d.dependsOn("PropD")) == set(["PropA", "PropB", "PropC"])

    # ------------ Failure cases ------------------

    def test_default_construction_raises_error(self):
        try:
            VisibleWhenProperty()
            self.fail("Expected default constructor to raise an error")
        except Exception as e:
            # boost.python.ArgumentError are not catchable
            if "Python argument types in" not in str(e):
                raise RuntimeError("Unexpected exception type raised")

    def test_simple_dependsOn_circular(self):
        # Verify that the single-property circular dependency raises exception.
        a = VisibleWhenProperty("PropA", PropertyCriterion.IsDefault)
        with self.assertRaises(RuntimeError) as context:
            a.dependsOn("PropA")
        assert "circular dependency" in str(context.exception)

    def test_multiple_dependsOn_circular(self):
        # Verify that binary condition circular dependency raises exception.
        a = VisibleWhenProperty("PropA", PropertyCriterion.IsDefault)
        b = VisibleWhenProperty("PropB", PropertyCriterion.IsDefault)
        ab = VisibleWhenProperty(a, b, LogicOperator.Or)
        with self.assertRaises(RuntimeError) as context:
            ab.dependsOn("PropA")
        assert "circular dependency" in str(context.exception)


if __name__ == "__main__":
    unittest.main()
