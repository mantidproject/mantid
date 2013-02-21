""" 
Test generation of Algorithm Dialogs
"""
import mantidplottests
from mantidplottests import *
import mantidqtpython
import sys

class AlgorithmDialogTest(unittest.TestCase):
    
    def test_OpenDialog(self):
        if not sys.platform == 'darwin':
            interface_manager = mantidqtpython.MantidQt.API.InterfaceManager()
            dialog = threadsafe_call( interface_manager.createDialogFromName, "CreateMDWorkspace", False)
            is_instance_of_alg_dialog = isinstance(dialog, mantidqtpython.MantidQt.API.AlgorithmDialog)
            self.assertEqual(is_instance_of_alg_dialog, True)
    
# Run the unit tests
mantidplottests.runTests(AlgorithmDialogTest) 
    
    
    