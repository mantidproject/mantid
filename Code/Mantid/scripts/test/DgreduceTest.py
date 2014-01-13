from mantid.simpleapi import *
from mantid import api
import unittest
import os, sys
from dgreduce import *



class DgreduceTest(unittest.TestCase):
    def __init__(self, methodName):
        setup("MAPS")
        return super(DgreduceTest, self).__init__(methodName)

    def setUp(self):
        pass

    def tearDown(self):
        pass

    def test_run_help(self):
        self.assertRaises(ValueError,help,'rubbish')
        help("monovan_lo_bound")
    def test_process_legacy_parameters(self):
        kw=dict();
        kw["hardmaskOnly"]="someFileName"
        kw["someKeyword"] ="aaaa"
        kw["normalise_method"] ="Monitor-1"
        params = process_legacy_parameters(**kw);
        self.assertEqual(len(params),5)
        self.assertTrue("someKeyword" in params);
        self.assertTrue("hard_mask_file" in params); 
        self.assertTrue("use_hard_mask_only" in params)
        self.assertEqual(params['normalise_method'],'Monitor-1')
        self.assertEqual(params['use_hard_mask_only'],True)
        self.assertEqual(params['hard_mask_file'],"someFileName")

    def test_process_leg_par_harmaskPlus(self):
        kw=dict();
        kw['hardmaskPlus']='SomeFileName'
        params = process_legacy_parameters(**kw);
        self.assertEqual(len(params),2)
        self.assertEqual(params["hard_mask_file"],'SomeFileName');
        self.assertEqual(params['use_hard_mask_only'],False)
    def test_setup(self):
       

        setup('mari')
        Reducer = getReducer()
        self.assertTrue(not (Reducer is None))

        self.assertEqual(Reducer.instr_name,'MAR')

        Reducer.save_format = ''
        self.assertTrue(Reducer.save_format is None)

        Reducer.save_format = 'none'
        self.assertTrue(Reducer.save_format is None)

        Reducer.save_format = []
        self.assertTrue(Reducer.save_format is None)

    def test_setup_empty(self):
        # clear up singleton
        global Reducer
        Reducer = None


        setup(None)
        
if __name__=="__main__":
    unittest.main()        
