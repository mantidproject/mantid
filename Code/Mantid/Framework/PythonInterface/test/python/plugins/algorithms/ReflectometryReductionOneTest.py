import unittest
from mantid.simpleapi import ReflectometryReductionOne
from ReflectometryReductionOneBaseTest import ReflectometryReductionOneBaseTest

class ReflectometryReductionOneTest(unittest.TestCase, ReflectometryReductionOneBaseTest):
    
    def __init__(self, *args, **kwargs):
        super(ReflectometryReductionOneTest, self).__init__(*args, **kwargs)
    
    def setUp(self):
        ReflectometryReductionOneBaseTest.setUp(self)
        
    def tearDown(self):
        ReflectometryReductionOneBaseTest.setUp(self)
    
    def algorithm_type(self):
        return ReflectometryReductionOne
    
            
if __name__ == '__main__':
    unittest.main()