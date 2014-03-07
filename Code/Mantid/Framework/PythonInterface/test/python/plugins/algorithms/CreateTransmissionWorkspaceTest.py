import unittest
from mantid.simpleapi import CreateTransmissionWorkspace
from CreateTransmissionWorkspaceBaseTest import CreateTransmissionWorkspaceBaseTest

class CreateTransmissionWorkspaceTest(unittest.TestCase, CreateTransmissionWorkspaceBaseTest):
    
    def setUp(self):
        CreateTransmissionWorkspaceBaseTest.setUp(self)
        
    def tearDown(self):
        CreateTransmissionWorkspaceBaseTest.setUp(self)
    
    def __init__(self, *args, **kwargs):
        super(CreateTransmissionWorkspaceTest,self).__init__(*args, **kwargs)
    
    def algorithm_type(self):
        return CreateTransmissionWorkspace
    
            
if __name__ == '__main__':
    unittest.main()