from mantid.simpleapi import *
from mantid import api
import DirectReductionHelpers as prop_helpers
import inspect
import os



class DirectReductionProperties(object):
    """ Class describes basic properties, used in reduction

        These properties describe basic set of properties, user should set up 
        for reduction to work with defaults
    """
    def __init__(self): 
        self._wb_for_monovan_run = None;
        self._monovanadium_run   = None;
        pass

    @property 
    def ei(self):
        """ incident energy or list of incident energies """ 
        return self._ei;

    @ei.setter
    def ei(self,value):
        pass

    @property 
    def energy_bins(self):
        """ binning range for the result of convertToenergy procedure or list of such ranges """
        pass

    @energy_bins.setter
    def energy_bins(self,values):
       if value != None:
          if isinstance(value,str):
             list = str.split(value,',');
             nBlocks = len(list);
             for i in xrange(0,nBlocks,3):
                value = [float(list[i]),float(list[i+1]),float(list[i+2])]
          else:
              nBlocks = len(value);
          if nBlocks%3 != 0:
               raise KeyError("Energy_bin value has to be either list of n-blocks of 3 number each or string representation of this list with numbers separated by commas")
       #TODO: implement single value settings according to rebin
       self._energy_bins= value

    @property
    def run_number(self):
        """ run number to process or list of the run numbers """
        return _run_number;

    @run_number.setter
    def run_number(self,value):
        """ sets a run number to process or list of run numbers """
        pass

    @property
    def wb_run_number(self):
        return _wb_run_number;
    @wb_run_number.setter
    def wb_run_number(self,value):
        pass;


    @property 
    def monovanadium_run(self): 
        """ run number for monochromatic vanadium used in normalization """
        return _monovanadium_run;

    @monovanadium_run.setter
    def monovanadium_run(self,value): 
        """ run number for monochromatic vanadium used in normalization """
        pass

    @property 
    def wb_for_monovan_run(self):
        """ white beam  run used for calculating monovanadium integrals. 
            If not explicitly set, white beam for processing run is used instead
        """
        if _wb_for_monovan_run:
            return self._wb_for_monovan_run;
        else:
            return self._white_beam_run;




