from mantid.simpleapi import *
from mantid import api
import unittest
import inspect
import os, sys
import DirectEnergyConversionHelpers as helpers


class DirectReductionHelpersTest(unittest.TestCase):
    def __init__(self, methodName):
        return super(DirectReductionHelpersTest, self).__init__(methodName)

    def test_build_subst_dictionary(self):
       self.assertEqual(dict(), helpers.build_subst_dictionary(""))
       self.assertEqual(dict(),helpers.build_subst_dictionary())

       self.assertRaises(AttributeError,helpers.build_subst_dictionary,10)
       self.assertRaises(AttributeError,helpers.build_subst_dictionary,"A=")
       self.assertRaises(AttributeError,helpers.build_subst_dictionary,"B=C;A=")

       rez=dict();
       rez['A']='B';
       self.assertEqual(rez, helpers.build_subst_dictionary(rez))

       myDict =  helpers.build_subst_dictionary("A=B")
       self.assertEqual(myDict['B'],'A')

       myDict =  helpers.build_subst_dictionary("A=B;C=DD")
       self.assertEqual(myDict['B'],'A')
       self.assertEqual(myDict['DD'],'C')
       myDict =  helpers.build_subst_dictionary("A=B=C=DD")
       self.assertEqual(myDict['B'],'A')
       self.assertEqual(myDict['DD'],'A')
       self.assertEqual(myDict['C'],'A')

       myDict =  helpers.build_subst_dictionary("A = B = C=DD")
       self.assertEqual(myDict['B'],'A')
       self.assertEqual(myDict['DD'],'A')
       self.assertEqual(myDict['C'],'A')

if __name__=="__main__":
        unittest.main()




