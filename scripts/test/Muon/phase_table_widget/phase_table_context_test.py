from Muon.GUI.Common.contexts.phase_table_context import PhaseTableContext, default_dict
import unittest

class PhaseTableContextTest(unittest.TestCase):
    def setUp(self):
        self.context = PhaseTableContext()

    def test_initialised_with_default_values(self):
        self.assertEquals(self.context.options_dict, default_dict)
        self.assertEquals(self.context.phase_tables, [])

    def test_add_phase_table_adds_phase_table_name_to_dict_with_run_as_key(self):
        name = 'MUSR22222_phase_table'
        self.context.add_phase_table(name)

        self.assertEquals(self.context.phase_tables, [name])

    def test_get_phase_table_list_retrieves_all_tables_wth_correct_instrument(self):
        name = 'MUSR22222_phase_table'
        self.context.add_phase_table(name)
        name = 'EMU22222_phase_table'
        self.context.add_phase_table(name)
        name = 'MUSR33333_phase_table'
        self.context.add_phase_table(name)

        self.assertEquals(self.context.get_phase_table_list('MUSR'), ['MUSR22222_phase_table', 'MUSR33333_phase_table'])


if __name__ == '__main__':
    unittest.main(buffer=False, verbosity=2)