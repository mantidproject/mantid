""" 
Test generation of Algorithm Dialogs
"""
import mantidplottests
from mantidplottests import *
import mantidqtpython

class AlgorithmDialogTest(unittest.TestCase):
    
    def test_OpenDialog(self):
        dialog = threadsafe_call( interface_manager.createDialogFromName, "CreateMDWorkspace", False)
        is_instance_of_alg_dialog = isinstance(dialog, mantidqtpython.MantidQt.API.AlgorithmDialog)
        self.assertEqual(is_instance_of_alg_dialog, True)
    
# Run the unit tests
mantidplottests.runTests(AlgorithmDialogTest) 
    
    
    