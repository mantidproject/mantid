""" File contains classes defining the interface for Direct inelastic reduction with properties 
    present in Instrument_Properties.xml file
"""

from mantid.simpleapi import *
from mantid import api
from mantid import geometry
from mantid import config
import DirectReductionHelpers as prop_helpers
from DirectReductionProperties import DirectReductionProperties

import CommonFunctions as common
import os




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
    #if  Reducer.det_cal_file != None :
    #    if isinstance(Reducer.det_cal_file,str) and not Reducer.det_cal_file in mtd : # it is a file
    #        Reducer.log('Setting detector calibration file to '+Reducer.det_cal_file)
    #    else:
    #       Reducer.log('Setting detector calibration to {0}, which is probably a workspace '.format(str(Reducer.det_cal_file)))
    #else:
    #    Reducer.log('Setting detector calibration to detector block info from '+str(sample_run))

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
        else:
            if self._field_name=='hard_mask_file':
                if instance.use_hard_mask_only:
                    instance.run_diagnostics = False;
            
        prop_helpers.gen_setter(instance.__dict__,self._field_name,value);
#end MapMaskFile

class HardMaskOnly(object):
    def __get__(self,instance,type=None):
          return prop_helpers.gen_getter(instance.__dict__,'use_hard_mask_only');
    def __set__(self,instance,value):
        prop_helpers.gen_setter(instance.__dict__,'use_hard_mask_only',value);
        if not(value) and instance.hard_mask_file is None:
            instance.run_diagnostics = False;
#end HardMaskOnly

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
            if ei is None:
                raise AttributeError('Attempted to obtain relative to ei monovan integration range, but incident energy has not been set');
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
        elif isinstance(value,list) or isinstance(value,set):
            # set single default save format recursively
             for val in value:
                    self.__set__(instance,val);
             return;
        else:
            raise KeyError(' Attempting to set unknown saving format type. Allowed values can be spe, nxspe or nxs');
        if instance.__dict__['save_format'] is None:
            ts = set();
            ts.add(value)
            instance.__dict__['save_format'] = ts;
        else:
            instance.__dict__['save_format'].add(value);
#end SaveFormat

class DiagSpectra(object):
    """ class describes spectra list which should be used in diagnostics 

        consist of tuples list where each tuple are the numbers 
        indicating first-last spectra in the group.
        if None, all spectra are used in diagnostics

        TODO: not fully completed, a setter does not verify string and non-string values
    """
    def __get__(self,instance,type=None):
       sp_string = prop_helpers.gen_getter(instance.__dict__,'diag_spectra');
       if sp_string:
            return self.__process_spectra_list(sp_string);
       else:
            return None

    def __set__(self,instance,spectra_list):
            prop_helpers.gen_setter(instance.__dict__,'diag_spectra',spectra_list);

    def __process_spectra_list(self,specta_sring):
        """ process IDF description of the spectra string """
        if specta_sring.lower() in ['none','no']:
            return None
        else:
            banks = specta_sring.split(";")
            bank_spectra = []
            for b in banks:
                token = b.split(",")  # b = "(,)"
                if len(token) != 2:
                    raise ValueError("Invalid bank spectra specification in diagnostics properties %s" % specta_sring)
                start = int(token[0].lstrip('('))
                end = int(token[1].rstrip(')'))
                bank_spectra.append((start,end))
        return bank_spectra;

class BackbgroundTestRange(object):
    """ The TOF range used in diagnostics to reject high background spectra. 

        Usually it is the same range as the TOF range used to remove 
        background (in powders) though it may be set up separately.        
    """
    def __get__(self,instance,type=None):
       range = prop_helpers.gen_getter(instance.__dict__,'_background_test_range');
       if range  :
            return range 
       else:
            return instance.bkgd_range;

    def __set__(self,instance,value):
        if value is None:
           range = prop_helpers.gen_setter(instance.__dict__,'_background_test_range',None);
           return
        if isinstance(value,str):
            value = str.split(value,',')
        if len(value) != 2:
            raise ValueError("background test range can be set to a 2 element list of floats")

        range = prop_helpers.gen_setter(instance.__dict__,'_background_test_range',[float(value[0]),float(value[1])]);


#end DiagSpectra
#-----------------------------------------------------------------------------------------
# END Descriptors, Direct property manager itself
#-----------------------------------------------------------------------------------------

