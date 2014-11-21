from mantid.simpleapi import *
from mantid import api
import DirectReductionHelpers as prop_helpers
import CommonFunctions as common
import os



class DirectReductionProperties(object):
    """ Class describes basic properties, used in reduction

        These properties are main set of properties, user have to set up 
        for reduction to work with defaults. 
    """

    # logging levels available for user
    log_options = \
        { "error" :       lambda (msg):   logger.error(msg),
          "warning" :     lambda (msg):   logger.warning(msg),
          "notice" :      lambda (msg):   logger.notice(msg),
          "information" : lambda (msg):   logger.information(msg),
          "debug" :       lambda (msg):   logger.debug(msg)}


    def __init__(self,InsturmentName,run_number=None,wb_run_number=None): 
        object.__setattr__(self,'_run_number',run_number);
        object.__setattr__(self,'_wb_run_number',wb_run_number);

        object.__setattr__(self,'_monovan_run_number',None);
        object.__setattr__(self,'_wb_for_monovan_run',None);

        object.__setattr__(self,'_incident_energy',None)

        object.__setattr__(self,'_energy_bins',None);

        #
        object.__setattr__(self,'_pInstrument',self._set_instrument(InsturmentName,run_number,wb_run_number));
        # Helper properties, defining logging options 
        object.__setattr__(self,'_log_level','notice');
        object.__setattr__(self,'_log_to_mantid',True);

    #end

    def getDefaultParameterValue(self,par_name):
        """ method to get default parameter value, specified in IDF """
        return prop_helpers.get_default_parameter(self.instrument,par_name);

    @property 
    def instrument(self):
        return self._pInstrument;

    @property 
    def incident_energy(self):
        """ incident energy or list of incident energies """ 
        return self._incident_energy;

    @incident_energy.setter
    def incident_energy(self,value):
       """ Set up incident energy in a range of various formats """
       if value != None:
          if isinstance(value,str):
             en_list = str.split(value,',');
             if len(en_list)>1:                 
                rez = list;
                for en_str in en_list:
                    val = float(en_str);
                    rez.append(val)
                self._incident_energy=rez;
             else:
               self._incident_energy=float(value);
          else:
            if isinstance(value,list):
                self._incident_energy = []
                for val in value:
                    en_val = float(val);
                    if en_val<=0:
                        raise KeyError("Incident energy has to be positive, got numberL {0} ".format(en_val));
                    else:
                     self._incident_energy.append(en_val);
            else:
               self._incident_energy =float(value);
       else:
         raise KeyError("Incident energy have to be positive number of list of positive numbers. Got None")       
       if isinstance(self._incident_energy,list):
           for en in self._incident_energy:
               if en<= 0:
                 raise KeyError("Incident energy have to be positive number of list of positive numbers."+
                           " For input argument {0} got negative value {1}".format(value,en))     
       else:
         if self._incident_energy<= 0:
            raise KeyError("Incident energy have to be positive number of list of positive numbers."+
                           " For value {0} got negative {1}".format(value,self._incident_energy))
    @property 
    def energy_bins(self):
        """ binning range for the result of convertToenergy procedure or list of such ranges """
        return self._energy_bins

    @energy_bins.setter
    def energy_bins(self,values):
       if values != None:
          if isinstance(values,str):
             list = str.split(values,',');
             nBlocks = len(list);
             for i in xrange(0,nBlocks,3):
                value = [float(list[i]),float(list[i+1]),float(list[i+2])]
          else:
              value = values;
              nBlocks = len(value);
          if nBlocks%3 != 0:
               raise KeyError("Energy_bin value has to be either list of n-blocks of 3 number each or string representation of this list with numbers separated by commas")
       #TODO: implement single value settings according to rebin
       self._energy_bins= value

    @property
    def run_number(self):
        """ run number to process or list of the run numbers """
        if self._run_number is None:
            raise KeyError("Run number has not been set")
        return _run_number;

    @run_number.setter
    def run_number(self,value):
        """ sets a run number to process or list of run numbers """
        self._run_number = value

    @property
    def wb_run_number(self):
        if self._wb_run_number is None:
            raise KeyError("White beam run number has not been set")
        return _wb_run_number;

    @wb_run_number.setter
    def wb_run_number(self,value):
        self._wb_run_number = value


    @property 
    def monovanadium_run(self): 
        """ run number for monochromatic vanadium used in normalization """
        if self._wb_run_number is None:
            raise KeyError("White beam run number has not been set")

        return _monovan_run;

    @monovanadium_run.setter
    def monovanadium_run(self,value): 
        """ run number for monochromatic vanadium used in normalization """
        self._monovan_run = value

    @property 
    def wb_for_monovan_run(self):
        """ white beam  run used for calculating monovanadium integrals. 
            If not explicitly set, white beam for processing run is used instead
        """
        if _wb_for_monovan_run:
            return self._wb_for_monovan_run;
        else:
            return self._white_beam_run;

    @wb_for_monovan_run.setter
    def wb_for_monovan_run(self,value): 
        """ run number for monochromatic vanadium used in normalization """
        if value == self._white_beam_run:
            self._wb_for_monovan_run = None;
        else:
            self._wb_for_monovan_run = value

    # 
    def _set_instrument(self,InstrumentName,run_number,wb_run_number):
        """ test method used to obtain default instrument for testing """
        idf_dir = config.getString('instrumentDefinition.directory')
        idf_file=api.ExperimentInfo.getInstrumentFilename(InstrumentName)
        tmp_ws_name = '__empty_' + InstrumentName
        if not mtd.doesExist(tmp_ws_name):
               LoadEmptyInstrument(Filename=idf_file,OutputWorkspace=tmp_ws_name)
        return mtd[tmp_ws_name].getInstrument();


    def log(self, msg,level="notice"):
        """Send a log message to the location defined
        """
        if self._log_to_mantid:
            DirectReductionProperties.log_options[level](msg)
        else:
            print msg




