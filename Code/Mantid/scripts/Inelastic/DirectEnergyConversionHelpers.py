
def build_coupled_keys_dict(pInstrument,par_names,synonims) :
    """function to build the dictionary of the keys which are expressed through other keys values

       e.g. to substitute key1 = key2,key3  with key = [value[key1],value[key2]]
    """
    if pInstrument  is None:
            raise ValueError("Cannot initialize default parameter, instrument has not been loaded.")

    # dictionary used for substituting composite keys values.
    composite_keys_subst = dict();
    # set of keys which are composite keys
    composite_keys_set = set();


    for name in par_names :
            if pInstrument.getParameterType(name)=="string":
               val = self.get_default_parameter(name)
               if val is None :
                   continue
               val = val.strip()
               keys = val.split(":")
               n_keys = len(keys)
               if n_keys>1 : # this is the property we want
                   for i in xrange(0,len(keys)) :
                       key = keys[i];
                       if key in synonims:
                           key = synonims[key];

                       final_name = name
                       if final_name in synonims:
                           final_name = synonims[name];

                       composite_keys_subst[key] = (final_name,i,n_keys);
                       composite_keys_set.add(name)

    return composite_keys_subst,composite_keys_set



def build_subst_dictionary(synonims_list=None) :
    """Method to process the field "synonims_list" in the parameters string

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

