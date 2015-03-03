#pylint: disable=invalid-name
from Direct.NonIDF_Properties import *

from collections import OrderedDict


class PropertyManager(NonIDF_Properties):
    """ Class defines the interface for Direct inelastic reduction with properties
        present in Instrument_Properties.xml file

        These properties are responsible for fine turning up of the reduction

        Supported properties in IDF can be simple (prop[name]=value e.g.
        prop['vanadium_mass']=30.5

        or complex
        where prop[name_complex_prop] value is equal [prop[name_1],prop[name_2]]
        e.g. time interval used in normalization on monitor 1:
        prop[norm_mon_integration_range] = [prop['norm-mon1-min'],prop['norm-mon1-max']]
        prop['norm-mon1-min']=1000,prop['norm-mon1-max']=2000

        There are properties which provide even more complex functionality. These defined using Descriptors.


        The class is written to provide the following functionality.

        1) Properties are initiated from Instrument_Properties.xml file as defaults.
        2) Attempt to access property, not present in this file throws.
        3) Attempt to create property not present in this file throws.
        4) A standard behavior is defined for the most of the properties (get/set appropriate value) when there is number of
           overloaded properties, which support more complex behavior using specially written Descriptors
        5) Changes to the properties are recorded and the list of changed properties is available on request

        ########
        design remarks:

        1) Simple properties from IDF are stored in class dictionary in the form __dict__[property name]=property value

        2) Complex properties from IDF are generated as instances of ReductionHelpers.ComplexProperty class and stored
          in class dictionary in the form __dict__[_property_name] = ReductionHelpers.ComplexProperty([dependent properties list])
          (note underscore in front of property name)
          __getattr__ and __setattr__ are overloaded to understand such calls. The property_name itself is naturally not placed into
          system dictionary.

        3) Descriptors with the name present in IDF do not store their values and names in __dict__
          (the name is removed during IDF parsing) but keep their information in the descriptor.
          This is not considered a problem as only one instance of property manager is expected. If this need to be changed,
          adding property values to the __dict__ as values of _property_name keys should be safe.

        4) __getattr__ (and __setattr__ ) method are overloaded to provide call to a descriptor before the search in the system dictionary.
           Custom __getattr__ naturally works only if Python does not find a property name in the __dict__ or __class__.__dict__ (and mro()),
           e.g. in case when an descriptor is called through one of its synonym name.

           A problem will occur if a key with name equal to descriptor name is also present in __dict__. This name would override descriptor.
           This is why any new descriptor should never place a key with its name in __dict__. Current design automatically remove IDF name
           from __dict__ if a descriptor with such name exist, so further development should support this behavior.

        5) In many places (descriptors, RunDescriptor itself), PropertyManager assumed to be a singleton, so most Descriptors are defined on a
           class level. If this changes, careful refactoring will be necessary


    Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

    This file is part of Mantid and is distributed under GPL
    """

    #-----------------------------------------------------------------------------------

    def __init__(self,Instrument,instr_run=None):
        #
        NonIDF_Properties.__init__(self,Instrument,instr_run)
        # Overloaded parameters, defined through properties rather then descriptors
        object.__setattr__(self,'_mask_run',None)

        # define private properties served the class (Accessible through __Property_name call
        private_properties = {'descriptors':[],'subst_dict':{},'changed_properties':set(),
                              'file_properties':[],'abs_norm_file_properties':[]}
        # place these properties to __dict__  with proper decoration
        self._init_private_properties(private_properties)
        #
        class_dec = '_'+type(self).__name__+'__'
        # ---------------------------------------------------------------------------------------------
        # overloaded descriptors. These properties have their personal descriptors, different from default.
        all_methods = dir(self)
        object.__setattr__(self,class_dec+'descriptors',prop_helpers.extract_non_system_names(all_methods))
        # ---------------------------------------------------------------------------------------------
        # retrieve the dictionary of property-values described in IDF
        param_list = prop_helpers.get_default_idf_param_list(self.instrument)
        param_dict,descr_dict = self._convert_params_to_properties(param_list,True,self.__descriptors)
        #
        self.__dict__.update(param_dict)

        # use existing descriptors setter to define IDF-defined descriptor's state
        for key,val in descr_dict.iteritems():
            object.__setattr__(self,key,val)


        # file properties -- the properties described files which should exist for reduction to work.
        object.__setattr__(self,class_dec+'file_properties',['det_cal_file','map_file','hard_mask_file'])
        object.__setattr__(self,class_dec+'abs_norm_file_properties',['monovan_mapfile'])

        # Clear caches on construction to make class more like usual class then singleton (and cashes are dangerous)
        PropertyManager.mono_correction_factor.set_cash_mono_run_number(None)


    def _convert_params_to_properties(self,param_list,detine_subst_dict=True,descr_list=[]):
        """ method processes parameters obtained from IDF and modifies the IDF properties
            to the form allowing them be assigned as python class properties.
        """
        subst_name = '_'+type(self).__name__+'__subst_dict'

        # build and use substitution dictionary
        if 'synonims' in param_list :
            synonyms_string  = param_list['synonims']
            if detine_subst_dict:
                object.__setattr__(self,subst_name,prop_helpers.build_subst_dictionary(synonyms_string))
            #end
            # this dictionary will not be needed any more
            del param_list['synonims']
        #end


        # build properties list and descriptors list with their initial values
        param_dict,descr_dict =  prop_helpers.build_properties_dict(param_list,self.__dict__[subst_name],descr_list)
        return (param_dict,descr_dict)

    def _init_private_properties(self,prop_dict):
        """ helper method used to define all private dictionaries at once
            during __init__ procedure
        """

        class_decor = '_'+type(self).__name__+'__'

        result = {}
        for key,val in prop_dict.iteritems():
            new_key = class_decor+key
            object.__setattr__(self,new_key,val)


    def __setattr__(self,name0,val):
        """ Overloaded generic set method, disallowing non-existing properties being set.

           It also provides common checks for generic properties groups
           and records all properties changed
        """

        # Let's set private properties directly. Normally, nobody should try
        # to set them through PropertyManager interface
        #if name0[:2]=='_P':
        #    self.__dict__[name0] = val
        #    return
        ##end
        if name0 in self.__subst_dict:
            name = self.__subst_dict[name0]
        else:
            name =name0
        #end

        # replace common substitutions for string value
        if type(val) is str :
           val1 = val.lower()
           if val1 == 'none' or len(val1) == 0:
              val = None
           if val1 == 'default':
              val = self.getDefaultParameterValue(name0)
           # boolean property?
           if val1 in ['true','yes']:
               val = True
           if val1 in ['false','no']:
               val = False


        if type(val) is list and len(val) == 0:
            val = None

        # set property value:
        if name in self.__descriptors:
           super(PropertyManager,self).__setattr__(name,val)
        else:
           other_prop=prop_helpers.gen_setter(self.__dict__,name,val)

        # record the fact that the property have changed
        self.__changed_properties.add(name)

   # ----------------------------
    def __getattr__(self,name):
       """ Overloaded get method, disallowing non-existing properties being get but allowing
          a property been called with  different names specified in substitution dictionary.
       """

       if name in self.__subst_dict:
          name = self.__subst_dict[name]
          return getattr(self,name)
       #end

       if name in self.__descriptors:
           # This can only happen only if descriptor is called through synonims dictionary
           # This to work, all descriptors should define getter to return self on Null instance.
           descr=getattr(PropertyManager,name)
           return descr.__get__(self,name)
       else:
           return prop_helpers.gen_getter(self.__dict__,name)
       ##end
