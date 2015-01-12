""" File contains collection of Descriptors used to define complex properties in NonIDF_Properties and PropertyManager classes """ 

from mantid.simpleapi import *
from mantid import api
from mantid.simpleapi import *
from mantid import geometry
from mantid import config
from mantid.kernel import funcreturns

import ReductionHelpers as prop_helpers

import CommonFunctions as common
import os

#-----------------------------------------------------------------------------------------
# Descriptors, providing overloads for complex properties in NonIDF_Properties class
#-----------------------------------------------------------------------------------------
class IncidentEnergy(object):
    """ descriptor for incident energy or range of incident energies to be processed """
    def __init__(self): 
        self._incident_energy = None
        pass
    def __get__(self,instance,owner=None):
        """ return  incident energy or list of incident energies """ 
        if instance is None:
           return self

        return self._incident_energy 
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
                self._incident_energy=rez;
             else:
               self._incident_energy =float(value);
          else:
            if isinstance(value,list):
                rez = [];
                for val in value:
                    en_val = float(val);
                    if en_val<=0:
                        raise KeyError("Incident energy has to be positive, but is: {0} ".format(en_val));
                    else:
                        rez.append(en_val);
                self._incident_energy = rez
            else:
                self._incident_energy = float(value)
       else:
         raise KeyError("Incident energy have to be positive number of list of positive numbers. Got None")
       
       # 
       inc_en= self._incident_energy
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
    def __init__(self):
        self._energy_bins=None
    def __get__(self,instance,owner=None):
        """ binning range for the result of convertToenergy procedure or list of such ranges """
        if instance is None:
           return self
        return self._energy_bins

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
       else:
          value = None              
       #TODO: implement single value settings according to rebin
       self._energy_bins = value
#end EnergyBins
class SaveFileName(object):
    """ Property defines default file name to save result to

        See similar property get_sample_ws_name TODO: (leave only one)
    """
    def __init__(self,Name=None):
       self._file_name = Name
    def __get__(self,instance,owner=None):

        if instance is None:
           return self
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
    """ Generic property describing some aspects of instrument (e.g. name, short name etc), 
        which are undefined if no instrument is defined
    """
    def __init__(self,prop_name):
        self._prop_name = prop_name;
    def __get__(self,instance,owner=None):

         if instance is None:
           return self

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

class VanadiumRMM(object):
    """ define constant static rmm for vanadium """ 
    def __get__(self,instance,owner=None):
        """ return rmm for vanadium """

        return 50.9415;
    def __set__(self,instance,value):
        raise AttributeError("Can not change vanadium rmm");
#end VanadiumRMM
#-----------------------------------------------------------------------------------------
# END Descriptors for NonIDF_Properties class
#-----------------------------------------------------------------------------------------

#-----------------------------------------------------------------------------------------
# Descriptors, providing overloads for some complex properties in PropertyManager
#-----------------------------------------------------------------------------------------
class PropertyFromRange(object):
    """ Descriptor for property, which can have one value from a list of values """
    def __init__(self,availible_values,default_value):
        self._availible_values = availible_values
        self.__set__(None,default_value)

    def __get__(self,instance,owner):
        """ Return current value for the property with range of values. """
        if instance is None:
           return self
        return self._current_value

    def __set__(self,instance,val):
       """ set detector calibration file using various formats """ 
       if val in self._availible_values:
           self._current_value = val
       else:
           raise KeyError(' Property can not have value {0}'.format(val))


class DetCalFile(object):
    """ property describes various sources for the detector calibration file """
    def __init__(self):
        self._det_cal_file = None

    def __get__(self,instance,owner):
        if instance is None:
           return self

        return self._det_cal_file

    def __set__(self,instance,val):
       """ set detector calibration file using various formats """ 
       
       if val is None or isinstance(val,api.Workspace) or isinstance(val,str):
       # nothing provided or workspace provided or filename probably provided
          if str(val) in mtd:
                # workspace name provided
                val = mtd[str(val)]
          self._det_cal_file = val
          return
  

       if isinstance(val,int):
          file_name= common.find_file(val)
          self._det_cal_file = file_name
          return

       raise NameError('Detector calibration file name can be a workspace name present in Mantid or string describing an file name');
    #if  Reducer.det_cal_file != None :
    #    if isinstance(Reducer.det_cal_file,str) and not Reducer.det_cal_file in mtd : # it is a file
    #        Reducer.log('Setting detector calibration file to '+Reducer.det_cal_file)
    #    else:
    #       Reducer.log('Setting detector calibration to {0}, which is probably a workspace '.format(str(Reducer.det_cal_file)))
    #else:
    #    Reducer.log('Setting detector calibration to detector block info from '+str(sample_run))
#end DetCalFile

