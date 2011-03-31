"""
    Module that defines the implementation of script elements, 
    one class per tab
"""
import xml.dom.minidom
import copy
import os
from scripter import BaseScriptElement

# Check whether Mantid is available, in case we ask
# the interface to execute the output script
try:
    from MantidFramework import *
    mtd.initialise(False)
    HAS_MANTID = True
except:
    HAS_MANTID = False  

class %INSTR_NAME%ScriptElement(BaseScriptElement):
    
    some_value = "my_wksp"
        
    def to_script(self):
        """
            Generate reduction script
            @param execute: if true, the script will be executed
        """
        script = "CreateWorkspace('%s', [1,1,1], [1,1,1],[0,0,0])\n" % self.some_value
        
        return script
    
    def to_xml(self):
        """
            Create XML from the current data.
        """
        xml  = "<%INSTR_NAME%ScriptElement>\n"
        xml += "  <some_value>%s</some_value>" % self.some_value
        xml += "</%INSTR_NAME%ScriptElement>\n"
        return xml
    
    def from_xml(self, xml_str):
        """
            Read in data from XML
            @param xml_str: text to read the data from
        """       
        dom = xml.dom.minidom.parseString(xml_str)
        element_list = dom.getElementsByTagName("%INSTR_NAME%ScriptElement")
        if len(element_list)>0: 
            script_dom = element_list[0]
            self.some_value = BaseScriptElement.getStringElement(script_dom, "some_value", default="my_favorite_wksp")
            

    def reset(self):
        """
            Reset default state
        """
        self.some_value = %INSTR_NAME%ScriptElement.some_value

        
    
