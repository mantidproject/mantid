#pylint: disable=invalid-name
"""
    Classes for each reduction step. Those are kept separately
    from the the interface class so that the HFIRReduction class could
    be used independently of the interface implementation
"""
import xml.dom.minidom
import os
from reduction_gui.reduction.scripter import BaseScriptElement

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

        def from_setup_info(self, xml_str):
            """
                Read in data from XML using the string representation of the setup algorithm used
                to prepare the reduction properties.
                @param xml_str: text to read the data from
            """
            self.reset()
            from mantid.api import Algorithm
            dom = xml.dom.minidom.parseString(xml_str)

            process_dom = dom.getElementsByTagName("SASProcess")[0]
            setup_alg_str = BaseScriptElement.getStringElement(process_dom, 'SetupInfo')
            alg=Algorithm.fromString(str(setup_alg_str))

            self.sample_file = BaseScriptElement.getPropertyValue(alg, "TransmissionSampleDataFile", default='')
            self.direct_beam = BaseScriptElement.getPropertyValue(alg, "TransmissionEmptyDataFile", default='')
            self.beam_radius = BaseScriptElement.getPropertyValue(alg, "TransmissionBeamRadius",
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
                self.spreader_trans = BaseScriptElement.getFloatElement(instrument_dom, "spreader_trans",\
                                                                     default=SampleData.BeamSpreader.spreader_trans)
                self.spreader_trans_spread = BaseScriptElement.getFloatElement(instrument_dom, "spreader_trans_spread",\
                                                                     default=SampleData.BeamSpreader.spreader_trans_spread)

        def from_setup_info(self, xml_str):
            """
                Read in data from XML using the string representation of the setup algorithm used
                to prepare the reduction properties.
                @param xml_str: text to read the data from
            """
            self.reset()
            from mantid.api import Algorithm
            dom = xml.dom.minidom.parseString(xml_str)

            process_dom = dom.getElementsByTagName("SASProcess")[0]
            setup_alg_str = BaseScriptElement.getStringElement(process_dom, 'SetupInfo')
            alg=Algorithm.fromString(str(setup_alg_str))

            self.sample_scatt = BaseScriptElement.getPropertyValue(alg, "TransSampleScatteringFilename", default='')
            self.sample_spreader = BaseScriptElement.getPropertyValue(alg, "TransSampleSpreaderFilename", default='')
            self.direct_scatt = BaseScriptElement.getPropertyValue(alg, "TransDirectScatteringFilename", default='')
            self.direct_spreader = BaseScriptElement.getPropertyValue(alg, "TransDirectSpreaderFilename", default='')
            self.spreader_trans = BaseScriptElement.getPropertyValue(alg, "SpreaderTransmissionValue",
                                                                     default=SampleData.BeamSpreader.spreader_trans)
            self.spreader_trans_spread = BaseScriptElement.getPropertyValue(alg, "SpreaderTransmissionError",
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
    sample_thickness = 1.0

    # Data file
    data_files = []
    separate_jobs = False

    # Option list
    option_list = [DirectBeam, BeamSpreader]

    def get_data_file_list(self):
        """
            Get the list of data files.
            Includes expanding run ranges.
        """
        data_list = []
        for f in self.data_files:
            data_list.extend(SampleData.parse_runs(f))
        return data_list

    def to_script(self, data_file=None):
        """
            Generate reduction script
            @param execute: if true, the script will be executed
        """
        script = ""

        # Sample thickness
        if not self.sample_thickness == 1.0:
            script += "DivideByThickness(%g)\n" % self.sample_thickness

        if not self.calculate_transmission:
            script += "SetTransmission(%g, %g)\n" % (self.transmission, self.transmission_spread)
        else:
            script += str(self.calculation_method)

        script += "ThetaDependentTransmission(%s)\n" % str(self.theta_dependent)
        if self.dark_current is not None and len(str(self.dark_current))>0:
            script += "TransmissionDarkCurrent(\"%s\")\n" % str(self.dark_current)

        # Data files
        if len(self.data_files)==0:
            raise RuntimeError, "Trying to generate reduction script without a data file."

        if data_file is None:
            data_file_list = self.get_data_file_list()
            parts = os.path.split(str(data_file_list[0]).strip())
            if len(parts[0])>0:
                script += "DataPath(\"%s\")\n" % parts[0]
            else:
                script += "#Note: Data path was not found at script generation, will try at run time.\n"

            if self.separate_jobs is False:
                script += "AppendDataFile([\"%s\"])\n" % '\",\"'.join(data_file_list)
            else:
                for f in data_file_list:
                    script += "AppendDataFile([\"%s\"])\n" % f
        else:
            parts = os.path.split(str(data_file).strip())
            if len(parts[0])>0:
                script += "DataPath(\"%s\")\n" % parts[0]
            else:
                script += "#Note: Data path was not found at script generation, will try at run time.\n"
            script += "AppendDataFile([\"%s\"])\n" % str(data_file)

        return script

    def update(self):
        """
            Update transmission from reduction output
        """
        if IS_IN_MANTIDPLOT:
            from mantid.api import PropertyManagerDataService
            from reduction_workflow.command_interface import ReductionSingleton
            property_manager_name = ReductionSingleton().get_reduction_table_name()
            property_manager = PropertyManagerDataService.retrieve(property_manager_name)
            if property_manager.existsProperty("MeasuredTransmissionValue"):
                self.transmission = property_manager.getProperty("MeasuredTransmissionValue").value
            if property_manager.existsProperty("MeasuredTransmissionError"):
                self.transmission_spread = property_manager.getProperty("MeasuredTransmissionError").value

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
        xml += self.calculation_method.to_xml()
        xml += "</Transmission>\n"
        xml += "<SampleData>\n"
        xml += "  <separate_jobs>%s</separate_jobs>\n" % str(self.separate_jobs)
        xml += "  <sample_thickness>%g</sample_thickness>\n" % self.sample_thickness
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

        element_list = dom.getElementsByTagName("Transmission")
        if len(element_list)>0:
            instrument_dom = element_list[0]
            self.transmission = BaseScriptElement.getFloatElement(instrument_dom, "trans",
                                                                  default=SampleData.transmission)
            self.transmission_spread = BaseScriptElement.getFloatElement(instrument_dom, "trans_spread",\
                                                                  default=SampleData.transmission_spread)
            self.calculate_transmission = BaseScriptElement.getBoolElement(instrument_dom, "calculate_trans",
                                                                           default = SampleData.calculate_transmission)
            self.theta_dependent = BaseScriptElement.getBoolElement(instrument_dom, "theta_dependent",\
                                                                           default = SampleData.theta_dependent)
            self.dark_current = BaseScriptElement.getStringElement(instrument_dom, "dark_current")

            for m in self.option_list:
                method = m()
                if method.find(instrument_dom):
                    method.from_xml(instrument_dom)
                    self.calculation_method = method
                    break

        # Data file section
        element_list = dom.getElementsByTagName("SampleData")
        if len(element_list)>0:
            sample_data_dom = element_list[0]
            self.data_files = BaseScriptElement.getStringList(sample_data_dom, "data_file")
            self.sample_thickness = BaseScriptElement.getFloatElement(sample_data_dom, "sample_thickness",
                                                                      default=SampleData.sample_thickness)
            self.separate_jobs = BaseScriptElement.getBoolElement(sample_data_dom, "separate_jobs",
                                                                  default = SampleData.separate_jobs)

    def from_setup_info(self, xml_str):
        """
            Read in data from XML using the string representation of the setup algorithm used
            to prepare the reduction properties.
            @param xml_str: text to read the data from
        """
        self.reset()
        from mantid.api import Algorithm
        dom = xml.dom.minidom.parseString(xml_str)

        process_dom = dom.getElementsByTagName("SASProcess")[0]
        setup_alg_str = BaseScriptElement.getStringElement(process_dom, 'SetupInfo')
        alg=Algorithm.fromString(str(setup_alg_str))

        # Transmission
        self.transmission = BaseScriptElement.getPropertyValue(alg, "TransmissionValue", default=SampleData.transmission)
        self.transmission_spread = BaseScriptElement.getPropertyValue(alg, "TransmissionError", default=SampleData.transmission_spread)
        self.dark_current = BaseScriptElement.getPropertyValue(alg, "TransmissionDarkCurrentFile", default='')
        self.theta_dependent = BaseScriptElement.getPropertyValue(alg, "ThetaDependentTransmission",
                                                                  default = SampleData.theta_dependent)
        self.sample_thickness = BaseScriptElement.getPropertyValue(alg, "SampleThickness",
                                                                   default = SampleData.sample_thickness)

        trans_method = BaseScriptElement.getPropertyValue(alg, "TransmissionMethod", default='Value')

        self.calculate_transmission = trans_method in ['DirectBeam', 'BeamSpreader']
        if trans_method=='DirectBeam':
            self.calculation_method = SampleData.DirectBeam()
            self.calculation_method.from_setup_info(xml_str)
        elif trans_method=='BeamSpreader':
            self.calculation_method = SampleData.BeamSpreader()
            self.calculation_method.from_setup_info(xml_str)

        # Data file section
        self.data_files = [BaseScriptElement.getStringElement(process_dom, 'Filename', '')]

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
        self.sample_thickness = SampleData.sample_thickness
        self.data_files = []
        self.separate_jobs = SampleData.separate_jobs


