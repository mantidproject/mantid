"""
    This class holds all the necessary information to create a reduction script.
    This is a fake version of the Reducer for testing purposes.
"""
import time
import copy
import hfir_reduction_steps
from scripter import BaseReductionScripter

# Check whether Mantid is available
try:
    from MantidFramework import *
    mtd.initialise(False)
    from HFIRCommandInterface import *
    HAS_MANTID = True
except:
    HAS_MANTID = False  


class HFIRReductionScripter(BaseReductionScripter):
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
    
    def __init__(self, name="BIOSANS"):
        super(HFIRReductionScripter, self).__init__(name=name)
        
        self.instrument_name = name
        self.beam_finder = hfir_reduction_steps.BeamFinder()
        self.instrument = hfir_reduction_steps.InstrumentDescription()
        self.transmission = hfir_reduction_steps.Transmission()
        self.background = hfir_reduction_steps.Background()

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
        
        # Instrument description
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
        
        script += "SaveIqAscii()\n"
        script += "Reduce1D()\n"
        
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
            self.instrument.update()
            self.beam_finder.update()
            self.transmission.update()
            self.background.update()
            
            return ReductionSingleton().log_text
        else:
            raise RuntimeError, "Reduction could not be executed: Mantid could not be imported"
            
    def get_data(self):
        #TODO: fix this
        return ReductionSingleton()._azimuthal_averager.get_data(ReductionSingleton()._data_files.keys()[0])

    