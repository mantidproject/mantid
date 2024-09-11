# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=invalid-name
"""
Classes for each reduction step. Those are kept separately
from the interface class so that the DgsReduction class could
be used independently of the interface implementation
"""

import os
import xml.dom.minidom

from reduction_gui.reduction.scripter import BaseScriptElement


class SampleSetupScript(BaseScriptElement):
    sample_file = ""
    live_button = False
    output_wsname = ""
    detcal_file = ""
    relocate_dets = False
    incident_energy_guess = ""
    use_ei_guess = False
    tzero_guess = 0.0
    monitor1_specid = ""
    monitor2_specid = ""
    rebin_et = False
    et_range_low = ""
    et_range_width = ""
    et_range_high = ""
    et_is_distribution = True
    hardmask_file = ""
    grouping_file = ""
    show_workspaces = False
    savedir = ""

    def __init__(self, inst_name):
        super(SampleSetupScript, self).__init__()
        self.set_default_pars(inst_name)
        self.reset()

    def set_default_pars(self, inst_name):
        from . import dgs_utils

        ip = dgs_utils.InstrumentParameters(inst_name)
        SampleSetupScript.monitor1_specid = str(int(ip.get_parameter("ei-mon1-spec")))
        SampleSetupScript.monitor2_specid = str(int(ip.get_parameter("ei-mon2-spec")))

    def to_script(self):
        script = ""
        if not self.live_button:
            script += 'SampleInputFile="%s",\n' % self.sample_file
        else:
            script += "SampleInputWorkspace=input,\n"
        tmp_wsname = ""
        if self.output_wsname == SampleSetupScript.output_wsname:
            # Make a default name from the incoming file
            tmp = os.path.split(os.path.splitext(str(self.sample_file))[0])[-1]
            tmp_wsname = tmp + "_spe"
        else:
            tmp_wsname = self.output_wsname
        script += 'OutputWorkspace="%s",\n' % tmp_wsname
        if self.detcal_file != SampleSetupScript.detcal_file:
            script += 'DetCalFilename="%s",\n' % self.detcal_file
        if self.relocate_dets != SampleSetupScript.relocate_dets:
            script += "RelocateDetectors=%s,\n" % self.relocate_dets
        if self.incident_energy_guess != SampleSetupScript.incident_energy_guess:
            script += "IncidentEnergyGuess=%s,\n" % float(self.incident_energy_guess)
        if self.use_ei_guess != SampleSetupScript.use_ei_guess:
            script += "UseIncidentEnergyGuess=%s,\n" % self.use_ei_guess
            if self.tzero_guess != SampleSetupScript.tzero_guess:
                script += "TimeZeroGuess=%s,\n" % str(self.tzero_guess)
        if self.monitor1_specid != SampleSetupScript.monitor1_specid:
            try:
                temp1 = int(self.monitor1_specid)
                script += "Monitor1SpecId=%s,\n" % temp1
            except ValueError:
                pass
        if self.monitor2_specid != SampleSetupScript.monitor2_specid:
            try:
                temp2 = int(self.monitor2_specid)
                script += "Monitor2SpecId=%s,\n" % temp2
            except ValueError:
                pass
        if (
            self.et_range_low != SampleSetupScript.et_range_low
            or self.et_range_width != SampleSetupScript.et_range_width
            or self.et_range_high != SampleSetupScript.et_range_high
        ):
            script += 'EnergyTransferRange="%s,%s,%s",\n' % (self.et_range_low, self.et_range_width, self.et_range_high)
        if self.et_is_distribution != SampleSetupScript.et_is_distribution:
            script += "SofPhiEIsDistribution=%s,\n" % self.et_is_distribution
        if self.hardmask_file != SampleSetupScript.hardmask_file:
            script += 'HardMaskFile="%s",\n' % self.hardmask_file
        if self.grouping_file != SampleSetupScript.grouping_file:
            script += 'GroupingFile="%s",\n' % self.grouping_file
        if self.show_workspaces:
            script += "ShowIntermediateWorkspaces=%s,\n" % self.show_workspaces
        if self.savedir != SampleSetupScript.savedir:
            script += 'OutputDirectory="%s",\n' % self.savedir
        return script

    def to_xml(self):
        """
        Create XML from the current data.
        """
        xml_str = "<SampleSetup>\n"
        xml_str += "  <sample_input_file>%s</sample_input_file>\n" % self.sample_file
        xml_str += "  <live_button>%s</live_button>\n" % self.live_button
        xml_str += "  <output_wsname>%s</output_wsname>\n" % self.output_wsname
        xml_str += "  <detcal_file>%s</detcal_file>\n" % self.detcal_file
        xml_str += "  <relocate_dets>%s</relocate_dets>\n" % self.relocate_dets
        xml_str += "  <incident_energy_guess>%s</incident_energy_guess>\n" % self.incident_energy_guess
        xml_str += "  <use_ei_guess>%s</use_ei_guess>\n" % str(self.use_ei_guess)
        xml_str += "  <tzero_guess>%s</tzero_guess>\n" % str(self.tzero_guess)
        xml_str += "  <monitor1_specid>%s</monitor1_specid>\n" % self.monitor1_specid
        xml_str += "  <monitor2_specid>%s</monitor2_specid>\n" % self.monitor2_specid
        xml_str += "  <et_range>\n"
        xml_str += "    <low>%s</low>\n" % self.et_range_low
        xml_str += "    <width>%s</width>\n" % self.et_range_width
        xml_str += "    <high>%s</high>\n" % self.et_range_high
        xml_str += "  </et_range>\n"
        xml_str += "  <sofphie_is_distribution>%s</sofphie_is_distribution>\n" % str(self.et_is_distribution)
        xml_str += "  <hardmask_file>%s</hardmask_file>\n" % self.hardmask_file
        xml_str += "  <grouping_file>%s</grouping_file>\n" % self.grouping_file
        xml_str += "  <show_workspaces>%s</show_workspaces>\n" % self.show_workspaces
        xml_str += "  <savedir>%s</savedir>\n" % self.savedir
        xml_str += "</SampleSetup>\n"
        return xml_str

    def from_xml(self, xml_str):
        """
        Read in data from XML
        @param xml_str: text to read the data from
        """
        dom = xml.dom.minidom.parseString(xml_str)
        element_list = dom.getElementsByTagName("SampleSetup")
        if len(element_list) > 0:
            instrument_dom = element_list[0]
            self.sample_file = BaseScriptElement.getStringElement(
                instrument_dom, "sample_input_file", default=SampleSetupScript.sample_file
            )
            self.live_button = BaseScriptElement.getBoolElement(instrument_dom, "live_button", default=SampleSetupScript.live_button)
            self.output_wsname = BaseScriptElement.getStringElement(
                instrument_dom, "output_wsname", default=SampleSetupScript.output_wsname
            )
            self.detcal_file = BaseScriptElement.getStringElement(instrument_dom, "detcal_file", default=SampleSetupScript.detcal_file)
            self.relocate_dets = BaseScriptElement.getBoolElement(instrument_dom, "relocate_dets", default=SampleSetupScript.relocate_dets)
            self.incident_energy_guess = BaseScriptElement.getStringElement(
                instrument_dom, "incident_energy_guess", default=SampleSetupScript.incident_energy_guess
            )
            self.use_ei_guess = BaseScriptElement.getBoolElement(instrument_dom, "use_ei_guess", default=SampleSetupScript.use_ei_guess)
            self.tzero_guess = BaseScriptElement.getFloatElement(instrument_dom, "tzero_guess", default=SampleSetupScript.tzero_guess)
            self.monitor1_specid = BaseScriptElement.getStringElement(
                instrument_dom, "monitor1_specid", default=SampleSetupScript.monitor1_specid
            )
            self.monitor2_specid = BaseScriptElement.getStringElement(
                instrument_dom, "monitor2_specid", default=SampleSetupScript.monitor2_specid
            )
            self.et_range_low = BaseScriptElement.getStringElement(instrument_dom, "et_range/low", default=SampleSetupScript.et_range_low)
            self.et_range_width = BaseScriptElement.getStringElement(
                instrument_dom, "et_range/width", default=SampleSetupScript.et_range_width
            )
            self.et_range_high = BaseScriptElement.getStringElement(
                instrument_dom, "et_range/high", default=SampleSetupScript.et_range_high
            )
            self.et_is_distribution = BaseScriptElement.getBoolElement(
                instrument_dom, "sofphie_is_distribution", default=SampleSetupScript.et_is_distribution
            )
            self.hardmask_file = BaseScriptElement.getStringElement(
                instrument_dom, "hardmask_file", default=SampleSetupScript.hardmask_file
            )
            self.grouping_file = BaseScriptElement.getStringElement(
                instrument_dom, "grouping_file", default=SampleSetupScript.grouping_file
            )
            self.show_workspaces = BaseScriptElement.getBoolElement(
                instrument_dom, "show_workspaces", default=SampleSetupScript.show_workspaces
            )
            self.savedir = BaseScriptElement.getStringElement(instrument_dom, "savedir", default=SampleSetupScript.savedir)

    def reset(self):
        """
        Reset state
        """
        self.sample_file = SampleSetupScript.sample_file
        self.live_button = SampleSetupScript.live_button
        self.output_wsname = SampleSetupScript.output_wsname
        self.detcal_file = SampleSetupScript.detcal_file
        self.relocate_dets = SampleSetupScript.relocate_dets
        self.incident_energy_guess = SampleSetupScript.incident_energy_guess
        self.use_ei_guess = SampleSetupScript.use_ei_guess
        self.tzero_guess = SampleSetupScript.tzero_guess
        self.monitor1_specid = SampleSetupScript.monitor1_specid
        self.monitor2_specid = SampleSetupScript.monitor2_specid
        self.rebin_et = SampleSetupScript.rebin_et
        self.et_range_low = SampleSetupScript.et_range_low
        self.et_range_width = SampleSetupScript.et_range_width
        self.et_range_high = SampleSetupScript.et_range_high
        self.et_is_distribution = SampleSetupScript.et_is_distribution
        self.hardmask_file = SampleSetupScript.hardmask_file
        self.grouping_file = SampleSetupScript.grouping_file
        self.show_workspaces = SampleSetupScript.show_workspaces
        self.savedir = SampleSetupScript.savedir
