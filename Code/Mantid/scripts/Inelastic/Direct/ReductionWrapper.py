from mantid.simpleapi import *
from mantid import config
from mantid.kernel import funcreturns

from PropertyManager import PropertyManager
# this import is used by children
from Direct.DirectEnergyConversion import DirectEnergyConversion
#import inspect
import os
from abc import abstractmethod


class ReductionWrapper(object):
    """ Abstract class provides interface to direct inelastic reduction 
        allowing it to be run  from Mantid, web services, or system tests 
        using the same interface and the same run file placed in different 
        locations.
    """ 
    class var_holder(object):
        """ A simple wrapper class to keep web variables"""
        def __init__(self):
            self.standard_vars = None
            self.advanced_vars = None
            pass

    def __init__(self,instrumentName,web_var=None):
      """ sets properties defaults for the instrument with Name 
          and define if wrapper runs from web services or not
      """
      # internal variable, indicating if we should try to wait for input files to appear
      self._wait_for_file=False

      # The variables which are set up from web interface or to be exported to 
      # web interface
      if web_var: 
        self._run_from_web = True
        self._wvs = web_var
      else:
        self._run_from_web = False
        self._wvs = ReductionWrapper.var_holder()
      # Initialize reduced for given instrument
      self.reducer = DirectEnergyConversion(instrumentName)
#
    @property
    def wait_for_file(self):
        """ If this variable set to positive value, this value
            is interpreted as time to wait until check for specified run file 
            if this file have not been find immediately. 

            if this variable is 0 or false and the the file have not been found,
            reduction will fail
        """ 
        return self._wait_for_file
    @wait_for_file.setter
    def wait_for_file(self,value):
        if value>0:
            self._wait_for_file = value
        else:
            self._wait_for_file = False
#
    def save_web_variables(self,FileName=None):
        """ Method to write simple and advanced properties and help 
            information  into dictionary, to use by web reduction
            interface

            If no file is provided, reduce_var.py file will be written 
            to 

        """
        if not FileName:
            FileName = 'reduce_vars.py'
       
        f=open(FileName,'w')
        f.write("standard_vars = {\n")
        str_wrapper = '         '
        for key,val in self._wvs.standard_vars.iteritems():
                  if isinstance(val,str):
                      row = "{0}\'{1}\':\'{2}\'".format(str_wrapper,key,val)
                  else:
                      row = "{0}\'{1}\':{2}".format(str_wrapper,key,val)
                  f.write(row)
                  str_wrapper=',\n         '
        f.write("\n}\nadvanced_vars={\n")

        str_wrapper='         '
        for key,val in self._wvs.advanced_vars.iteritems():
                  if isinstance(val,str):
                      row = "{0}\'{1}\':\'{2}\'".format(str_wrapper,key,val)
                  else:
                      row = "{0}\'{1}\':{2}".format(str_wrapper,key,val)
                  f.write(row)
                  str_wrapper=',\n        '
        f.write("\n}\n")
        f.close()

#
    def get_validation_file_name(self):
        """ Define file used as sample to ensure reduction validity            
        """ 
        return None
