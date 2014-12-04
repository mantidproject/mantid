from mantid.simpleapi import *
from mantid import api
import unittest
import inspect
import numpy as np
from DirectPropertyManager import DirectPropertyManager


#-----------------------------------------------------------------------------------------------------------------------------------------
#-----------------------------------------------------------------------------------------------------------------------------------------
#-----------------------------------------------------------------------------------------------------------------------------------------
#-----------------------------------------------------------------------------------------------------------------------------------------
class ReductionWrapperTest(unittest.TestCase):

    def __init__(self, methodName):
        self.prop_man = DirectPropertyManager("MAR");
        return super(DirectPropertyManagerTest, self).__init__(methodName)

    def setUp(self):
        if self.prop_man == None or type(self.prop_man) != type(DirectPropertyManager):
            self.prop_man  = DirectPropertyManager("MAR");
    def tearDown(self):
        pass

 
    def test_export_advanced_values(self):
        propman = self.prop_man 

        propman.sample_run = 11001;
        propman.wb_run     = 11060;

        propman.incident_energy =  12;
        propman.energy_bins     = [-11,0.05,11]

      # Absolute units reduction properties.
        propman.monovan_run= 11015;
        propman.sample_mass= 10;
        propman.sample_rmm = 435.96


        propman.record_advanced_properties = True;
        #
        propman.map_file = "mari_res.map"
        propman.monovan_mapfile = "mari_res.map"
        propman.hard_mask_file ="mar11015.msk"
        propman.det_cal_file =11060
        propman.save_format='nxs'
        propman.record_advanced_properties = False;

        # see what have changed and what have changed as advanced properties. 
        all_initial_prop = propman.getChangedProperties();
        adv_initial_prop = propman.getChangedAdvancedProperties();

        test_dir = config['defaultsave.directory'];
        file = os.path.join(test_dir,'reduce_vars.py');
        propman.export_changed_values(file);
        self.assertTrue(os.path.isfile(file));

        # restore saved parameters.
        sys.path.insert(0,test_dir);

        import reduce_vars as rv

        # apply them to new properties.
        other_prop = self.prop_man;
        other_prop.set_input_parameters(**(rv.standard_vars));
        simple_prop = other_prop.getChangedProperties();

        propman.record_advanced_properties = True;
        # bad -- TODO: deal with this. 
        del rv.advanced_vars['record_advanced_properties'];
        other_prop.set_input_parameters(**(rv.advanced_vars));
        propman.record_advanced_properties = False;

        os.remove(file);

        # look what have changed and compare with initial data. 
        all_prop = other_prop.getChangedProperties();
        adv_prop = other_prop.getChangedAdvancedProperties();

        self.assertEqual(all_initial_prop,all_prop)
        self.assertEqual(adv_initial_prop,adv_prop)

        fbase,fext = os.path.splitext(file)
        fcomp = fbase+'.pyc'
        if os.path.isfile(fcomp):
            os.remove(fcomp);
