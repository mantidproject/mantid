import unittest

class ImportModuleTest(unittest.TestCase):
  
    def test_import_succeeds(self):
        import mantid
        # Check content
        attrs = dir(mantid)
        self.assertTrue('api' in attrs)
        self.assertTrue('kernel' in attrs)