#----------------------------------------------------------------------------------
#              Overloaded setters/getters
#----------------------------------------------------------------------------------
    #
    det_cal_file    = DetCalFile()
    #
    map_file        = MapMaskFile('map_file','.map',"Spectra to detector mapping file for the sample run")
    #
    monovan_mapfile = MapMaskFile('monovan_map_file','.map',"Spectra to detector mapping file for the monovanadium integrals calculation")
    #
    hard_mask_file  = MapMaskFile('hard_mask_file','.msk',"Hard mask file")
    #
    monovan_integr_range     = MonovanIntegrationRange()
    #
    spectra_to_monitors_list = SpectraToMonitorsList()
    #
    save_format = SaveFormat()
    #
    hardmaskOnly = HardMaskOnly()
    hardmaskPlus = HardMaskPlus()
    #
    diag_spectra = DiagSpectra()
    #
    mon2_norm_energy_range = mon2NormalizationEnergyRange()
    #
    background_test_range = BackbgroundTestRange()
    # Properties with allowed value
    normalise_method= PropertyFromRange([None,'monitor-1','monitor-2','current'],'current')
    deltaE_mode     = PropertyFromRange(['direct'],'direct') # other modes should not be considered here
    #
    multirep_tof_specta_list=MultirepTOFSpectraList()
    #
    mono_correction_factor = MonoCorrectionFactor(NonIDF_Properties.incident_energy,
                                                  NonIDF_Properties.monovan_run)
    # property responsible for summing runs
    sum_runs = SumRuns(NonIDF_Properties.sample_run)
