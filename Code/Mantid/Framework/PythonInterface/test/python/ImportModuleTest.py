import unittest

class ImportModuleTest(unittest.TestCase):
  
    def test_import_succeeds(self):
        import mantid
        # Check content
        attrs = dir(mantid)
        self.assertTrue('api' in attrs)
        self.assertTrue('kernel' in attrs)
    
    def test_on_import_gui_flag_is_set_to_false_here(self):
        import mantid
        self.assertEquals(False, mantid.__gui__)