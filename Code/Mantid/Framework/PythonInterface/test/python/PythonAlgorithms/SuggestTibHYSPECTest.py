import unittest,os
import mantid


class SuggestTibHYSPECTest(unittest.TestCase):
	def test_simple(self):
		result=mantid.simpleapi.SuggestTibHYSPEC(5.)
		self.assertAlmostEqual(result[0],39515.,delta=1)
		self.assertAlmostEqual(result[1],41515.,delta=1)
		result=mantid.simpleapi.SuggestTibHYSPEC(40.)
		self.assertAlmostEqual(result[0],11898.,delta=1)
		self.assertAlmostEqual(result[1],13898.,delta=1)

if __name__=="__main__":
	unittest.main()
