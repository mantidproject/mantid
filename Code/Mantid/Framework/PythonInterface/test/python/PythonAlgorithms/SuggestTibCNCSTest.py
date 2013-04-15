import unittest,os
import mantid


class SuggestTibCNCSTest(unittest.TestCase):
	def test_simple(self):
		result=mantid.simpleapi.SuggestTibCNCS(3.)
		self.assertAlmostEqual(result[0],44201.,delta=1)
		self.assertAlmostEqual(result[1],47314.,delta=1)
		result=mantid.simpleapi.SuggestTibCNCS(1.)
		self.assertAlmostEqual(result[0],96621.,delta=1)
		self.assertAlmostEqual(result[1],99021.,delta=1)

if __name__=="__main__":
	unittest.main()
