import unittest
import types

class ImportTest(unittest.TestCase):
    def test_import(self):
        from MantidFramework import mtd
        mtd.initialise()
        
    def test_alias(self):
        from mantidsimple import *
        self.assertEqual(type(UpdateInstrumentFromRaw), types.FunctionType)

    def test_version_number_equals_1(self):
        from MantidFramework import apiVersion
        self.assertEquals(apiVersion(), 1)