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

    #TODO: write help
    #def test_run_help(self):
    #    self.assertRaises(ValueError,help,'rubbish')
    #    help("monovan_lo_bound")
    def test_process_legacy_parameters(self):
        kw=dict();
        kw["hardmaskOnly"]="someFileName"
        kw["someKeyword"] ="aaaa"
        kw["normalise_method"] ="Monitor-1"
        params = process_legacy_parameters(**kw);
        self.assertEqual(len(params),4)
        self.assertTrue("someKeyword" in params);
        self.assertTrue("hard_mask_file" in params);
        self.assertTrue("use_hard_mask_only" in params)
        self.assertEqual(params['normalise_method'],'Monitor-1')
        self.assertEqual(params['use_hard_mask_only'],True)
        self.assertEqual(params['hard_mask_file'],"someFileName")
    def test_process_leg_param_hardmaskfile(self):
        kw=dict();
        kw["hard_mask_file"]="someFileName"
        params = process_legacy_parameters(**kw);
        self.assertEqual(params['hard_mask_file'],"someFileName")

        kw["hard_mask_file"]=""
        params = process_legacy_parameters(**kw);
        self.assertTrue(params['hard_mask_file'] is None)

        kw["hard_mask_file"]=[]
        params = process_legacy_parameters(**kw);
        self.assertTrue(params['hard_mask_file'] is None)

        kw["hard_mask_file"]=None
        params = process_legacy_parameters(**kw);
        self.assertTrue(params['hard_mask_file'] is None)

        kw["hard_mask_file"]="None"
        params = process_legacy_parameters(**kw);
        self.assertTrue(params['hard_mask_file'] is None)

        kw["hard_mask_file"]=False
        params = process_legacy_parameters(**kw);
        self.assertTrue(params['hard_mask_file'] is None)

        kw["hard_mask_file"]=True
        self.assertRaises(TypeError,process_legacy_parameters,**kw)
        # no hard mask file
        kw1=dict();
        kw1["irrelevant_paramter"]=False
        params = process_legacy_parameters(**kw1);
        self.assertTrue(params['irrelevant_paramter'] is False)



    def test_process_leg_par_harmaskPlus(self):
        kw=dict();
        kw['hardmaskPlus']='SomeFileName'
        params = process_legacy_parameters(**kw);
        self.assertEqual(len(params),2)
        self.assertEqual(params["hard_mask_file"],'SomeFileName');
        self.assertEqual(params['use_hard_mask_only'],False)


    def test_setup_empty(self):
        # clear up singleton
        global Reducer
        Reducer = None


        setup(None)

if __name__=="__main__":
    unittest.main()
