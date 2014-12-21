import os
#os.environ["PATH"] = r"c:/Mantid/Code/builds/br_10803/bin/Release;"+os.environ["PATH"]
from mantid.simpleapi import *
from mantid import api
import unittest
import Direct.ReductionHelpers as helpers

class SomeDescriptor(object):
         def __init__(self):
             self._val=None

         def __get__(self,instance,owner=None):
             if instance is None:
                 return self

             return self._val

         def __set__(self,instance,value):
                self._val = value
         def get_helper(self):
             return "using helper"
         def set_helper(self,value):
             self._val=value



class DirectReductionHelpersTest(unittest.TestCase):
    def __init__(self, methodName):
        return super(DirectReductionHelpersTest, self).__init__(methodName)

    @staticmethod
    def getInstrument(InstrumentName='MAR'):
        """ test method used to obtain default instrument for testing """
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


    def testbuild_properties_dict(self):
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

        subst_dict,descr = helpers.build_properties_dict(kkdict,subst)

        self.assertEqual(len(subst_dict),6);

        val = subst_dict['_first'];
        self.assertTrue(type(val) is helpers.ComplexProperty)

        #self.assertEqual(val[0],'kkk1');
        #self.assertEqual(val[1],'kkk2');

        val = subst_dict['other']
        self.assertFalse(type(val) is helpers.ComplexProperty)
        self.assertEqual(val,'unrelated');

        val = subst_dict['_second']

        self.assertTrue(type(val) is helpers.ComplexProperty)
        #self.assertTrue(isinstance(val,list))
        #self.assertEqual(val[0],'kkk1')
        #self.assertEqual(val[1],'other')
        #self.assertEqual(val[2],'third')


    def testbuild_properties_dict_pref(self):
        kkdict = {};
        kkdict['first']='kkk1:kkk2'
        kkdict['kkk1']=19
        kkdict['kkk2']=1000;
        kkdict['other']='unrelated'
        kkdict['second']='ssss1:ssss2:third'
        kkdict['third']='Babara'
        kkdict['descr1']='ddd'
        kkdict['descr2']='kkk1:kkk2'
        kkdict['descr3']=10

        subst = {}
        subst['ssss1']='kkk1'
        subst['ssss2']='other'
        subst['descr2']='des222'


        prop_dict,desct = helpers.build_properties_dict(kkdict,subst,['descr1','des222','descr3'])
        self.assertEqual(len(desct),3)
        self.assertEqual(desct['descr3'],10)
        self.assertEqual(desct['descr1'],'ddd')
        self.assertTrue('des222' in desct.keys())


        self.assertEqual(len(prop_dict),6)

        val = prop_dict['_first']
        self.assertTrue(type(val) is helpers.ComplexProperty)

        #elf.assertEqual(val[0],'_kkk1');
        #self.assertEqual(val[1],'_kkk2');

        val = prop_dict['other']
        self.assertFalse(type(val) is helpers.ComplexProperty)
        self.assertEqual(val,'unrelated');

        val = prop_dict['_second']
        self.assertTrue(type(val) is helpers.ComplexProperty)

        #self.assertEqual(val[0],'_kkk1')
        #self.assertEqual(val[1],'_other')
        #self.assertEqual(val[2],'_third')


        val = prop_dict['third']
        self.assertFalse(type(val) is helpers.ComplexProperty)
        self.assertEqual(val,'Babara')


    def test_build_properties_dict_ksubst(self):
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

        subst_dict,descr_dict = helpers.build_properties_dict(kkdict,subst)

        self.assertEqual(len(subst_dict),6);

        val = subst_dict['_1']
        self.assertTrue(type(val) is helpers.ComplexProperty)

        #self.assertEqual(val[0],'kkk1');
        #self.assertEqual(val[1],'kkk2');

        val = subst_dict['other']
        self.assertFalse(type(val) is helpers.ComplexProperty)
        self.assertEqual(val,'unrelated');

        val = subst_dict['_2']
        self.assertTrue(type(val) is helpers.ComplexProperty)

        #self.assertEqual(val[0],'kkk1')
        #self.assertEqual(val[1],'other')
        #self.assertEqual(val[2],'3')

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

        subst_dict,descr = helpers.build_properties_dict(kkdict,subst)
        self.assertEqual(helpers.gen_getter(subst_dict,'kkk1'),19);
        self.assertEqual(helpers.gen_getter(subst_dict,'kkk2'),1000);
        self.assertEqual(helpers.gen_getter(subst_dict,'first'),[19,1000]);
        self.assertEqual(helpers.gen_getter(subst_dict,'other'),'unrelated');
        self.assertEqual(helpers.gen_getter(subst_dict,'second'),[19,'unrelated','Babara']);
        self.assertEqual(helpers.gen_getter(subst_dict,'third'),'Babara');

    def test_gen_setter(self):
        kkdict = {};
        kkdict['A']=helpers.ComplexProperty(['B','C']);
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
                object.__setattr__(self,'A',helpers.ComplexProperty(['B','C']))
                #kkdict['A']=
                kkdict = {};
                kkdict['B']=19;
                kkdict['C']=1000; 
                self.__dict__.update(kkdict)

            oveloaded_prop = SomeDescriptor()

            def __setattr__(self,name,val):
                if name in self.__class__.__dict__:
                    fob = self.__class__.__dict__[name]
                    fob.__set__(self,val)
                    return
                if name in self.__dict__:
                    pw = self.__dict__[name]
                    if isinstance(pw,helpers.ComplexProperty):
                        pw.__set__(self,val)
                    else:
                        self.__dict__[name] = val
                        #object.__setattr__(self,name,val)
                    return
                raise KeyError("Property {0} is not defined for class {1}".format(name,self.__class__))
                #helpers.gen_setter(self,name,val);
                #object.__setattr__(self,name,val)

            
            def __getattribute__(self,name):
                if name[:2] == '__':
                    attr = object.__getattribute__(self,name);
                    return attr
                else:
                    attr_dic = object.__getattribute__(self,'__dict__');
                    if name is '__dict__':
                        return attr_dic;
                    else:
                        return helpers.gen_getter(attr_dic,name)
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

        # Very bad -- fails silently
        t1.A[0] = 10;
        self.assertEqual(t1.A,[1,10]);


        t1.oveloaded_prop = 'BlaBla'
        #And this become too complicated:: to implement access to __class__.__dict__
        #self.assertEqual(t1.oveloaded_prop ,'BlaBla');

    def test_class_property_setter2(self):
        class test_class(object):
            def __init__(self):
                kkdict = {};
                kkdict['_A']=helpers.ComplexProperty(['B','C']);
                kkdict['B']=19;
                kkdict['C']=1000; 
                class_decor = '_'+type(self).__name__;

                kkdict[class_decor+'__special']='D'; 
                self.__dict__.update(kkdict)

            some_descriptor = SomeDescriptor()

            def __setattr__(self,name,val):
                if name is 'special':
                    return;
                elif name in self.__class__.__dict__:
                    fp = self.__class__.__dict__[name]
                    fp.__set__(self,val)
                else:
                    helpers.gen_setter(self.__dict__,name,val);

   
            def __getattr__(self,name):
                if name is 'special':
                    return self.__special;
                else:
                    tDict = object.__getattribute__(self,'__dict__');
                    return helpers.gen_getter(tDict,name)
  
            def access(self,obj_name):
               try:
                  obj = self.__class__.__dict__[obj_name]
                  return obj
               except:
                  priv_name = '_'+obj_name
                  if priv_name in self.__dict__:
                     return self.__dict__[priv_name]
                  else:
                      raise KeyError("Property {0} is not among class descriptors or complex properties ".format(obj_name))
               


        t1 =test_class()

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

        # Very bad -- fails silently
        t1.A[0] = 10;
        self.assertEqual(t1.A,[1,10]);

        t1.special = 10;
        self.assertEqual(t1.special,'D');

        t1.some_descriptor = 'blaBla'
        self.assertEqual(t1.some_descriptor ,'blaBla');

        self.assertEqual(t1.access('some_descriptor').get_helper() ,'using helper');

        t1.access('some_descriptor').set_helper('other')
        self.assertEqual(t1.some_descriptor ,'other');

        dep = t1.access('A').dependencies()
        self.assertEqual(dep[0],'B')
        self.assertEqual(dep[1],'C')

    def test_class_property_setter3(self):
        class test_class(object):
            def __init__(self):
                all_methods = dir(self)
                existing=[]
                for meth in all_methods:
                    if meth[:1] != '_':
                        existing.append(meth)
                kkdict = {};
                kkdict['_A']=helpers.ComplexProperty(['B','C']);
                kkdict['B']=19;
                kkdict['C']=1000;

                class_decor = '_'+type(self).__name__;
                object.__setattr__(self,class_decor+'__exmeth',existing)
                

                self.__dict__.update(kkdict)

            some_descriptor = SomeDescriptor()

            def __setattr__(self,name,val):
                if  name in self.__exmeth:
                    object.__setattr__(self,name,val)
                else:
                    helpers.gen_setter(self.__dict__,name,val);
                    #raise KeyError("Property {0} is not among recognized properties".format(name))

            def __getattr__(self,name):
                return helpers.gen_getter(self.__dict__,name);

  
            def access(self,obj_name):
               try:
                  obj = self.__class__.__dict__[obj_name]
                  return obj
               except:
                  priv_name = '_'+obj_name
                  if priv_name in self.__dict__:
                     return self.__dict__[priv_name]
                  else:
                      raise KeyError("Property {0} is not among class descriptors or complex properties ".format(obj_name))
               


        t1 =test_class()

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

        # Very bad -- fails silently
        t1.A[0] = 10;
        self.assertEqual(t1.A,[1,10]);


        t1.some_descriptor = 'blaBla'
        self.assertEqual(t1.some_descriptor ,'blaBla');

        self.assertEqual(t1.access('some_descriptor').get_helper() ,'using helper')
        t1.access('some_descriptor').set_helper('other')
        self.assertEqual(t1.some_descriptor ,'other');

        dep = t1.access('A').dependencies()
        self.assertEqual(dep[0],'B')
        self.assertEqual(dep[1],'C')

        self.assertEqual(test_class.some_descriptor.get_helper(),'using helper')

        prop = getattr(test_class,'some_descriptor')
        self.assertEqual(prop.get_helper(),'using helper')


        
if __name__=="__main__":
        unittest.main()




