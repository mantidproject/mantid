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


class RunSetupScript(BaseScriptElement):
    """Run setup script for tab 'Run Setup'"""

    # Class static variables
    runnumbers = ""
    calibfilename = ""
    groupfilename = ""
    exp_ini_file_name = ""
    charfilename = ""
    dosum = False
    binning = -0.0001
    doresamplex = False
    tofmin = ""
    tofmax = ""
    resamplex = 0
    binindspace = True
    saveas = "NeXus"
    outputdir = ""
    finalunits = "dSpacing"

    bkgdrunnumber = ""
    vanrunnumber = ""
    vannoiserunnumber = ""
    vanbkgdrunnumber = ""
    interpolatetemp = 0.0

    disablebkgdcorrection = False
    disablevancorrection = False
    disablevanbkgdcorrection = False
    doresamplex = False
    enableinterpolate = False

    parnamelist = None

    def __init__(self, inst_name):
        """Initialization"""
        super(RunSetupScript, self).__init__()
        self.createParametersList()

        self.set_default_pars(inst_name)
        self.reset()

        return

    def createParametersList(self):
        """Create a list of parameter names for SNSPowderReductionPlus()"""
        self.parnamelist = []
        self.parnamelist.append("RunNumber")
        self.parnamelist.append("Sum")
        self.parnamelist.append("CalibrationFile")
        self.parnamelist.append("GroupingFile")
        self.parnamelist.append("CharacterizationRunsFile")
        self.parnamelist.append("ExpIniFilename")
        self.parnamelist.append("Binning")
        self.parnamelist.append("ResampleX")
        self.parnamelist.append("BinInDspace")
        self.parnamelist.append("SaveAs")
        self.parnamelist.append("OutputDirectory")
        self.parnamelist.append("FinalDataUnits")
        self.parnamelist.append("BackgroundNumber")
        self.parnamelist.append("VanadiumNumber")
        self.parnamelist.append("VanadiumNoiseNumber")
        self.parnamelist.append("VanadiumBackgroundNumber")
        self.parnamelist.append("DisableBackgroundCorrection")
        self.parnamelist.append("DisableVanadiumCorrection")
        self.parnamelist.append("DisableVanadiumBackgroundCorrection")
        self.parnamelist.append("DoReSampleX")
        self.parnamelist.append("InterpolateTargetTemp")
        self.parnamelist.append("EnableInterpolate")
        return

    def buildParameterDict(self):
        """Create a dictionary for parameter and parameter values for SNSPowderReductionPlus()"""
        pardict = {}

        pardict["RunNumber"] = str(self.runnumbers)
        pardict["Sum"] = str(int(self.dosum))
        pardict["CalibrationFile"] = self.calibfilename
        pardict["GroupingFile"] = self.groupfilename
        pardict["ExpIniFilename"] = self.exp_ini_file_name
        pardict["CharacterizationRunsFile"] = self.charfilename
        if self.doresamplex is True:
            # ResampleX is used instead binning: resamplex is always bigger than 0
            pardict["ResampleX"] = "%d" % int(self.resamplex)
            pardict["Binning"] = ""
        else:
            # binnign parameter
            pardict["ResampleX"] = ""
            pardict["Binning"] = "%.7f" % self.binning
        # END-IF

        pardict["BinInDspace"] = str(int(self.binindspace))
        pardict["SaveAs"] = self.saveas
        pardict["OutputDirectory"] = self.outputdir
        pardict["FinalDataUnits"] = self.finalunits

        pardict["BackgroundNumber"] = self.bkgdrunnumber
        pardict["VanadiumNumber"] = self.vanrunnumber
        pardict["VanadiumNoiseNumber"] = self.vannoiserunnumber
        pardict["VanadiumBackgroundNumber"] = self.vanbkgdrunnumber

        pardict["DisableBackgroundCorrection"] = str(int(self.disablebkgdcorrection))
        pardict["DisableVanadiumCorrection"] = str(int(self.disablevancorrection))
        pardict["DisableVanadiumBackgroundCorrection"] = str(int(self.disablevanbkgdcorrection))
        pardict["DoReSampleX"] = str(int(self.doresamplex))
        if self.interpolatetemp == "":
            pardict["InterpolateTargetTemp"] = 0.0
        else:
            pardict["InterpolateTargetTemp"] = self.interpolatetemp
        pardict["EnableInterpolate"] = str(int(self.enableinterpolate))
        return pardict

    def set_default_pars(self, inst_name):
        """Set default value to parameters

        Arguments:
         - inst_name:  name of the instrument
        """
        if inst_name == "PG3":
            pass
        elif inst_name == "NOM":
            pass
        elif inst_name == "VULCAN":
            pass
        else:
            print("Instrument %s is not supported for set default parameter value." % str(inst_name))

        return

    def to_script(self):
        """'Public'  method  to save the current GUI to string via str() and general class ReductionScript"""
        # 1. Build parameter dictionary
        parnamevaluedict = self.buildParameterDict()

        # 2. Write out the script as keys
        script = ""
        for parname in self.parnamelist:
            parvalue = parnamevaluedict[parname]
            if parvalue != "":
                script += '%-10s = "%s",\n' % (parname, parvalue)
        # ENDFOR

        return script

    def to_xml(self):
        """'Public' method to create XML from the current data."""
        parnamevaluedict = self.buildParameterDict()

        xml_str = "<RunSetup>\n"
        for parname in self.parnamelist:
            keyname = parname.lower()
            parvalue = parnamevaluedict[parname]
            if str(parvalue) == "True":
                parvalue = "1"
            elif str(parvalue) == "False":
                parvalue = "0"
            xml_str += "  <%s>%s</%s>\n" % (keyname, str(parvalue), keyname)
        # ENDFOR
        xml_str += "</RunSetup>\n"

        return xml_str

    def from_xml(self, xml_str):
        """'Public' method to read in data from XML
        @param xml_str: text to read the data from
        """
        dom = xml.dom.minidom.parseString(xml_str)
        element_list = dom.getElementsByTagName("RunSetup")
        if len(element_list) > 0:
            #
            instrument_dom = element_list[0]

            #
            self.runnumbers = BaseScriptElement.getStringElement(instrument_dom, "runnumber", default=RunSetupScript.runnumbers)

            tempbool = BaseScriptElement.getStringElement(instrument_dom, "sum", default=str(int(RunSetupScript.dosum)))
            self.dosum = bool(int(tempbool))

            self.calibfilename = BaseScriptElement.getStringElement(instrument_dom, "calibrationfile", default=RunSetupScript.calibfilename)

            self.groupfilename = BaseScriptElement.getStringElement(instrument_dom, "groupingfile", default=RunSetupScript.groupfilename)

            self.exp_ini_file_name = BaseScriptElement.getStringElement(
                instrument_dom, "expinifilename", default=RunSetupScript.exp_ini_file_name
            )

            self.charfilename = BaseScriptElement.getStringElement(
                instrument_dom, "characterizationrunsfile", default=RunSetupScript.charfilename
            )

            try:
                self.binning = BaseScriptElement.getFloatElement(instrument_dom, "binning", default=RunSetupScript.binning)
            except ValueError:
                self.binning = ""

            try:
                self.resamplex = BaseScriptElement.getIntElement(instrument_dom, "resamplex", default=RunSetupScript.resamplex)
            except ValueError:
                self.resamplex = 0

            self.doresamplex = BaseScriptElement.getIntElement(instrument_dom, "doresamplex", default=RunSetupScript.resamplex)
            self.doresamplex = bool(self.doresamplex)

            tempbool = BaseScriptElement.getStringElement(instrument_dom, "binindspace", default=str(int(RunSetupScript.binindspace)))
            self.binindspace = bool(int(tempbool))

            self.saveas = BaseScriptElement.getStringElement(instrument_dom, "saveas", default=RunSetupScript.saveas)

            self.outputdir = BaseScriptElement.getStringElement(instrument_dom, "outputdirectory", default=RunSetupScript.outputdir)

            self.finalunits = BaseScriptElement.getStringElement(instrument_dom, "finaldataunits", default=RunSetupScript.finalunits)

            self.bkgdrunnumber = BaseScriptElement.getStringElement(
                instrument_dom, "backgroundnumber", default=RunSetupScript.bkgdrunnumber
            )
            tempbool = BaseScriptElement.getStringElement(
                instrument_dom, "disablebackgroundcorrection", default=str(int(RunSetupScript.disablebkgdcorrection))
            )
            self.disablebkgdcorrection = bool(int(tempbool))

            self.vanrunnumber = BaseScriptElement.getStringElement(instrument_dom, "vanadiumnumber", default=RunSetupScript.vanrunnumber)
            tempbool = BaseScriptElement.getStringElement(
                instrument_dom, "disablevanadiumcorrection", default=str(int(RunSetupScript.disablevancorrection))
            )
            self.disablevancorrection = bool(int(tempbool))

            self.vanbkgdrunnumber = BaseScriptElement.getStringElement(
                instrument_dom, "vanadiumbackgroundnumber", default=RunSetupScript.vanbkgdrunnumber
            )
            tempbool = BaseScriptElement.getStringElement(
                instrument_dom, "disablevanadiumbackgroundcorrection", default=str(int(RunSetupScript.disablevanbkgdcorrection))
            )
            self.disablevanbkgdcorrection = bool(int(tempbool))

            self.interpolatetemp = BaseScriptElement.getFloatElement(
                instrument_dom, "interpolatetargettemp", default=RunSetupScript.interpolatetemp
            )
            tempbool = BaseScriptElement.getStringElement(
                instrument_dom, "enableinterpolate", default=str(int(RunSetupScript.enableinterpolate))
            )
            self.enableinterpolate = bool(int(tempbool))

            # tempint = BaseScriptElement.getStringElement(instrument_dom,
            # try:
            #     self.vannoiserunnumber = int(tempint)
            # except ValueError:
            #     self.vannoiserunnumber = ""

            return

    def reset(self):
        """'Public' method to reset state"""
        self.runnumbers = RunSetupScript.runnumbers
        self.calibfilename = RunSetupScript.calibfilename
        self.groupfilename = RunSetupScript.groupfilename
        self.exp_ini_file_name = RunSetupScript.exp_ini_file_name
        self.charfilename = RunSetupScript.charfilename
        self.dosum = RunSetupScript.dosum
        self.binning = RunSetupScript.binning
        self.resamplex = RunSetupScript.resamplex
        self.binindspace = RunSetupScript.binindspace
        self.saveas = RunSetupScript.saveas
        self.outputdir = RunSetupScript.outputdir
        self.finalunits = RunSetupScript.finalunits

        self.disablebkgdcorrection = RunSetupScript.disablebkgdcorrection
        self.disablevancorrection = RunSetupScript.disablevancorrection
        self.disablevanbkgdcorrection = RunSetupScript.disablevanbkgdcorrection

        self.bkgdrunnumber = RunSetupScript.bkgdrunnumber
        self.vanrunnumber = RunSetupScript.vanrunnumber
        self.vanbkgdrunnumber = RunSetupScript.vanbkgdrunnumber

        self.interpolatetemp = RunSetupScript.interpolatetemp
        self.enableinterpolate = RunSetupScript.enableinterpolate

        return
