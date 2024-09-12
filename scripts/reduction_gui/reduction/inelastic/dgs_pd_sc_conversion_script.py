# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
"""
Classes for each reduction step. Those are kept separately
from the interface class so that the DgsReduction class could
be used independently of the interface implementation
"""

import xml.dom.minidom

from reduction_gui.reduction.scripter import BaseScriptElement


class PdAndScConversionScript(BaseScriptElement):
    do_pd_convert = False
    pd_q_range_low = ""
    pd_q_range_width = ""
    pd_q_range_high = ""
    save_powder_nxs = True
    save_powder_nxs_file = ""

    def __init__(self, inst_name):
        super(PdAndScConversionScript, self).__init__()
        self.reset()

    def to_script(self):
        script = ""
        if self.do_pd_convert:
            script += "DoPowderDataConversion=%s,\n" % self.do_pd_convert
            if (
                self.pd_q_range_low != PdAndScConversionScript.pd_q_range_low
                or self.pd_q_range_width != PdAndScConversionScript.pd_q_range_width
                or self.pd_q_range_high != PdAndScConversionScript.pd_q_range_high
            ):
                script += 'PowderMomTransferRange="%s,%s,%s",\n' % (self.pd_q_range_low, self.pd_q_range_width, self.pd_q_range_high)
            if not self.save_powder_nxs:
                script += "SavePowderNexusFile=%s,\n" % self.save_powder_nxs
            if self.save_powder_nxs_file != PdAndScConversionScript.save_powder_nxs_file and self.save_powder_nxs:
                script += 'SavePowderNexusFilename="%s",\n' % self.save_powder_nxs_file
        return script

    def to_xml(self):
        """
        Create XML from the current data.
        """
        xml = "<PdAndScConversion>\n"
        xml += "  <do_powder_conversion>%s</do_powder_conversion>\n" % str(self.do_pd_convert)
        xml += "  <powder_q_range>\n"
        xml += "    <low>%s</low>\n" % self.pd_q_range_low
        xml += "    <width>%s</width>\n" % self.pd_q_range_width
        xml += "    <high>%s</high>\n" % self.pd_q_range_high
        xml += "  </powder_q_range>\n"
        xml += "  <save_powder_nexus>%s</save_powder_nexus>\n" % str(self.save_powder_nxs)
        xml += "  <save_powder_nexus_filename>%s</save_powder_nexus_filename>\n" % self.save_powder_nxs_file
        xml += "</PdAndScConversion>\n"

        return xml

    def from_xml(self, xml_str):
        """
        Read in data from XML
        @param xml_str: text to read the data from
        """
        dom = xml.dom.minidom.parseString(xml_str)
        element_list = dom.getElementsByTagName("PdAndScConversion")
        if len(element_list) > 0:
            instrument_dom = element_list[0]
            self.do_pd_convert = BaseScriptElement.getBoolElement(
                instrument_dom, "do_powder_conversion", default=PdAndScConversionScript.do_pd_convert
            )
            self.pd_q_range_low = BaseScriptElement.getStringElement(
                instrument_dom, "pd_q_range/low", default=PdAndScConversionScript.pd_q_range_low
            )
            self.pd_q_range_width = BaseScriptElement.getStringElement(
                instrument_dom, "pd_q_range/width", default=PdAndScConversionScript.pd_q_range_width
            )
            self.pd_q_range_high = BaseScriptElement.getStringElement(
                instrument_dom, "pd_q_range/high", default=PdAndScConversionScript.pd_q_range_high
            )
            self.save_powder_nxs = BaseScriptElement.getBoolElement(
                instrument_dom, "save_powder_nexus", default=PdAndScConversionScript.save_powder_nxs
            )
            self.save_powder_nxs_file = BaseScriptElement.getStringElement(
                instrument_dom, "save_powder_nexus_filename", default=PdAndScConversionScript.save_powder_nxs_file
            )

    def reset(self):
        self.do_pd_convert = PdAndScConversionScript.do_pd_convert
        self.pd_q_range_low = PdAndScConversionScript.pd_q_range_low
        self.pd_q_range_width = PdAndScConversionScript.pd_q_range_width
        self.pd_q_range_high = PdAndScConversionScript.pd_q_range_high
        self.save_powder_nxs = PdAndScConversionScript.save_powder_nxs
        self.save_powder_nxs_file = PdAndScConversionScript.save_powder_nxs_file
