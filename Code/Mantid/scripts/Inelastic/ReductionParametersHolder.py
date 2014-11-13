from mantid.simpleapi import *
from mantid import api
import unittest
import inspect


class ReductionParametersHolder():
    def __init__(self,pInstrument):
        pass

    @property
    def instr_name(self):
        return self._instr_name

    @instr_name.setter
    def instr_name(self,new_name):

       if not hasattr(self,'instr_name') :
           self._instr_name=None

       if new_name is None:
           return

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
       self.__facility = str(config.getFacility())
       if new_name == self.instr_name:
           return

       self._instr_name = new_name
       config['default.instrument'] = full_name



       if not hasattr(self,'instument') or self.instrument.getName() != instr_name :
            # Load an empty instrument if one isn't already there
            idf_dir = config.getString('instrumentDefinition.directory')
            try:
                idf_file=api.ExperimentInfo.getInstrumentFilename(new_name)
                tmp_ws_name = '__empty_' + new_name
                if not mtd.doesExist(tmp_ws_name):
                    LoadEmptyInstrument(Filename=idf_file,OutputWorkspace=tmp_ws_name)
                self.instrument = mtd[tmp_ws_name].getInstrument()
            except:
                self.instrument = None
                self._instr_name = None
                raise RuntimeError('Cannot load instrument for prefix "%s"' % new_name)


       # Initialize other IDF parameters
       self.init_idf_params(True)