class DirectPropertyManager(DirectReductionProperties):
    """Class provides interface to all reduction properties, present in IDF

       These properties are responsible for fine turning up of the reduction

       Supported properties in IDF can be simple (prop[name]=value e.g. 
       prop['vanadium_mass']=30.5 
       
       or complex 
       where prop[name_complex_prop] value is equal [prop[name_1],prop[name_2]]
       e.g. time interval used in normalization on monitor 1:
       prop[norm_mon_integration_range] = [prop['norm-mon1-min'],prop['norm-mon1-max']]
       prop['norm-mon1-min']=1000,prop['norm-mon1-max']=2000

       properties which values described by more complex function have to have Descriptors. 
    """

    _class_wrapper ='_DirectPropertyManager__';
    def __init__(self,Instrument,instr_run=None):
        #
        DirectReductionProperties.__init__(self,Instrument,instr_run);
        #
        # define private properties served the class
        private_properties = {'descriptors':[],'subst_dict':{},'prop_allowed_values':{},'changed_properties':set(),
        'advanced_properties':set(),
        'file_properties':[],'abs_norm_file_properties':[]};
        # place these properties to __dict__  with proper decoration
        self._set_private_properties(private_properties);
        # ---------------------------------------------------------------------------------------------
        # overloaded descriptors. These properties have their personal descriptors, different from default
        all_methods = dir(self);
        self.__descriptors = prop_helpers.extract_non_system_names(all_methods,DirectPropertyManager._class_wrapper);

        # retrieve the dictionary of property-values described in IDF
        param_list = prop_helpers.get_default_idf_param_list(self.instrument);


        # build and use substitution dictionary
        if 'synonims' in param_list:
            synonyms_string  = param_list['synonims'];
            self.__subst_dict = prop_helpers.build_subst_dictionary(synonyms_string);
            # this dictionary will not be needed any more
            del param_list['synonims']
        #end


        # build and initiate  properties with default descriptors (TODO: consider complex automatic descriptors)
        param_list =  prop_helpers.build_properties_dict(param_list,self.__subst_dict)

        # modify some IDF properties, which need overloaded getter (and this getter is provided somewhere in this class)
        if 'background_test_range' in param_list:
            val = param_list['background_test_range']
            param_list['_background_test_range'] = val;
            del param_list['background_test_range']
        else:
            param_list['_background_test_range'] = None;

        self.__dict__.update(param_list)


        # file properties -- the properties described files which should exist for reduction to work.
        self.__file_properties = ['det_cal_file','map_file','hard_mask_file']
        self.__abs_norm_file_properties = ['monovan_mapfile']

        # properties with allowed values
        self.__prop_allowed_values['normalise_method']=[None,'monitor-1','monitor-2','current']  # 'uamph', peak -- disabled/unknown at the moment
        self.__prop_allowed_values['deltaE_mode']=['direct'] # I do not think we should try other modes here


        # properties with special(overloaded and non-standard) setters/getters: Properties name dictionary returning tuple(getter,setter)
        # if some is None, standard one is used. 
        #self.__special_properties['monovan_integr_range']=(self._get_monovan_integr_range,lambda val : self._set_monovan_integr_range(val));
        # list of the parameters which should always be taken from IDF unless explicitly set from elsewhere. These parameters MUST have setters and getters
  

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

        # replace common substitutions for None
        if type(val) is str :
           val = val.lower()
           if (val == 'none' or len(val) == 0):
              val = None;
           if val == 'default':
              val = self.getDefaultParameterValue(name0);

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

        # record changes in the property
        self.__changed_properties.add(name);
        if self.record_advanced_properties:
           self.__advanced_properties.add(name);

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
    #
    use_hard_mask_only = HardMaskOnly();
    #
    diag_spectra = DiagSpectra();
    #
    background_test_range = BackbgroundTestRange();