class DirectPropertyManager(DirectReductionProperties):
    """Class provides interface to all reduction properties, present in IDF

       These properties are responsible for accurate turning of the reduction

    """

    _class_wrapper ='_DirectPropertyManager__';
    def __init__(self,pInstrument):


        private_properties = {'special_properties':{},
        'subst_dict':{},'prop_allowed_values':{},'changed_properties':set(),
        'instrument':None,'file_properties':[],'abs_norm_file_properties':[],
        'prop_allowed_values':{},'abs_units_par_to_change':[],'instr_par_located':[]};
        #
        self._set_private_properties(private_properties);

        self.__instrument = pInstrument;

        param_list = prop_helpers.get_default_idf_param_list(pInstrument);

        # build and use substitution dictionary
        if 'synonims' in param_list:
            synonyms_string  = param_list['synonims'];
            self.__subst_dict = prop_helpers.build_subst_dictionary(synonyms_string);
            # this dictionary will not be needed any more
            del param_list['synonims']
        #end

        param_list =  prop_helpers.build_properties_dict(param_list,self.__subst_dict)
        self.__dict__.update(param_list)

        # ---------------------------------------------------------------------------------------------
        # file properties
        self.__file_properties = ['det_cal_file','map_file','hard_mask_file']
        self.__abs_norm_file_properties = ['monovan_mapfile']

        # properties with allowed values
        self.__prop_allowed_values['normalise_method']=[None,'monitor-1','monitor-2','current']  # 'uamph', peak -- disabled/unknown at the moment
        self.__prop_allowed_values['save_format']=[None,'.spe','.nxspe','.nxs']
        self.__prop_allowed_values['deltaE_mode']=['direct'] # I do not think we should try other modes


        # properties with special(overloaded and non-standard) setters/getters: Properties name dictionary returning tuple(getter,setter)
        # if some is None, standard one is used. 
        # TODO: this should be implemented using descriptors?
        self.__special_properties['van_rmm']=(lambda: 50.9415,lambda val : self._raiseError('Can not change vanadium rmm')  );
        self.__special_properties['monovan_integr_range']=(self._get_monovan_integr_range,lambda val : self._set_monovan_integr_range(val));
        self.__special_properties['det_cal_file']=(None,lambda name: self._set_det_cal_file(name));
        self.__special_properties['map_file']=(None,lambda name: self._set_map_file(name));
        self.__special_properties['monovan_mapfile']=(None,lambda name: self._set_monovan_mapfile(name));
        self.__special_properties['hard_mask_file']=(None,lambda name: self._set_hard_mask_file(name));

        # list of the parameters which should usually be changed by user and if not, user should be warn about it.
        self.__abs_units_par_to_change=['sample_mass','sample_rmm']
        # list of the parameters which should always be taken from IDF unless explicitly set from elsewhere. These parameters MUST have setters and getters
        self.__instr_par_located =['ei_mon_spectra']
        # dictionary of the special setters & getters:

        # list of the properties which have been changed:

    def _set_private_properties(self,prop_dict):

        result = {};
        for key,val  in prop_dict.iteritems():
            new_key = self._class_wrapper+key;
            result[new_key]  = val;

        self.__dict__.update(result);

    @staticmethod
    def _is_private_property(prop_name):
        """ specify if property is private """
        if prop_name[:len(DirectPropertyManager._class_wrapper)] == DirectPropertyManager._class_wrapper :
            return True
        else:
            return False
        #end if



    def __setattr__(self,name,val):
        """ Overloaded generic set method, disallowing non-existing properties being set.
               
           It also provides common checks for generic properties groups """ 

        if self._is_private_property(name):
            self.__dict__[name] = val;
            return

        # process synonyms
        if name in self.__subst_dict:
            name = self.__subst_dict[name]
        #end

        # redefine generic substitutions for None
        if type(val) is str and (val is 'None' or val is 'none'):
            val = None;
        if type(val) is list and len(val) == 0:
            val = None;

        # check overloaded setters:
        if name in self.__special_properties:
            special_getter,special_setter = self.__special_properties[name];
            if special_setter:
               special_setter(val);
               self.__changed_properties.add(name);
               return;                
            else:
                pass
  

        # Check allowed values property if value allowed
        if name in self.__prop_allowed_values:
            allowed_values = self.__prop_with_allowed_values[name];
            if not(val in allowed_values):
                raise KeyError(" Property {0} can not have value: {1}".format(name,val));


        # set property value:
        prop_helpers.gen_setter(self.__dict__,name,val);
        # record parameter change
        self.__changed_properties.add(name);

   # ----------------------------
    def __getattr__(self,name):
       """ Overloaded get method, disallowing non-existing properties being get """ 

       tDict = object.__getattribute__(self,'__dict__');
       if name is '__dict__':
           return tDict;
       else:
           subst_dict = tDict[self._class_wrapper+'subst_dict'];
           if name in subst_dict:
                name = subst_dict[name]
           #end

           # check overloaded getters:
           overloads = tDict[self._class_wrapper+'special_properties'];
           if name in overloads:
                special_getter,special_setter = overloads[name];
                if special_getter:
                    return special_getter();
                else:
                    pass

           return prop_helpers.gen_getter(tDict,name)
       pass
#----------------------------------------------------------------------------------------------------------------
    def getChangedProperties(self):
        """ method returns set of the properties changed from defaults """
        return self.__dict__[self._class_wrapper+'changed_properties'];
    changed_properties = property(getChangedProperties);

#----------------------------------------------------------------------------------------------------------------
#   special overloads
#----------------------------------------------------------------------------------------------------------------
    @staticmethod
    def _raiseError(ErrorCode):
        raise KeyError(ErrorCode)