#----------------------------------------------------------------------------------------------------------------
    def getChangedProperties(self):
        """ method returns set of the properties changed from defaults """
        decor_prop = '_'+type(self).__name__+'__changed_properties'
        return self.__dict__[decor_prop]
    #
    def setChangedProperties(self,value=set([])):
        """ Method to clear changed properties list"""
        if isinstance(value,set):
            decor_prop = '_'+type(self).__name__+'__changed_properties'
            self.__dict__[decor_prop] =value
        else:
            raise KeyError("Changed properties can be initialized by appropriate properties set only")
    #
    @property
    def relocate_dets(self) :
        if self.det_cal_file != None:
            return True
        else:
            return False
    #
    def set_input_parameters_ignore_nan(self,**kwargs):
        """ Like similar method set_input_parameters this one is used to
            set changed parameters from dictionary of parameters.

            Unlike set_input_parameters, this method does not set parameter,
            with value  equal to None. As such, this method is used as interface to
            set data from a function with a list of given parameters (*args vrt **kwargs),
            with some parameters missing.
        """   
  
        for par_name,value in kwargs.items() :
            if not value is None:
                setattr(self,par_name,value)
    #
    def set_input_parameters(self,**kwargs):
        """ Set input properties from a dictionary of parameters

        """
        for par_name,value in kwargs.items() :
            setattr(self,par_name,value)

        return self.getChangedProperties()
    #
    def get_used_monitors_list(self):
        """ Method returns list of monitors ID used during reduction """

        used_mon=set()
        for case in common.switch(self.normalise_method):
            if case('monitor-1'):
                used_mon.add(self.mon1_norm_spec)
                break
            if case('monitor-2'):
                used_mon.add(self.mon2_norm_spec)
                break
            if case(): # default, could also just omit condition or 'if True'
               pass

        used_mon.add(self.ei_mon1_spec)
        used_mon.add(self.ei_mon2_spec)

        return used_mon
    #
    def get_diagnostics_parameters(self):
        """ Return the dictionary of the properties used in diagnostics with their values defined in IDF

            if some values are not in IDF, default values are used instead
        """

        diag_param_list ={'tiny':1e-10, 'huge':1e10, 'samp_zero':False, 'samp_lo':0.0, 'samp_hi':2,'samp_sig':3,\
                           'van_out_lo':0.01, 'van_out_hi':100., 'van_lo':0.1, 'van_hi':1.5, 'van_sig':0.0, 'variation':1.1,\
                           'bleed_test':False,'bleed_pixels':0,'bleed_maxrate':0,\
                           'hard_mask_file':None,'use_hard_mask_only':False,'background_test_range':None,\
                           'instr_name':'','print_diag_results':True}
        result = {}

        for key,val in diag_param_list.iteritems():
            try:
                result[key] = getattr(self,key)
            except KeyError:
                self.log('--- Diagnostics property {0} is not found in instrument properties. Default value: {1} is used instead \n'.format(key,value),'warning')

        return result
    #
    def update_defaults_from_instrument(self,pInstrument,ignore_changes=False):
        """ Method used to update default parameters from the same instrument (with different parameters).

            Used if initial parameters correspond to instrument with one validity dates and
            current instrument has different validity dates and different default values for
            these dates.

            List of synonims is not modified and new properties are not added assuming that
            recent dictionary and properties are most comprehensive one

            ignore_changes==True when changes, caused by setting properties from instrument are not recorded
            ignore_changes==False -- getChangedProperties properties after applied this method would return set
                            of all properties changed when applying this method

        """
        if self.instr_name != pInstrument.getName():
            self.log("*** WARNING: Setting reduction properties of the instrument {0} from the instrument {1}.\n"
                     "*** This only works if both instruments have the same reduction properties!"\
                      .format(self.instr_name,pInstrument.getName()),'warning')

        # Retrieve the properties, changed from interface earlier
        old_changes_list  = self.getChangedProperties()
        # record all changes, present in the old changes list
        old_changes=OrderedDict()
        for prop_name in old_changes_list:
            old_changes[prop_name] = getattr(self,prop_name)


        param_list = prop_helpers.get_default_idf_param_list(pInstrument)
        # remove old changes which are not related to IDF (not to reapply it again)
        for prop_name in old_changes:
            if not prop_name in param_list:
               try:
                     dependencies = getattr(PropertyManager,prop_name).dependencies()
               except:
                     dependencies = []
               modified = False
               for name in dependencies:
                   if name in param_list:
                      modified = True
                      break
               if not modified:
                  del old_changes[prop_name]
        #end

        param_list,descr_dict =  self._convert_params_to_properties(param_list,False,self.__descriptors)
        # clear record about previous changes
        self.setChangedProperties(set())

        #sort parameters to have complex properties (with underscore _) first
        sorted_param =  OrderedDict(sorted(param_list.items(),key=lambda x : ord((x[0][0]).lower())))

        # Walk through descriptors list and set their values
        # Assignment to descriptors should accept the form, descriptor is written in IDF
        changed_descriptors = set()
        for key,val in descr_dict.iteritems():
            if not key in old_changes_list:
                try: # this is reliability check, and except ideally should never be hit. May occur if old IDF contains
                   # properties, not present in recent IDF.
                  cur_val = getattr(self,key)
                  setattr(self,key,val)
                  new_val = getattr(self,key)
                except:
                   self.log("property {0} have not been found in existing IDF. Ignoring this property"\
                       .format(key),'warning')
                   continue
                if isinstance(new_val,api.Workspace) and isinstance(cur_val,api.Workspace):
                # do simplified workspace comparison which is appropriate here
                  if new_val.name() == cur_val.name() and \
                     new_val.getNumberHistograms() == cur_val.getNumberHistograms() and \
                     new_val.getNEvents() == cur_val.getNEvents() and \
                     new_val.getAxis(0).getUnit().unitID() == cur_val.getAxis(0).getUnit().unitID():
                        new_val =1; cur_val = 1
                   #
                #end
                if new_val != cur_val:
                   changed_descriptors.add(key)
                # dependencies removed either properties are equal or not
                try:
                     dependencies = getattr(PropertyManager,key).dependencies()
                except:
                     dependencies = []

                for dep_name in dependencies:
                    if dep_name in sorted_param:
                       del sorted_param[dep_name]
            else: # remove property from old changes list not to reapply it again?
                pass
        #end loop
        # clear record about all changes and store only changed descriptors list
        self.setChangedProperties(changed_descriptors)

        # Walk through the complex properties first and then through simple properties
        for key,val in sorted_param.iteritems():
            if not key in old_changes_list:
                # complex properties change through their dependencies so we are setting them first
                if isinstance(val,prop_helpers.ComplexProperty):
                    public_name = key[1:]
                    prop_new_val = val.__get__(param_list)
                else:
                    # no complex properties left so we have simple key-value pairs
                    public_name = key
                    prop_new_val = val

                try: # this is reliability check, and except ideally should never be hit. May occur if old IDF contains
                    # properties, not present in recent IDF.
                     cur_val = getattr(self,public_name)
                except:
                    self.log("property {0} have not been found in existing IDF. Ignoring this property"\
                        .format(public_name),'warning')
                    continue

                if prop_new_val !=cur_val :
                   setattr(self,public_name,prop_new_val)
                # Dependencies removed either properties are equal or not
                try:
                     dependencies = val.dependencies()
                except:
                     dependencies =[]
                for dep_name in dependencies:
                    # delete dependent properties not to deal with them again
                    del sorted_param[dep_name]
        #end


        new_changes_list  = self.getChangedProperties()
        self.setChangedProperties(set())
        # set back all changes stored earlier and may be overwritten by new IDF
        # (this is just to be sure -- should not change anything as we do not set properties changed)
        for key,val in old_changes.iteritems():
            setattr(self,key,val)

        # Clear changed properties list (is this wise?, may be we want to know that some defaults changed?)
        if ignore_changes:
            self.setChangedProperties(old_changes_list)
            all_changes = old_changes
        else:
            new_changes_list=new_changes_list.union(old_changes_list)
            all_changes = old_changes_list.union(new_changes_list)
            self.setChangedProperties(all_changes)

        n=funcreturns.lhs_info('nreturns')
        if n>0:
            return all_changes
        else:
            return None
    #end
    def _get_properties_with_files(self):
        """ Method returns list of properties, which may have 
            files as their values
            
            it does not include sample run, as this one will be 
            treated separately.
        """ 

        run_files_prop=['wb_run','monovan_run','mask_run','wb_for_monovan_run','second_white']
        map_mask_prop =['det_cal_file','map_file','hard_mask_file']

        abs_units = not(self.monovan_run is None)
        files_to_check =[]
        # run files to check
        for prop_name in run_files_prop:
            theProp = getattr(PropertyManager,prop_name)
            if theProp.has_own_value():
               if theProp.is_existing_ws(): # it is loaded workspace 
                  continue   # we do not care if it has file or not
               val = theProp.__get__(self,PropertyManager)
               if not(val is None) :
                   files_to_check.append(prop_name)

        # other files to check:
        for prop_name in map_mask_prop:
            val = getattr(self,prop_name)
            if not(val is None or isinstance(val,api.Workspace)):
               files_to_check.append(prop_name)
        # Absolute units files (only one?)
        if abs_units:
           val = self.monovan_mapfile
           if not(val is None) :
              files_to_check.append('monovan_mapfile')
        #
        return files_to_check
    #
    def find_files_to_sum(self,num_files=None):
        """ method searches for run files in run list to sum and returns 
            list of runs with run-files missing or ok and empty list if all files 
            are there 

            if num_files is not None, find specified number of files out of total 
            file list to sum
        """ 
        # this returns only runs, left to sum with current sample_run sum settings
        runs,sum_ws,added      = PropertyManager.sample_run.get_runs_to_sum(None,num_files)
        if len(runs) == 0:
           return (True,[],[])

        ok,not_found_list,found_list = PropertyManager.sample_run.find_run_files(runs)     
        return (ok,not_found_list,found_list)
    #
    def _check_file_properties(self):
        """ Method verifies if all files necessary for a reduction are available.

            useful for long runs to check if all files necessary for it are 
            present/accessible before starting the run
        """
        file_prop_names = self._get_properties_with_files()
        file_errors={}
        for prop_name in file_prop_names:
            theProp = getattr(PropertyManager,prop_name)
            ok,file = theProp.find_file(be_quet=True)
            if not ok:
               file_errors[prop_name]=file

        if self.sum_runs :
           ok,missing,found=self.find_files_to_sum()
           if not ok and not self.cashe_sum_ws:
              file_errors['missing_runs_toSum']=str(missing)

        result = (len(file_errors)==0)
        return (result,file_errors)
    #
    def _check_ouptut_dir(self):
       """ check if default save directory is accessible for writing """ 
       targ_dir = config['defaultsave.directory']
       test_file = os.path.join(targ_dir,'test_file.txt')
       try:
           fp = open(test_file,'w')
           fp.close()
           os.remove(test_file)
           return (True,'')
       except:
           return (False,'Can not write to default save directory {0}.\n Reduction results can be lost'.format(targ_dir))
    #
    def validate_properties(self,fail_on_errors=True):
        """ Method validates if some properties values for 
            properties set up in the property manager are correct 
        """ 

        if self.mono_correction_factor: # disable check for monovan_run, as it is not used if mono_correction provided
           PropertyManager.monovan_run._in_cash = True  # as soon as monovan_run is set up (mono correction disabled) 
        # this will be dropped
        error_list={}
        error_level=0

        ok,fail_prop = self._check_file_properties()
        if not ok :
           for prop in fail_prop:
               mess = "*** ERROR  : properties : {0} -->{1}".format(prop,fail_prop[prop])
               if fail_on_errors:
                 self.log(mess,'warning')
               else:
                 error_list[prop]=mess
           error_level=2

        ok,mess= self._check_ouptut_dir()
        if not ok:
           mess = '*** WARNING: saving results: --> {1}'.format(mess)

           if fail_on_errors:
              self.log(mess,'warning')
           else:
               error_list['file_output']=mess
           error_level=max(1,error_level)

        # verify interconnected properties
        changed_prop = self.getChangedProperties()
        for prop in changed_prop:
            try:
                theProp =getattr(PropertyManager,prop)
            except: # not all changed properties are property manager properties
                continue # we are not validating them
            try:
               ok,sev,message = theProp.validate(self,PropertyManager)
               if not (ok):
                  error_level=max(sev,error_level)
                  if sev == 1:
                     base = '*** WARNING: properties : {0} --> {1}'
                  else:
                     base = '*** ERROR  : properties : {0} --> {1}'
                  mess =  base.format(prop,message)
                  if fail_on_errors:
                      self.log(mess,'warning')
                  else:
                      error_list[prop]=mess
            except: # its simple dictionary value, which do not have validator or 
               pass # other property without validator
        #end
        if error_level>1 and fail_on_errors:
           raise RuntimeError('*** Invalid properties found. Can not run convert_to energy') 
        if error_level>0:
           OK = False
        else:
           OK = True
        return (OK,error_level,error_list)
    #
    def _check_monovan_par_changed(self):
        """ method verifies, if properties necessary for monovanadium reduction have indeed been changed  from defaults """

        # list of the parameters which should usually be changed by user and if not, user should be warn about it.
        momovan_properties=['sample_mass','sample_rmm']
        changed_prop = self.getChangedProperties()
        non_changed = []
        for property in momovan_properties:
            if not property in changed_prop:
                non_changed.append(property)
        return non_changed

    #
    def log_changed_values(self,log_level='notice',display_header=True,already_changed=set()):
      """ inform user about changed parameters and about the parameters that should be changed but have not

        This method is abstract method of NonIDF_Properties but is fully defined in PropertyManager

        display_header==True prints nice additional information about run. If False, only
        list of changed properties displayed.
      """
      if display_header:
        # we may want to run absolute units normalization and this function has been called with monovan run or helper procedure
        if self.monovan_run != None :
            # check if mono-vanadium is provided as multiple files list or just put in brackets occasionally
            self.log("****************************************************************",'notice')
            self.log('*** Output will be in absolute units of mb/str/mev/fu','notice')
            non_changed = self._check_monovan_par_changed()
            if len(non_changed) > 0:
                for prop in non_changed:
                    value = getattr(self,prop)
                    message = "\n***WARNING!: Abs units norm. parameter : {0} not changed from default val: {1}"\
                              "\n             This may need to change for correct absolute units reduction\n"

                    self.log(message.format(prop,value),'warning')


          # now let's report on normal run.
        if PropertyManager.incident_energy.multirep_mode():
            ei = self.incident_energy
            mess = "*** Provisional Incident energies: {0:>8.3f}".format(ei[0])
            for en in ei[1:]:
              mess += "; {0:>8.3f}".format(en)
            mess+=" mEv"
            self.log(mess,log_level)
        else:
            self.log("*** Provisional Incident energy: {0:>12.3f} mEv".format(self.incident_energy),log_level)
      #end display_header

      self.log("****************************************************************",log_level)
      changed_Keys= self.getChangedProperties()
      for key in changed_Keys:
          if key in already_changed:
              continue
          val = getattr(self,key)
          self.log("  Value of : {0:<25} is set to : {1:<20} ".format(key,val),log_level)

      if not display_header:
          return

      save_dir = config.getString('defaultsave.directory')
      self.log("****************************************************************",log_level)
      if self.monovan_run != None and not 'van_mass' in changed_Keys:  # This output is Adroja request from may 2014
         self.log("*** Monochromatic vanadium mass used : {0} ".format(self.van_mass),log_level)
      #
      self.log("*** By default results are saved into: {0}".format(save_dir),log_level)
      self.log("*** Output will be normalized to {0}".format(self.normalise_method),log_level)
      if  self.map_file == None:
            self.log('*** one2one map selected',log_level)
      self.log("****************************************************************",log_level)


    #def help(self,keyword=None) :
    #    """function returns help on reduction parameters.

    #       if provided without arguments it returns the list of the parameters available
    #    """
    #    raise KeyError(' Help for this class is not yet implemented: see {0}_Parameter.xml in the file for the parameters description'.format())

if __name__=="__main__":
    pass