class MapMaskFile(object):
    """ common method to wrap around an auxiliary file name """
    def __init__(self,file_ext,doc_string=None):
        self._file_name=None;
        self._file_ext  =file_ext;
        if not(doc_string is None):
            self.__doc__ = doc_string;

    def __get__(self,instance,type=None):
        if instance is None:
           return self

        return self._file_name

    def __set__(self,instance,value):
        if value != None:
           fileName, fileExtension = os.path.splitext(value)
           if (not fileExtension):
               value=value+self._file_ext
        self._file_name=value;
  
#end MapMaskFile

class HardMaskPlus(prop_helpers.ComplexProperty):
    """ Legacy HardMaskPlus class which sets up hard_mask_file to file and use_hard_mask_only to True""" 
    def __init__(self):
        prop_helpers.ComplexProperty.__init__(self,['use_hard_mask_only','run_diagnostics'])
    def __get__(self,instance,type=None):
        if instance is None:
           return self

        return instance.hard_mask_file

    def __set__(self,instance,value):
        if value != None:
           fileName, fileExtension = os.path.splitext(value)
           if (not fileExtension):
               value=value+'.msk'
           instance.hard_mask_file = value
           prop_helpers.ComplexProperty.__set__(self,instance.__dict__,[False,True])
        else:
           prop_helpers.ComplexProperty.__set__(self,instance.__dict__,[True,False])
        try:
             del instance.__changed_properties['hardmaskOnly']
        except:
           pass
 




class HardMaskOnly(prop_helpers.ComplexProperty):
    """ Sets diagnostics algorithm to use hard mask file provided and to disable all other diagnostics routines

        It controls two options, where the first one is use_hard_mask_only=True/False, controls diagnostics algorithm
        and another one: hard_mask_file provides file for masking.         
    """
    def __init__(self):
        prop_helpers.ComplexProperty.__init__(self,['use_hard_mask_only','run_diagnostics'])

    def __get__(self,instance,type=None):
        if instance is None:
           return self

        return prop_helpers.gen_getter(instance.__dict__,'use_hard_mask_only');
    def __set__(self,instance,value):
        if value is None:
            use_hard_mask_only = False
            instance.hard_mask_file = None
            hard_mask_file = None
        elif isinstance(value,bool) or isinstance(value,int):
            use_hard_mask_only = bool(value)
            hard_mask_file= instance.hard_mask_file
        elif isinstance(value,str):
            if value.lower() in ['true','yes']:
                use_hard_mask_only = True
            elif value.lower() in ['false','no']:
                use_hard_mask_only = False
            else: # it is probably a hard mask file provided:
                instance.hard_mask_file = value             
                use_hard_mask_only = True
            hard_mask_file = instance.hard_mask_file
        #end

        # if no hard mask file is there and use_hard_mask_only is True, diagnostics should not run
        if use_hard_mask_only and hard_mask_file is None:
            run_diagnostics = False
        else:
            run_diagnostics = True
        prop_helpers.ComplexProperty.__set__(self,instance.__dict__,[use_hard_mask_only,run_diagnostics])
        try:
             del instance.__changed_properties['hardmaskPlus']
        except:
           pass
 

  
#end HardMaskOnly

class MonovanIntegrationRange(prop_helpers.ComplexProperty):
    """ integration range for monochromatic vanadium 

        Defined either directly or as the function of the incident energy(s)

        If list of incident energies is provided, map of ranges in the form 'ei'=range is returned 
    """
    def __init__(self,DepType=None):
        if DepType:
            self._rel_range=False
            prop_helpers.ComplexProperty.__init__(self,['monovan_lo_value','monovan_hi_value'])
        else:
            self._rel_range=True 
            prop_helpers.ComplexProperty.__init__(self,['monovan_lo_frac','monovan_hi_frac'])
        pass

    def __get__(self,instance,type=None):

        if instance is None:
           return self

        if isinstance(instance,dict):
                ei = 1
                tDict = instance
        else:
                ei = instance.incident_energy
                tDict = instance.__dict__

        if self._rel_range: # relative range
            if ei is None:
                raise AttributeError('Attempted to obtain relative to ei monovan integration range, but incident energy has not been set');
            rel_range = prop_helpers.ComplexProperty.__get__(self,tDict)
            if isinstance(ei,list):
                range = dict();
                for en in ei:
                    range[en] = [rel_range[0]*en,rel_range[1]*en]
            else:
                range = [rel_range[0]*ei,rel_range[1]*ei]
            return range
        else: # absolute range
            return prop_helpers.ComplexProperty.__get__(self,tDict)

    def __set__(self,instance,value):
        if isinstance(instance,dict):
                dDict = instance
        else:
                tDict = instance.__dict__
        if value is None:
            if (not self._rel_range):
                self._rel_range = True
                self._other_prop =['monovan_lo_frac','monovan_hi_frac']
        else:
            if self._rel_range:
               self._rel_range = False
               self._other_prop =['monovan_lo_value','monovan_hi_value']

            if isinstance(value,str):
                values = value.split(',')
                result = [];
                for val in values :
                    result.append(int(val));
                value = result;
            if len(value) != 2:
                raise KeyError("monovan_integr_range has to be list of two values, "\
                    "defining min/max values of integration range or None to use relative to incident energy limits")
            prop_helpers.ComplexProperty.__set__(self,tDict,value)
