"""
    Reduction scripter used to take reduction parameters
    end produce a Mantid reduction script
"""
# Check whether Mantid is available
try:
    from MantidFramework import *
    mtd.initialise(False)
    HAS_MANTID = True
except:
    HAS_MANTID = False  

import xml.dom.minidom
import sys
import time
import platform

class BaseScriptElement(object):
    """
        Base class for each script element (panel on the UI).
        Contains only data and is UI implementation agnostic.
    """
    
    UPDATE_1_CHANGESET_CUTOFF = 10735
    
    def __str__(self):
        """
            Script representation of the object.
            The output is meant to be executable as a Mantid python script
        """
        return self.to_script()
    
    def to_script(self):
        """
            Generate reduction script
        """
        return ""
    
    def update(self):
        """
            Update data member after the reduction has been executed
        """
        return NotImplemented
    
    def apply(self):
        """
            Method called to apply the reduction script element
            to a Mantid Reducer
        """
        return NotImplemented
    
    def to_xml(self):
        """
            Return an XML representation of the data / state of the object
        """
        return ""
    
    def from_xml(self, xml_str):
        """
            Parse the input text as XML to populate the data members
            of this object
        """
        return NotImplemented
    
    def reset(self):
        """
            Reset the state to default
        """
        return NotImplemented
    
    @classmethod
    def getText(cls, nodelist):
        """
            Utility method to extract text out of an XML node
        """
        rc = ""
        for node in nodelist:
            if node.nodeType == node.TEXT_NODE:
                rc = rc + node.data
        return rc       

    @classmethod
    def getContent(cls, dom, tag):
        element_list = dom.getElementsByTagName(tag)
        if len(element_list)>0:
            return BaseScriptElement.getText(element_list[0].childNodes)
        else:
            return None
        
    @classmethod
    def getIntElement(cls, dom, tag, default=None):
        value = BaseScriptElement.getContent(dom, tag)
        if value is not None:
            return int(value)
        else:
            return default

    @classmethod
    def getFloatElement(cls, dom, tag, default=None):
        value = BaseScriptElement.getContent(dom, tag)
        if value is not None:
            return float(value)
        else:
            return default
        
    @classmethod
    def getStringElement(cls, dom, tag, default=''):
        value = BaseScriptElement.getContent(dom, tag)
        if value is not None:
            return value
        else:
            return default
        
    @classmethod
    def getStringList(cls, dom, tag, default=[]):
        elem_list = []
        element_list = dom.getElementsByTagName(tag)
        if len(element_list)>0:
            for l in element_list:
                elem_list.append(BaseScriptElement.getText(l.childNodes).strip())
        return elem_list    

    @classmethod
    def getBoolElement(cls, dom, tag, true_tag='true', default=False):
        value = BaseScriptElement.getContent(dom, tag)
        if value is not None:
            return value.lower()==true_tag.lower()
        else:
            return default
        
    @classmethod
    def getMantidBuildVersion(cls, dom):
        """
            Get the mantid commit number. The format of the build version
            is <release>.<sub-version>.<commit number>
        """
        element_list = dom.getElementsByTagName("mantid_version")
        if len(element_list)>0:
            version_str = BaseScriptElement.getText(element_list[0].childNodes)
            version_list = version_str.split('.')
            if len(version_list)==3:
                change_set = int(version_list[2])
                if change_set>0:
                    return change_set
        return -1


