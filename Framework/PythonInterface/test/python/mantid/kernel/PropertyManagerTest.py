from __future__ import (absolute_import, division, print_function)

import unittest
from mantid.kernel import PropertyManager, IPropertyManager

class PropertyManagerTest(unittest.TestCase):
    def test_propertymanager(self):
        manager = PropertyManager()

        # check that it is empty
        self.assertEquals(manager.__len__(), 0)
        self.assertEquals(len(manager), 0)

        # add some values
        manager["f"] = 1.
        manager["i"] = 2
        manager["s"] = "3"

        self.assertEquals(len(manager), 3)
        self.assertEquals(manager.propertyCount(), 3)

        # confirm they are in there
        self.assertTrue("f" in manager)
        self.assertTrue("i" in manager)
        self.assertTrue("s" in manager)
        self.assertFalse("nonsense" in manager)

        # check string return values
        self.assertEquals(manager.getPropertyValue("f"), "1")
        self.assertEquals(manager.getPropertyValue("i"), "2")
        self.assertEquals(manager.getPropertyValue("s"), "3")

        # check actual values
        self.assertEquals(manager.getProperty("f").value, 1.)
        self.assertEquals(manager.getProperty("i").value, 2)
        self.assertEquals(manager.getProperty("s").value, "3")

        # ...and accessing them through dict interface
        self.assertEquals(manager["f"].value, 1.)
        self.assertEquals(manager["i"].value, 2)
        self.assertEquals(manager["s"].value, "3")

        # see that you can get keys and values
        self.assertTrue(len(manager.values()), 3)
        keys = manager.keys()
        self.assertEquals(len(keys), 3)
        self.assertTrue("f" in keys)
        self.assertTrue("i" in keys)
        self.assertTrue("s" in keys)

        # check for members
        self.assertTrue(manager.has_key("f"))
        self.assertTrue(manager.has_key("i"))
        self.assertTrue(manager.has_key("s"))
        self.assertFalse(manager.has_key("q"))

        self.assertTrue("f" in manager)
        self.assertTrue("i" in manager)
        self.assertTrue("s" in manager)
        self.assertFalse("q" in manager)

        # check for delete
        self.assertTrue(len(manager), 3)
        del manager["f"]
        self.assertTrue(len(manager), 2)

if __name__ == "__main__":
    unittest.main()