#end MonovanIntegrationRange


class SpectraToMonitorsList(object):
   """ property describes list of spectra, used as monitors to estimate incident energy
       in a direct scattering experiment. 

       Necessary when a detector working in event mode is used as monitor. Specifying this number would copy 
       correspondent spectra to monitor workspace and rebin it according to monitors binning

       Written to work with old IDF too, where this property is absent.
   """ 
   def __init__(self):
       self._spectra_to_monitors_list = None


   def __get__(self,instance,type=None):
       if instance is None:
           return self
       return self._spectra_to_monitors_list

   def __set__(self,instance,spectra_list):
        """ Sets copy spectra to monitors variable as a list of monitors using different forms of input """
        self._spectra_to_monitors_list = self._convert_to_list(spectra_list)

   def _convert_to_list(self,spectra_list):
       """ convert any spectra_list representation into a list """ 
       if spectra_list is None:
            return None

       if isinstance(spectra_list,str):
            if spectra_list.lower() is 'none':
                result = None;
            else:
                spectra = spectra_list.split(',')
                result = [];
                for spectum in spectra :
                    result.append(int(spectum))

       else:
            if isinstance(spectra_list,list):
                if len(spectra_list) == 0:
                    result=None;
                else:
                    result=[];
                    for i in range(0,len(spectra_list)):
                        result.append(int(spectra_list[i]))
            else:
                result =[int(spectra_list)];
       return result
#end SpectraToMonitorsList

class SaveFormat(object):
   # formats available for saving the data
   save_formats = ['spe','nxspe','nxs']
   def __init__(self):
       self._save_format = set()

   def __get__(self,instance,type=None):
        if instance is None:
           return self

        return self._save_format

   def __set__(self,instance,value):
        """ user can clear save formats by setting save_format=None or save_format = [] or save_format=''
            if empty string or empty list is provided as part of the list, all save_format-s set up earlier are cleared"""

        # clear format by using None 
        if value is None:
            self._save_format = set()
            return

        # check string
        if isinstance(value,str):
            value = value.strip('[]().')
            subformats = value.split(',')
            if len(subformats)>1:
                self.__set__(instance,subformats)
                return
            else:
                value = subformats[0]      

                if not(value  in SaveFormat.save_formats):
                    instance.log("Trying to set saving in unknown format: \""+str(value)+"\" No saving will occur for this format")
                    return
        else: 
            try:
                 # set single default save format recursively
                 for val in value:
                    self.__set__(instance,val)
                 return
            except:    
               raise KeyError(' Attempting to set unknown saving format {0} of type {1}. Allowed values can be spe, nxspe or nxs'\
                   .format(value,type(value)))
        #end if different types
        self._save_format.add(value)
#end SaveFormat

class DiagSpectra(object):
    """ class describes spectra list which should be used in diagnostics 

        consist of tuples list where each tuple are the numbers 
        indicating first-last spectra in the group.
        if None, all spectra are used in diagnostics

    """
    def __init__(self):
        self._diag_spectra = None

    def __get__(self,instance,type=None):
        if instance is None:
           return self

        return self._diag_spectra

    def __set__(self,instance,spectra_list):
        self._diag_spectra = self._process_spectra_list(spectra_list)

    def _process_spectra_list(self,specta_sring):
        """ process IDF description of the spectra string """
        if specta_sring is None:
            return None
        if isinstance(specta_sring,str):
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
            return bank_spectra
        else:
            raise ValueError("Spectra For diagnostics can be a string inthe form (num1,num2);(num3,num4) etc. or None")
#end class DiagSpectra

class BackbgroundTestRange(object):
    """ The TOF range used in diagnostics to reject high background spectra. 

        Usually it is the same range as the TOF range used to remove 
        background (usually in powders) though it may be set up separately.        
    """
    def __init__(self):
        self._background_test_range = None

    def __get__(self,instance,type=None):
       if instance is None:
           return self

       if self._background_test_range:
            return self._background_test_range  
       else:
            return instance.bkgd_range;

    def __set__(self,instance,value):
        if value is None:
           self._background_test_range  = None
           return
        if isinstance(value,str):
            value = str.split(value,',')
        if len(value) != 2:
            raise ValueError("background test range can be set to a 2 element list of floats")
        self._background_test_range = [float(value[0]),float(value[1])]

#end BackbgroundTestRange

#-----------------------------------------------------------------------------------------
# END Descriptors, property manager itself
#-----------------------------------------------------------------------------------------


