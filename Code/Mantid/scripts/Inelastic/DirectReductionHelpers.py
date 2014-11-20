"""
Set of functions to assist with processing instrument parameters relevant to reduction. 
"""

def get_default_parameter(instrument, name):
        """ Function gets the value of a default instrument parameter and 
            assign proper(the one defined in IPF ) type to this parameter 
            @param instrument -- 
        """ 

        if instrument is None:
            raise ValueError("Cannot initiate default parameter, instrument has not been properly defined.")

        type_name = instrument.getParameterType(name)
        if type_name == "double":
            val = instrument.getNumberParameter(name)
        elif type_name == "bool":
            val = instrument.getBoolParameter(name)
        elif type_name == "string":
            val = instrument.getStringParameter(name)
            if val[0] == "None" :
                return None
        elif type_name == "int" :
              val = instrument.getIntParameter(name)
        else :
            raise KeyError(" Instrument: {0} does not have parameter with name: {1}".format(instrument.getName(),name))

        return val[0]

def get_default_idf_param_list(pInstrument,synonims_list=None):
    """ Obtain default reduction parameters list from the instrument """

    params = pInstrument.getParameterNames();
    par_list = {};
    for name in params:
        par_list[name] = get_default_parameter(pInstrument,name);


    return par_list;



def build_properties_dict(param_map,synonims,preffix='') :
    """function to build the dictionary of the keys which are expressed through other keys

       e.g. builds dictionary from strings in a form key1 = key2:key3  
       in the form key1 = ['key2','key3'] 

    """
 
    # dictionary used for substituting composite keys.
    composite_keys = dict();

    for name in param_map:
       if name in synonims:
          final_name = preffix+str(synonims[name]);
       else:
          final_name = preffix+str(name)
       composite_keys[final_name]=None;

    param_keys = composite_keys.keys();

    for name,val in param_map.items() :
        if name in synonims:
            final_name = preffix+str(synonims[name]);
        else:
            final_name = preffix+str(name)

        if isinstance(val,str):  
               val = val.strip()
               keys_candidates = val.split(":")
               n_keys = len(keys_candidates)
               #
               if n_keys>1 : # this is the property we want to modify
                   composite_keys[final_name]= list();
                   for key in keys_candidates :
                       if key in synonims:
                           rkey = preffix+str(synonims[key]);
                       else:
                           rkey = preffix+str(key);
                       if rkey in param_keys:
                          composite_keys[final_name].append(rkey);
                       else:
                          raise KeyError('Substitution key : {0} is not in the list of allowed keys'.format(rkey));
               else:
                   composite_keys[final_name] = keys_candidates[0];
        else:
            composite_keys[final_name]=val;

    return composite_keys



def build_subst_dictionary(synonims_list=None) :
    """Function to process "synonims_list" in the instrument parameters string, used to support synonyms in the reduction script

       it takes string of synonyms in the form key1=subs1=subst2=subts3;key2=subst4 and returns the dictionary
       in the form dict[subs1]=key1 ; dict[subst2] = key1 ... dict[subst4]=key2

       e.g. if one wants to use the IDF key word my_detector instead of e.g. norm-mon1-spec, he has to type
       norm-mon1-spec=my_detector in the synonyms field of the IDF parameters file.
    """
    if not synonims_list :  # nothing to do
            return dict();
    if type(synonims_list) == dict : # all done
            return synonims_list
    if type(synonims_list) != str :
            raise AttributeError("The synonyms field of Reducer object has to be special format string or the dictionary")
        # we are in the right place and going to transform string into dictionary

    subst_lines = synonims_list.split(";")
    rez  = dict()
    for lin in subst_lines :
        lin=lin.strip()
        keys = lin.split("=")
        if len(keys) < 2 :
                raise AttributeError("The pairs in the synonyms fields have to have form key1=key2=key3 with at least two values present")
        if len(keys[0]) == 0:
                raise AttributeError("The pairs in the synonyms fields have to have form key1=key2=key3 with at least two values present, but the first key is empty")
        for i in xrange(1,len(keys)) :
                if len(keys[i]) == 0 :
                    raise AttributeError("The pairs in the synonyms fields have to have form key1=key2=key3 with at least two values present, but the key"+str(i)+" is empty")
                kkk = keys[i].strip();
                rez[kkk]=keys[0].strip()

    return rez;

