import unittest
from mantid.kernel import EnabledWhenProperty, PropertyCriterion

class EnabledWhenPropertyTest(unittest.TestCase):

    def test_construction_with_name_criterion_only_succeeds(self):
        p = EnabledWhenProperty("OtherProperty", PropertyCriterion.IsDefault)

    def test_construction_with_name_criterion_value_succeeds(self):
        p = EnabledWhenProperty("OtherProperty", PropertyCriterion.IsEqualTo, "value")

    def test_Property_Criterion_Has_Expected_Attrs(self):
        attrs = ["IsNotDefault", "IsEqualTo", "IsNotEqualTo", "IsMoreOrEqual"]
        for att in attrs:
            self.assertTrue(hasattr(PropertyCriterion, att))

    #------------ Failure cases ------------------

    def test_default_construction_raises_error(self):
        try:
            EnabledWhenProperty()
            self.fail("Expected default constructor to raise an error")
        except Exception, e:
            # boost.python.ArgumentError are not catchable
            if "Python argument types in" not in str(e):
                raise RuntimeError("Unexpected exception type raised")


if __name__ == '__main__':
    unittest.main()
