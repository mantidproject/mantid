from mantid.simpleapi import *
from mantid import api

from Direct.ReductionWrapper import *
from MariReduction import ReduceMARI
import os,sys

#
import unittest

#-----------------------------------------------------------------------------------------------------------------------------------------
#-----------------------------------------------------------------------------------------------------------------------------------------
#-----------------------------------------------------------------------------------------------------------------------------------------
#-----------------------------------------------------------------------------------------------------------------------------------------
class ReductionWrapperTest(unittest.TestCase):

    def __init__(self, methodName):
        return super(ReductionWrapperTest, self).__init__(methodName)

        pass

    def setUp(self):
        pass
    def tearDown(self):
        pass

    def test_default_fails(self):
        red=ReductionWrapper('MAR');


        self.assertRaises(NotImplementedError,red.def_main_properties)
        self.assertRaises(NotImplementedError,red.def_advanced_properties)
        self.assertRaises(NotImplementedError,red.main)


 
    def test_export_advanced_values(self):
        red = ReduceMARI();

        main_prop=red.def_main_properties();
        adv_prop=red.def_advanced_properties();


        # see what have changed and what have changed as advanced properties. 
        all_changed_prop = red.iliad_prop.getChangedProperties();

        self.assertEqual(set(main_prop.keys()+adv_prop.keys()),all_changed_prop);

        test_dir = config['defaultsave.directory'];
        file = os.path.join(test_dir,'reduce_vars.py');
        red.export_changed_values(file);
        self.assertTrue(os.path.isfile(file));

        # restore saved parameters.
        sys.path.insert(0,test_dir);

        import reduce_vars as rv

        self.assertEqual(rv.standard_vars,main_prop);
        self.assertEqual(rv.advanced_vars,adv_prop);



        os.remove(file);
        fbase,fext = os.path.splitext(file)
        fcomp = fbase+'.pyc'
        if os.path.isfile(fcomp):
            os.remove(fcomp);

if __name__=="__main__":
        unittest.main()