#-----------------------------------------------------------------------------------------
# Descriptors, providing overloads for some complex properties in DirectPropertyManager
#-----------------------------------------------------------------------------------------
class VanadiumRMM(object):
    """ define constant static rmm for vanadium """ 
    def __get__(self,instance,owner=None):
        """ return rmm for vanadium """
        return 50.9415;
    def __set__(self,instance,value):
        raise AttributeError(("Can not change vanadium rmm"));
#end VanadiumRMM
#
class DetCalFile(object):
    """ property describes various sources for the detector calibration file """
    def __set__(self,instance,owner):
          return prop_helpers.gen_getter(instance.__dict__,'det_cal_file');

    def __set__(self,instance,val):

       if isinstance(val,api.Workspace):
         # workspace provided
          prop_helpers.gen_setter(instance.__dict__,'det_cal_file',val);
          return;

        # workspace name
       if str(val) in mtd:
          ws = mtd[str(val)];
          prop_helpers.gen_setter(instance.__dict__,'det_cal_file',val);
          return;

       # file name probably provided
       if isinstance(val,str):
          prop_helpers.gen_setter(instance.__dict__,'det_cal_file',val);
          return;


       if isinstance(val,int):
          file_name= common.find_file(val);
          prop_helpers.gen_setter(instance.__dict__,'det_cal_file',file_name);
          return;

       raise NameError('Detector calibration file name can be a workspace name present in Mantid or string describing an file name');
#end DetCalFile
#
class MapMaskFile(object):
    """ common method to wrap around an auxiliary file name """
    def __init__(self,field_name,file_ext,doc_string=None):
        self._field_name=field_name;
        self._file_ext  =file_ext;
        if not(doc_string is None):
            self.__doc__ = doc_string;

    def __get__(self,instance,type=None):
          return prop_helpers.gen_getter(instance.__dict__,self._field_name);

    def __set__(self,instance,value):
        if value != None:
           fileName, fileExtension = os.path.splitext(value)
           if (not fileExtension):
               value=value+self._file_ext;
        prop_helpers.gen_setter(instance.__dict__,self._field_name,value);
#end MapMaskFile

