import unittest
from mantid import *
import pearl_routines


class PearlRoutinesTest(unittest.TestCase):
    def test_startup(self):
        pearl_routines.PEARL_startup('Mantid_Developer', '15_4')
        pearl_routines.PEARL_focus(74795, fmode='trans', ttmode='TT70')

if __name__ == '__main__':
    unittest.main()