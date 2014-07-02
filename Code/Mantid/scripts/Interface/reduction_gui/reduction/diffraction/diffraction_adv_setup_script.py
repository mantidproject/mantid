"""
    Classes for each reduction step. Those are kept separately 
    from the the interface class so that the DgsReduction class could 
    be used independently of the interface implementation
"""
import os
import time
import xml.dom.minidom

from reduction_gui.reduction.scripter import BaseScriptElement

def getBooleanElement(instrument_dom, keyname, default):
    """ Get a boolean value from an element. 
    Boolean can be recorded as 
    (1) True/False
    (2) 1/0
    """
    tempbool = BaseScriptElement.getStringElement(instrument_dom,
            keyname, default=default)

    if tempbool == "True":
        tempbool = 1 
    elif tempbool == "False": 
        tempbool = 0

    returnbool = bool(int(tempbool))

    return returnbool

class AdvancedSetupScript(BaseScriptElement):
    """ Run setup script for tab 'Run Setup'
    """

    # Class static variables
    pushdatapositive = "None"
    unwrapref = ""
    lowresref = ""
    cropwavelengthmin = ""
    removepropmppulsewidth = 50.0
    maxchunksize = ""
    filterbadpulses = True
    stripvanadiumpeaks = True
    vanadiumfwhm = ""
    vanadiumpeaktol = ""
    vanadiumsmoothparams = ""
    preserveevents = True
    extension = "_event.nxs"
    outputfileprefix = ""
    scaledata = ""

    def __init__(self, inst_name):
        """ Initialization
        """
        super(AdvancedSetupScript, self).__init__()
        self.createParametersList()

        self.set_default_pars(inst_name)
        self.reset()

        return

    def createParametersList(self):
        """ Create a list of parameter names for SNSPowderReductionPlus()
        """
        self.parnamelist = []
        self.parnamelist.append("UnwrapRef")
        self.parnamelist.append("LowResRef")
        self.parnamelist.append("CropWavelengthMin")
        self.parnamelist.append("RemovePromptPulseWidth")
        self.parnamelist.append("MaxChunkSize")
        self.parnamelist.append("StripVanadiumPeaks")
        self.parnamelist.append("VanadiumFWHM")
        self.parnamelist.append("VanadiumPeakTol")
        self.parnamelist.append("VanadiumSmoothParams")
        self.parnamelist.append("FilterBadPulses")
        self.parnamelist.append("PushDataPositive")
        self.parnamelist.append("Extension")
        self.parnamelist.append("PreserveEvents")
        self.parnamelist.append("OutputFilePrefix")
        self.parnamelist.append("ScaleData")

        return
        
    def set_default_pars(self, inst_name):
        """ Set default values
        """

        return
        
    def to_script(self):
        """ 'Public'  method  to save the current GUI to string via str() and general class ReductionScript
        """
        # 1. Form (partial) script
        parnamevaluedict = self.buildParameterDict()
        script = ""
        for parname in self.parnamelist:
            parvalue = parnamevaluedict[parname]
            if parvalue != "" and parname != "Instrument" and parname != "Facility":
                if str(parvalue) == "True":
                    parvalue = "1"
                elif str(parvalue) == "False":
                    parvalue = "0"
                script += "%-10s = \"%s\",\n" % (parname, parvalue)
        #ENDFOR

        return script


    def buildParameterDict(self):
        """ Create a dictionary for parameter and parameter values for SNSPowderReductionPlus()
        """
        pardict = {}
       
        pardict["UnwrapRef"] = self.unwrapref
        pardict["LowResRef"] = self.lowresref
        pardict["CropWavelengthMin"] = self.cropwavelengthmin
        pardict["RemovePromptPulseWidth"] = self.removepropmppulsewidth
        pardict["MaxChunkSize"] = self.maxchunksize
        pardict["FilterBadPulses"] = self.filterbadpulses
        pardict["PushDataPositive"] = self.pushdatapositive
        pardict["StripVanadiumPeaks"] = self.stripvanadiumpeaks
        pardict["VanadiumFWHM"] = self.vanadiumfwhm
        pardict["VanadiumPeakTol"] = self.vanadiumpeaktol
        pardict["VanadiumSmoothParams"] = self.vanadiumsmoothparams
        pardict["Extension"] = str(self.extension)
        pardict["PreserveEvents"] = str(int(self.preserveevents))
        pardict["OutputFilePrefix"] = self.outputfileprefix
        pardict["ScaleData"] = self.scaledata

        return pardict
        
    def to_xml(self):
        """ 'Public' method to create XML from the current data.
        """
        pardict = self.buildParameterDict()

        xml = "<AdvancedSetup>\n"
        for parname in self.parnamelist:
            value = pardict[parname]
            keyname = parname.lower()
            if str(value) == "True":
                value = '1'
            elif str(value) == "False":
                value = '0'
            xml += " <%s>%s</%s>\n" % (keyname, str(value), keyname)
        # ENDFOR
        xml += "</AdvancedSetup>\n"

        return xml
    
    def from_xml(self, xml_str):
        """ 'Public' method to read in data from XML
            @param xml_str: text to read the data from
        """       
        dom = xml.dom.minidom.parseString(xml_str)
        element_list = dom.getElementsByTagName("AdvancedSetup")
        if len(element_list)>0:
            instrument_dom = element_list[0]

            tempfloat = BaseScriptElement.getStringElement(instrument_dom,
                    "unwrapref", default=AdvancedSetupScript.unwrapref)
            try:
                self.unwrapref = float(tempfloat)
            except ValueError:
                self.unwrapref = ""

            tempfloat = BaseScriptElement.getStringElement(instrument_dom,
                    "lowresref", default=AdvancedSetupScript.lowresref)
            try:
                self.lowresref = float(tempfloat)
            except ValueError:
                self.lowresref = ""

            tempfloat = BaseScriptElement.getStringElement(instrument_dom,
                    "cropwavelengthmin", default=AdvancedSetupScript.cropwavelengthmin)
            try:
                self.cropwavelengthmin = float(tempfloat)
            except ValueError:
                self.cropwavelengthmin = ""

            tempfloat = BaseScriptElement.getStringElement(instrument_dom,
                    "removepromptpulsewidth", default=AdvancedSetupScript.removepropmppulsewidth)
            try:
                self.removepropmppulsewidth = float(tempfloat)
            except ValueError:
                self.removepropmppulsewidth = ""

            tempint = BaseScriptElement.getStringElement(instrument_dom,
                    "maxchunksize", default=AdvancedSetupScript.maxchunksize)
            try:
                self.maxchunksize = int(tempint)
            except ValueError:
                self.maxchunksize = ""

            self.filterbadpulses = getBooleanElement(instrument_dom, 
                    "filterbadpulses", AdvancedSetupScript.filterbadpulses)

            self.pushdatapositive = BaseScriptElement.getStringElement(instrument_dom,
                    "pushdatapositive", default=AdvancedSetupScript.pushdatapositive)

            self.stripvanadiumpeaks = getBooleanElement(instrument_dom, 
                    "stripvanadiumpeaks", AdvancedSetupScript.stripvanadiumpeaks)

            tempfloat = BaseScriptElement.getStringElement(instrument_dom,
                    "vanadiumfwhm", default=AdvancedSetupScript.vanadiumfwhm)
            try:
                self.vanadiumfwhm = float(tempfloat)
            except ValueError:
                self.vanadiumfwhm = ""

            tempfloat = BaseScriptElement.getStringElement(instrument_dom,
                    "vanadiumpeaktol", default=AdvancedSetupScript.vanadiumpeaktol)
            try:
                self.vanadiumpeaktol = float(tempfloat)
            except ValueError:
                self.vanadiumpeaktol = ""

            self.vanadiumsmoothparams = BaseScriptElement.getStringElement(instrument_dom,
                "vanadiumsmoothparams", default=AdvancedSetupScript.vanadiumsmoothparams)

            self.extension = BaseScriptElement.getStringElement(instrument_dom, 
                    "extension", default=AdvancedSetupScript.extension)

            tempbool = BaseScriptElement.getStringElement(instrument_dom, 
                    "preserveevents", default=str(int(AdvancedSetupScript.preserveevents)))
            self.preserveevents = bool(int(tempbool))

            self.outputfileprefix = BaseScriptElement.getStringElement(instrument_dom,
                    "outputfileprefix", default = AdvancedSetupScript.outputfileprefix)

            try:
                self.scaledata = BaseScriptElement.getFloatElement(instrument_dom,
                                 "scaledata", default=AdvancedSetupScript.scaledata)
            except ValueError:
                self.scaledata = AdvancedSetupScript.scaledata
            return

    def reset(self):
        """ 'Public' method to reset state
        """
        self.pushdatapositive       = AdvancedSetupScript.pushdatapositive
        self.unwrapref              = AdvancedSetupScript.unwrapref
        self.lowresref              = AdvancedSetupScript.lowresref
        self.cropwavelengthmin      = AdvancedSetupScript.cropwavelengthmin
        self.removepropmppulsewidth = AdvancedSetupScript.removepropmppulsewidth
        self.maxchunksize           = AdvancedSetupScript.maxchunksize
        self.filterbadpulses        = AdvancedSetupScript.filterbadpulses
        self.stripvanadiumpeaks     = AdvancedSetupScript.stripvanadiumpeaks
        self.vanadiumfwhm           = AdvancedSetupScript.vanadiumfwhm
        self.vanadiumpeaktol        = AdvancedSetupScript.vanadiumpeaktol
        self.vanadiumsmoothparams   = AdvancedSetupScript.vanadiumsmoothparams
        self.preserveevents         = AdvancedSetupScript.preserveevents
        self.extension              = AdvancedSetupScript.extension
        self.outputfileprefix       = AdvancedSetupScript.outputfileprefix
        self.scaledata              = AdvancedSetupScript.scaledata

        return

