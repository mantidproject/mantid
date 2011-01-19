import unittest

class ImportTest(unittest.TestCase):
    def test_import(self):
        from MantidFramework import mtd
        mtd.initialise()