class BaseReductionScripter(object):
    """
        Organizes the set of reduction parameters that will be used to
        create a reduction script. Parameters are organized by groups that
        will each have their own UI representation.
    """
    ## List of observers
    _observers = []
    
    class ReductionObserver(object):
        
        ## Script element class (for type checking)
        _state_cls = None
        ## Script element object
        _state = None
        ## Observed widget object
        _subject = None
        
        def __init__(self, subject):
            self._subject = subject
            self.update(True)
         
        def update(self, init=False):
            """
                Retrieve state from observed widget
                @param init: if True, the state class will be kept for later type checking
            """
            self._state = self._subject.get_state()
            
            # If we are initializing, store the object class
            if init:
                self._state_cls = self._state.__class__
  
            # check that the object class is consistent with what was initially stored 
            elif not self._state.__class__ == self._state_cls:
                raise RuntimeError, "State class changed at runtime, was %s, now %s" % (self._state_cls, self._state.__class__)
            
        def push(self):
            """
                Push the state to update the observed widget
            """
            self._subject.set_state(self.state())
            
        def state(self):
            """
                Returns the state, if one is defined.
            """
            if self._state == NotImplemented:
                return None
            elif self._state is None:
                raise RuntimeError, "Error with %s widget: state not initialized" % self._subject.__class__
            return self._state
        
        def reset(self):
            """
                Reset state
            """
            if self._state is not None:
                self._state.reset()
            else:
                raise RuntimeError, "State reset called without a valid initialized state"
            
    
    def __init__(self, name=""):
        self.instrument_name = name
        self._observers = []

    def attach(self, subject):
        """
            Append a new widget to be observed
            @param subject: BaseWidget object
        """
        observer = BaseReductionScripter.ReductionObserver(subject)
        self._observers.append(observer)
        return observer

    def update(self):
        """
            Tell all observers to update their state.
        """
        for item in self._observers:
            item.update()
            
    def push_state(self):
        """
            Tell the observers to push their state to the their observed widget
        """
        for item in self._observers:
            item.push()

    def verify_instrument(self, file_name):
        """
            Verify that the current scripter object is of the right 
            class for a given data file
            @param file_name: name of the file to check 
        """
        f = open(file_name, 'r')
        xml_str = f.read()
        dom = xml.dom.minidom.parseString(xml_str)
        element_list = dom.getElementsByTagName("Reduction")
        if len(element_list)>0:
            instrument_dom = element_list[0]       
            found_name = BaseScriptElement.getStringElement(instrument_dom, 
                                                            "instrument_name", 
                                                            default=self.instrument_name).strip()
            return found_name
        else:
            raise RuntimeError, "The format of the provided file is not recognized"

    def to_xml(self, file_name=None):
        """
            Write all reduction parameters to XML
            @param file_name: name of the file to write the parameters to 
        """
        xml_str = "<Reduction>\n"
        xml_str += "  <instrument_name>%s</instrument_name>\n" % self.instrument_name
        xml_str += "  <timestamp>%s</timestamp>\n" % time.ctime()
        xml_str += "  <python_version>%s</python_version>\n" % sys.version
        xml_str += "  <platform>%s</platform>\n" % platform.system()
        xml_str += "  <architecture>%s</architecture>\n" % str(platform.architecture())
        if HAS_MANTID:
            xml_str += "  <mantid_version>%s</mantid_version>\n" % mantid_build_version()
        
        for item in self._observers:
            if item.state() is not None:
                xml_str += item.state().to_xml()
            
        xml_str += "</Reduction>\n"
            
        if file_name is not None:
            f = open(file_name, 'w')
            f.write(xml_str)
            f.close()
            
        return xml_str
    
    def _write_to_file(self, file_name, content):
        """
            Write content to a file
            @param file_name: file path
            @param content: content to be written
        """
        if file_name is not None:
            f = open(file_name, 'w')
            f.write(content)
            f.close()
        
    def from_xml(self, file_name):
        """
            Read in reduction parameters from XML
            @param file_name: name of the XML file to read
        """
        f = open(file_name, 'r')
        xml_str = f.read()
        for item in self._observers:
            if item.state() is not None:
                item.state().from_xml(xml_str)

    def to_script(self, file_name=None):
        """
            Spits out the text of a reduction script with the current state.
            @param file_name: name of the file to write the script to
        """
        script = "# Reduction script\n"
        script += "# Script automatically generated on %s\n\n" % time.ctime(time.time())
        
        script += "from MantidFramework import *\n"
        script += "mtd.initialise(False)\n"
        script += "\n"
        
        for item in self._observers:
            if item.state() is not None:
                script += str(item.state())
        
        if file_name is not None:
            f = open(file_name, 'w')
            f.write(script)
            f.close()
        
        return script
    
    def apply(self):
        """
            Apply the reduction process to a Mantid SANSReducer
        """
        if HAS_MANTID:
            script = self.to_script(None)
            exec script               
            
            # Update scripter
            for item in self._observers:
                if item.state() is not None:
                    item.state().update()
        else:
            raise RuntimeError, "Reduction could not be executed: Mantid could not be imported"

    def reset(self):
        """
            Reset reduction state to default
        """
        for item in self._observers:
            item.reset()
            