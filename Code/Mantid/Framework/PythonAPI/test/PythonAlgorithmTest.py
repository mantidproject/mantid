import unittest

from MantidFramework import mtd
from MantidFramework import PythonAlgorithm
from MantidFramework import Direction
mtd.initialise()
from mantidsimple import *

class DummyAlg3(PythonAlgorithm):
    """
        Dummy algorithm that runs an algorithm property.
    """
     
    def category(self):
        return "SANS"
    
    def name(self):
        return "DummyAlg"
    
    def PyInit(self):
        self.declareWorkspaceProperty("OutputWorkspace", "", Direction = Direction.Output, Description = '')
        self.declareAlgorithmProperty("Algo")
    
    def PyExec(self):
        # Get the algorithm property
        sub_alg = self._getAlgorithmProperty("Algo")
        sub_alg.execute()
        self.setPropertyValue("OutputWorkspace", sub_alg.getPropertyValue("OutputWorkspace"))


class DummyAlg(PythonAlgorithm):
    """
        Dummy algorithm that uses executeSubAlg()
    """
     
    def category(self):
        return "SANS"
    
    def name(self):
        return "DummyAlg"
    
    def PyInit(self):
        self.declareWorkspaceProperty("OutputWorkspace", "", Direction = Direction.Output, Description = '')
        self.declareAlgorithmProperty("Algo")
    
    def PyExec(self):
        output_ws = self.getPropertyValue("OutputWorkspace")
        a = self.executeSubAlg(CreateWorkspace, output_ws, [0,1,2], [0,1,2], [0,0,0])
        self._setWorkspaceProperty("OutputWorkspace", a._getWorkspaceProperty("OutputWorkspace"))


class DummyAlg2(DummyAlg):
    def name(self):
        return "DummyAlg2"
    def PyExec(self):
        output_ws = self.getPropertyValue("OutputWorkspace")
        a = self.executeSubAlg(CreateWorkspace, OutputWorkspace=output_ws, DataX=[0,1,2], DataY=[0,1,2], DataE=[0,0,0])
        self._setWorkspaceProperty("OutputWorkspace", a._getWorkspaceProperty("OutputWorkspace"))

class PythonAlgorithmTest(unittest.TestCase):
    """
        Tests for python algorithms
    """
    
    def setUp(self):
        pass
        
    def test_sub_alg_wksp_transfer(self):
        """
            Check that we can execute a sub-algorithm and pass
            ownership of an output workspace to the parent algo. 
        """
        algm = DummyAlg2()
        algm.initialize()
        algm.setPropertyValue("OutputWorkspace", "subalgtest")
        algm.setRethrows(True)
        algm.execute()
        self.assertTrue(mtd.workspaceExists("subalgtest"))
        mtd.deleteWorkspace("subalgtest")
    
    def test_sub_alg_variation(self):
        """
            Call signature variation for sub-algorithm execution
        """
        algm = DummyAlg2()
        algm.initialize()
        algm.setPropertyValue("OutputWorkspace", "subalgtest2")
        algm.setRethrows(True)
        algm.execute()
        self.assertTrue(mtd.workspaceExists("subalgtest2"))
        mtd.deleteWorkspace("subalgtest2")
        
        
    def test_cpp_alg_property_using_python_alg(self):
        """
            Declare, set and get a C++ algorithm from a python algorithm
        """
        algm_par = mtd._createAlgProxy("CreateWorkspace")
        algm_par.setPropertyValues(OutputWorkspace="test", DataX=1, DataY=1, DataE=1)
        algm_par.initialize()
        
        algm = DummyAlg()
        algm.initialize()
        algm.setPropertyValue("OutputWorkspace", "subalgtest")
        algm._setAlgorithmProperty("Algo", algm_par._getHeldObject())
        algm.execute()
        self.assertEqual(str(algm._getAlgorithmProperty("Algo")), 
                         "CreateWorkspace.1(OutputWorkspace=test,DataX=1,DataY=1,DataE=1)")
        self.assertTrue(mtd.workspaceExists("subalgtest"))
        mtd.deleteWorkspace("subalgtest")
        
    def test_python_alg_property_using_python_alg(self):
        """
            Declare, set and get an algorithm property from a python algorithm
        """
        algm_par = DummyAlg2()
        algm_par.initialize()
        
        algm = DummyAlg()
        algm.initialize()
        algm.setPropertyValue("OutputWorkspace", "subalgtest")
        algm._setAlgorithmProperty("Algo", algm_par)
        algm.execute()
        self.assertEqual(str(algm._getAlgorithmProperty("Algo")), "DummyAlg2.1()")
        self.assertTrue(mtd.workspaceExists("subalgtest"))
        mtd.deleteWorkspace("subalgtest")

    def test_algp_property_using_public_API(self):
        """
            Declare, set and get an algorithm property from a python algorithm
            using the public API. Only works if the algorithm owning the AlgorithmProperty
            is a PythonAlgorithm.
        """
        algm_par = mtd._createAlgProxy("CreateWorkspace")
        algm_par.setPropertyValues(OutputWorkspace="test", DataX=1, DataY=1, DataE=1)
        algm_par.initialize()
        
        algm = DummyAlg()
        algm.initialize()
        algm.setPropertyValue("OutputWorkspace", "subalgtest")
        algm.setProperty("Algo", algm_par._getHeldObject())
        algm.execute()
        self.assertEqual(str(algm._getAlgorithmProperty("Algo")), 
                         "CreateWorkspace.1(OutputWorkspace=test,DataX=1,DataY=1,DataE=1)")
        self.assertTrue(mtd.workspaceExists("subalgtest"))
        mtd.deleteWorkspace("subalgtest")
                
        
    def test_cpp_alg_property_using_python_alg_and_execute_the_propertied_algorithm(self):
        """
            Declare, seta C++ algorithm from a python algorithm.
            Then, in the "super" python algorithm, run the "sub" algorithm that
            was passed as a parameter
        """
        algm_par = mtd._createAlgProxy("CreateWorkspace")
        algm_par.setPropertyValues(OutputWorkspace="test", DataX=1, DataY=1, DataE=1)
        algm_par.initialize()
        
        algm = DummyAlg3()
        algm.initialize()
        algm.setPropertyValue("OutputWorkspace", "subalgtest")
        algm._setAlgorithmProperty("Algo", algm_par._getHeldObject())
        algm.execute()
        # THe algorithm created the workspace name given in the PROPERTY ...
        self.assertTrue(mtd.workspaceExists("test"))
        mtd.deleteWorkspace("test")
        # ... not the one given as a parameter of DummyAlg3, because that's not the way that algo was written
        self.assertFalse(mtd.workspaceExists("subalgtest"))
                
if __name__ == '__main__':
    unittest.main()
