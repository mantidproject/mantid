"""
    General options for EQSANS reduction
"""
import xml.dom.minidom
from reduction_gui.reduction.scripter import BaseScriptElement
from hfir_options_script import ReductionOptions as BaseOptions

class ReductionOptions(BaseOptions):
    instrument_name = "EQSANS"
    nx_pixels = None
    ny_pixels = None
    pixel_size = None
    detector_offset = 0.0

    # Use TOF cuts from configuration file
    use_config_cutoff = True
    low_TOF_cut = 0.0
    high_TOF_cut = 0.0
    # Correct for flight path at larger angle
    correct_for_flight_path = True

    # Use the mask defined in the configuration file
    use_config_mask = True

    # Don't use data directory because it's not writable
    use_data_directory = False

    # Resolution parameters
    compute_resolution = True
    sample_aperture_diameter = 10.0

    # Perform EQSANS TOF correction
    perform_TOF_correction = True
    
    # Turn off the wedges
    n_wedges = 0
    
    # Turn off log binning alignment with decades
    align_log_with_decades = False

    # Normalize to beam monitor
    use_beam_monitor = False
    beam_monitor_reference = ''

    def __init__(self):
        super(ReductionOptions, self).__init__()
        self.reset()

    def reset(self):
        super(ReductionOptions, self).reset()
        self.instrument_name = ReductionOptions.instrument_name
        self.nx_pixels = ReductionOptions.nx_pixels
        self.ny_pixels = ReductionOptions.ny_pixels
        self.pixel_size = ReductionOptions.pixel_size
        self.detector_offset = ReductionOptions.detector_offset

        self.use_config_cutoff = ReductionOptions.use_config_cutoff
        self.low_TOF_cut = ReductionOptions.low_TOF_cut
        self.high_TOF_cut = ReductionOptions.high_TOF_cut
        self.correct_for_flight_path = ReductionOptions.correct_for_flight_path
        self.normalization = ReductionOptions.NORMALIZATION_MONITOR

        self.use_config_mask = ReductionOptions.use_config_mask
        self.use_data_directory = ReductionOptions.use_data_directory

        self.compute_resolution = ReductionOptions.compute_resolution
        self.sample_aperture_diameter = ReductionOptions.sample_aperture_diameter

        self.perform_TOF_correction = True
        self.n_wedges = ReductionOptions.n_wedges
        self.align_log_with_decades = ReductionOptions.align_log_with_decades
        self.use_beam_monitor = False
        self.beam_monitor_reference = ''

    def options(self):
        """
            Set up the reduction options
        """
        # Load options
        script = ""

        if self.use_config_cutoff:
            script += "  UseConfigTOFCuts=1,\n"
        else:
            script += "  UseConfigTOFCuts=0,\n"
            script += "  LowTOFCut=%g,\n" % self.low_TOF_cut
            script += "  HighTOFCut=%g,\n" % self.high_TOF_cut

        if self.use_config_mask:
            script += "  UseConfigMask=1,\n"
        else:
            script += "  UseConfigMask=0,\n"

        if self.correct_for_flight_path:
            script += "  CorrectForFlightPath=1,\n"
        else:
            script += "  CorrectForFlightPath=0,\n"

        if self.solid_angle_corr:
            script += "  SolidAngleCorrection=1,\n"
        else:
            script += "  SolidAngleCorrection=0,\n"

        if self.dark_current_corr:
            script += "  DarkCurrentFile='%s',\n" % self.dark_current_data

        if self.normalization==ReductionOptions.NORMALIZATION_MONITOR:
            if self.use_beam_monitor:
                script += "  Normalisation='Monitor',\n"
                script += "  MonitorReferenceFile='%s',\n" % self.beam_monitor_reference
            else:
                script += "  Normalisation='BeamProfileAndCharge',\n"

        return script

    def _normalization_options(self):
        """
            Generate the normalization portion of the reduction script
        """
        if self.normalization==ReductionOptions.NORMALIZATION_NONE:
            return "NoNormalization()\n"
        elif self.normalization==ReductionOptions.NORMALIZATION_MONITOR:
            if self.use_beam_monitor:
                return "BeamMonitorNormalization(\"%s\")\n" % self.beam_monitor_reference
            else:
                return "TotalChargeNormalization()\n"
        return ""

    def to_script(self):
        """
            Generate reduction script
            @param execute: if true, the script will be executed
        """
        script = super(ReductionOptions, self).to_script()

        # TOF cutoff
        if self.use_config_cutoff:
            script += "UseConfigTOFTailsCutoff(True)\n"
        else:
            script += "UseConfigTOFTailsCutoff(False)\n"
            if self.low_TOF_cut>0 or self.high_TOF_cut>0:
                script += "SetTOFTailsCutoff(%g, %g)\n" % (self.low_TOF_cut, self.high_TOF_cut)

        # Use mask defined in configuration file
        script += "UseConfigMask(%s)\n" % self.use_config_mask

        # Resolution
        if self.compute_resolution:
            script += "Resolution(sample_aperture_diameter=%g)\n" % self.sample_aperture_diameter

        if not self.perform_TOF_correction:
            script += "SkipTOFCorrection()\n"
        else:
            # Flight path correction
            script += "PerformFlightPathCorrection(%s)\n" % self.correct_for_flight_path

        return script

    def to_xml(self):
        """
            Create XML from the current data.
        """
        xml = super(ReductionOptions, self).to_xml()

        # TOF cutoff and correction
        xml += "<TOFcorr>\n"
        xml += "  <low_tof_cut>%g</low_tof_cut>\n" % self.low_TOF_cut
        xml += "  <high_tof_cut>%g</high_tof_cut>\n" % self.high_TOF_cut
        xml += "  <use_config_cutoff>%s</use_config_cutoff>\n" % str(self.use_config_cutoff)
        xml += "  <perform_flight_path_corr>%s</perform_flight_path_corr>\n" % str(self.correct_for_flight_path)
        xml += "</TOFcorr>\n"

        # Mask
        xml += "<UseConfigMask>%s</UseConfigMask>\n" % self.use_config_mask

        # Resolution
        xml += "<ComputeResolution>%s</ComputeResolution>\n" % self.compute_resolution
        xml += "<SampleApertureDiameter>%g</SampleApertureDiameter>\n" % self.sample_aperture_diameter

        # TOF correction
        xml += "<PerformTOFCorrection>%s</PerformTOFCorrection>\n" % self.perform_TOF_correction
        # Normalization option
        xml += "<UseBeamMonitor>%s</UseBeamMonitor>\n" % self.use_beam_monitor
        xml += "<BeamMonitorRef>%s</BeamMonitorRef>\n" % self.beam_monitor_reference

        return xml

    def from_xml(self, xml_str):
        """
            Read in data from XML
            @param xml_str: text to read the data from
        """
        self.reset()
        super(ReductionOptions, self).from_xml(xml_str)

        dom = xml.dom.minidom.parseString(xml_str)

        # TOF cutoff and correction
        element_list = dom.getElementsByTagName("TOFcorr")
        if len(element_list)>0:
            tof_dom = element_list[0]

            self.use_config_cutoff = BaseScriptElement.getBoolElement(tof_dom, "use_config_cutoff",
                                                                      default = ReductionOptions.use_config_cutoff)
            self.correct_for_flight_path = BaseScriptElement.getBoolElement(tof_dom, "perform_flight_path_corr",
                                                                            default = ReductionOptions.correct_for_flight_path)
            self.low_TOF_cut = BaseScriptElement.getFloatElement(tof_dom, "low_tof_cut",
                                                                 default=ReductionOptions.low_TOF_cut)
            self.high_TOF_cut = BaseScriptElement.getFloatElement(tof_dom, "high_tof_cut",
                                                                  default=ReductionOptions.high_TOF_cut)

        # Mask
        self.use_config_mask = BaseScriptElement.getBoolElement(dom, "UseConfigMask",
                                                                default = ReductionOptions.use_config_mask)

        # Resolution
        self.compute_resolution = BaseScriptElement.getBoolElement(dom, "ComputeResolution",
                                                                   default = ReductionOptions.compute_resolution)
        self.sample_aperture_diameter = BaseScriptElement.getFloatElement(dom, "SampleApertureDiameter",
                                                                         default = ReductionOptions.sample_aperture_diameter)

        # TOF correction
        self.perform_TOF_correction = BaseScriptElement.getBoolElement(dom, "PerformTOFCorrection",
                                                                       default = ReductionOptions.perform_TOF_correction)
        # Normalization option
        self.use_beam_monitor = BaseScriptElement.getBoolElement(dom, "UseBeamMonitor",
                                                                 default = ReductionOptions.use_beam_monitor)
        self.beam_monitor_reference = BaseScriptElement.getStringElement(dom, "BeamMonitorRef",
                                                                         default = ReductionOptions.beam_monitor_reference)

    def from_setup_info(self, xml_str):
        """
            Read in data from XML using the string representation of the setup algorithm used
            to prepare the reduction properties.
            @param xml_str: text to read the data from
        """
        self.reset()
        super(ReductionOptions, self).from_setup_info(xml_str)

        from mantid.api import Algorithm
        dom = xml.dom.minidom.parseString(xml_str)

        process_dom = dom.getElementsByTagName("SASProcess")[0]
        setup_alg_str = BaseScriptElement.getStringElement(process_dom, 'SetupInfo')
        alg=Algorithm.fromString(str(setup_alg_str))
        self.use_config_cutoff = BaseScriptElement.getPropertyValue(alg, "UseConfigTOFCuts",
                                                                    default=ReductionOptions.use_config_cutoff)
        self.correct_for_flight_path = BaseScriptElement.getPropertyValue(alg, "CorrectForFlightPath",
                                                                          default=ReductionOptions.correct_for_flight_path)
        self.low_TOF_cut = BaseScriptElement.getPropertyValue(alg, "LowTOFCut",
                                                              default=ReductionOptions.low_TOF_cut)
        self.high_TOF_cut = BaseScriptElement.getPropertyValue(alg, "HighTOFCut",
                                                               default=ReductionOptions.high_TOF_cut)
        self.use_config_mask = BaseScriptElement.getPropertyValue(alg, "UseConfigMask",
                                                                  default=ReductionOptions.use_config_mask)
        self.compute_resolution = BaseScriptElement.getPropertyValue(alg, "ComputeResolution",
                                                                     default=ReductionOptions.compute_resolution)
        self.sample_aperture_diameter = BaseScriptElement.getPropertyValue(alg, "SampleApertureDiameter",
                                                                           default=ReductionOptions.sample_aperture_diameter)
        self.perform_TOF_correction = not BaseScriptElement.getPropertyValue(alg, "SkipTOFCorrection",
                                                                             default=False)
        norm_option = BaseScriptElement.getPropertyValue(alg, "Normalisation", default = 'Monitor')
        self.use_beam_monitor = norm_option=='Monitor'
        self.beam_monitor_reference = BaseScriptElement.getPropertyValue(alg, "MonitorReferenceFile",
                                                                         default=ReductionOptions.beam_monitor_reference)
