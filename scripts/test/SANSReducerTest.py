from __future__ import (absolute_import, division, print_function)
import unittest
import mantid

from isis_reducer import ReductionStateTransferer
import ISISCommandInterface as ici


class TestReductionStateTransferer(unittest.TestCase):
    def test_that_state_is_transfered(self):
        """
        This test shows that some state can be transfered between the logic instances
        of the ReducerSingelton. 
        """
        # 1. Create a ReudcerSingleton and apply the user file settings
        ici.SANS2D()
        ici.MaskFile("MaskSANS2D.txt")
        self.assertTrue(ici.ReductionSingleton().to_Q._use_gravity)

        # 2. Change a setting (simulates a gui change)
        ici.ReductionSingleton().to_Q._use_gravity = False

        # 3. Creat a transfer object such that this change is stored when the user file is applied again
        transfer_state = ReductionStateTransferer()
        transfer_state.get_copy_of_reducer(ici.ReductionSingleton())

        # 4. Reload the user file (this will overwrite the user-defined setting)
        ici.MaskFile("MaskSANS2D.txt")
        self.assertTrue(ici.ReductionSingleton().to_Q._use_gravity)

        # 5. Reapply the transfer object.
        transfer_state.apply_gui_changes_from_old_reducer_to_new_reducer(ici.ReductionSingleton())
        self.assertFalse(ici.ReductionSingleton().to_Q._use_gravity)


if __name__ == "__main__":
    unittest.main()
