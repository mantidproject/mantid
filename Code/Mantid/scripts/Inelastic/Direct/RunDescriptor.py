""" File contains Descriptors used describe run for direct inelastic reduction """ 

from mantid.simpleapi import *
from PropertiesDescriptors import *


class RunDescriptor(PropDescriptor):
    """ descriptor for property energy or range of incident energies to be processed """
    def __init__(self,DocString=None): 
        self._run_number  = None
        self._run_ws_name = None
        if not DocString is None:
            self.__doc__ = DocString

    def __get__(self,instance,owner=None):
        """ return current run number""" 
        if instance is None:
           return self
        return self._run_number 

    def __set__(self,instance,value):
       """ Set up Run number from any source """
       if value == None: # clear current run number
           self._run_number = None
           self._run_ws_name = None
           return

       if isinstance(value,str): # it may be run number as string or it may be a workspace name
          if value in mtd: # workspace
              self._run_ws_name = value
              ws = mtd[value]
              self._run_number = ws.getRunNumber()
          elif [',',':'] in value: # range of runs provided # TODO: parser
              raise NotImplementedError('Range of run numbers is not yet implemented')
          else:  #  filename or run number is provided
              self._run_number = value # TODO: parser      
       elif isinstance(value,list):
           self._run_number = value
       else:
           self._run_number = int(value)

    def get_workspace(self):
        """ Method returns workspace correspondent to current run number(s) """ 
        if self._run_ws_name:
           return mtd[self._run_ws_name]
        else:
           if self._run_number:
              raise  NotImplementedError('Load is not yet implemented')
           else:
              return None
             
#-------------------------------------------------------------------------------------------------------------------------------
class RunDescriptorDependent(RunDescriptor):
    def __init__(self,host_run,DocString=None):
        self._host = host_run
        self._this_run_defined=False
    def __get__(self,instance,owner=None):
       """ return dependent run number which is host run number if this one has not been set or this run number if it was""" 
       if instance is None:
           return self
       if self._this_run_defined:
          return self._run_number
       else:
          return self._host.__get__(instance,owner)

    def __set__(self,instance,value):
        if value is None:
            self._this_run_defined = False
            return
        self._this_run_defined = True
        super(RunDescriptorDependent,self).__set__(instance,value)
