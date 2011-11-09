import unittest

from mantid.api import algorithm_mgr

class PropertyWithValueTest(unittest.TestCase):
  
    def test_set_property_basic_types(self):
        prop_mgr = algorithm_mgr.create_unmanaged("Integration")
        prop_mgr.initialize()
        # Int type
        prop_mgr.set_property("StartWorkspaceIndex", 5) 
        self.assertEquals(prop_mgr.get_property("StartWorkspaceIndex").value, 5)

        # Bool type
        prop_mgr.set_property("IncludePartialBins", True) 
        self.assertEquals(prop_mgr.get_property("IncludePartialBins").value, True)

        # Float
        prop_mgr.set_property("RangeLower", 100.5)
        self.assertAlmostEqual(prop_mgr.get_property("RangeLower").value, 100.5)
        prop_mgr.set_property("RangeLower", 50) # Set with an int should still work
        self.assertAlmostEqual(prop_mgr.get_property("RangeLower").value, 50)

	# List type
	prop_mgr = algorithm_mgr.create_unmanaged("MaskDetectors")
	prop_mgr.initialize()
	value = [2,3,4,5,6]
	prop_mgr.set_property("WorkspaceIndexList", value)
        
