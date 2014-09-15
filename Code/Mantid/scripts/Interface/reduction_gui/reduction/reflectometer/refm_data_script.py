"""
    Reduction script for REFM
"""
import xml.dom.minidom
import os
import time
from reduction_gui.reduction.scripter import BaseScriptElement

class DataSets(BaseScriptElement):

    DataPeakPixels = [215, 225]
    DataBackgroundFlag = False
    DataBackgroundRoi = [115, 137, 123, 137]
    DataTofRange = [10700., 24500.]
    crop_TOF_range = True
    TOFstep = 400.0

    data_x_range_flag = False
    data_x_range = [115, 210]

    norm_x_range_flag = False
    norm_x_range = [90, 160]

    NormFlag = True
    NormPeakPixels = [120, 130]
    NormBackgroundFlag = False
    NormBackgroundRoi = [115, 137]

    # Data files
    data_files = [0]
    norm_file = 0

    # Q range
    q_min = 0.0025
    q_step = -0.01
    q_bins = 40
    q_log = True

    # scattering angle
    theta = 0.0
    use_center_pixel = True

    # Sample log overwrites
    set_detector_angle = False
    detector_angle = 0.0
    set_detector_angle_offset = False
    detector_angle_offset = 0.0
    set_direct_pixel = False
    direct_pixel = 0.0

    output_dir = ''

    def __init__(self):
        super(DataSets, self).__init__()
        self.reset()

    def to_script(self, for_automated_reduction=False):
        """
            Generate reduction script
            @param execute: if true, the script will be executed
        """
        if for_automated_reduction:
            return self._automated_reduction()

        script = "import mantid\n"
        script += "from mantid.api import *\n"
        script += "from mantid.kernel import *\n"
        script += "from mantid.simpleapi import *\n"

        script = "a = RefReduction(DataRun='%s',\n" % ','.join([str(i) for i in self.data_files])
        script += "              NormalizationRun='%s',\n" % str(self.norm_file)
        script += "              Instrument='REF_M',\n"
        script += "              PolarizedData=True,\n"

        script += "              SignalPeakPixelRange=%s,\n" % str(self.DataPeakPixels)
        script += "              SubtractSignalBackground=%s,\n" % str(self.DataBackgroundFlag)
        script += "              SignalBackgroundPixelRange=%s,\n" % str(self.DataBackgroundRoi[:2])
        script += "              PerformNormalization=%s,\n" % str(self.NormFlag)
        script += "              NormPeakPixelRange=%s,\n" % str(self.NormPeakPixels)
        script += "              NormBackgroundPixelRange=%s,\n" % str(self.NormBackgroundRoi)
        script += "              SubtractNormBackground=%s,\n" % str(self.NormBackgroundFlag)

        script += "              CropLowResDataAxis=%s,\n" % str(self.data_x_range_flag)
        if self.data_x_range_flag:
            script += "              LowResDataAxisPixelRange=%s,\n" % str(self.data_x_range)

        script += "              CropLowResNormAxis=%s,\n" % str(self.norm_x_range_flag)
        if self.norm_x_range_flag:
            script += "              LowResNormAxisPixelRange=%s,\n" % str(self.norm_x_range)

        if self.crop_TOF_range:
            script += "              TOFMin=%s,\n" % str(self.DataTofRange[0])
            script += "              TOFMax=%s,\n" % str(self.DataTofRange[1])
            script += "              TOFStep=%s,\n" % str(self.TOFstep)
        else:
            script += "              NBins=%s,\n" % str(self.q_bins)

        # Scattering angle options
        if self.use_center_pixel:
            if self.set_detector_angle:
                script += "              DetectorAngle=%s,\n" % str(self.detector_angle)
            if self.set_detector_angle_offset:
                script += "              DetectorAngle0=%s,\n" % str(self.detector_angle_offset)
            if self.set_direct_pixel:
                script += "              DirectPixel=%s,\n" % str(self.direct_pixel)

        else:
            script += "              Theta=%s,\n" % str(self.theta)

        # The output should be slightly different if we are generating
        # a script for the automated reduction
        basename = os.path.basename(str(self.data_files[0]))
        script += "              OutputWorkspacePrefix='reflectivity_%s')\n" % basename
        script += "\n"

        # Store the log output so it can be shown in the UI
        script += "from reduction_workflow.command_interface import ReductionSingleton\n"
        script += "reducer_log = ReductionSingleton()\n"
        script += "output_log = 'Please make sure the new-style Python API is turned ON by default\\n'\n"
        script += "output_log += 'In MantiPlot, go in View > Preferences > Mantid > Options '\n"
        script += "output_log += 'and check the appropriate box at the bottom.'\n"
        script += "for item in a:\n"
        script += "    if type(item)==str: output_log = item\n"
        script += "reducer_log.log_text += output_log\n\n"

        # Save the reduced data
        script += "ws_list = ['reflectivity_%s-Off_Off',\n" % basename
        script += "           'reflectivity_%s-On_Off',\n" % basename
        script += "           'reflectivity_%s-Off_On',\n" % basename
        script += "           'reflectivity_%s-On_On']\n" % basename

        script += "outdir = '%s'\n" % self.output_dir
        script += "if not os.path.isdir(outdir):\n"
        script += "    outdir = os.path.expanduser('~')\n\n"

        script += "for ws in ws_list:\n"
        script += "    if AnalysisDataService.doesExist(ws):\n"
        script += "        outpath = os.path.join(outdir, ws+'.txt')\n"
        script += "        SaveAscii(Filename=outpath, InputWorkspace=ws, Separator='Space')\n\n"

        return script

    def _automated_reduction(self):
        script = "# REF_M automated reduction\n"
        script += "RefReduction(DataRun='%s',\n" % ','.join([str(i) for i in self.data_files])
        script += "              NormalizationRun='%s',\n" % str(self.norm_file)
        script += "              Instrument='REF_M',\n"
        script += "              PolarizedData=True,\n"
        script += "              SignalPeakPixelRange=%s,\n" % str(self.DataPeakPixels)
        script += "              SubtractSignalBackground=False,\n"
        script += "              PerformNormalization=%s,\n" % str(self.NormFlag)
        script += "              NormPeakPixelRange=%s,\n" % str(self.NormPeakPixels)
        script += "              SubtractNormBackground=False,\n"

        script += "              CropLowResDataAxis=%s,\n" % str(self.data_x_range_flag)
        if self.data_x_range_flag:
            script += "              LowResDataAxisPixelRange=%s,\n" % str(self.data_x_range)

        script += "              CropLowResNormAxis=%s,\n" % str(self.norm_x_range_flag)
        if self.norm_x_range_flag:
            script += "              LowResNormAxisPixelRange=%s,\n" % str(self.norm_x_range)

        # The output should be slightly different if we are generating
        # a script for the automated reduction
        basename = os.path.basename(str(self.data_files[0]))
        script += "              OutputWorkspacePrefix='reflectivity_'+%s)\n" % basename

        return script

    def update(self):
        """
            Update transmission from reduction output
        """
        pass

    def to_xml(self):
        """
            Create XML from the current data.
        """
        xml = "<RefMData>\n"
        xml += "<from_peak_pixels>%s</from_peak_pixels>\n" % str(self.DataPeakPixels[0])
        xml += "<to_peak_pixels>%s</to_peak_pixels>\n" % str(self.DataPeakPixels[1])
        xml += "<background_flag>%s</background_flag>\n" % str(self.DataBackgroundFlag)
        xml += "<back_roi1_from>%s</back_roi1_from>\n" % str(self.DataBackgroundRoi[0])
        xml += "<back_roi1_to>%s</back_roi1_to>\n" % str(self.DataBackgroundRoi[1])
        xml += "<back_roi2_from>%s</back_roi2_from>\n" % str(self.DataBackgroundRoi[2])
        xml += "<back_roi2_to>%s</back_roi2_to>\n" % str(self.DataBackgroundRoi[3])
        xml += "<crop_tof>%s</crop_tof>\n" % str(self.crop_TOF_range)
        xml += "<from_tof_range>%s</from_tof_range>\n" % str(self.DataTofRange[0])
        xml += "<to_tof_range>%s</to_tof_range>\n" % str(self.DataTofRange[1])
        xml += "<tof_step>%s</tof_step>\n" % str(self.TOFstep)
        xml += "<data_sets>%s</data_sets>\n" % ','.join([str(i) for i in self.data_files])
        xml += "<x_min_pixel>%s</x_min_pixel>\n" % str(self.data_x_range[0])
        xml += "<x_max_pixel>%s</x_max_pixel>\n" % str(self.data_x_range[1])
        xml += "<x_range_flag>%s</x_range_flag>\n" % str(self.data_x_range_flag)

        xml += "<norm_flag>%s</norm_flag>\n" % str(self.NormFlag)
        xml += "<norm_x_range_flag>%s</norm_x_range_flag>\n" % str(self.norm_x_range_flag)
        xml += "<norm_x_max>%s</norm_x_max>\n" % str(self.norm_x_range[1])
        xml += "<norm_x_min>%s</norm_x_min>\n" % str(self.norm_x_range[0])

        xml += "<norm_from_peak_pixels>%s</norm_from_peak_pixels>\n" % str(self.NormPeakPixels[0])
        xml += "<norm_to_peak_pixels>%s</norm_to_peak_pixels>\n" % str(self.NormPeakPixels[1])
        xml += "<norm_background_flag>%s</norm_background_flag>\n" % str(self.NormBackgroundFlag)
        xml += "<norm_from_back_pixels>%s</norm_from_back_pixels>\n" % str(self.NormBackgroundRoi[0])
        xml += "<norm_to_back_pixels>%s</norm_to_back_pixels>\n" % str(self.NormBackgroundRoi[1])
        xml += "<norm_dataset>%s</norm_dataset>\n" % str(self.norm_file)

        # Q cut
        xml += "<q_min>%s</q_min>\n" % str(self.q_min)
        xml += "<q_step>%s</q_step>\n" % str(self.q_step)
        xml += "<q_bins>%s</q_bins>\n" % str(self.q_bins)
        xml += "<q_log>%s</q_log>\n" % str(self.q_log)

        # Scattering angle
        xml += "<theta>%s</theta>\n" % str(self.theta)
        xml += "<use_center_pixel>%s</use_center_pixel>\n" % str(self.use_center_pixel)

        # Sample log overwrites
        xml += "<set_detector_angle>%s</set_detector_angle>\n" % str(self.set_detector_angle)
        xml += "<detector_angle>%s</detector_angle>\n" % str(self.detector_angle)
        xml += "<set_detector_angle_offset>%s</set_detector_angle_offset>\n" % str(self.set_detector_angle_offset)
        xml += "<detector_angle_offset>%s</detector_angle_offset>\n" % str(self.detector_angle_offset)
        xml += "<set_direct_pixel>%s</set_direct_pixel>\n" % str(self.set_direct_pixel)
        xml += "<direct_pixel>%s</direct_pixel>\n" % str(self.direct_pixel)

        xml += "<output_dir>%s</output_dir>\n" % str(self.output_dir)

        xml += "</RefMData>\n"

        return xml

    def from_xml(self, xml_str):
        self.reset()
        dom = xml.dom.minidom.parseString(xml_str)
        self.from_xml_element(dom)
        element_list = dom.getElementsByTagName("RefMData")
        if len(element_list) > 0:
            instrument_dom = element_list[0]

    def from_xml_element(self, instrument_dom):
        """
            Read in data from XML
            @param xml_str: text to read the data from
        """
        #Peak from/to pixels
        self.DataPeakPixels = [BaseScriptElement.getIntElement(instrument_dom, "from_peak_pixels"),
                               BaseScriptElement.getIntElement(instrument_dom, "to_peak_pixels")]


        #low resolution range
        self.data_x_range_flag = BaseScriptElement.getBoolElement(instrument_dom, "x_range_flag",
                                                                  default=DataSets.data_x_range_flag)

        self.data_x_range = [BaseScriptElement.getIntElement(instrument_dom, "x_min_pixel"),
                             BaseScriptElement.getIntElement(instrument_dom, "x_max_pixel")]

        self.norm_x_range_flag = BaseScriptElement.getBoolElement(instrument_dom, "norm_x_range_flag",
                                                                  default=DataSets.norm_x_range_flag)

        self.norm_x_range = [BaseScriptElement.getIntElement(instrument_dom, "norm_x_min"),
                             BaseScriptElement.getIntElement(instrument_dom, "norm_x_max")]

        #discrete selection string
        self.DataPeakDiscreteSelection = BaseScriptElement.getStringElement(instrument_dom, "peak_discrete_selection")

        #background flag
        self.DataBackgroundFlag = BaseScriptElement.getBoolElement(instrument_dom,
                                                                   "background_flag",
                                                                   default=DataSets.DataBackgroundFlag)

        #background from/to pixels
        self.DataBackgroundRoi = [BaseScriptElement.getIntElement(instrument_dom, "back_roi1_from"),
                                  BaseScriptElement.getIntElement(instrument_dom, "back_roi1_to"),
                                  BaseScriptElement.getIntElement(instrument_dom, "back_roi2_from"),
                                  BaseScriptElement.getIntElement(instrument_dom, "back_roi2_to")]

        #from TOF and to TOF
        #self.crop_TOF_range = BaseScriptElement.getBoolElement(instrument_dom, "crop_tof",
        #                                                       default=DataSets.crop_TOF_range)
        self.DataTofRange = [BaseScriptElement.getFloatElement(instrument_dom, "from_tof_range"),
                             BaseScriptElement.getFloatElement(instrument_dom, "to_tof_range")]
        self.TOFstep = BaseScriptElement.getFloatElement(instrument_dom, "tof_step",
                                                         default = DataSets.TOFstep)

        self.data_files = BaseScriptElement.getStringList(instrument_dom, "data_sets")

        #with or without norm
        self.NormFlag = BaseScriptElement.getBoolElement(instrument_dom, "norm_flag",
                                                         default=DataSets.NormFlag)

        #Peak from/to pixels
        self.NormPeakPixels = [BaseScriptElement.getIntElement(instrument_dom, "norm_from_peak_pixels"),
                               BaseScriptElement.getIntElement(instrument_dom, "norm_to_peak_pixels")]

        #background flag
        self.NormBackgroundFlag = BaseScriptElement.getBoolElement(instrument_dom,
                                                                   "norm_background_flag",
                                                                   default=DataSets.NormBackgroundFlag)

        #background from/to pixels
        self.NormBackgroundRoi = [BaseScriptElement.getIntElement(instrument_dom, "norm_from_back_pixels"),
                                  BaseScriptElement.getIntElement(instrument_dom, "norm_to_back_pixels")]

        self.norm_file = BaseScriptElement.getStringElement(instrument_dom, "norm_dataset")

        # Q cut
        self.q_min = BaseScriptElement.getFloatElement(instrument_dom, "q_min", default=DataSets.q_min)
        self.q_step = BaseScriptElement.getFloatElement(instrument_dom, "q_step", default=DataSets.q_step)
        self.q_bins = BaseScriptElement.getIntElement(instrument_dom, "q_bins", default=DataSets.q_bins)
        self.q_log = BaseScriptElement.getBoolElement(instrument_dom, "q_log", default=DataSets.q_log)

        # scattering angle
        self.theta = BaseScriptElement.getFloatElement(instrument_dom, "theta", default=DataSets.theta)
        #self.use_center_pixel = BaseScriptElement.getBoolElement(instrument_dom,
        #                                                         "use_center_pixel",
        #                                                         default=DataSets.use_center_pixel)

        # Sample log overwrites
        self.set_detector_angle = BaseScriptElement.getBoolElement(instrument_dom,
                                                                   "set_detector_angle",
                                                                   default=DataSets.set_detector_angle)
        self.detector_angle = BaseScriptElement.getFloatElement(instrument_dom,
                                                                "detector_angle",
                                                                default=DataSets.detector_angle)
        self.set_detector_angle_offset = BaseScriptElement.getBoolElement(instrument_dom,
                                                                          "set_detector_angle_offset",
                                                                          default=DataSets.set_detector_angle_offset)
        self.detector_angle_offset = BaseScriptElement.getFloatElement(instrument_dom,
                                                                       "detector_angle_offset",
                                                                       default=DataSets.detector_angle_offset)
        self.set_direct_pixel = BaseScriptElement.getBoolElement(instrument_dom,
                                                                 "set_direct_pixel",
                                                                 default=DataSets.set_direct_pixel)
        self.direct_pixel = BaseScriptElement.getFloatElement(instrument_dom,
                                                              "direct_pixel",
                                                              default=DataSets.direct_pixel)

        self.output_dir = BaseScriptElement.getStringElement(instrument_dom,
                                                             "output_dir",
                                                             default=DataSets.output_dir)

    def reset(self):
        """
            Reset state
        """
        self.DataBackgroundFlag = DataSets.DataBackgroundFlag
        self.DataBackgroundRoi = DataSets.DataBackgroundRoi
        self.DataPeakPixels = DataSets.DataPeakPixels
        self.DataTofRange = DataSets.DataTofRange
        self.TOFstep = DataSets.TOFstep
        self.crop_TOF_range = DataSets.crop_TOF_range
        self.data_files = DataSets.data_files

        self.NormFlag = DataSets.NormFlag
        self.NormBackgroundFlag = DataSets.NormBackgroundFlag
        self.NormBackgroundRoi = DataSets.NormBackgroundRoi
        self.NormPeakPixels = DataSets.NormPeakPixels
        self.norm_file = DataSets.norm_file
        self.data_x_range_flag = DataSets.data_x_range_flag
        self.data_x_range = DataSets.data_x_range
        self.norm_x_range_flag = DataSets.norm_x_range_flag
        self.norm_x_range = DataSets.norm_x_range

        # Q range
        self.q_min = DataSets.q_min
        self.q_step = DataSets.q_step
        self.q_bins = DataSets.q_bins
        self.q_log = DataSets.q_log

        # Scattering angle
        self.theta = DataSets.theta
        self.use_center_pixel = DataSets.use_center_pixel

        # Sample log overwrites
        self.set_detector_angle = DataSets.set_detector_angle
        self.detector_angle = DataSets.detector_angle
        self.set_detector_angle_offset = DataSets.set_detector_angle_offset
        self.detector_angle_offset = DataSets.detector_angle_offset
        self.set_direct_pixel = DataSets.set_direct_pixel
        self.direct_pixel = DataSets.direct_pixel

        self.output_dir = DataSets.output_dir

