from mantid.simpleapi import *
from mantid import api
import unittest
import DirectReductionHelpers as helpers


class DirectReductionHelpersTest(unittest.TestCase):
    def __init__(self, methodName):
        return super(DirectReductionHelpersTest, self).__init__(methodName)

    @staticmethod
    def getInstrument(InstrumentName='MAR'):
        idf_dir = config.getString('instrumentDefinition.directory')
        idf_file=api.ExperimentInfo.getInstrumentFilename(InstrumentName)
        tmp_ws_name = '__empty_' + InstrumentName
        if not mtd.doesExist(tmp_ws_name):
               LoadEmptyInstrument(Filename=idf_file,OutputWorkspace=tmp_ws_name)
        return mtd[tmp_ws_name].getInstrument();


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

    def test_get_default_idf_param_list(self):
        pInstr=self.getInstrument();

        param_list = helpers.get_default_idf_param_list(pInstr);
        self.assertTrue(isinstance(param_list,dict))
        # check couple of parameters which are certainly in IDF
        self.assertTrue('deltaE-mode' in param_list)
        self.assertTrue('normalise_method' in param_list)
        self.assertTrue('diag_samp_lo' in param_list)


    def test_build_coupled_keys_dict(self):
        kkdict = {};
        kkdict['first']='kkk1:kkk2';
        kkdict['kkk1']=19;
        kkdict['kkk2']=1000;
        kkdict['other']='unrelated';
        kkdict['second']='ssss1:ssss2:third';
        kkdict['third']='Babara';

        subst = {};
        subst['ssss1']='kkk1';
        subst['ssss2']='other';

        subst_dict = helpers.build_coupled_keys_dict(kkdict,subst)

        self.assertEqual(len(subst_dict),6);
        self.assertTrue(isinstance(subst_dict['first'],list))
        self.assertEqual(subst_dict['first'][0],'kkk1');
        self.assertEqual(subst_dict['first'][1],'kkk2');
        self.assertEqual(subst_dict['other'],'unrelated');
        self.assertTrue(isinstance(subst_dict['second'],list))
        self.assertEqual(subst_dict['second'][0],'kkk1')
        self.assertEqual(subst_dict['second'][1],'other')
        self.assertEqual(subst_dict['second'][2],'third')

    def test_build_coupled_keys_dict_ksubst(self):
        kkdict = {};
        kkdict['first']='kkk1:kkk2';
        kkdict['kkk1']=19;
        kkdict['kkk2']=1000;
        kkdict['other']='unrelated';
        kkdict['second']='ssss1:ssss2:third';
        kkdict['third']='Babara';

        subst = {};
        subst['first']=1;
        subst['ssss1']='kkk1';
        subst['ssss2']='other';
        subst['third']=3;
        subst['second']=2;

        subst_dict = helpers.build_coupled_keys_dict(kkdict,subst)

        self.assertEqual(len(subst_dict),6);
        self.assertTrue(isinstance(subst_dict[1],list))
        self.assertEqual(subst_dict[1][0],'kkk1');
        self.assertEqual(subst_dict[1][1],'kkk2');
        self.assertEqual(subst_dict['other'],'unrelated');
        self.assertTrue(isinstance(subst_dict[2],list))
        self.assertEqual(subst_dict[2][0],'kkk1')
        self.assertEqual(subst_dict[2][1],'other')
        self.assertEqual(subst_dict[2][2],3)

    def test_gen_getter(self):
        kkdict = {};
        kkdict['first']='kkk1:kkk2';
        kkdict['kkk1']=19;
        kkdict['kkk2']=1000;
        kkdict['other']='unrelated';
        kkdict['second']='ssss1:ssss2:third';
        kkdict['third']='Babara';

        subst = {};
        subst['ssss1']='kkk1';
        subst['ssss2']='other';

        subst_dict = helpers.build_coupled_keys_dict(kkdict,subst)
        self.assertEqual(helpers.gen_getter(subst_dict,'kkk1'),19);
        self.assertEqual(helpers.gen_getter(subst_dict,'kkk2'),1000);
        self.assertEqual(helpers.gen_getter(subst_dict,'first'),[19,1000]);
        self.assertEqual(helpers.gen_getter(subst_dict,'other'),'unrelated');
        self.assertEqual(helpers.gen_getter(subst_dict,'second'),[19,'unrelated','Babara']);
        self.assertEqual(helpers.gen_getter(subst_dict,'third'),'Babara');

    def test_gen_setter(self):
        kkdict = {};
        kkdict['A']=['B','C'];
        kkdict['B']=19;
        kkdict['C']=1000;
   

        helpers.gen_setter(kkdict,'B',0)
        self.assertEqual(kkdict['B'],0);
        helpers.gen_setter(kkdict,'C',10)
        self.assertEqual(kkdict['C'],10);

        self.assertRaises(KeyError,helpers.gen_setter,kkdict,'A',100)
        self.assertEqual(kkdict['B'],0);

        helpers.gen_setter(kkdict,'A',[1,10])
        self.assertEqual(kkdict['B'],1);
        self.assertEqual(kkdict['C'],10);

    def test_class_property_setter(self):
        class test_class(object):
            def __init__(self):
                kkdict = {};
                kkdict['A']=['B','C'];
                kkdict['B']=19;
                kkdict['C']=1000; 
                self.__dict__.update(kkdict)


            def __setattr__(self,name,val):
                helpers.gen_setter(self.__dict__,name,val);

   
            def __getattribute__(self,name):
                tDict = object.__getattribute__(self,'__dict__');
                if name is '__dict__':
                    # first call with empty dictionary
                    return tDict;
                else:
                    return helpers.gen_getter(tDict,name)
                pass


        t1 =test_class();

        self.assertEqual(t1.B,19);

        t1.B=0;
        self.assertEqual(t1.B,0);

        self.assertRaises(KeyError,setattr,t1,'non_existing_property','some value')


        t1.A = [1,10];
        self.assertEqual(t1.A,[1,10]);

        # This does not work as the assignment occurs to temporary vector
        # lets ban partial assignment
        #t1.D[0] = 200;
        #self.assertEqual(t1.B,200);
        # This kind of assignment requests the whole list to be setup  
        self.assertRaises(KeyError,setattr,t1,'A',200)
        
if __name__=="__main__":
        unittest.main()




