"""
    Classes for each reduction step. Those are kept separately 
    from the the interface class so that the HFIRReduction class could 
    be used independently of the interface implementation
"""
import xml.dom.minidom
import copy
import os
from reduction_gui.reduction.scripter import BaseScriptElement

# Check whether Mantid is available
try:
    from MantidFramework import *
    mtd.initialise(False)
    from reduction.instruments.sans.hfir_command_interface import *
    HAS_MANTID = True
except:
    HAS_MANTID = False  

# Check whether we are running in MantidPlot
IS_IN_MANTIDPLOT = False
try:
    import mantidplot
    IS_IN_MANTIDPLOT = True
except:
    pass

class SampleData(BaseScriptElement):

    class DirectBeam(BaseScriptElement):
        sample_file = ''
        direct_beam = ''
        # Beam radius in pixels
        beam_radius = 3.0
        
        def to_script(self):
            """
                Generate reduction script
                @param execute: if true, the script will be executed
            """
            if len(str(self.sample_file).strip())==0 \
                or len(str(self.direct_beam).strip())==0:
                raise RuntimeError, "Transmission with direct beam was selected but data files were not entered."

            return "DirectBeamTransmission(\"%s\", \"%s\", beam_radius=%g)\n" % \
            (self.sample_file, self.direct_beam, self.beam_radius)
            
        def to_xml(self):
            """
                Create XML from the current data.
            """
            xml  = "<DirectBeam>\n"
            xml += "  <sample_file>%s</sample_file>\n" % self.sample_file
            xml += "  <direct_beam>%s</direct_beam>\n" % self.direct_beam
            xml += "  <beam_radius>%g</beam_radius>\n" % self.beam_radius
            xml += "</DirectBeam>\n"
            return xml
        
        def find(self, dom):
            element_list = dom.getElementsByTagName("DirectBeam")
            return len(element_list)>0
        
        def from_xml(self, dom):
            """
                Read in data from XML
                @param dom: text to read the data from
            """
            element_list = dom.getElementsByTagName("DirectBeam")
            if len(element_list)>0:
                instrument_dom = element_list[0]       
                self.sample_file = BaseScriptElement.getStringElement(instrument_dom, "sample_file")
                self.direct_beam = BaseScriptElement.getStringElement(instrument_dom, "direct_beam")
                self.beam_radius = BaseScriptElement.getFloatElement(instrument_dom, "beam_radius",
                                                                     default=SampleData.DirectBeam.beam_radius)           
        
        def reset(self):
            """
                Reset state
            """
            self.sample_file = ''
            self.direct_beam = ''
            self.beam_radius = SampleData.DirectBeam.beam_radius   
            
    class BeamSpreader(BaseScriptElement):
        sample_scatt = ''
        sample_spreader = ''
        direct_scatt = ''
        direct_spreader = ''
        spreader_trans = 1.0
        spreader_trans_spread = 0.0
        
        def to_script(self):
            """
                Generate reduction script
                @param execute: if true, the script will be executed
            """
            if len(str(self.sample_scatt).strip())==0 \
                or len(str(self.sample_spreader).strip())==0 \
                or len(str(self.direct_scatt).strip())==0 \
                or len(str(self.direct_spreader).strip())==0:
                raise RuntimeError, "Transmission with beam spreader was selected but data files were not entered."
                        
            return "BeamSpreaderTransmission(\"%s\",\n \"%s\",\n \"%s\",\n \"%s\", %g, %g)\n" % \
            (self.sample_spreader, self.direct_spreader, 
             self.sample_scatt, self.direct_scatt, 
             self.spreader_trans, self.spreader_trans_spread) 
            
        def to_xml(self):
            """
                Create XML from the current data.
            """
            xml  = "<BeamSpreader>\n"
            xml += "  <sample_scatt>%s</sample_scatt>\n" % self.sample_scatt
            xml += "  <sample_spreader>%s</sample_spreader>\n" % self.sample_spreader
            xml += "  <direct_scatt>%s</direct_scatt>\n" % self.direct_scatt
            xml += "  <direct_spreader>%s</direct_spreader>\n" % self.direct_spreader

            xml += "  <spreader_trans>%g</spreader_trans>\n" % self.spreader_trans
            xml += "  <spreader_trans_spread>%g</spreader_trans_spread>\n" % self.spreader_trans_spread
            xml += "</BeamSpreader>\n"
            return xml
            
        def find(self, dom):
            element_list = dom.getElementsByTagName("BeamSpreader")
            return len(element_list)>0
        
        def from_xml(self, dom):
            """
                Read in data from XML
                @param dom: text to read the data from
            """       
            element_list = dom.getElementsByTagName("BeamSpreader")
            if len(element_list)>0:
                instrument_dom = element_list[0]      
                self.sample_scatt = BaseScriptElement.getStringElement(instrument_dom, "sample_scatt")
                self.sample_spreader = BaseScriptElement.getStringElement(instrument_dom, "sample_spreader")
                self.direct_scatt = BaseScriptElement.getStringElement(instrument_dom, "direct_scatt")
                self.direct_spreader = BaseScriptElement.getStringElement(instrument_dom, "direct_spreader")
                self.spreader_trans = BaseScriptElement.getFloatElement(instrument_dom, "spreader_trans",
                                                                     default=SampleData.BeamSpreader.spreader_trans)           
                self.spreader_trans_spread = BaseScriptElement.getFloatElement(instrument_dom, "spreader_trans_spread",
                                                                     default=SampleData.BeamSpreader.spreader_trans_spread)           

        def reset(self):
            """
                Reset state
            """
            self.sample_scatt = ''
            self.sample_spreader = ''
            self.direct_scatt = ''
            self.direct_spreader = ''
            self.spreader_trans = SampleData.BeamSpreader.spreader_trans           
            self.spreader_trans_spread = SampleData.BeamSpreader.spreader_trans_spread     

    transmission = 1.0
    transmission_spread = 0.0
    calculate_transmission = False
    calculation_method = DirectBeam()
    theta_dependent = True
    dark_current = ''
    
    # Data file
    data_files = []

            
    def to_script(self):
        """
            Generate reduction script
            @param execute: if true, the script will be executed
        """
        script = ""
        if not self.calculate_transmission:
            script += "SetTransmission(%g, %g)\n" % (self.transmission, self.transmission_spread)
        else:
            script += str(self.calculation_method)
            
        script += "ThetaDependentTransmission(%s)\n" % str(self.theta_dependent)
        if self.dark_current is not None and len(str(self.dark_current))>0:
            script += "TransmissionDarkCurrent(\"%s\")\n" % str(self.dark_current)
        
        # Data files
        if len(self.data_files)>0:
            parts = os.path.split(str(self.data_files[0]).strip())
            if len(parts[0])==0:
                raise RuntimeError, "Trying to generate reduction script without a data file."
            script += "DataPath(\"%s\")\n" % parts[0]
            script += "AppendDataFile([\"%s\"])\n" % '\",\"'.join(self.data_files)
        else:
            raise RuntimeError, "Trying to generate reduction script without a data file."

        return script

    def update(self):
        """
            Update transmission from reduction output
        """
        if HAS_MANTID and ReductionSingleton()._transmission_calculator is not None:
            trans = ReductionSingleton()._transmission_calculator.get_transmission()
            self.transmission = trans[0]
            self.transmission_spread = trans[1]
            

    def to_xml(self):
        """
            Create XML from the current data.
        """
        xml  = "<Transmission>\n"
        xml += "  <trans>%g</trans>\n" % self.transmission
        xml += "  <trans_spread>%g</trans_spread>\n" % self.transmission_spread
        xml += "  <calculate_trans>%s</calculate_trans>\n" % str(self.calculate_transmission)
        xml += "  <theta_dependent>%s</theta_dependent>\n" % str(self.theta_dependent)
        xml += "  <dark_current>%s</dark_current>\n" % str(self.dark_current)
        if self.calculate_transmission:
            xml += self.calculation_method.to_xml()
        xml += "</Transmission>\n"
        xml += "<SampleData>\n"
        for item in self.data_files:
            xml += "  <data_file>%s</data_file>\n" % item.strip()        
        xml += "</SampleData>\n"

        return xml

    def from_xml(self, xml_str):
        """
            Read in data from XML
            @param xml_str: text to read the data from
        """   
        self.reset()    
        dom = xml.dom.minidom.parseString(xml_str)
        
        # Get Mantid version
        mtd_version = BaseScriptElement.getMantidBuildVersion(dom)

        element_list = dom.getElementsByTagName("Transmission")
        if len(element_list)>0:
            instrument_dom = element_list[0]      
            self.transmission = BaseScriptElement.getFloatElement(instrument_dom, "trans",
                                                                  default=SampleData.transmission)      
            self.transmission_spread = BaseScriptElement.getFloatElement(instrument_dom, "trans_spread",
                                                                  default=SampleData.transmission_spread)  
            self.calculate_transmission = BaseScriptElement.getBoolElement(instrument_dom, "calculate_trans",
                                                                           default = SampleData.calculate_transmission)
            self.theta_dependent = BaseScriptElement.getBoolElement(instrument_dom, "theta_dependent",
                                                                           default = SampleData.theta_dependent)
            self.dark_current = BaseScriptElement.getStringElement(instrument_dom, "dark_current")
            
            if self.calculate_transmission:
                for m in [SampleData.DirectBeam, SampleData.BeamSpreader]:
                    method = m()
                    if method.find(instrument_dom):
                        method.from_xml(instrument_dom)
                        self.calculation_method = method
                        break
                    
        # Data file section - take care of backward compatibility
        if mtd_version>0 and mtd_version<BaseScriptElement.UPDATE_1_CHANGESET_CUTOFF:
            element_list = dom.getElementsByTagName("Instrument")
        else:
            element_list = dom.getElementsByTagName("SampleData")
        if len(element_list)>0:
            sample_data_dom = element_list[0]      
            self.data_files = BaseScriptElement.getStringList(sample_data_dom, "data_file")
    
    def reset(self):
        """
            Reset state
        """
        self.transmission = SampleData.transmission      
        self.transmission_spread = SampleData.transmission_spread  
        self.calculate_transmission = SampleData.calculate_transmission
        self.calculation_method = SampleData.calculation_method
        self.theta_dependent = SampleData.theta_dependent
        self.dark_current = SampleData.dark_current
        self.data_files = []
    

    