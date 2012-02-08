import unittest

from MantidFramework import mtd
mtd.initialise()
from mantidsimple import *

class WorkspaceGroupTest(unittest.TestCase):
    """
    Test the interface to WorkspaceGroups
    """
    
    def setUp(self):
        pass
    
    """ Test various combinations of arithmetic with workspace groups """
    def test_operators(self):
       CreateWorkspace('A', DataX=[1,2,3], DataY=[2,3], DataE=[2,3])
       CreateWorkspace('B', DataX=[1,2,3], DataY=[2,3], DataE=[2,3])
       CreateWorkspace('C', DataX=[1,2,3], DataY=[2,3], DataE=[2,3])
       CreateWorkspace('D', DataX=[1,2,3], DataY=[2,3], DataE=[2,3])
       GroupWorkspaces(InputWorkspaces='A,B', OutputWorkspace='AB')
       GroupWorkspaces(InputWorkspaces='C,D', OutputWorkspace='CD')
       A = mtd['A']
       B = mtd['B']
       C = mtd['C']
       D = mtd['D']
       AB = mtd['AB']
       CD = mtd['CD']

       # Matrix + group
       Q = A + AB
       Q = A - AB
       Q = A * AB
       Q = A / AB
       Q -= AB
       Q += AB
       Q *= AB
       Q /= AB
       
       # group + matrix 
       Q = AB + A
       Q = AB - A
       Q = AB * A
       Q = AB / A
       CD -= A
       CD += A
       CD *= A
       CD /= A
       
       # group + double
       n = 123.456
       Q = AB + n
       Q = AB - n
       Q = AB * n
       Q = AB / n
       CD -= n
       CD += n
       CD *= n
       CD /= n
       
       # Commutative: double + group
       Q = n * AB
       Q = n + AB
        
    def test_size_method(self):
       CreateWorkspace('A', DataX=[1,2,3], DataY=[2,3], DataE=[2,3])
       CreateWorkspace('B', DataX=[1,2,3], DataY=[2,3], DataE=[2,3])
       GroupWorkspaces(InputWorkspaces='A,B', OutputWorkspace='AB')
       grouped_ws = mtd['AB']
       self.assertEquals(2, grouped_ws.size())

if __name__ == '__main__':
    unittest.main()

    