class MonovanIntegrationRange(object):
    """ integration range for monochromatic vanadium 

        Defined either directly or as the function of the incident energy(s)

        If list of incident energies is provided, map of ranges in the form 'ei'=range is returned 
    """
    def __init__(self):
        pass

    def __get__(self,instance,type=None):
        def_range = prop_helpers.gen_getter(instance.__dict__,'monovan_integr_range');
        if def_range is None:
            ei = instance.incident_energy
            lo_frac = instance.monovan_lo_frac;
            hi_frac = instance.monovan_hi_frac;
            if isinstance(ei,list):
                range = dict();
                for en in ei:
                    range[en] = [lo_frac*en,hi_frac*en]
            else:
                range = [lo_frac*ei,hi_frac*ei]
            return range
        else:
            return def_range;

    def __set__(self,instance,value):
        if value != None:
            raise NotImplementedError('Changing monovan_integr_range is not yet implemented. Try to set up monovan_lo_frac and monovan_hi_frac, which would specify energy limits') 
        prop_helpers.gen_setter(instance.__dict__,'monovan_integr_range',value);
#end MonovanIntegrationRange


class SpectraToMonitorsList(object):
   """ property describes list of spectra, used as monitors to estimate incident energy
       in a direct scattering experiment. 

       Necessary when a detector working in event mode is used as monitor. Specifying this number would copy 
       correspondent spectra to monitor workspace and rebin it according to monitors binning

       Written to work with old IDF too, where this property is absent.
   """ 
   def __init__(self):
       pass

   def __get__(self,instance,type=None):
       try:
           return  prop_helpers.gen_getter(instance.__dict__,'spectra_to_monitors_list');
       except KeyError:
           return None
   def __set__(self,instance,spectra_list):
        """ Sets copy spectra to monitors variable as a list of monitors using different forms of input """
        if spectra_list is None:
            prop_helpers.gen_setter(instance.__dict__,'spectra_to_monitors_list',None);
            return;

        result = None;
        if isinstance(spectra_list,str):
            if spectra_list.lower() is 'none':
                result = None;
            else:
                spectra = spectra_list.split(',');
                result = [];
                for spectum in spectra :
                    result.append(int(spectum));

        else:
            if isinstance(spectra_list,list):
                if len(spectra_list) == 0:
                    result=None;
                else:
                    result=[];
                    for i in range(0,len(spectra_list)):
                        result.append(int(spectra_list[i]));
            else:
                result =[int(spectra_list)];

        prop_helpers.gen_setter(instance.__dict__,'spectra_to_monitors_list',result);
        return;
#end SpectraToMonitorsList
    # format to save data
class SaveFormat(object):
   # formats available for saving
   save_formats = ['spe','nxspe','nxs'];

   def __get__(self,instance,type=None):
        formats=  prop_helpers.gen_getter(instance.__dict__,'save_format');
        if formats is None:
            return set();
        else:
            return formats;

   def __set__(self,instance,value):
        """ user can clear save formats by setting save_format=None or save_format = [] or save_format=''
            if empty string or empty list is provided as part of the list, all save_format-s set up earlier are cleared"""

        # clear format by using None 
        if value is None:
            prop_helpers.gen_setter(instance.__dict__,'save_format',set());
            return

        # check string, if it is empty, clear save format, if not -- continue
        if isinstance(value,str):
            if value[:1] == '.':
                value = value[1:];

            if not(value in SaveFormat.save_formats):
                instance.log("Trying to set saving in unknown format: \""+str(value)+"\" No saving will occur for this format")
                return 
        elif isinstance(value,list):
            # set single default save format recursively
             for val in value:
                    self.__set__(instance,val);
             return;
        else:
            raise KeyError(' Attempting to set unknown saving format type. Allowed values can be spe,nxspe or nxs');
        if instance.__dict__['save_format'] is None:
            ts = set()
            ts.add(value);
            instance.__dict__['save_format'] = ts;
        else:
            instance.__dict__['save_format'].add(value);
#end SaveFormat


