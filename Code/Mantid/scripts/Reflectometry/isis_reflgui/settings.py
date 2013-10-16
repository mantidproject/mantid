import xml.etree.ElementTree as XML
import os.path
from mantid.simpleapi import *

class MissingSettings(Exception):
    pass

class Settings(object):

    __contents = None
    __filename = None
    
    def __init__(self, filename = None):
        self.__filename = filename
        if not filename:
            filename = os.path.join( os.path.dirname(os.path.realpath(__file__)), "settings.xml")
            
        self.__check_file(filename)
        
        doc = None
        try:
            tree = XML.parse(filename)
            doc = tree.getroot()
        except:
            raise ValueError("The file does not contain valid XML")
        
        self.__extract_to_dictionary(doc)
        
    def __check_file(self, filename):
        path, extension = os.path.splitext(filename)
        if extension.lower().strip() != ".xml":
            raise ValueError("Wrong file extension. *.xml expected not %s." % extension)
        if not os.path.isfile(filename):
            ''' Deliberately swallow and re-throw at this point. Consise reinterpreted error, will be much nicer for client code.'''
            raise MissingSettings("Settings file does not exist filename %s" % filename) 
        
    def __extract_to_dictionary(self, doc):
        temp = dict()
        for elem in doc:
            key = elem.attrib.get('name')
            value = elem.text
            if not key:
                raise ValueError("Missing name attribute on Setting element")
            if not value:
                raise ValueError("Missing value for Setting element")
            temp[key] = value
        self.__contents = dict(frozenset(temp.items()))
        
    def get_all_entries(self):
        return self.__contents
    
    def get_named_setting(self, name):
        return self.__contents[name]
      
    def get_contents_file(self):
        return self.__filename

        
        