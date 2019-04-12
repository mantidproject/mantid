from Muon.GUI.Common.contexts.phase_table_context import PhaseTableContext, default_dict
import unittest

class PhaseTableContextTest(unittest.TestCase):
    def setUp(self):
        self.context = PhaseTableContext()

    def test_initialised_with_default_values(self):
        self.assertEquals(self.context.options_dict, default_dict)
        self.assertEquals(self.context.phase_tables, [])


if __name__ == '__main__':
    unittest.main(buffer=False, verbosity=2)