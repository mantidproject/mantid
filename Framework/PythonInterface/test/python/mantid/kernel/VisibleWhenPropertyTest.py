import unittest
from mantid.kernel import VisibleWhenProperty, PropertyCriterion

class VisibleWhenPropertyTest(unittest.TestCase):

    def test_construction_with_name_criterion_only_succeeds(self):
        p = VisibleWhenProperty("OtherProperty", PropertyCriterion.IsDefault)

    def test_construction_with_name_criterion_value_succeeds(self):
        p = VisibleWhenProperty("OtherProperty", PropertyCriterion.IsEqualTo, "value")

    #------------ Failure cases ------------------

    def test_default_construction_raises_error(self):
        try:
            VisibleWhenProperty()
            self.fail("Expected default constructor to raise an error")
        except Exception, e:
            # boost.python.ArgumentError are not catchable
            if "Python argument types in" not in str(e):
                raise RuntimeError("Unexpected exception type raised")

if __name__ == '__main__':
    unittest.main()
