""" File contains number of various classes defining interface for Direct inelastic reduction  """

from mantid.simpleapi import *
from mantid import api
from mantid import geometry
from mantid import config
import DirectReductionHelpers as prop_helpers
import CommonFunctions as common
import os


#-----------------------------------------------------------------------------------------
# Descriptors, providing overloads for some complex properties in DirectReductionProperties
#-----------------------------------------------------------------------------------------
class IncidentEnergy(object):
    """ descriptor for incident energy or range of incident energies to be processed """
    def __init__(self): 
        pass
    def __get__(self,instance,owner=None):
        """ return  incident energy or list of incident energies """ 
        return instance._incident_energy;
    def __set__(self,instance,value):
       """ Set up incident energy or range of energies in various formats """
       if value != None:
          if isinstance(value,str):
             en_list = str.split(value,',');
             if len(en_list)>1:                 
                rez = list;
                for en_str in en_list:
                    val = float(en_str);
                    rez.append(val)
                instance._incident_energy=rez;
             else:
               instance._incident_energy=float(value);
          else:
            if isinstance(value,list):
                rez = [];
                for val in value:
                    en_val = float(val);
                    if en_val<=0:
                        raise KeyError("Incident energy has to be positive, but is: {0} ".format(en_val));
                    else:
                        rez.append(en_val);

                object.__setattr__(instance,'_incident_energy',rez);
            else:
               object.__setattr__(instance,'_incident_energy',float(value));
       else:
         raise KeyError("Incident energy have to be positive number of list of positive numbers. Got None")
       
       # 
       if isinstance(instance._incident_energy,list):
           for en in self._incident_energy:
               if en<= 0:
                 raise KeyError("Incident energy have to be positive number of list of positive numbers."+
                           " For input argument {0} got negative value {1}".format(value,en))     
       else:
         if instance._incident_energy<= 0:
            raise KeyError("Incident energy have to be positive number of list of positive numbers."+
                           " For value {0} got negative {1}".format(value,instance._incident_energy))
# end IncidentEnergy
class EnergyBins(object):
    """ Property provides various energy bin possibilities """
    def __get__(self,instance,owner=None):
        """ binning range for the result of convertToenergy procedure or list of such ranges """
        return instance._energy_bins

    def __set__(self,instance,values):
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
       object.__setattr__(instance,'_energy_bins',value);

#end EnergyBins
#
class InstrumentDependentProp(object):
    def __init__(self,prop_name):
        self._prop_name = prop_name;
    def __get__(self,instance,owner=None):
         if instance._pInstrument is None:
            raise KeyError("Attempt to use uninitialized property manager");
         else:
            return getattr(instance,self._prop_name);
    def __set__(self,instance,values):
        raise AttributeError("Property {0} can not be assigned".format(self._prop_name))
#end InstrumentDependentProp

def check_ei_bin_consistent(ei,binning_range):
    """ function verifies if the energy binning is consistent with incident energies """ 
    if isinstance(ei,list):
        for en in ei:
            range = binning_range[en]
            if range[2]>en:
                return (False,'Max rebin range {0:f} exceeds incident energy {1:f}'.format(range[2],en))
    else:
        if binning_range[2]>ei:
            return (False,'Max rebin range {0:f} exceeds incident energy {1:f}'.format(binning_range[2],ei))

    return (True,'')

#-----------------------------------------------------------------------------------------
# END Descriptors for DirectReductionProperties
#-----------------------------------------------------------------------------------------

