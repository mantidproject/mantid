"""
    Classes for each reduction step. Those are kept separately
    from the the interface class so that the DgsReduction class could
    be used independently of the interface implementation
"""
import os
import time
import xml.dom.minidom

from reduction_gui.reduction.scripter import BaseScriptElement

class RunSetupScript(BaseScriptElement):
    """ Run setup script for tab 'Run Setup'
    """

    # Class static variables
    runnumbers = ""
    calibfilename = ""
    charfilename = ""
    dosum = False
    binning = -0.0001
    binningtype = "Logarithmic"
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

    disablebkgdcorrection = False
    disablevancorrection = False
    disablevanbkgdcorrection = False
    doresamplex = False

    parnamelist = None


    def __init__(self, inst_name):
        """ Initialization
        """
        super(RunSetupScript, self).__init__()
        self.createParametersList()

        self.set_default_pars(inst_name)
        self.reset()

        return

    def createParametersList(self):
        """ Create a list of parameter names for SNSPowderReductionPlus()
        """
        self.parnamelist = []
        self.parnamelist.append("RunNumber")
        self.parnamelist.append("Sum")
        self.parnamelist.append("CalibrationFile")
        self.parnamelist.append("CharacterizationRunsFile")
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

        return

    def buildParameterDict(self):
        """ Create a dictionary for parameter and parameter values for SNSPowderReductionPlus()
        """
        pardict = {}

        pardict["RunNumber"] = str(self.runnumbers)
        pardict["Sum"] = str(int(self.dosum))
        pardict["CalibrationFile"] = self.calibfilename
        pardict["CharacterizationRunsFile"] = self.charfilename
        if self.binningtype == "Logarithmic":
            pardict["Binning"] = -1.0*abs(self.binning)
        else:
            pardict["Binning"] = self.binning
        pardict["ResampleX"] = str(self.resamplex)
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

        return pardict

    def set_default_pars(self, inst_name):
        """ Set default value to parameters

        Arguments:
         - inst_name:  name of the instrument
        """
        return

    def to_script(self):
        """ 'Public'  method  to save the current GUI to string via str() and general class ReductionScript
        """
        # 1. Build parameter dictionary
        parnamevaluedict = self.buildParameterDict()

        # 2. Write out the script as keys
        script = ""
        for parname in self.parnamelist:
            parvalue = parnamevaluedict[parname]
            if parvalue != "":
                script += "%-10s = \"%s\",\n" % (parname, parvalue)
        #ENDFOR

        return script

    def to_xml(self):
        """ 'Public' method to create XML from the current data.
        """
        parnamevaluedict = self.buildParameterDict()

        xml = "<RunSetup>\n"
        for parname in self.parnamelist:
            keyname = parname.lower()
            parvalue = parnamevaluedict[parname]
            if str(parvalue) == "True":
                parvalue = "1"
            elif str(parvalue) == "False":
                parvalue = "0"
            xml += "  <%s>%s</%s>\n" % (keyname, str(parvalue), keyname)
        #ENDFOR
        xml += "</RunSetup>\n"

        return xml

    def from_xml(self, xml_str):
        """ 'Public' method to read in data from XML
            @param xml_str: text to read the data from
        """
        dom = xml.dom.minidom.parseString(xml_str)
        element_list = dom.getElementsByTagName("RunSetup")
        if len(element_list)>0:
            #
            instrument_dom = element_list[0]

            #
            self.runnumbers = BaseScriptElement.getStringElement(instrument_dom,\
                    "runnumber", default=RunSetupScript.runnumbers)


            tempbool = BaseScriptElement.getStringElement(instrument_dom,\
                    "sum", default=str(int(RunSetupScript.dosum)))
            self.dosum = bool(int(tempbool))

            self.calibfilename = BaseScriptElement.getStringElement(instrument_dom,\
                    "calibrationfile", default=RunSetupScript.calibfilename)

            self.charfilename = BaseScriptElement.getStringElement(instrument_dom,\
                    "characterizationrunsfile", default=RunSetupScript.charfilename)

            self.binning = BaseScriptElement.getFloatElement(instrument_dom,\
                    "binning", default=RunSetupScript.binning)

            tempbool =  BaseScriptElement.getStringElement(instrument_dom,\
                    "binindspace", default=str(int(RunSetupScript.binindspace)))
            self.binindspace = bool(int(tempbool))

            self.resamplex = BaseScriptElement.getIntElement(instrument_dom,\
                    "resamplex", default=RunSetupScript.resamplex)

            self.saveas = BaseScriptElement.getStringElement(instrument_dom,\
                    "saveas",  default=RunSetupScript.saveas)

            self.outputdir = BaseScriptElement.getStringElement(instrument_dom,\
                    "outputdirectory", default=RunSetupScript.outputdir)

            self.finalunits = BaseScriptElement.getStringElement(instrument_dom,\
                    "finaldataunits", default=RunSetupScript.finalunits)

            tempint = BaseScriptElement.getStringElement(instrument_dom,\
                    "backgroundnumber", default=RunSetupScript.bkgdrunnumber)
            try:
                self.bkgdrunnumber = int(tempint)
            except ValueError:
                self.bkgdrunnumber = None
            tempbool = BaseScriptElement.getStringElement(instrument_dom,\
                    "disablebackgroundcorrection", default=str(int(RunSetupScript.disablebkgdcorrection)))
            self.disablebkgdcorrection = bool(int(tempbool))

            tempint = BaseScriptElement.getStringElement(instrument_dom,\
                    "vanadiumnumber", default=RunSetupScript.vanrunnumber)
            try:
                self.vanrunnumber = int(tempint)
            except ValueError:
                self.vanrunnumber = ""
            tempbool = BaseScriptElement.getStringElement(instrument_dom,\
                    "disablevanadiumcorrection", default=str(int(RunSetupScript.disablevancorrection)))
            self.disablevancorrection = bool(int(tempbool))

            tempint = BaseScriptElement.getStringElement(instrument_dom,\
                    "vanadiumbackgroundnumber", default=RunSetupScript.vanbkgdrunnumber)
            try:
                self.vanbkgdrunnumber = int(tempint)
            except ValueError:
                self.vanbkgdrunnumber = None
            tempbool = BaseScriptElement.getStringElement(instrument_dom,\
                    "disablevanadiumbackgroundcorrection", default=str(int(RunSetupScript.disablevanbkgdcorrection)))
            self.disablevanbkgdcorrection = bool(int(tempbool))

            # tempint = BaseScriptElement.getStringElement(instrument_dom,
            # try:
            #     self.vannoiserunnumber = int(tempint)
            # except ValueError:
            #     self.vannoiserunnumber = ""

            return

    def reset(self):
        """ 'Public' method to reset state
        """
        self.runnumbers = RunSetupScript.runnumbers
        self.calibfilename = RunSetupScript.calibfilename
        self.charfilename  = RunSetupScript.charfilename
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

        self.bkgdrunnumber      = RunSetupScript.bkgdrunnumber
        self.vanrunnumber        = RunSetupScript.vanrunnumber
        self.vanbkgdrunnumber    = RunSetupScript.vanbkgdrunnumber

        return