#
    def validate_result(self,Error=1.e-3,ToleranceRelErr=True):
        """ Method validates results of the reduction against reference file provided
            by get_validation_file_name() method 
            
            At the moment, get_validation_file_name method should return the name of a file,
            where workspace sample reduced workspace with default properties 
            is stored. 
            CheckWorkspaceMatch method is applied to verify if current reduced workspace is 
            equivalent to the workspace, stored in the reference file. 
        """

        validation_file = self.get_validation_file_name()
        if not validation_file:
           return True,'No validation defined'
        
        sample = Load(validation_file)

        # just in case, to be sure
        current_web_state = self._run_from_web
        current_wait_state= self.wait_for_file
        # disable wait for input and 
        self._run_from_web = False
        self.wait_for_file = False
        #
        self.def_advanced_properties()
        self.def_main_properties()
        reduced = self.reduce()

        result = CheckWorkspaceMatch(Workspace1=sample,Workspace2=reduced,Tolerance=Error,CheckSampe=False,
                                     ChceckInstrument=False,ToleranceRelErr=ToleranceRelErr)

        self.wait_for_file = current_wait_state
        self._run_from_web = current_web_state 
        if result == 'Success!':
            return True,'Reference file and reduced workspace are equivalent'
        else:
            return False,result

    @abstractmethod
    def def_main_properties(self):
        """ Define properties which considered to be main properties changeable by user
            
            Should be overwritten by special reduction and decorated with  @MainProperties decorator. 

            Should return dictionary with key are the properties names and values -- the default 
            values these properties should have.
        """ 
        raise NotImplementedError('def_main_properties  has to be implemented')
    @abstractmethod
    def def_advanced_properties(self):
        """ Define properties which considered to be advanced but still changeable by instrument scientist or advanced user
            
            Should be overwritten by special reduction and decorated with  @AdvancedProperties decorator. 

            Should return dictionary with key are the properties names and values -- the default 
            values these properties should have.
        """ 

        raise NotImplementedError('def_advanced_properties  has to be implemented')


    def reduce(self,input_file=None,output_directory=None):
        """ The method performs all main reduction operations over 
            single run file
            
            Wrap it into @iliad wrapper to switch input for 
            reduction properties between script and web variables
        """ 
        if input_file:
           self.reducer.sample_run = input_file

        timeToWait = self._wait_for_file
        if timeToWait:
            file = PropertyManager.sample_run.find_file()
            while len(file)==0:
                file_hint,fext = PropertyManager.sample_run.file_hint()
                self.reduced.prop_man.log("*** Waiting {0}sec for file {1} to appear on the data search path"\
                    .format(timeToWait,file_hint),'notice')
                Pause(timeToWait)
                file = PropertyManager.sample_run.find_file()
            ws = self.reducer.convert_to_energy(None,input_file)

        else:
            ws = self.reducer.convert_to_energy(None,input_file)

        return ws




def MainProperties(main_prop_definition):
    """ Decorator stores properties dedicated as main and sets these properties
        as input to reduction parameters.""" 
    def main_prop_wrapper(*args):
        # execute decorated function
        prop_dict = main_prop_definition(*args)
        #print "in decorator: ",properties
        host = args[0]
        if not host._run_from_web: # property run locally
            host._wvs.standard_vars = prop_dict
            host.reducer.prop_man.set_input_parameters(**prop_dict)
        return prop_dict

    return main_prop_wrapper
#
def AdvancedProperties(adv_prop_definition):
    """ Decorator stores properties decided to be advanced and sets these properties
        as input for reduction parameters
    """ 
    def advanced_prop_wrapper(*args):
        prop_dict = adv_prop_definition(*args)
        #print "in decorator: ",properties
        host = args[0]
        if not host._run_from_web: # property run locally
            host._wvs.advanced_vars =prop_dict
            host.reducer.prop_man.set_input_parameters(**prop_dict)
        return prop_dict

    return advanced_prop_wrapper


def iliad(reduce):
    """ This decorator wraps around main procedure and switch input from 
        web variables to properties or vise versa depending on web variables
        presence
    """
    def iliad_wrapper(*args):
        #seq = inspect.stack()
        # output workspace name.
        try:
            n,r = funcreturns.lhs_info('both')
            out_ws_name = r[0]
        except:
            out_ws_name = None

        host = args[0]
        if len(args)>1:
            input_file = args[1]
            if len(args)>2:
                output_directory = args[2]
            else:
                output_directory =None
        else:
            input_file=None
            output_directory=None
        # add input file folder to data search directory if file has it
        if input_file:
           data_path = os.path.dirname(input_file)
           if len(data_path)>0:
              try:               
                 config.appendDataSearchDir(str(data_path))
              except: # if mantid is not available, this should ignore config
                 pass
        if output_directory:
           config['defaultsave.directory'] = output_directory

        if host._run_from_web:
            web_vars = dict(host._wvs.standard_vars.items()+host._wvs.advanced_vars.items())
            host.reducer.prop_man.set_input_parameters(**web_vars)
        else:
            pass # we should set already set up variables using 

 
        rez = reduce(*args)

        # prohibit returning workspace to web services. 
        if host._run_from_web and not isinstance(rez,str):
            rez=""
        else:
          if out_ws_name and rez.name() != out_ws_name :
              rez=RenameWorkspace(InputWorkspace=rez,OutputWorkspace=out_ws_name)
             
        return rez

    return iliad_wrapper
    

if __name__=="__main__":
    pass