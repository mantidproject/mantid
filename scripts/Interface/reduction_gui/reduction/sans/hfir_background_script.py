#pylint: disable=invalid-name
"""
    Classes for each reduction step. Those are kept separately
    from the the interface class so that the HFIRReduction class could
    be used independently of the interface implementation
"""
import xml.dom.minidom
from reduction_gui.reduction.scripter import BaseScriptElement
from reduction_gui.reduction.sans.hfir_sample_script import SampleData

# Check whether we are running in MantidPlot
IS_IN_MANTIDPLOT = False
try:
    import mantidplot
    IS_IN_MANTIDPLOT = True
except:
    pass


class Background(BaseScriptElement):

    class DirectBeam(SampleData.DirectBeam):
        def __init__(self, state=None):
            SampleData.DirectBeam.__init__(self)
            if state is not None:
                self.sample_file = state.sample_file
                self.direct_beam = state.direct_beam
                self.beam_radius = state.beam_radius

        def to_script(self):
            """
                Generate reduction script
                @param execute: if true, the script will be executed
            """
            if len(str(self.sample_file).strip())==0 or len(str(self.direct_beam).strip())==0:
                raise RuntimeError, "Direct beam method for background transmission was selected but was selected but all the appropriate data files were not entered."

            return "BckDirectBeamTransmission(\"%s\", \"%s\", beam_radius=%g)\n" % \
            (self.sample_file, self.direct_beam, self.beam_radius)

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

            self.sample_file = BaseScriptElement.getPropertyValue(alg, "BckTransmissionSampleDataFile", default='')
            self.direct_beam = BaseScriptElement.getPropertyValue(alg, "BckTransmissionEmptyDataFile", default='')
            self.beam_radius = BaseScriptElement.getPropertyValue(alg, "BckTransmissionBeamRadius",
                                                                  default=SampleData.DirectBeam.beam_radius)

    class BeamSpreader(SampleData.BeamSpreader):
        def __init__(self, state=None):
            SampleData.BeamSpreader.__init__(self)
            if state is not None:
                self.sample_scatt = state.sample_scatt
                self.sample_spreader = state.sample_spreader
                self.direct_scatt = state.direct_scatt
                self.direct_spreader = state.direct_spreader
                self.spreader_trans = state.spreader_trans
                self.spreader_trans_spread = state.spreader_trans_spread

        def to_script(self):
            """
                Generate reduction script
                @param execute: if true, the script will be executed
            """
            if len(str(self.sample_scatt).strip())==0\
                or len(str(self.sample_spreader).strip())==0\
                or len(str(self.direct_scatt).strip())==0\
                or len(str(self.direct_spreader).strip())==0:
                raise RuntimeError, "Beam spreader method for background transmission was selected but all the appropriate data files were not entered."

            return "BckBeamSpreaderTransmission(\"%s\",\n \"%s\",\n \"%s\",\n \"%s\", %g, %g)\n" % \
            (self.sample_spreader, self.direct_spreader,
             self.sample_scatt, self.direct_scatt,
             self.spreader_trans, self.spreader_trans_spread)

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

            self.sample_scatt = BaseScriptElement.getPropertyValue(alg, "BckTransSampleScatteringFilename", default='')
            self.sample_spreader = BaseScriptElement.getPropertyValue(alg, "BckTransSampleSpreaderFilename", default='')
            self.direct_scatt = BaseScriptElement.getPropertyValue(alg, "BckTransDirectScatteringFilename", default='')
            self.direct_spreader = BaseScriptElement.getPropertyValue(alg, "BckTransDirectSpreaderFilename", default='')
            self.spreader_trans = BaseScriptElement.getPropertyValue(alg, "BckSpreaderTransmissionValue",
                                                                     default=SampleData.BeamSpreader.spreader_trans)
            self.spreader_trans_spread = BaseScriptElement.getPropertyValue(alg, "BckSpreaderTransmissionError",
                                                                            default=SampleData.BeamSpreader.spreader_trans_spread)

    dark_current_corr = False
    dark_current_file = ''

    background_corr = False
    background_file = ''

    bck_transmission_enabled = True
    bck_transmission = 1.0
    bck_transmission_spread = 0.0
    calculate_transmission = False
    theta_dependent = True
    trans_dark_current = ''
    trans_calculation_method = DirectBeam()
    sample_thickness = 1.0

    def to_script(self):
        """
            Generate reduction script
        """
        script = ""

        # Dark current
        if self.dark_current_corr:
            if len(str(self.dark_current_file).strip())==0:
                raise RuntimeError, "Dark current subtraction was selected but no dark current data file was entered."
            script += "DarkCurrent(\"%s\")\n" % self.dark_current_file

        # Background
        if self.background_corr:
            if len(str(self.background_file).strip())==0:
                raise RuntimeError, "Background subtraction was selected but no background data file was entered."
            script += "Background(\"%s\")\n" % self.background_file

            # Background transmission
            if self.bck_transmission_enabled:
                if not self.calculate_transmission:
                    script += "SetBckTransmission(%g, %g)\n" % (self.bck_transmission, self.bck_transmission_spread)
                else:
                    script += str(self.trans_calculation_method)

                script += "BckThetaDependentTransmission(%s)\n" % str(self.theta_dependent)
                if self.trans_dark_current is not None and len(str(self.trans_dark_current))>0:
                    script += "BckTransmissionDarkCurrent(\"%s\")\n" % str(self.trans_dark_current)

        return script

    def update(self):
        """
            Update data member from reduction output
        """
        if IS_IN_MANTIDPLOT:
            from mantid.api import PropertyManagerDataService
            from reduction_workflow.command_interface import ReductionSingleton
            property_manager_name = ReductionSingleton().get_reduction_table_name()
            property_manager = PropertyManagerDataService.retrieve(property_manager_name)
            if property_manager.existsProperty("MeasuredBckTransmissionValue"):
                self.bck_transmission = property_manager.getProperty("MeasuredBckTransmissionValue").value
            if property_manager.existsProperty("MeasuredBckTransmissionError"):
                self.bck_transmission_spread = property_manager.getProperty("MeasuredBckTransmissionError").value

    def to_xml(self):
        """
            Create XML from the current data.
        """
        xml  = "<Background>\n"
        xml += "  <dark_current_corr>%s</dark_current_corr>\n" % str(self.dark_current_corr)
        xml += "  <dark_current_file>%s</dark_current_file>\n" % self.dark_current_file

        xml += "  <background_corr>%s</background_corr>\n" % str(self.background_corr)
        xml += "  <background_file>%s</background_file>\n" % self.background_file
        xml += "  <bck_trans_enabled>%s</bck_trans_enabled>\n" % str(self.bck_transmission_enabled)
        xml += "  <bck_trans>%g</bck_trans>\n" % self.bck_transmission
        xml += "  <bck_trans_spread>%g</bck_trans_spread>\n" % self.bck_transmission_spread
        xml += "  <calculate_trans>%s</calculate_trans>\n" % str(self.calculate_transmission)
        xml += "  <theta_dependent>%s</theta_dependent>\n" % str(self.theta_dependent)
        xml += "  <trans_dark_current>%s</trans_dark_current>\n" % str(self.trans_dark_current)
        xml += self.trans_calculation_method.to_xml()
        xml += "</Background>\n"
        return xml

    def from_xml(self, xml_str):
        """
            Read in data from XML
            @param xml_str: text to read the data from
        """
        dom = xml.dom.minidom.parseString(xml_str)
        element_list = dom.getElementsByTagName("Background")
        if len(element_list)>0:
            instrument_dom = element_list[0]

            self.sample_thickness = BaseScriptElement.getFloatElement(instrument_dom, "sample_thickness",
                                                                      default=Background.sample_thickness)

            self.dark_current_corr = BaseScriptElement.getBoolElement(instrument_dom, "dark_current_corr",
                                                                      default = Background.dark_current_corr)
            self.dark_current_file = BaseScriptElement.getStringElement(instrument_dom, "dark_current_file")

            self.background_corr = BaseScriptElement.getBoolElement(instrument_dom, "background_corr",\
                                                                      default = Background.background_corr)
            self.background_file = BaseScriptElement.getStringElement(instrument_dom, "background_file")

            self.bck_transmission_enabled = BaseScriptElement.getBoolElement(instrument_dom, "bck_trans_enabled",\
                                                                           default = Background.bck_transmission_enabled)

            self.bck_transmission = BaseScriptElement.getFloatElement(instrument_dom, "bck_trans",\
                                                                  default=Background.bck_transmission)
            self.bck_transmission_spread = BaseScriptElement.getFloatElement(instrument_dom, "bck_trans_spread",\
                                                                  default=Background.bck_transmission_spread)
            self.calculate_transmission = BaseScriptElement.getBoolElement(instrument_dom, "calculate_trans",
                                                                           default = Background.calculate_transmission)
            self.theta_dependent = BaseScriptElement.getBoolElement(instrument_dom, "theta_dependent",\
                                                                           default = Background.theta_dependent)
            self.trans_dark_current = BaseScriptElement.getStringElement(instrument_dom, "trans_dark_current")

            for m in [Background.DirectBeam, Background.BeamSpreader]:
                method = m()
                if method.find(instrument_dom):
                    method.from_xml(instrument_dom)
                    self.trans_calculation_method = method
                    break

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

        self.background_file = BaseScriptElement.getPropertyValue(alg, "BackgroundFiles", default='')
        self.background_corr = len(self.background_file)>0
        self.bck_transmission_enabled = True
        trans_method = BaseScriptElement.getPropertyValue(alg, "BckTransmissionMethod", default='Value')

        # Transmission
        self.bck_transmission = BaseScriptElement.getPropertyValue(alg, "BckTransmissionValue", default=SampleData.transmission)
        self.bck_transmission_spread = BaseScriptElement.getPropertyValue(alg, "BckTransmissionError", default=SampleData.transmission_spread)

        self.trans_dark_current = BaseScriptElement.getPropertyValue(alg, "BckTransmissionDarkCurrentFile", default='')
        self.theta_dependent = BaseScriptElement.getPropertyValue(alg, "BckThetaDependentTransmission",
                                                                  default = SampleData.theta_dependent)

        self.calculate_transmission = trans_method in ['DirectBeam', 'BeamSpreader']
        if trans_method=='DirectBeam':
            self.trans_calculation_method = Background.DirectBeam()
            self.trans_calculation_method.from_setup_info(xml_str)
        elif trans_method=='BeamSpreader':
            self.trans_calculation_method = Background.BeamSpreader()
            self.trans_calculation_method.from_setup_info(xml_str)

    def reset(self):
        """
            Reset state
        """
        self.dark_current_corr = Background.dark_current_corr
        self.dark_current_file = ''
        self.background_corr = Background.background_corr
        self.background_file = ''
        self.bck_transmission_enabled = Background.bck_transmission_enabled
        self.bck_transmission = Background.bck_transmission
        self.bck_transmission_spread = Background.bck_transmission_spread
        self.calculate_transmission = Background.calculate_transmission
        self.theta_dependent = Background.theta_dependent
        self.trans_dark_current = Background.trans_dark_current
        self.trans_calculation_method = Background.trans_calculation_method
        self.sample_thickness = Background.sample_thickness

