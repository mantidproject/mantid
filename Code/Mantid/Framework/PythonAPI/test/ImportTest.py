import unittest
import types

class ImportTest(unittest.TestCase):
    def test_import(self):
        from MantidFramework import mtd
        mtd.initialise()
        
    def test_alias(self):
        from mantidsimple import *
        self.assertEqual(type(UpdateInstrumentFromRaw), types.FunctionType)