#-----------------------------------------------------------------------------------------
# END Descriptors, Direct property manager itself
#-----------------------------------------------------------------------------------------
class DirectPropertyManager(DirectReductionProperties):
    """Class provides interface to all reduction properties, present in IDF

       These properties are responsible for accurate turning up of the reduction
    """

    _class_wrapper ='_DirectPropertyManager__';
    def __init__(self,InstrumentName,run_number=None,wb_run_number=None):
        #
        DirectReductionProperties.__init__(self,InstrumentName,run_number,wb_run_number);
        #

        private_properties = {'descriptors':[],'subst_dict':{},'prop_allowed_values':{},'changed_properties':set(),
        'file_properties':[],'abs_norm_file_properties':[],
        'abs_units_par_to_change':[],'':[]};
        #
        self._set_private_properties(private_properties);
        # ---------------------------------------------------------------------------------------------
        # overloaded descriptors: Their generation should be automated
        all_methods = dir(self);
        self.__descriptors = prop_helpers.extract_non_system_names(all_methods);


        param_list = prop_helpers.get_default_idf_param_list(self.instrument);

        # build and use substitution dictionary
        if 'synonims' in param_list:
            synonyms_string  = param_list['synonims'];
            self.__subst_dict = prop_helpers.build_subst_dictionary(synonyms_string);
            # this dictionary will not be needed any more
            del param_list['synonims']
        #end


        param_list =  prop_helpers.build_properties_dict(param_list,self.__subst_dict)
        self.__dict__.update(param_list)


        # file properties
        self.__file_properties = ['det_cal_file','map_file','hard_mask_file']
        self.__abs_norm_file_properties = ['monovan_mapfile']

        # properties with allowed values
        self.__prop_allowed_values['normalise_method']=[None,'monitor-1','monitor-2','current']  # 'uamph', peak -- disabled/unknown at the moment
        self.__prop_allowed_values['deltaE_mode']=['direct'] # I do not think we should try other modes


        # properties with special(overloaded and non-standard) setters/getters: Properties name dictionary returning tuple(getter,setter)
        # if some is None, standard one is used. 
        #self.__special_properties['monovan_integr_range']=(self._get_monovan_integr_range,lambda val : self._set_monovan_integr_range(val));

        # list of the parameters which should usually be changed by user and if not, user should be warn about it.
        self.__abs_units_par_to_change=['sample_mass','sample_rmm']
        # list of the parameters which should always be taken from IDF unless explicitly set from elsewhere. These parameters MUST have setters and getters
        self.__idf_param_located =['ei_mon_spectra']
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


    def __setattr__(self,name0,val):
        """ Overloaded generic set method, disallowing non-existing properties being set.
               
           It also provides common checks for generic properties groups """ 

        if self._is_private_property(name0):
            self.__dict__[name0] = val;
            return

        if name0 in self.__subst_dict:
            name = self.__subst_dict[name0]
        else:
            name =name0;
        #end

        # redefine generic substitutions for None
        if type(val) is str and (val is 'None' or val is 'none' or len(val) == 0):
            val = None;
        if type(val) is list and len(val) == 0:
            val = None;
 

        # Check allowed values property if value allowed
        if name in self.__prop_allowed_values:
            allowed_values = self.__prop_allowed_values[name];
            if not(val in allowed_values):
                raise KeyError(" Property {0} can not have value: {1}".format(name,val));
        #end

        # set property value:
        if name in self.__descriptors:
            super(DirectPropertyManager,self).__setattr__(name,val)
        else:
            prop_helpers.gen_setter(self.__dict__,name,val);
        # record changes in parameter
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

           return prop_helpers.gen_getter(tDict,name)
       pass
#----------------------------------------------------------------------------------------------------------------
    def getChangedProperties(self):
        """ method returns set of the properties changed from defaults """
        return self.__dict__[self._class_wrapper+'changed_properties'];
    changed_properties = property(getChangedProperties);

#----------------------------------------------------------------------------------
#              Overloaded setters/getters
#----------------------------------------------------------------------------------
    #
    van_rmm = VanadiumRMM();
    #
    det_cal_file    = DetCalFile();
    #
    map_file        = MapMaskFile('map_file','.map',"Spectra to detector mapping file for the sample run");
    #
    monovan_mapfile = MapMaskFile('monovan_mapfile','.map',"Spectra to detector mapping file for the monovanadium integrals calculation");
    #
    hard_mask_file  = MapMaskFile('hard_mask_file','.msk',"Hard mask file");
    #
    monovan_integr_range     = MonovanIntegrationRange();
    #
    spectra_to_monitors_list = SpectraToMonitorsList();
    # 
    save_format = SaveFormat();
  



    @property
    def relocate_dets(self) :
        if self.det_cal_file != None:
            return True
        else:
            return False
    
        

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




    def help(self,keyword=None) :
        """function returns help on reduction parameters.

           if provided without arguments it returns the list of the parameters available
        """
        raise KeyError(' Help for this class is not yet implemented: see {0}_Parameter.xml in the file for the parameters description'.format());

if __name__=="__main__":
    pass


