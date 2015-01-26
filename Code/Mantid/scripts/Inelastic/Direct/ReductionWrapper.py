from mantid.simpleapi import *
from mantid import config

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
    def var_holder(object):
        """ A simple wrapper class to keep web variables"""
        def __init__(self):
            pass

    def __init__(self,instrumentName,web_var=None):
      """ sets properties defaults for the instrument with Name 
          and define if wrapper runs from web services or not
      """

       # The variables which are set up from web interface or to be exported to 
      # web interface
      if web_var: 
        self._run_from_web = True
        self._wvs = web_var
      else:
        self._run_from_web = False
        self._wvs = ReductionWrapper.var_holder
      # Initialize reduced for given instrument
      self._reducer = DirectEnergyConversion(instrumentName)

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
        for key,val in self._main_properties.iteritems():
                  if isinstance(val,str):
                      row = "{0}\'{1}\':\'{2}\'".format(str_wrapper,key,val)
                  else:
                      row = "{0}\'{1}\':{2}".format(str_wrapper,key,val)
                  f.write(row)
                  str_wrapper=',\n         '
        f.write("\n}\nadvanced_vars={\n")

        str_wrapper='         '
        for key,val in self._advanced_properties.iteritems():
                  if isinstance(val,str):
                      row = "{0}\'{1}\':\'{2}\'".format(str_wrapper,key,val)
                  else:
                      row = "{0}\'{1}\':{2}".format(str_wrapper,key,val)
                  f.write(row)
                  str_wrapper=',\n        '
        f.write("\n}\n")
        f.close()


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
    @abstractmethod
    def reduce(self,input_file=None,output_directory=None):
        """ The method which performs all main reduction operations. 

        """ 
        raise NotImplementedError('main routine has to be implemented')


def MainProperties(main_prop_definition):
    """ Decorator stores properties dedicated as main and sets these properties as input to reduction parameters.""" 
    def main_prop_wrapper(*args):
        # execute decorated function
        prop_dict = main_prop_definition(*args)
        #print "in decorator: ",properties
        host = args[0]
        if not host._run_from_web: # property run locally
            host._wvs.standard_vars = prop_dict
            host._reducer.prop_man.set_input_parameters(**prop_dict)
        return properties

    return main_prop_wrapper
#
def AdvancedProperties(adv_prop_definition):
    """ Decorator stores properties decided to be advanced and sets these properties as input for reduction parameters """ 
    def advanced_prop_wrapper(*args):
        prop_dict = adv_prop_definition(*args)
        #print "in decorator: ",properties
        host = args[0]
        if not host._run_from_web: # property run locally
            host._wvs.advanced_vars =prop_dict
            host._reducer.prop_man.set_input_parameters(**prop_dict)
        return properties

    return advanced_prop_wrapper


def iliad(main):
    """ This decorator wraps around main procedure and switch input from 
        web variables to properties or vise versa depending on web variables
        presence
    """
    def iliad_wrapper(*args):
        #seq = inspect.stack()

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
           try:
               config.appendDataSearchDir(str(output_directory))
           except: # if mantid is not available, this should ignore config
               pass

        if host._run_from_web:
            web_vars = dict(host._web_var.standard_vars.items()+host._web_var.standard_vars.items())
            host.iliad_prop.set_input_parameters(**web_vars)
            host.iliad_prop.sample_run = input_file

        rez = run_reducer(*args)
        # prohibit returning workspace to web services. 
        if host._run_from_web and not isinstance(rez,str):
            rez=""
        return rez

    return iliad_wrapper
    

if __name__=="__main__":
    pass