#----------------------------------------------------------------------------------
#              Overloaded setters/getters
#----------------------------------------------------------------------------------
    def _set_det_cal_file(self,val):

       if val is None:
          prop_helpers.gen_setter(self.__dict__,'det_cal_file',None);
          return;

       if isinstance(val,api.Workspace):
         # workspace provided
          prop_helpers.gen_setter(self.__dict__,'det_cal_file',val);
          return;

        # workspace name
       if str(val) in mtd:
          ws = mtd[str(val)];
          prop_helpers.gen_setter(self.__dict__,'det_cal_file',val);
          return;

       # file name probably provided
       if isinstance(val,str):
          prop_helpers.gen_setter(self.__dict__,'det_cal_file',val);
          return;


       if isinstance(val,int):
          fiel_name= common.find_file(val);
          prop_helpers.gen_setter(self.__dict__,'det_cal_file',fiel_name);
          return;

       raise NameError('Detector calibration file name can be a workspace name present in Mantid or string describing an file name');



    # integration range for monochromatic vanadium,
    def _get_monovan_integr_range(self):
        if self.monovan_integr_range is None:
            ei = self.incident_energy
            range = [self.monovan_lo_frac*ei,self.monovan_hi_frac*ei]
            return range
        else:
            return self.monovan_integr_range

    def _set_monovan_integr_range(self,value):
        prop_helpers.gen_setter(self.__dict__,'monovan_integr_range',value);



    # format to save data
    @property
    def save_format(self):
        return self._save_format
    @save_format.setter
    def save_format(self, value):
    # user can clear save formats by setting save_format=None or save_format = [] or save_format=''
    # if empty string or empty list is provided as part of the list, all save_format-s set up earlier are cleared

    # clear format by using None
        if value is None:
            self._save_format = None
            return
    # check string, if it is empty, clear save format, if not -- continue
        if isinstance(value,str):
            if value not in self.__save_formats :
                self.log("Trying to set saving in unknown format: \""+str(value)+"\" No saving will occur for this format")
                if len(value) == 0: # user wants to clear internal save formats
                   self._save_format = None
                return
        elif isinstance(value,list):
            if len(value) > 0 :
                # set single default save format recursively
                for val in value:
                    self.save_format = val
                return;
      # clear all previous formats by providing empty list
            else:
                self._save_format = None;
                return

        # here we came to setting list of save formats
        if self._save_format is None:
            self._save_format = [];

        self._save_format.append(value);

  
    def _set_map_file(self,value):
        if value != None:
           fileName, fileExtension = os.path.splitext(value)
           if (not fileExtension):
               value=value+'.map'
           prop_helpers.gen_setter(self.__dict__,'map_file',value);


    def _set_monovan_mapfile(self,value):
        """ set monovanadium map file name """
        if value != None:
           fileName, fileExtension = os.path.splitext(value)
           if (not fileExtension):
               value=value+'.map'
           prop_helpers.gen_setter(self.__dict__,'monovan_mapfile',value);



    def _set_hard_mask_file(self,value):
        if value != None:
           fileName, fileExtension = os.path.splitext(value)
           if (not fileExtension):
               value=value+'.msk'
           prop_helpers.gen_setter(self.__dict__,'hard_mask_file',value);


    @property
    def relocate_dets(self) :
        if self.det_cal_file != None:
            return True
        else:
            return False
    
        
    @property
    def spectra_to_monitors_list(self):
        """ property used when a """ 
        if not hasattr(self,'_spectra_to_monitors_list'):
           return None;
        return self._spectra_to_monitors_list;
    @spectra_to_monitors_list.setter
    def spectra_to_monitors_list(self,spectra_list):
        """ Sets copy spectra to monitors variable as a list of monitors using different forms of input

        """
        if spectra_list is None:
            self._spectra_to_monitors_list=None;
            return;

        if isinstance(spectra_list,str):
            if spectra_list is 'None':
                self._spectra_to_monitors_list=None;
            else:
                spectra = spectra_list.split(',');
                self._spectra_to_monitors_list = [];
                for spectum in spectra :
                    self._spectra_to_monitors_list.append(int(spectum));
        else:
            if isinstance(spectra_list,list):
                if len(spectra_list) == 0:
                    self._spectra_to_monitors_list=None;
                else:
                    self._spectra_to_monitors_list=[];
                    for i in range(0,len(spectra_list)):
                        self._spectra_to_monitors_list.append(int(spectra_list[i]));
            else:
                self._spectra_to_monitors_list =[int(spectra_list)];
        return;


    @property
    def background_test_range(self):
        if not hasattr(self,'_background_test_range') or self._background_test_range is None:
            return self.bkgd_range;
        else:
            return self._background_test_range
    @background_test_range.setter
    def background_test_range(self,value):
        if value is None:
            self._background_test_range=None
            return
        if isinstance(value,str):
            value = str.split(value,',')
        if len(value) != 2:
            raise ValueError("background test range can be set to a 2 element list of floats")

        self._background_test_range=[float(value[0]),float(value[1])]



    def _set_input_parameters(self,synonims,composite_keys_set,composite_keys_subst,**kwargs):
        """ Method analyzes input parameters list, substitutes the synonyms in this list with predefined synonyms
            and sets the existing class parameters with its non-default values taken from input

            returns the list of changed properties.
        """

        properties_changed=[];
        for par_name,value in kwargs.items() :

            if par_name in synonims :
                par_name = synonims[par_name]
            # may be a problem, one tries to set up non-existing value
            if not (hasattr(self,par_name) or hasattr(self,'_'+par_name)):
                # it still can be a composite key which sets parts of the composite property
                if par_name in self.composite_keys_subst :
                    composite_prop_name,index,nc = composite_keys_subst[par_name]
                    val = getattr(self,composite_prop_name)
                    val[index] = value;
                    setattr(self,composite_prop_name,val)
                    properties_changed.append(composite_prop_name)
                    continue
                else:
                    raise KeyError("Attempt to set unknown parameter: "+par_name)
            # whole composite key is modified by input parameters
            if par_name in composite_keys_set :
               default_value = getattr(self,par_name) # get default value
               if isinstance(value,str) and value.lower()[0:7] == 'default' : # Property changed but default value requested explicitly
                   value = default_value
               if type(default_value) != type(value):
                   raise KeyError("Attempt to change range property: "+par_name+" of type : "+str(type(default_value))+ " with wrong type value: "+str(type(value)))
               if len(default_value) != len(value) :
                    raise KeyError("Attempt to change range property : "+par_name+" with default value: ["+",".join(str(vv) for vv in val)+
                                   "] to wrong length value: ["+",".join(str(vv) for vv in value)+"]\n")
               else:
                   setattr(self,par_name,value)
                   properties_changed.append(par_name)
                   continue

            # simple case of setting simple value
            if isinstance(value,str) and value.lower()[0:7] == 'default' : # Property changed but default value requested explicitly
                value = getattr(self,par_name)


            setattr(self,par_name,value)
            properties_changed.append(par_name)

        # some properties have collective meaning
        if self.use_hard_mask_only and self.hard_mask_file is None:
          if self.run_diagnostics:
              self.run_diagnostics=False;
              properties_changed.append('run_diagnostics');

        return properties_changed


  

    def init_idf_params(self, reinitialize_parameters=False):
        """
        Initialize some default parameters and add the one from the IDF file
        """
        if self._idf_values_read == True and reinitialize_parameters == False:
            return

        """
        Attach analysis arguments that are particular to the ElasticConversion

        specify some parameters which may be not in IDF Parameters file
        """
       # mandatory command line parameter. Real value can not be negative
        self.incident_energy= -666
        # mandatory command line parameter
        self.energy_bins = None

          # should come from Mantid
        # Motor names-- SNS stuff -- psi used by nxspe file
        # These should be reconsidered on moving into _Parameters.xml
        self.monitor_workspace = None  # looks like unused parameter
        self.motor = None
        self.motor_offset = None
        self.psi = float('NaN')

        if self.__facility == 'SNS':
            self.normalise_method  = 'current'



        # special property -- synonyms -- how to treat external parameters.
        try:
            synonims = self.get_default_parameter("synonyms")
        except Exception:
            synonims=dict();

        par_names = self.pInstrument.getParameterNames()
        if len(par_names) == 0 :
            raise RuntimeError("Can not obtain instrument parameters describing inelastic conversion ")

        # build the dictionary of necessary allowed substitution names and substitute parameters with their values
        synonims = build_subst_dictionary(synonims)

        # build the dictionary which allows to process coupled property, namely the property, expressed through other properties values
        composite_keys_subst,composite_keys_set= build_coupled_keys_dict(self.pInstrument,par_names,synonims)

        # Add IDF parameters as properties to the reducer
        self.build_idf_parameters(par_names)

        if reinitialize_parameters:
            # clear Instrument Parameters defined fields:
            for name in self.__instr_par_located:
                setattr(self,name,None);


        # Mark IDF files as read
        self._idf_values_read = True

    def _check_file_exist(self,file_name):
        file_path = FileFinder.getFullPath(file);
        if len(file_path) == 0:
            try:
                file_path = common.find_file(file)
            except:
                file_path ='';


    def _check_necessary_files(self,monovan_run):
        """ Method verifies if all files necessary for a run are available.

           useful for long runs to check if all files necessary for it are present/accessible
        """

        def check_files_list(files_list):
            file_missing = False
            for prop in files_list :
                file = getattr(self,prop)
                if not (file is None) and isinstance(file,str):
                    file_path = self._check_file_exist(file);
                    if len(file_path)==0:
                       self.log(" Can not find file ""{0}"" for property: {1} ".format(file,prop),'error')
                       file_missing=True

            return file_missing

        base_file_missing = check_files_list(self.__file_properties)
        abs_file_missing=False
        if not (monovan_run is None):
            abs_file_missing = check_files_list(self.__abs_norm_file_properties)

        if  base_file_missing or abs_file_missing:
             raise RuntimeError(" Files needed for the run are missing ")




    def log(self, msg,level="notice"):
        """Send a log message to the location defined
        """
        log_options = \
        {"notice" :      lambda (msg):   logger.notice(msg),
         "warning" :     lambda (msg):   logger.warning(msg),
         "error" :       lambda (msg):   logger.error(msg),
         "information" : lambda (msg):   logger.information(msg),
         "debug" :       lambda (msg):   logger.debug(msg)}
        if self._to_stdout:
            print msg
        if self._log_to_mantid:
            log_options[level](msg)

    def help(self,keyword=None) :
        """function returns help on reduction parameters.

           if provided without arguments it returns the list of the parameters available
        """
        raise KeyError(' Help for this class is not yet implemented: see {0}_Parameter.xml in the file for the parameters description'.format());

if __name__=="__main__":
    pass


