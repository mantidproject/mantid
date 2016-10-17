import unittest
import mantid as mantid


class PearlRoutinesTest(unittest.TestCase):
    def test_basic_exec(self):
        try:
            mantid.pearl_routines.PEARL_startup('Mantid_Developer', '15_4')
            mantid.pearl_routines.PEARL_focus(74795, fmode='trans', ttmode='TT70')
        except:
            self.fail("Pearl_routines threw")