def gen_getter(keyval_dict,key):
    """ function returns value from dictionary with substitution 
    
        e.g. if keyval_dict[A] = 10, keyval_dict[B] = 20 and key_val[C] = [A,B]
        gen_getter(keyval_dict,A) == 10;  gen_getter(keyval_dict,B) == 20;
        and gen_getter(keyval_dict,C) == [10,20];
    """

    if key in keyval_dict:
        test_val = keyval_dict[key];
 

        if not isinstance(test_val,list):
            return test_val;

        fin_val = list();
        for ik in test_val:
            fin_val.append(keyval_dict[ik]);
        return fin_val;

    else:
        raise KeyError(' key with name: {0} is not in the data dictionary'.format(key));

def gen_setter(keyval_dict,key,val):
    """ function sets value to dictionary with substitution 
    
        e.g. if keyval_dict[A] = 10, keyval_dict[B] = 20 and key_val[C] = [A,B]

        gen_setter(keyval_dict,A,20)  causes keyval_dict[A] == 20
        gen_setter(keyval_dict,B,30)  causes keyval_dict[B] == 30
        and gen_getter(keyval_dict,C,[1,2]) causes keyval_dict[A] == 1 and keyval_dict[B] == 2
    """

    if key in keyval_dict:
        test_val = keyval_dict[key];
        if not isinstance(test_val,list):
            # this is temporary check, disallowing assigning values to complex properties 
            # As such, it also prohibits properties from having list values. TODO: Should be done more intelligently
            if isinstance(val,list):
                raise KeyError(' Key {0} can not assigned a list value'.format(key));
            else:
                keyval_dict[key] = val;
            return;

        if isinstance(val,list):
            if len(val) != len(test_val):
                raise KeyError('Property: {0} needs list of the length {1} to be assigned to it'.format(key,len(test_val)))
        else:
            raise KeyError(' You can not assign non-list value to list property {0}'.format(key));
        pass


        for i,key in enumerate(test_val):
            keyval_dict[key] = val[i];
        return;
    else:
        raise KeyError(' key with name: {0} is not in the data dictionary'.format(key));


def check_instrument_name(old_name,new_name):
    """ function checks if new instrument name is acceptable instrument name"""


    if new_name is None:
       if not(old_name is None):
            return
       else:
            raise KeyError("No instrument name is defined")

    if old_name == new_name:
       return;

    # Instrument name might be a prefix, query Mantid for the full name
    short_name=''
    full_name=''
    try :
        instrument = config.getFacility().instrument(new_name)
        short_name = instrument.shortName()
        full_name = instrument.name()
    except:
        # it is possible to have wrong facility:
        facilities = config.getFacilities()
        old_facility = str(config.getFacility())
        for facility in facilities:
               config.setString('default.facility',facility.name())
               try :
                   instrument = facility.instrument(new_name)
                   short_name = instrument.shortName()
                   full_name = instrument.name()
                   if len(short_name)>0 :
                       break
               except:
                   pass
        if len(short_name)==0 :
           config.setString('default.facility',old_facility)
           raise KeyError(" Can not find/set-up the instrument: "+new_name+' in any supported facility')

    new_name = short_name
    facility = str(config.getFacility())

    config['default.instrument'] = full_name
    return (new_name,full_name,facility);



       #if not hasattr(self,'instrument') or self.instrument.getName() != instr_name :
       #     # Load an empty instrument if one isn't already there
       #     idf_dir = config.getString('instrumentDefinition.directory')
       #     try:
       #         idf_file=api.ExperimentInfo.getInstrumentFilename(new_name)
       #         tmp_ws_name = '__empty_' + new_name
       #         if not mtd.doesExist(tmp_ws_name):
       #             LoadEmptyInstrument(Filename=idf_file,OutputWorkspace=tmp_ws_name)
       #         self.instrument = mtd[tmp_ws_name].getInstrument()
       #     except:
       #         self.instrument = None
       #         self._instr_name = None
       #         raise RuntimeError('Cannot load instrument for prefix "%s"' % new_name)

