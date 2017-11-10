from __future__ import (absolute_import, division, print_function)

import unittest
from mantid.kernel import Direction, OptionalBool, OptionalBoolValue, OptionalBoolPropertyWithValue

class OptionalBoolTest(unittest.TestCase):


    def test_default_construction(self):
        obj = OptionalBool()
        self.assertEquals(OptionalBoolValue.Unset, obj.getValue())

    def test_construction_unset(self):
        obj = OptionalBool(OptionalBoolValue.Unset)
        self.assertEquals(OptionalBoolValue.Unset, obj.getValue())

    def test_construction_false(self):
        obj = OptionalBool(OptionalBoolValue.False_)
        self.assertEquals(OptionalBoolValue.False_, obj.getValue())

    def test_construction_true(self):
        obj = OptionalBool(OptionalBoolValue.True_)
        self.assertEquals(OptionalBoolValue.True_, obj.getValue())


if __name__ == '__main__':
    unittest.main()

