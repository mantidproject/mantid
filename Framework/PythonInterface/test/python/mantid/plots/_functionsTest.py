from __future__ import (absolute_import, division, print_function)

import unittest
from mantid.simpleapi import CreateWorkspace
import mantid.plots._functions as funcs

class PlotsFunctionsTest(unittest.TestCase):

    def test_getWkspIndexDistAndLabel(self):
        dataX=[1,2,3,1,2,3]
        dataY=[2,3,4,5]
        ws = CreateWorkspace(DataX=dataX, 
                             DataY=dataY,
                             NSpec=2,
                             Distribution=True)
        index, dist, kwargs = funcs._getWkspIndexDistAndLabel(ws,specNum=2)
        self.assertEquals(index,1)
        self.assertTrue(dist)
        self.assertEquals(kwargs['label'],'spec 2')
        
if __name__ == '__main__':
    unittest.main()
