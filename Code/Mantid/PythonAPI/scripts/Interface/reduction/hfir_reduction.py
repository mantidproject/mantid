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
    def __str__(self):
        return NotImplemented
    
    def apply(self):
        return NotImplemented
    
    def to_xml(self, file_name=None):
        return NotImplemented
    
    def from_xml(self, file_name):
        return NotImplemented
    
    @classmethod
    def getText(cls, nodelist):
        rc = ""
        for node in nodelist:
            if node.nodeType == node.TEXT_NODE:
                rc = rc + node.data
        return rc       


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
    
    def __init__(self, name="BIOSANS"):
        self.instrument_name = name
        self.beam_finder = hfir_reduction_steps.BeamFinder()
        self.instrument = hfir_reduction_steps.InstrumentDescription()
        

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

    def to_script(self):
        """
            Spits out the text of a reduction script with the current state.
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
        
        return script
        
    def apply(self):
        """
            Apply the reduction process to a Mantid SANSReducer
        """
        
        return script

    