"""
    Classes for each reduction step. Those are kept separately 
    from the the interface class so that the HFIRReduction class could 
    be used independently of the interface implementation
"""
import xml.dom.minidom
import copy
from hfir_reduction import BaseScriptElement


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
    
    def to_xml(self):
        """
            Create XML from the current data.
            @param file_name: file name to write the XML to [optional]
        """
        xml  = "<Instrument>\n"
        xml += "  <name>%s</name>\n" % self.instrument_name
        xml += "  <nx_pixels>%g</nx_pixels>\n" % self.nx_pixels
        xml += "  <ny_pixels>%g</ny_pixels>\n" % self.ny_pixels
        xml += "  <pixel_size>%g</pixel_size>\n" % self.pixel_size
        xml += "</Instrument>\n"

        return xml
        
    def from_xml(self, xml_str):
        """
            Read in data from XML
            @param file_name: file to read the data from
        """       
        dom = xml.dom.minidom.parseString(xml_str)
        instrument_dom = dom.getElementsByTagName("Instrument")[0]
        self.nx_pixels = int(BeamFinder.getText(instrument_dom.getElementsByTagName("nx_pixels")[0].childNodes))
        self.ny_pixels = int(BeamFinder.getText(instrument_dom.getElementsByTagName("ny_pixels")[0].childNodes))
        self.name = BeamFinder.getText(instrument_dom.getElementsByTagName("name")[0].childNodes)
        self.pixel_size = float(BeamFinder.getText(instrument_dom.getElementsByTagName("pixel_size")[0].childNodes))
        

class BeamFinder(BaseScriptElement):
    """
        Small class to hold the state of the interface
    """
    # Beam finder
    x_position = 0
    y_position = 0
    use_finder = False
    beam_file = ''
    beam_radius = 3.0
    use_direct_beam = True
    
    def __str__(self):
        """
            Representation as a Mantid script
        """
        script = ""
        if not self.use_finder:
            script += "SetBeamCenter(%g, %g)\n" % (self.x_position, self.y_position) 
        else:
            if self.use_direct_beam:
                script += "DirectBeamCenter(\"%s\")\n" % self.beam_file
            else:
                script += "ScatteringBeamCenter(\"%s\", %g)\n" % (self.beam_file, self.beam_radius)
        return script

    def apply(self, reducer):
        """
            The equivalent of the command line implementation, directly
            applied to a SANSReducer object
            @param reducer: SANSReducer object
        """
        return NotImplemeted
    
    def to_xml(self):
        """
            Create XML from the current data.
            @param file_name: file name to write the XML to [optional]
        """
        xml  = "<BeamFinder>\n"
        xml += "  <position>\n"
        xml += "    <x>%g</x>\n" % self.x_position
        xml += "    <y>%g</y>\n" % self.y_position
        xml += "  </position>\n"
        xml += "  <use_finder>%s</use_finder>\n" % str(self.use_finder)
        xml += "  <beam_file>%s</beam_file>\n" % self.beam_file
        xml += "  <use_direct_beam>%s</use_direct_beam>\n" % str(self.use_direct_beam)
        xml += "</BeamFinder>\n"

        return xml
        
    def from_xml(self, xml_str):
        """
            Read in data from XML
            @param file_name: file to read the data from
        """
        dom = xml.dom.minidom.parseString(xml_str)
        beam_finder_dom = dom.getElementsByTagName("BeamFinder")[0]
        self.x_position = float(BeamFinder.getText(beam_finder_dom.getElementsByTagName("x")[0].childNodes))
        self.y_position = float(BeamFinder.getText(beam_finder_dom.getElementsByTagName("y")[0].childNodes))
        self.use_finder = BeamFinder.getText(beam_finder_dom.getElementsByTagName("use_finder")[0].childNodes).strip().lower()=='true'
        self.beam_file = BeamFinder.getText(beam_finder_dom.getElementsByTagName("beam_file")[0].childNodes)
        self.use_direct_beam = BeamFinder.getText(beam_finder_dom.getElementsByTagName("use_direct_beam")[0].childNodes).strip().lower()=='true'
        
        