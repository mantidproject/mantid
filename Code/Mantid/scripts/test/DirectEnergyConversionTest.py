import unittest
import DirectEnergyConversion

class DirectEnergyConversionTest(unittest.TestCase):
    def setUp(self):
        self.reducer = DirectEnergyConversion();

    def test_build_subst_dictionary(self):
       self.assertEqual(dict(), DirectEnergyConversion.build_subst_dictionary(""))
       self.assertEqual(dict(),DirectEnergyConversion.build_subst_dictionary())

       #self.assertRaises(AttributeError,DirectEnergyConversion.build_subst_dictionary(10))
       #self.assertRaises(AttributeError,DirectEnergyConversion.build_subst_dictionary("A="))
       #self.assertRaises(AttributeError,DirectEnergyConversion.build_subst_dictionary("B=C;A="))

       rez=dict();
       rez['A']='B';
       self.assertEqual(rez, DirectEnergyConversion.build_subst_dictionary(rez))

       myDict =  DirectEnergyConversion.build_subst_dictionary("A=B")
       self.assertEqual(myDict['B'],'A')

       myDict =  DirectEnergyConversion.build_subst_dictionary("A=B;C=DD")
       self.assertEqual(myDict['B'],'A')
       self.assertEqual(myDict['DD'],'C')
       myDict =  DirectEnergyConversion.build_subst_dictionary("A=B=C=DD")
       self.assertEqual(myDict['B'],'A')
       self.assertEqual(myDict['DD'],'A')
       self.assertEqual(myDict['C'],'A')
 
    def test_init_reducer(self):
        self.reducer.initialise("MAP",true);

if __name__=="__main__":
    unittest.main()
