from mantid.simpleapi import *
from mantid import config

from PropertyManager import PropertyManager;
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
    def __init__(self,instrumentName,web_var=None):
      """ sets properties defaults for the instrument with Name"""
      self.iliad_prop = PropertyManager(instrumentName)
      # the variables which are set up from the main properties
      self._main_properties=[];
      # the variables which are set up from the advanced properties.
      self._advanced_properties=[];
      # The variables which are set up from web interface. 
      self._web_var = web_var;

    def export_changed_values(self,FileName='reduce_vars.py'):
        """ Method to write changed simple and advanced properties into dictionary, to process by 
            web reduction interface
        """
       
        f=open(FileName,'w')
        f.write("standard_vars = {\n")
        str_wrapper = '         '
        for key,val in self._main_properties.iteritems():
                  if isinstance(val,str):
                      row = "{0}\'{1}\':\'{2}\'".format(str_wrapper,key,val)
                  else:
                      row = "{0}\'{1}\':{2}".format(str_wrapper,key,val)
                  f.write(row);
                  str_wrapper=',\n         '
        f.write("\n}\nadvanced_vars={\n")

        str_wrapper='         '
        for key,val in self._advanced_properties.iteritems():
                  if isinstance(val,str):
                      row = "{0}\'{1}\':\'{2}\'".format(str_wrapper,key,val)
                  else:
                      row = "{0}\'{1}\':{2}".format(str_wrapper,key,val)
                  f.write(row);
                  str_wrapper=',\n        '
        f.write("\n}\n")
        f.close();


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
    def main(self,input_file=None,output_directory=None):
        """ The method which performs all main reduction operations. 

        """ 
        raise NotImplementedError('main routine has to be implemented')


def MainProperties(main_prop_definition):
    """ Decorator stores properties dedicated as main and sets these properties as input to reduction parameters.""" 
    def main_prop_wrapper(*args):
        properties = main_prop_definition(*args)
        #print "in decorator: ",properties
        host = args[0];
        host._main_properties=properties
        host.iliad_prop.set_input_parameters(**properties)
        return properties

    return main_prop_wrapper
#
def AdvancedProperties(adv_prop_definition):
    """ Decorator stores properties decided to be advanced and sets these properties as input for reduction parameters """ 
    def advanced_prop_wrapper(*args):
        properties = adv_prop_definition(*args)
        #print "in decorator: ",properties
        host = args[0];
        host._advanced_properties=properties
        host.iliad_prop.set_input_parameters(**properties)
        return properties

    return advanced_prop_wrapper

def using_web_data(self):
    if self._web_var:
        return True;
    else:
        return False;



def iliad(main):
    """ This decorator wraps around main procedure, tries to identify if the procedure is run from web services or 
        from Mantid directly and sets up web-modified variables as input for reduction if it runs from web services. 

        The procedure to identify web services presence is simplified and contains two checks: 
        1) file reduce_vars.py is present and has been imported by reduction script as web_vars 
        2) the method, this decorators frames, is called with arguments, where second argument defines output directory for reduction data
           (this variable is present and not empty)

    """
    def iliad_wrapper(*args):
        #seq = inspect.stack();

        host = args[0];
        if len(args)>1:
            input_file = args[1];
            if len(args)>2:
                output_directory = args[2];
            else:
                output_directory =None
        else:
            input_file=None
            output_directory=None

        use_web_variables= False
        if host._web_var and output_directory:
            use_web_variables = True;
            config.appendDataSearchDir(str(output_directory))
            web_vars = dict(host._web_var.standard_vars.items()+host._web_var.advanced_vars.items());
            host.iliad_prop.set_input_parameters(**web_vars);
            host.iliad_prop.sample_run = input_file;

        rez = main(*args)
        # prohibit returning workspace to web services. 
        if use_web_variables and not isinstance(rez,str):
            rez="";
        return rez;

    return iliad_wrapper
    

if __name__=="__main__":
    pass