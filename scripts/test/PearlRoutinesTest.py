import unittest
from mantid import *
import pearl_routines


class PearlRoutinesTest(unittest.TestCase):
    def test_inst_new2_exec(self):
        # Tests the instrument with a "new2" data file
        pearl_routines.PEARL_startup('Mantid_Developer', '15_4')
        pearl_routines.PEARL_focus(74795, fmode='trans', ttmode='TT70')

    def test_inst_new_exec(self):
        # Tests the instrument with a "new" data file
        pearl_routines.PEARL_startup('Mantid_Developer', '15_4')
        pearl_routines.PEARL_focus(73987, fmode='trans', ttmode='TT70')

if __name__ == '__main__':
    unittest.main()