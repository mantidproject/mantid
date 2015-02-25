import os,sys
#os.environ["PATH"] = r"c:/Mantid/Code/builds/br_master/bin/Release;"+os.environ["PATH"]

from mantid.simpleapi import *
from mantid import api

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

if __name__=="__main__":
    unittest.main()

