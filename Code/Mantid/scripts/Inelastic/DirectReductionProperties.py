"""  File contains classes defining the interface for Direct inelastic reduction with properties 
     necessary for reduction but not present in Instrument_Properties.xml file

     Main example of such properties are run numbers, energy bins and incident energies. 
"""

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
                rez = [];
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
       inc_en=instance._incident_energy
       if isinstance(inc_en,list):
           for en in inc_en:
               if en<= 0:
                 raise KeyError("Incident energy have to be positive number of list of positive numbers."+
                           " For input argument {0} got negative value {1}".format(value,en))     
       else:
         if inc_en<= 0:
            raise KeyError("Incident energy have to be positive number of list of positive numbers."+
                           " For value {0} got negative {1}".format(value,inc_en))
# end IncidentEnergy
class EnergyBins(object):
    """ Property provides various energy bin possibilities """
    def __get__(self,instance,owner=None):
        """ binning range for the result of convertToenergy procedure or list of such ranges """
        return instance._energy_bins

    def __set__(self,instance,values):
       if values != None:
          if isinstance(values,str):
             lst = str.split(values,',');
             nBlocks = len(lst);
             for i in xrange(0,nBlocks,3):
                value = [float(lst[i]),float(lst[i+1]),float(lst[i+2])]
          else:
              value = values;
              nBlocks = len(value);
          if nBlocks%3 != 0:
               raise KeyError("Energy_bin value has to be either list of n-blocks of 3 number each or string representation of this list with numbers separated by commas")
       #TODO: implement single value settings according to rebin
       object.__setattr__(instance,'_energy_bins',value);
#end EnergyBins
class SaveFileName(object):
    """ Property defines default file name to save result to

        See similar property get_sample_ws_name TODO: (leave only one)
    """
    def __init__(self,Name=None):
       self._file_name = Name
    def __get__(self,instance,owner=None):
        if self._file_name:
            return self._file_name
        else:
            if instance.instr_name:
                name = instance.short_inst_name 
            else:
                name = '_EMPTY'
            try:
                sr = instance.sample_run
            except:
                sr = 0
            try:
                name +='{0:0<5}Ei{1:<4.2f}meV'.format(sr,instance.incident_energy)
                if instance.sum_runs:
                    name +='sum'
                if instance.monovan_run:
                    name +='_Abs'
            except:
                name = None
        return name

    def __set__(self,instance,value):
        self._file_name = value
#end SaveFileName


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
        { "error" :       (1,lambda (msg):   logger.error(msg)),
          "warning" :     (2,lambda (msg):   logger.warning(msg)),
          "notice" :      (3,lambda (msg):   logger.notice(msg)),
          "information" : (4,lambda (msg):   logger.information(msg)),
          "debug" :       (5,lambda (msg):   logger.debug(msg))}


    def __init__(self,Instrument,run_workspace=None): 
        """ initialize main properties, defined by the class
            @parameter Instrument  -- name or pointer to the instrument, 
                       deployed in reduction
        """
        #
        object.__setattr__(self,'_sample_run',run_workspace)
        object.__setattr__(self,'_wb_run',None)

        object.__setattr__(self,'_monovan_run',None)
        object.__setattr__(self,'_wb_for_monovan_run',None)
        object.__setattr__(self,'_mask_run',None)


        object.__setattr__(self,'_incident_energy',None)
        object.__setattr__(self,'_energy_bins',None)

        # Helper properties, defining logging options 
        object.__setattr__(self,'_log_level','notice')
        object.__setattr__(self,'_log_to_mantid',False)
        object.__setattr__(self,'_current_log_level',3)

        
        object.__setattr__(self,'_psi',float('NaN'))
        # SNS motor stuff which is difficult to test as I've never seen it
        object.__setattr__(self,'_motor_name',None)
        object.__setattr__(self,'_motor_offset',0)


        object.__setattr__(self,'_second_white',None)
        object.__setattr__(self,'_mono_correction_factor',None)

        object.__setattr__(self,'_save_file_name',None)
 
        self._set_instrument_and_facility(Instrument,run_workspace)
  
    #end
    def get_sample_ws_name(self):
        """ build and return sample workspace name 

            See similar property save_file_name TODO: (leave only one)
        """ 
        if not self.sum_runs:
            return common.create_resultname(self.sample_run,self.instr_name);
        else:
            return common.create_resultname(self.sample_run,self.instr_name,'-sum');

    def getDefaultParameterValue(self,par_name):
        """ method to get default parameter value, specified in IDF """
        return prop_helpers.get_default_parameter(self.instrument,par_name);
    #-----------------------------------------------------------------------------
    incident_energy = IncidentEnergy()
    #
    energy_bins     = EnergyBins()
    #
    save_file_name = SaveFileName()
    #
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
    #-----------------------------------------------------------------------------------
    #TODO: do something about it
    @property
    def print_diag_results(self):
        """ property-sink used in diagnostics """
        return True;
    @print_diag_results.setter
    def print_diag_results(self,value):
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

    # -----------------------------------------------------------------------------
    @property
    def log_to_mantid(self):
        """ Property specify if high level log should be printed to stdout or added to common Mantid log""" 
        return self._log_to_mantid

    @log_to_mantid.setter
    def log_to_mantid(self,val):
        object.__setattr__(self,'_log_to_mantid',bool(val))
    # -----------------------------------------------------------------------------
    #-----------------------------------------------------------------------------------
    @property 
    def psi(self):
        """ rotation angle (not available from IDF)"""
        return self._psi;
    @psi.setter 
    def psi(self,value):
        """set rotation angle (not available from IDF). This value will be saved into NXSpe file"""
        object.__setattr__(self,'_psi',value)
    # -----------------------------------------------------------------------------
    @property
    def motor_name(self):
        return self._motor_name
    @motor_name.setter
    def motor_name(self,val):
        object.__setattr__(self,'_motor_name',val)
    #
    @property
    def motor_offset(self):
        return self._motor_offset
    @motor_offset.setter
    def motor_offset(self,val):
        object.__setattr__(self,'_motor_offset',val)
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
                try: 
                    new_name,full_name,facility_ = prop_helpers.check_instrument_name(None,instr_name);
                except KeyError: # the instrument pointer is not found in any facility but we have it after all
                    new_name=instr_name
                    full_name=instr_name
                    facility_= 'TEST'
                #end


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
        lev,logger = DirectReductionProperties.log_options[level]
        if self._log_to_mantid:
            logger(msg)
        else:
        # TODO: reconcile this with Mantid. 
           if lev<=self._current_log_level:
              print msg




if __name__=="__main__":
    pass


