import os,sys
#os.environ["PATH"] = r"c:/Mantid/Code/builds/br_master/bin/Release;"+os.environ["PATH"]

from mantid.simpleapi import *
from mantid import api,config

from Direct.ReductionWrapper import ReductionWrapper
import MariReduction as mr


#
import unittest

#-----------------------------------------------------------------------------------------------------------------------------------------
#-----------------------------------------------------------------------------------------------------------------------------------------
#-----------------------------------------------------------------------------------------------------------------------------------------
#-----------------------------------------------------------------------------------------------------------------------------------------
class ReductionWrapperTest(unittest.TestCase):

    def __init__(self, methodName):
        return super(ReductionWrapperTest, self).__init__(methodName)


    def setUp(self):
        pass
    def tearDown(self):
        pass

    def test_default_fails(self):
        red=ReductionWrapper('MAR')


        self.assertRaises(NotImplementedError,red.def_main_properties)
        self.assertRaises(NotImplementedError,red.def_advanced_properties)
        self.assertTrue('reduce' in dir(red))


 
    def test_export_advanced_values(self):
        red = mr.ReduceMARI()

        main_prop=red.def_main_properties()
        adv_prop=red.def_advanced_properties()


        # see what have changed and what have changed as advanced properties. 
        all_changed_prop = red.reducer.prop_man.getChangedProperties()

        self.assertEqual(set(main_prop.keys()+adv_prop.keys()),all_changed_prop)

        test_dir = config['defaultsave.directory']
        file = os.path.join(test_dir,'reduce_vars.py')
        red.save_web_variables(file)
        self.assertTrue(os.path.isfile(file))

        # restore saved parameters.
        sys.path.insert(0,test_dir)

        import reduce_vars as rv

        self.assertEqual(rv.standard_vars,main_prop)
        self.assertEqual(rv.advanced_vars,adv_prop)

        reload(mr)

        # tis will run MARI reduction, which probably not work from unit tests
        # will move this to system tests
        #rez = mr.main()

        self.assertTrue(mr.web_var)
        self.assertEqual(mr.web_var.standard_vars,main_prop)
        self.assertEqual(mr.web_var.advanced_vars,adv_prop)


        os.remove(file)
        fbase,fext = os.path.splitext(file)
        fcomp = fbase+'.pyc'
        if os.path.isfile(fcomp):
            os.remove(fcomp)

    def test_validate_settings(self):
        dsp = config.getDataSearchDirs()
        # clear all not to find any files
        config.setDataSearchDirs('')

        red = mr.ReduceMARI()
        ok,level,errors = red.validate_settings()

        self.assertFalse(ok)
        self.assertEqual(level,2)
        self.assertEqual(len(errors),7)


 

        # this run should be in data search directory for basic Mantid
        red.reducer.wb_run       = 11001
        red.reducer.det_cal_file = '11001'
        red.reducer.monovan_run = None
        red.reducer.hard_mask_file = None
        red.reducer.map_file = None
        red.reducer.save_format = 'nxspe'

        path = []
        for item in dsp:
            path.append(item)
        config.setDataSearchDirs(path)


        # hack -- let's pretend we are running from webservices 
        # but web var are empty (not to overwrite values above)
        red._run_from_web = True
        red._wvs.standard_vars={}
        red._wvs.advanced_vars={}
        ok,level,errors = red.validate_settings()

        self.assertTrue(ok)
        self.assertEqual(level,0)
        self.assertEqual(len(errors),0)

        # this is how we set it up from web
        red._wvs.advanced_vars={'save_format':''}
        ok,level,errors = red.validate_settings()

        self.assertFalse(ok)
        self.assertEqual(level,1)
        self.assertEqual(len(errors),1)



if __name__=="__main__":
    unittest.main()

