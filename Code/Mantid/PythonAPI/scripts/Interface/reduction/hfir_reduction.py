"""
    This class holds all the necessary information to create a reduction script.
    This is a fake version of the Reducer for testing purposes.
"""
import time

class InstrumentDescription(object):
    instrument_name = "BIOSANS"
    nx_pixels = 192
    ny_pixels = 192
    pixel_size = 5.1
        
    def __str__(self):
        """
            Representation as a Mantid script
        """
        return "HFIRSANS()\n"

    def apply(self, reducer):
        """
            The equivalent of the command line implementation, directly
            applied to a SANSReducer object
            @param reducer: SANSReducer object
        """
        return NotImplemeted    


class ReductionScripter(object):
    
    # Instrument description
    instrument = None
    
    # Beam finder
    beam_finder = None 
    
    def __init__(self, name="BIOSANS"):
        self.instrument_name = name

    def to_xml(self, file_name):
        return NotImplemented
    
    def from_xml(self, file_name):
        return NotImplemented

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

    