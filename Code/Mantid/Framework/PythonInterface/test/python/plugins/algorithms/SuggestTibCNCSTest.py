import unittest,os
import mantid


class SuggestTibCNCSTest(unittest.TestCase):
	def test_simple(self):
		result=mantid.simpleapi.SuggestTibCNCS(3.)
		self.assertAlmostEqual(result[0]*0.1,4420.1,0)
		self.assertAlmostEqual(result[1]*0.1,4731.4,0)
		result=mantid.simpleapi.SuggestTibCNCS(1.)
		self.assertAlmostEqual(result[0]*0.1,9662.1,0)
		self.assertAlmostEqual(result[1]*0.1,9902.1,0)

if __name__=="__main__":
	unittest.main()
