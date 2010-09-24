"""
    This class holds all the necessary information to create a reduction script.
    This is a fake version of the Reducer for testing purposes.
"""
import time
import copy
import hfir_reduction_steps

class BaseScriptElement(object):
    """
        Base class for each script element (panel on the UI).
        Contains only data and is UI implementation agnostic.
    """
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
        return NotImplemented
    
    def from_xml(self, xml_str):
        """
            Parse the input text as XML to populate the data members
            of this object
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
    def getBoolElement(cls, dom, tag, true_tag='true', default=False):
        value = BaseScriptElement.getContent(dom, tag)
        if value is not None:
            return value==true_tag
        else:
            return default

class ReductionScripter(object):
    """
        Organizes the set of reduction parameters that will be used to
        create a reduction script. Parameters are organized by groups that
        will each have their own UI representation.
    """
    # Instrument description
    instrument = None
    # Beam finder
    beam_finder = None 
    # Transmission panel
    transmission = None
    # Background panel
    background = None
    # Data set panel
    data_sets = None
    
    def __init__(self, name="BIOSANS"):
        self.instrument_name = name
        self.beam_finder = hfir_reduction_steps.BeamFinder()
        self.instrument = hfir_reduction_steps.InstrumentDescription()
        self.transmission = hfir_reduction_steps.Transmission()
        self.background = hfir_reduction_steps.Background()
        self.data_sets = hfir_reduction_steps.DataSets()
        

    def to_xml(self, file_name=None):
        """
            Write all reduction parameters to XML
            @param file_name: name of the file to write the parameters to 
        """
        xml_str = "<Reduction>\n"
        if self.instrument is not None:
            xml_str += self.instrument.to_xml()
            
        if self.beam_finder is not None:
            xml_str += self.beam_finder.to_xml()
            
        if self.transmission is not None:
            xml_str += self.transmission.to_xml()
            
        if self.background is not None:
            xml_str += self.background.to_xml()
            
        if self.data_sets is not None:
            xml_str += self.data_sets.to_xml()
            
        xml_str += "</Reduction>\n"
            
        if file_name is not None:
            f = open(file_name, 'w')
            f.write(xml_str)
            f.close()
            
        return xml_str
    
    def from_xml(self, file_name):
        """
            Read in reduction parameters from XML
            @param file_name: name of the XML file to read
        """
        f = open(file_name, 'r')
        xml_str = f.read()
        
        if self.instrument is not None:
            self.instrument.from_xml(xml_str)
            
        if self.beam_finder is not None:
            self.beam_finder.from_xml(xml_str)

        if self.transmission is not None:
            self.transmission.from_xml(xml_str)

        if self.background is not None:
            self.background.from_xml(xml_str)

        if self.data_sets is not None:
            self.data_sets.from_xml(xml_str)

    def to_script(self, file_name=None):
        """
            Spits out the text of a reduction script with the current state.
            @param file_name: name of the file to write the script to
        """
        script = "# HFIR reduction script\n"
        script += "# Script automatically generated on %s\n\n" % time.ctime(time.time())
        
        script += "from MantidFramework import *\n"
        script += "mtd.initialise(False)\n"
        script += "from HFIRCommandInterface import *\n"
        script += "\n"
        
        # Instrument dsecription
        if self.instrument is None:
            raise RuntimeError, "An instrument description was not established before starting reduction."
        script += str(self.instrument)
        
        # Beam center finder
        if self.beam_finder is None:
            raise RuntimeError, "A beam center option was not established before starting reduction."
        script += str(self.beam_finder)
        
        # Transmission
        if self.transmission is None:
            raise RuntimeError, "Transmission options were not established before starting reduction."
        script += str(self.transmission)
        
        # Background
        if self.background is None:
            raise RuntimeError, "A background option was not established before starting reduction."
        script += str(self.background)
        
        # Data sets
        if self.data_sets is None:
            raise RuntimeError, "Data set options were not established before starting reduction."
        script += str(self.data_sets)
        
        if file_name is not None:
            f = open(file_name, 'w')
            f.write(script)
            f.close()
        
        return script
        
    def apply(self):
        """
            Apply the reduction process to a Mantid SANSReducer
        """
        
        return script

    