#----------------------------------------------------------------------------------------------------------------
    def getChangedProperties(self):
        """ method returns set of the properties changed from defaults """
        return self.__dict__[self._class_wrapper+'changed_properties'];
    def getChangedAdvancedProperties(self):
        """ method returns advanced properties, changed from defaults 
          and recorded when record_advanced_properties was set to True

          TODO: deal with this recording better. 
          """ 
        return self.__dict__[self._class_wrapper+'advanced_properties'];

    @property
    def relocate_dets(self) :
        if self.det_cal_file != None:
            return True
        else:
            return False

    def get_sample_ws_name(self):
        """ build and return sample workspace name """ 
        if not self.sum_runs:
            return common.create_resultname(self.sample_run,self.instr_name);
        else:
            return common.create_resultname(self.sample_run,self.instr_name,'-sum');
   
       

    def set_input_parameters(self,**kwargs):
        """ Set input properties from a dictionary of parameters

        """

        for par_name,value in kwargs.items() :
            setattr(self,par_name,value);

        return self.getChangedProperties();

    def get_diagnostics_parameters(self):
        """ Return the dictionary of the properties used in diagnostics with their values defined in IDF
            
            if some values are not in IDF, default values are used instead
        """ 

        diag_param_list ={'tiny':1e-10, 'huge':1e10, 'samp_zero':False, 'samp_lo':0.0, 'samp_hi':2,'samp_sig':3,\
                           'van_out_lo':0.01, 'van_out_hi':100., 'van_lo':0.1, 'van_hi':1.5, 'van_sig':0.0, 'variation':1.1,\
                           'bleed_test':False,'bleed_pixels':0,'bleed_maxrate':0,\
                           'hard_mask_file':None,'use_hard_mask_only':False,'background_test_range':None,\
                           'instr_name':''}
        result = {};

        for key,val in diag_param_list.iteritems():
            try:
                result[key] = getattr(self,key);
            except KeyError: 
                self.log('--- Diagnostics property {0} is not found in instrument properties. Default value: {1} is used instead \n'.format(key,value),'warning')
  
        return result;


    # TODO: finish refactoring this. 
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

          # should come from Mantid
        # Motor names-- SNS stuff -- psi used by nxspe file
        # These should be reconsidered on moving into _Parameters.xml
        self.motor = None
        self.motor_offset = None

        if self.__facility == 'SNS':
            self.normalise_method  = 'current'



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
    #
    def _check_monovan_par_changed(self):
        """ method verifies, if properties necessary for monovanadium reduction have indeed been changed  from defaults """

        # list of the parameters which should usually be changed by user and if not, user should be warn about it.
        momovan_properties=['sample_mass','sample_rmm','monovan_run']
        changed_prop = self.getChangedProperties();
        non_changed = [];
        for property in momovan_properties:
            if not property in changed_prop:
                non_changed.append(property)
        return non_changed;

    #
    def log_changed_values(self,log_level='notice'):
      """ inform user about changed parameters and about the parameters that should be changed but have not
      
        This method is abstract method of DirectReductionProperties but is fully defined in 
        DirectPropertyManager
      """

      # we may want to run absolute units normalization and this function has been called with monovan run or helper procedure
      if self.monovan_run != None :
         # check if mono-vanadium is provided as multiple files list or just put in brackets occasionally
          self.log("****************************************************************",'notice');
          self.log('*** Output will be in absolute units of mb/str/mev/fu','notice')
          non_changed = self._check_monovan_par_changed();
          if len(non_changed) > 0:
              for prop in non_changed:
                 value = getattr(self,prop)
                 message = '\n***WARNING!: Absolute units reduction parameter : {0} has its default value: {1}'+\
                           '\n             This may need to change for correct absolute units reduction\n'

                 self.log(message.format(prop,value),'warning')


      # now let's report on normal run.
      self.log("*** Provisional Incident energy: {0:>12.3f} mEv".format(self.incident_energy),log_level)
      self.log("****************************************************************",log_level);
      changed_Keys= self.getChangedProperties();
      for key in changed_Keys:
          val = getattr(self,key);
          self.log("  Value of : {0:<25} is set to : {1:<20} ".format(key,val),log_level)


      save_dir = config.getString('defaultsave.directory')
      self.log("****************************************************************",log_level);
      if self.monovan_run != None and not('van_mass' in changed_Keys):                           # This output is 
         self.log("*** Monochromatic vanadium mass used : {0} ".format(self.van_mass),log_level) # Adroja request from may 2014
      #
      self.log("*** By default results are saved into: {0}".format(save_dir),log_level);
      self.log("*** Output will be normalized to {0}".format(self.normalise_method),log_level);
      if  self.map_file == None:
            self.log('*** one2one map selected',log_level)
      self.log("****************************************************************",log_level);

    def export_changed_values(self,FileName='reduce_vars.py'):
        """ Method to write changed simple and advanced properties into dictionary, to process by 
            web reduction interface
        """
        changed_Keys= self.getChangedProperties();
        advancedKeys= self.getChangedAdvancedProperties();
       
        f=open(FileName,'w')
        f.write("standard_vars = {\n")
        str_wrapper = '         '
        for key in changed_Keys:
            if not key in advancedKeys:
                  val = getattr(self,key);
                  if isinstance(val,str):
                      row = "{0}\'{1}\':\'{2}\'".format(str_wrapper,key,val)
                  else:
                      row = "{0}\'{1}\':{2}".format(str_wrapper,key,val)
                  f.write(row);
                  str_wrapper=',\n         '
        f.write("}\nadvanced_vars={\n")

        str_wrapper='         '
        for key in advancedKeys:
                  val = getattr(self,key);
                  if isinstance(val,str):
                      row = "{0}\'{1}\':\'{2}\'".format(str_wrapper,key,val)
                  else:
                      row = "{0}\'{1}\':{2}".format(str_wrapper,key,val)
                  f.write(row);
                  str_wrapper=',\n        '
        f.write("}\n")
        f.close();





    #def help(self,keyword=None) :
    #    """function returns help on reduction parameters.

    #       if provided without arguments it returns the list of the parameters available
    #    """
    #    raise KeyError(' Help for this class is not yet implemented: see {0}_Parameter.xml in the file for the parameters description'.format());

if __name__=="__main__":
    pass


