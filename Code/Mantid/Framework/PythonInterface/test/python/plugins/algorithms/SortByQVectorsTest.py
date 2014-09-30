import unittest,os
import mantid


class SortByQVectorsTest(unittest.TestCase):


    def test_output(self):
        ws = mantid.simpleapi.LoadSassena("outputSassena_1.4.1.h5", TimeUnit=1.0)
    	mantid.simpleapi.SortByQVectors('ws')
    	self.assertAlmostEqual(ws[0].getNumberHistograms(),5)
    	self.assertAlmostEqual(ws[0].dataY(0)[0],0.0)
    	self.assertAlmostEqual(ws[0].dataY(1)[0],0.00600600591861)
    	self.assertAlmostEqual(ws[0].dataY(2)[0],0.0120120118372)
    	self.assertAlmostEqual(ws[0].dataY(3)[0],0.0180180184543)
    	self.assertAlmostEqual(ws[0].dataY(4)[0],0.0240240236744)
    	mantid.api.AnalysisDataService.remove("ws")

if __name__=="__main__":
    unittest.main()