#----------------------------------------------------------------------------------
#              Complex setters/getters
#----------------------------------------------------------------------------------
    @property
    def load_monitors_with_workspace(self):
        if hasattr(self,'_load_monitors_with_workspace'):
            return self._load_monitors_with_workspace;
        else:
            return False;
    @load_monitors_with_workspace.setter
    def load_monitors_with_workspace(self,val):
        try:
            if val>0:
                do_load=True;
            else:
                do_load=False;
        except ValueError:
                do_load=False;

        setattr(self,'_load_monitors_with_workspace',do_load);

    @property
    def ei_mon_spectra(self):

        if hasattr(self,'_ei_mon1_spectra'):
            s1 = self._ei_mon1_spectra;
        else:
            s1 = self.get_default_parameter('ei-mon1-spec');
        if hasattr(self,'_ei_mon2_spectra'):
            s2 = self._ei_mon2_spectra;
        else:
            s2 = self.get_default_parameter('ei-mon2-spec');
        spectra = [s1,s2]
        return spectra;

    @ei_mon_spectra.setter
    def ei_mon_spectra(self,val):
        if val is None:
            if hasattr(self,'_ei_mon1_spectra'):
                delattr(self,'_ei_mon1_spectra')
            if hasattr(self,'_ei_mon2_spectra'):
                delattr(self,'_ei_mon2_spectra')
            return;
        if not isinstance(val,list) or len(val) != 2:
            raise SyntaxError(' should assign list of two integers numbers here')
        if not math.isnan(val[0]):
            setattr(self,'_ei_mon1_spectra',val[0])
        if not math.isnan(val[1]):
            setattr(self,'_ei_mon2_spectra',val[1])


    @property
    def det_cal_file(self):
        if hasattr(self,'_det_cal_file'):
            return self._det_cal_file
        return None;

    @det_cal_file.setter
    def det_cal_file(self,val):

       if val is None:
           self._det_cal_file = None;
           return;

       if isinstance(val,api.Workspace):
          # workspace provided
          self._det_cal_file = val;
          return;

        # workspace name
       if str(val) in mtd:
          self._det_cal_file = mtd[str(val)];
          return;

       # file name probably provided
       if isinstance(val,str):
          if val is 'None':
              val = None;
          self._det_cal_file = val;
          return;

       if isinstance(val,list) and len(val)==0:
           self._det_cal_file = None;
           return;

       if isinstance(val,int):
          self._det_cal_file = common.find_file(val);
          return;


       raise NameError('Detector calibration file name can be a workspace name present in Mantid or string describing file name');


    # Vanadium rmm
    @property
    def van_rmm(self):
      """The rmm of Vanadium is a constant, should not be instrument parameter. Atom not exposed to python :( """
      return 50.9415
    @van_rmm.deleter
    def van_rmm(self):
        pass

    @property
    def facility(self):
        return self.__facility

    # integration range for monochromatic vanadium,
    @property
    def monovan_integr_range(self):
        if not hasattr(self,'_monovan_integr_range') or self._monovan_integr_range is None:
            ei = self.incident_energy
            range = [self.monovan_lo_frac*ei,self.monovan_hi_frac*ei]
            return range
        else:
            return self._monovan_integr_range
    @monovan_integr_range.setter
    def monovan_integr_range(self,value):
        self._monovan_integr_range = value


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

    # bin ranges
    @property
    def energy_bins(self):
        return self._energy_bins;
    @energy_bins.setter
    def energy_bins(self,value):
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

    # map file name
    @property
    def map_file(self):
        return self._map_file;
    @map_file.setter
    def map_file(self,value):
        if value != None:
           fileName, fileExtension = os.path.splitext(value)
           if (not fileExtension):
               value=value+'.map'

        self._map_file = value

    # monovanadium map file name
    @property
    def monovan_mapfile(self):
        return self._monovan_mapfile;
    @monovan_mapfile.setter
    def monovan_mapfile(self,value):
        if value != None:
           fileName, fileExtension = os.path.splitext(value)
           if (not fileExtension):
               value=value+'.map'

        self._monovan_mapfile = value
    @property
    def relocate_dets(self) :
        if self.det_cal_file != None:
            return True
        else:
            return False
    @property
    def spectra_to_monitors_list(self):
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
    def normalise_method(self):
        return self._normalise_method
    @normalise_method.setter
    def normalise_method(self,value):
        if value is None:
            value = 'none'
        if not isinstance(value,str):
            self.log('Normalization method should be a string or None','error')

        value = value.lower()
        if value in self.__normalization_methods:
            self._normalise_method = value
        else:
            self.log('Attempt to set up unknown normalization method {0}'.format(value),'error')
            raise KeyError('Attempt to set up unknown normalization method {0}'.format(value))

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


    def build_idf_parameters(self,list_param_names) :
        """Method to process idf parameters, substitute duplicate values and
           add the attributes with the names, defined in IDF to the object

           also sets composite names through component values, e.g creating
           self.key1 = [value[key1],value[key2]] where key1 was defined as key1 = key2,key3

           @param Reducer object with defined coposite_keys dictionary and synonyms dictionary
           @param list of parameter names to transform
        """
        instr = self.instrument;
        if instr is None:
            raise ValueError("Cannot init default parameter, instrument has not been loaded.")

        new_comp_name_set = set();
        # process all default parameters and create property names from them
        for name in list_param_names :

            key_name = name;
            if key_name == 'synonims' : # this is special key we have already dealt with
                continue

            if key_name in self.__synonims: # replace name with its equivalent
                key_name = self.__synonims[name]
            # this key has to be always in IDF
            if key_name in self.__instr_par_located:
                continue;

            # redefine composite keys set through synonyms names for the future usage
            if name in self.composite_keys_set and not(name in self.__instr_par_located):
                new_comp_name_set.add(key_name)
                continue # composite names are created through their values

             # create or fill in additional values to the composite key
            if key_name in self.composite_keys_subst :
                composite_prop_name,index,nc = self.composite_keys_subst[key_name]
                if composite_prop_name in self.__instr_par_located:
                    continue;

                if not hasattr(self,composite_prop_name): # create new attribute value
                    val = [float('nan')]*nc;
                else:              # key is already created, get its value
                    val = getattr(self,composite_prop_name)
                    if val is None: # some composite property names were set to none. leave them this way.
                        continue

                val[index] = self.get_default_parameter(name)
                setattr(self,composite_prop_name, val);
            else :
                # just new ordinary key, assign the value to it
            #if hasattr(self,key_name):
            #    raise KeyError(" Duplicate key "+key_name+" found in IDF property file")
            #else:

                setattr(self,key_name, self.get_default_parameter(name));

        # reset the list of composite names defined using synonyms
        self.composite_keys_set=new_comp_name_set

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


    def _check_necessary_files(self,monovan_run):
        """ Method verifies if all files necessary for a run are available.

           useful for long runs to check if all files necessary for it are present/accessible
        """

        def check_files_list(files_list):
            file_missing = False
            for prop in files_list :
                file = getattr(self,prop)
                if not (file is None) and isinstance(file,str):
                    file_path = FileFinder.getFullPath(file)
                    if len(file_path) == 0:
                        # it still can be run number
                        try:
                            file_path = common.find_file(file)
                        except:
                            file_path=''

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
        if self.instrument is None:
             raise ValueError("instrument has not been defined, call setup(instrument_name) first.")

        if keyword==None :
            par_names = self.instrument.getParameterNames()
            n_params = len(par_names)
            print "****: ***************************************************************************** "
            print "****: There are ",n_params, " reduction parameters available to change, namely: "
            for i in xrange(0,n_params,4):
                print "****: ",
                for j in xrange(0,min(n_params-i,4)):
                    print "\t{0}".format(par_names[i+j]),
                print ""
                #print par_names[i],
                #print  type(self.instrument.getParameterType(par_names[i])),
                #print  self.instrument.getParameterType(par_names[i])
            print "****:"
            print "****: type help(parameter_name) to get help on a parameter with the name requested"
            print "****: ***************************************************************************** ";
        else:
            if self.instrument.hasParameter(keyword) :
                print "****: ***************************************************************************** ";
                print "****: IDF value for keyword: ",keyword," is: ",self.get_default_parameter(keyword)
                if keyword in self.__synonims :
                    fieldName = self.__synonims[keyword]
                    print "****: This keyword is known to reducer as: ",fieldName," and its value is: ",self.fieldName
                else:
                    print "****: Its current value in reducer is: ",getattr(self,keyword)

                print "****: help for "+keyword+" is not yet implemented, read "+self.instr_name+"_Parameters.xml\n"\
                      "****: in folder "+config.getString('instrumentDefinition.directory')+" for its description there"
                print "****: ***************************************************************************** ";
            else:
                raise ValueError('Instrument parameter file does not contain a definition for "%s". Cannot continue' % keyword)

if __name__=="__main__":
    pass