class DirectReductionProperties(object):
    """ Class contains main properties, used in reduction, and not described in 
        IDF

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


    def __init__(self,Instrument,run_workspace=None): 
        """ initialize main properties, defined by the class
            @parameter Instrument  -- name or pointer to the instrument, 
                       deployed in reduction
        """
        #
        object.__setattr__(self,'_sample_run',run_workspace);
        object.__setattr__(self,'_wb_run',None);

        object.__setattr__(self,'_monovan_run',None);
        object.__setattr__(self,'_wb_for_monovan_run',None);
        object.__setattr__(self,'_mask_run',None);


        object.__setattr__(self,'_incident_energy',None);
        object.__setattr__(self,'_energy_bins',None);

        # Helper properties, defining logging options 
        object.__setattr__(self,'_log_level','notice');
        object.__setattr__(self,'_log_to_mantid',True);

        object.__setattr__(self,'_psi',float('NaN'));
        object.__setattr__(self,'_second_white',None);
        object.__setattr__(self,'_mono_correction_factor',None);


        self._set_instrument_and_facility(Instrument,run_workspace);
  
    #end

    def getDefaultParameterValue(self,par_name):
        """ method to get default parameter value, specified in IDF """
        return prop_helpers.get_default_parameter(self.instrument,par_name);
    #-----------------------------------------------------------------------------
    incident_energy = IncidentEnergy();
    #
    energy_bins     = EnergyBins();

    instr_name      = InstrumentDependentProp('_instr_name')
    short_inst_name = InstrumentDependentProp('_short_instr_name')
    facility        = InstrumentDependentProp('_facility')
    #-----------------------------------------------------------------------------------            
    @property
    def instrument(self):
        if self._pInstrument is None:
            raise KeyError("Attempt to use uninitialized property manager");
        else: 
            return self._pInstrument;
    #
    #-----------------------------------------------------------------------------------
    # TODO: do something about it. Second white is explicitly used in diagnostics. 
    @property 
    def seclond_white(self):
        """ Second white beam currently unused in the  workflow """
        return self._second_white;
    @seclond_white.setter 
    def seclond_white(self,value):
        """ Second white beam currently unused in the  workflow """
        pass
        #return self._second_white;
    #TODO:
    #-----------------------------------------------------------------------------------
    @property 
    def apply_detector_eff(self):
        return True;
    #-----------------------------------------------------------------------------------
    @property 
    def psi(self):
        """ rotation angle (not available from IDF)"""
        return self._psi;

    @psi.setter 
    def psi(self,value):
        """set rotation angle (not available from IDF). This value will be saved into NXSpe file"""
        object.__setattr__(self,'_psi',value)

    #-----------------------------------------------------------------------------------
    #TODO: do something about it
    @property
    def print_results(self):
        """ property-sink used in diagnostics """
        return True;
    @print_results.setter
    def print_results(self,value):
        pass
    #-----------------------------------------------------------------------------------
    @property
    def mono_correction_factor(self):
        """ pre-calculated absolute units correction factor"""
        if self._mono_correction_factor:
            return self._mono_correction_factor;
        else:
            return None;
    @mono_correction_factor.setter
    def mono_correction_factor(self,value):
        object.__setattr__(self,'_mono_correction_factor',value)
    #-----------------------------------------------------------------------------------
    @property
    #-----------------------------------------------------------------------------------
    def sample_run(self):
        """ run number to process or list of the run numbers """
        if self._sample_run is None:
            raise KeyError("Sample run has not been defined")
        return self._sample_run;

    @sample_run.setter
    def sample_run(self,value):
        """ sets a run number to process or list of run numbers """
        object.__setattr__(self,'_sample_run',value)
    #-----------------------------------------------------------------------------------
    @property
    def wb_run(self):
        if self._wb_run is None:
            raise KeyError("White beam run has not been defined")
        return self._wb_run;
    @wb_run.setter
    def wb_run(self,value):
        object.__setattr__(self,'_wb_run',value)

    #-----------------------------------------------------------------------------------
    @property 
    def monovan_run(self): 
        """ run ID (number or workspace) for monochromatic vanadium used in absolute units normalization """
        return self._monovan_run;

    @monovan_run.setter
    def monovan_run(self,value): 
        """ run ID (number or workspace) for monochromatic vanadium used in normalization """
        object.__setattr__(self,'_monovan_run',value)
    #-----------------------------------------------------------------------------------
    @property 
    def wb_for_monovan_run(self):
        """ white beam  run used for calculating monovanadium integrals. 
            If not explicitly set, white beam for processing run is used instead
        """
        if self._wb_for_monovan_run:
            return self._wb_for_monovan_run;
        else:
            return self._wb_run;

    @wb_for_monovan_run.setter
    def wb_for_monovan_run(self,value): 
        """ run number for monochromatic vanadium used in normalization """
        if value == self._wb_run:
            object.__setattr__(self,'_wb_for_monovan_run',None)
        else:
            object.__setattr__(self,'_wb_for_monovan_run',value)

    #-----------------------------------------------------------------------------------
    @property
    def mask_run(self):
        """ run used to get masks to remove unreliable spectra

           Usually it is sample run but separate run may be used 
        """
        if self._mask_run:
            return self._mask_run
        else:
            return self._sample_run
    @mask_run.setter
    def mask_run(self,value):
       object.__setattr__(self,'_mask_run',value)
    #-----------------------------------------------------------------------------------
    @property 
    def apply_kikf_correction(self):
        """ Parameter specifies if ki/kf correction should be applied to the reduction result"""
        if not hasattr(self,'_apply_kikf_correction'):
            return True;
        else:
            return self._apply_kikf_correction;

    @apply_kikf_correction.setter 
    def apply_kikf_correction(self,value):
        """ Set up option if ki/kf correction should be applied to the reduction result (default -- true) """
        if isinstance(value,str):
            val = value.lower() in ['true','t','yes','y','1']
        else:
            val = bool(value)
        object.__setattr__(self,'_apply_kikf_correction',val);

    # -----------------------------------------------------------------------------
    # Service properties (used by class itself)
    #

    def _set_instrument_and_facility(self,Instrument,run_workspace=None):
        """ simple method used to obtain default instrument for testing """
        # TODO: implement advanced instrument setter, used in DirectEnergy conversion

        if run_workspace:
            instrument=run_workspace.getInstrument();
            instr_name = instrument.getFullName();
            new_name,full_name,facility_ = prop_helpers.check_instrument_name(None,instr_name);
        else:
            if isinstance(Instrument,geometry._geometry.Instrument):
                instrument = Instrument;
                instr_name = instrument.getFullName()
                new_name,full_name,facility_ = prop_helpers.check_instrument_name(None,instr_name);
            elif isinstance(Instrument,str): # instrument name defined
                new_name,full_name,facility_ = prop_helpers.check_instrument_name(None,Instrument);
                idf_dir = config.getString('instrumentDefinition.directory')
                idf_file=api.ExperimentInfo.getInstrumentFilename(full_name)
                tmp_ws_name = '__empty_' + full_name
                if not mtd.doesExist(tmp_ws_name):
                   LoadEmptyInstrument(Filename=idf_file,OutputWorkspace=tmp_ws_name)
                instrument = mtd[tmp_ws_name].getInstrument();
            else:
                raise TypeError(' neither correct instrument name nor instrument pointer provided as instrument parameter')
        #end if
        object.__setattr__(self,'_pInstrument',instrument);
        object.__setattr__(self,'_instr_name',full_name);
        object.__setattr__(self,'_facility',facility_);
        object.__setattr__(self,'_short_instr_name',new_name);



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
        elif isinstance(value,list):
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

            Auxiliary method used in old interface.
            Returns the list of changed properties.
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

    def check_monovan_par_changed(self):
        """ method verifies, if properties necessary for monovanadium reduction have indeed been changed  from defaults """

        # list of the parameters which should usually be changed by user and if not, user should be warn about it.
        momovan_properties=['sample_mass','sample_rmm','monovan_run']
        changed_prop = self.getChangedProperties();
        non_changed = [];
        for property in momovan_properties:
            if not property in changed_prop:
                non_changed.append(property)
        return non_changed;

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




    #def help(self,keyword=None) :
    #    """function returns help on reduction parameters.

    #       if provided without arguments it returns the list of the parameters available
    #    """
    #    raise KeyError(' Help for this class is not yet implemented: see {0}_Parameter.xml in the file for the parameters description'.format());

if __name__=="__main__":
    pass


